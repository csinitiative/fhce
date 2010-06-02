/*
 * Copyright (C) 2008, 2009, 2010 The Collaborative Software Foundation.
 *
 * This file is part of FeedHandlers (FH).
 *
 * FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with FH.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/wait.h>
#include <signal.h>

#include "fh_util.h"
#include "fh_log.h"
#include "fh_net.h"
#include "fh_tcp.h"
#include "fh_time.h"
#include "fh_plugin.h"

#include "fh_mgmt_admin.h"
#include "fh_mgmt_service.h"

/*
 * Service group local variables
 */
static sg_lh_t  sg_list;
static sg_lh_t  sg_timers;
static uint32_t sg_spawn_delay = 0;

#define MAX_SECS_PER_DAY   (24 * 60 * 60)

static fh_plugin_hook_t mgmt_stats_report = NULL;

/*
 * SIGTERM signal handler -- this will make sure that all processes that need to be killed
 * are killed before the manager exits
 */
void fh_mgmt_serv_sigterm(int signal)
{
    fh_mgmt_sg_t    *sg     = NULL;
    fh_mgmt_serv_t  *serv   = NULL;

    /* suppress warnings */
    if (signal) {}

    /* print an informational message that the SIGTERM was received */
    FH_LOG(MGMT, INFO, ("Received SIGTERM -- killing services that need to be killed"));

    /* loop through each service group, then through each service */
    TAILQ_FOREACH(sg, &sg_list, sg_le) {
        TAILQ_FOREACH(serv, &sg->sg_serv_lh, serv_le) {
            /*
             * if the service is configured to be killed, and is running, and has a connection
             * associated with it, and the request to stop the service fails, then display an
             * error
             */
            if (serv->serv_flags & FH_MGMT_SERV_SHUTDOWN &&
                serv->serv_flags & FH_MGMT_SERV_RUNNING && serv->serv_conn != NULL &&
                fh_mgmt_serv_stop(serv) != FH_OK) {
                FH_LOG(MGMT, ERR, ("Error stopping service on shutdown: %s", serv->serv_name));
            }
        }
    }

    /* once all killable feed handlers have been killed, time to exit */
    exit(0);
}

/*
 * fh_mgmt_serv_init
 *
 * Initialize the service library
 */
void fh_mgmt_serv_init(uint32_t spawn_delay)
{
    TAILQ_INIT(&sg_list);
    TAILQ_INIT(&sg_timers);

    /*
     * Load the plugins for the management interface
     */
    mgmt_stats_report = fh_plugin_get_hook(FH_PLUGIN_FHMGR_STATS_REPORT);
    if (mgmt_stats_report) {
        FH_LOG(MGMT, STATE, ("Management statistics report plugin loaded"));
    }

    /* install a sigterm handler that will kill all processes that need killing */
    signal(SIGTERM, &fh_mgmt_serv_sigterm);

    /* set the default service group spawn delay to the value passed in */
    sg_spawn_delay = spawn_delay;
}

/*-----------------------------------------------------------------------------*/
/* Service Group API                                                           */
/*-----------------------------------------------------------------------------*/

/**
 *  @brief Set the stats reporting period for a service group
 *
 *  @param sg the service group for which the period is being set
 *  @param period the period (in seconds)
 */
void fh_mgmt_sg_stats_period(fh_mgmt_sg_t *sg, uint32_t period)
{
    /* set the period to the specified period and reset ticks */
    sg->sg_stats_period = period;
    sg->sg_stats_ticks  = 0;

    /* ensure that the FH_MGMT_SG_STATS flag is set properly */
    if (sg->sg_stats_period > 0) {
        sg->sg_flags |= FH_MGMT_SG_STATS;
    }
    else {
        sg->sg_flags &= ~FH_MGMT_SG_STATS;
    }
}

/*
 * fh_mgmt_sg_restart_time
 *
 * Sets the restart time, and ticks/deadline values for the service group.
 */
void fh_mgmt_sg_restart_time(fh_mgmt_sg_t *sg, uint32_t restart_time)
{
    uint32_t secs_after_midnight;
    time_t curr_time = time(0);
    struct tm tm;

    if (!(sg->sg_flags & FH_MGMT_SG_RESTART)) {
        sg->sg_restart_deadline = 24 * 60 * 60;
        sg->sg_flags |= FH_MGMT_SG_RESTART;
    }

    if (restart_time > MAX_SECS_PER_DAY) {
        restart_time = MAX_SECS_PER_DAY;
    }
    sg->sg_restart_time = restart_time;

    /* Compute the ticks remaining before the next deadline */
    localtime_r(&curr_time, &tm);

    secs_after_midnight = tm.tm_hour * 3600 +
                          tm.tm_min  * 60   +
                          tm.tm_sec;

    if (secs_after_midnight <= sg->sg_restart_time) {
        sg->sg_restart_ticks = sg->sg_restart_deadline +
                               secs_after_midnight - sg->sg_restart_time;

    }
    else {
        sg->sg_restart_ticks = secs_after_midnight - sg->sg_restart_time;
    }

    FH_LOG(MGMT, DIAG, ("Service Group %s: restart at %d secs (current:%d ticks:%d)",
                        sg->sg_name, sg->sg_restart_time,
                        secs_after_midnight, sg->sg_restart_ticks));
}

/*
 * sg_new
 *
 * Creates a new service group
 */
fh_mgmt_sg_t *fh_mgmt_sg_new(const char *name, const char *report_name,
                             uint32_t restart_time, uint32_t stats_period, int disable)
{
    fh_mgmt_sg_t *sg = NULL;

    sg = (fh_mgmt_sg_t *) malloc(sizeof(fh_mgmt_sg_t));
    if (!sg) {
        FH_LOG(MGMT, ERR, ("Failed to allocate memory for new service group: %s", name));
        return NULL;
    }

    memset(sg, 0, sizeof(fh_mgmt_sg_t));

    pthread_mutex_init(&sg->sg_rpt_lock, NULL);

    sg->sg_name = strdup(name);
    if (!sg->sg_name) {
        FH_LOG(MGMT, ERR, ("Failed to allocate memory for service group name: %s", name));
        free(sg);
        return NULL;
    }

    sg->sg_report_name = strdup(report_name);
    if (!sg->sg_report_name) {
        FH_LOG(MGMT, ERR, ("Failed to allocate memory for service group report name: %s", name));
        free(sg->sg_name);
        free(sg);
        return NULL;
    }

    TAILQ_INIT(&sg->sg_serv_lh);

    /*
     * Mark the service group enabled or disabled
     */
    if (disable) {
        sg->sg_flags |= FH_MGMT_SG_DISABLED;
    }
    else {
        sg->sg_flags |= FH_MGMT_SG_ENABLED;
    }

    /*
     * Check whether the service group requires an automatic daily restart
     */
    if (restart_time > 0) {
        fh_mgmt_sg_restart_time(sg, restart_time);
    }

    /*
     * Check whether the service group requires statistics monitoring
     */
    if (stats_period > 0) {
        fh_mgmt_sg_stats_period(sg, stats_period);
    }

    /* set the service group's respawn period based on global spawn delay parameter */
    sg->sg_respawn_period = sg_spawn_delay;

    /*
     * Mark all service groups stopped by default
     */
    sg->sg_flags |= FH_MGMT_SG_STOPPED;

    TAILQ_INSERT_TAIL(&sg_list, sg, sg_le);

    /* return the newly created service group */
    return sg;
}

/*
 * fh_mgmt_sg_add_serv
 *
 * Add a service to a service group
 */
FH_STATUS fh_mgmt_sg_add_serv(fh_mgmt_sg_t *sg, fh_mgmt_serv_t *serv)
{
    FH_ASSERT(sg && serv);

    if (serv->serv_group) {
        FH_LOG(MGMT, ERR, ("Service already added to an existing service group: %s",
                           serv->serv_group->sg_name));
        return FH_ERROR;
    }

    TAILQ_INSERT_TAIL(&sg->sg_serv_lh, serv, serv_le);

    /*
     * We are tracking services for the group by assigning a bit per service
     */
    FH_ASSERT(sg->sg_serv_count < 32);

    serv->serv_mask  = (1 << sg->sg_serv_count);
    serv->serv_group = sg;

    if (sg->sg_flags & FH_MGMT_SG_DISABLED) {
        if (!(serv->serv_flags & FH_MGMT_SERV_DISABLED)) {
            serv->serv_flags |= FH_MGMT_SERV_DISABLED;
            serv->serv_flags &= ~FH_MGMT_SERV_ENABLED;
            serv->serv_flags &= ~FH_MGMT_SERV_STOPPED;
        }
    }

    sg->sg_serv_count++;

    return FH_OK;
}

/*
 * fh_mgmt_sg_lookup
 *
 * Looks up a service group based on service group name.
 */
fh_mgmt_sg_t *fh_mgmt_sg_lookup(char *name)
{
    fh_mgmt_sg_t *sg = NULL;

    /*
     * Walk through all the service groups, and return the first matching name
     */
    TAILQ_FOREACH(sg, &sg_list, sg_le) {
        if (strcmp(sg->sg_name, name) == 0) {
            return sg;
        }
    }

    return NULL;
}

/*
 * Provides the complete list of service groups to the requestor
 */
sg_lh_t *fh_mgmt_sg_getall()
{
    return &sg_list;
}

/*-----------------------------------------------------------------------------*/
/* Service API                                                                 */
/*-----------------------------------------------------------------------------*/

/*
 * fh_mgmt_serv_new
 *
 * Creates a new service.
 */
fh_mgmt_serv_t *fh_mgmt_serv_new(const char *name, const char *command, const char *args,
                                 int disable, int stats, int shutdown, const char *action)
{
    fh_mgmt_serv_t *serv = NULL;

    /* Allocate a new service object */
    serv = (fh_mgmt_serv_t *) malloc(sizeof(fh_mgmt_serv_t));
    if (!serv) {
        FH_LOG(MGMT, ERR, ("Failed to allocate memory for new service: %s", name));
        return NULL;
    }

    memset(serv, 0, sizeof(fh_mgmt_serv_t));

    /* Save the service name */
    serv->serv_name = strdup(name);
    if (!serv->serv_name) {
        FH_LOG(MGMT, ERR, ("Failed to allocate memory for service name: %s", name));
        goto error;
    }

    /* Save the service command */
    serv->serv_command = strdup(command);
    if (!serv->serv_command) {
        FH_LOG(MGMT, ERR, ("Failed to allocate memory for service command: %s (serv:%s)",
                           command, name));
        goto error;
    }

    /* Enable/Disable the service */
    if (disable) {
        serv->serv_flags |= FH_MGMT_SERV_DISABLED;
    }
    else {
        serv->serv_flags |= FH_MGMT_SERV_ENABLED;
    }

    /* All services are marked STOPPED initially */
    serv->serv_flags |= FH_MGMT_SERV_STOPPED;

    /* determine whether the manager needs to gather periodic stats for this service */
    if (stats) {
        serv->serv_flags |= FH_MGMT_SERV_STATS;
    }

    /* determine whether the manager needs to kill this process on a normal exit */
    if (shutdown) {
        serv->serv_flags |= FH_MGMT_SERV_SHUTDOWN;
    }

    /* Flag the service to be either a WAIT or SPAWN */
    if (strcmp(action, "wait") == 0) {
        serv->serv_flags |= FH_MGMT_SERV_WAIT;
    }
    else if (strcmp(action, "spawn") == 0) {
        serv->serv_flags |= FH_MGMT_SERV_SPAWN;
    }
    else if (strcmp(action, "respawn") == 0) {
        serv->serv_flags |= FH_MGMT_SERV_RESPAWN;
    }
    else {
        FH_LOG(MGMT, ERR, ("Invalid service action: %s (serv:%s)", action, name));
        goto error;
    }

    /*
     * Command arguments are optional
     */
    if (args == NULL) {
        return serv;
    }

    /* Save the command arguments if present */
    serv->serv_args = strdup(args);
    if (!serv->serv_args) {
        FH_LOG(MGMT, ERR, ("Failed to allocate memory for service command args: %s (serv:%s)",
                           args, name));
        goto error;
    }

    return serv;

error:
    if (serv) {
        if (serv->serv_name)    free(serv->serv_name);
        if (serv->serv_command) free(serv->serv_command);
        if (serv->serv_args)    free(serv->serv_args);
        free(serv);
    }
    return NULL;
}

/*
 * serv_post
 *
 * Post an event/request to a service.
 */
static FH_STATUS serv_post(fh_mgmt_serv_t *serv, int req_cmd,  void *req_data,  int req_len)
{
    fh_mgmt_conn_t *conn = serv->serv_conn;
    FH_STATUS rc;

    FH_ASSERT(serv);

    conn->conn_req_tid++;

    /*
     * Send the request
     */
    rc = fh_adm_send(conn->conn_fd, req_cmd, conn->conn_req_tid, req_data, req_len);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to send %s request", FH_ADM_CMD_NAME(req_cmd)));
        return rc;
    }

    return FH_OK;
}

/*
 * serv_reqresp
 *
 * Make a request to a service. And, wait for the response.
 */
static FH_STATUS serv_reqresp(fh_mgmt_serv_t *serv,
                              int req_cmd,  void *req_data,  int req_len,
                              int resp_cmd, void *resp_data, int resp_len)
{
    fh_mgmt_conn_t *conn = serv->serv_conn;
    FH_STATUS rc;

    FH_ASSERT(conn);

    conn->conn_req_tid++;

    /*
     * Send the request
     */
    rc = fh_adm_send(conn->conn_fd, req_cmd, conn->conn_req_tid, req_data, req_len);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to send %s request", FH_ADM_CMD_NAME(req_cmd)));
        return rc;
    }

    /*
     * Wait for the response
     */
    rc = fh_adm_expect(conn->conn_fd, resp_cmd, conn->conn_req_tid, resp_data, resp_len);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to receive %s response", FH_ADM_CMD_NAME(resp_cmd)));
        return rc;
    }

    return FH_OK;
}


/*
 * serv_get_version
 *
 * Get the version of a given service
 */
static FH_STATUS serv_get_version(fh_mgmt_serv_t *serv, void *resp)
{
    fh_adm_getver_resp_t *getver_resp = (fh_adm_getver_resp_t *)resp;
    fh_adm_getver_req_t  getver_req;
    FH_STATUS            rc;

    /*
     * Request to get the FH manager revision information
     */
    if (serv->serv_flags & FH_MGMT_SERV_RUNNING) {
        strcpy(getver_req.getver_service, serv->serv_name);

        rc = serv_reqresp(serv,
                          FH_ADM_CMD_GETVER_REQ,  &getver_req, sizeof(getver_req),
                          FH_ADM_CMD_GETVER_RESP, getver_resp, sizeof(fh_adm_getver_resp_t));
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("Failed to send get version Req/Resp to service: %s",
                               serv->serv_name));
            return rc;
        }
    }
    else {
        strcpy(getver_resp->getver_service, serv->serv_name);
        getver_resp->getver_build_rev[0]  = '\0';
        getver_resp->getver_build_date[0] = '\0';
    }

    getver_resp->getver_state = serv->serv_flags;

    return FH_OK;
}

/*
 * serv_get_stats
 *
 * Get the statistics of a given service
 */
static FH_STATUS serv_get_stats(fh_mgmt_serv_t *serv, void *resp)
{
    fh_adm_stats_resp_t *stats_resp = (fh_adm_stats_resp_t *)resp;
    fh_adm_stats_req_t  stats_req;
    FH_STATUS           rc;

    if (serv->serv_flags & FH_MGMT_SERV_RUNNING) {
        strcpy(stats_req.stats_service, serv->serv_name);

        rc = serv_reqresp(serv,
                          FH_ADM_CMD_STATS_REQ,  &stats_req, sizeof(stats_req),
                          FH_ADM_CMD_STATS_RESP, stats_resp, sizeof(fh_adm_stats_resp_t));
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("Failed to send STATS Req/Resp to service: %s",
                               serv->serv_name));
            return rc;
        }
    }
    else {
        memset(stats_resp, 0, sizeof(fh_adm_stats_resp_t));
        strcpy(stats_resp->stats_service, serv->serv_name);
        stats_resp->stats_line_cnt = 0;
    }

    stats_resp->stats_state = serv->serv_flags;

    return FH_OK;
}

/*
 * serv_monitor
 *
 * Look at the system statistics to compute percentage CPU for the Fast-Path CPU
 * since each FH service gets assigned to its own FP CPU, and percentage memory.
 */
static void serv_monitor(fh_mgmt_serv_t *serv, uint32_t pid, int32_t fp_cpu, uint32_t fp_tid,
                         uint32_t *pmem, uint32_t *putime, uint32_t *pstime)
{
#define FH_MAX_CPUS (16)
    FH_STATUS rc;
    fh_mon_proc_t proc;
    fh_mon_sys_t  sys;
    fh_mon_mem_t  mem;
    fh_mon_cpu_t  cpu_table[FH_MAX_CPUS];
    fh_mon_cpu_t *cpu;

    *pmem   = 0;
    *putime = 0;
    *pstime = 0;

    /*
     * The FP thread was not fully up yet.
     */
    if (fp_tid == 0) {
        return;
    }

    rc = fh_mon_mem_stats(&mem);
    if (rc != FH_OK) {
        return;
    }

    rc = fh_mon_cpu_stats(&sys, cpu_table, FH_MAX_CPUS);
    if (rc != FH_OK) {
        return;
    }

    rc = fh_mon_proc_stats(&proc, pid, fp_tid);
    if (rc != FH_OK) {
        return;
    }

    if (fp_cpu != proc.proc_lcpu) {
        FH_LOG(MGMT, ERR, ("Service %s: last CPU: %d vs. FP CPU: %d",
                           serv->serv_name, proc.proc_lcpu, fp_cpu));
    }

    if (fp_cpu >= sys.sys_num_cpus) {
        FH_LOG(MGMT, ERR, ("Service %s: FP CPU: %d >= number of CPUs: %d",
                           serv->serv_name, fp_cpu, sys.sys_num_cpus));
        return;
    }

    cpu = &cpu_table[fp_cpu];

    uint64_t curr_cpu_total = cpu->cpu_user   +
                              cpu->cpu_system +
                              cpu->cpu_nice   +
                              cpu->cpu_idle   +
                              cpu->cpu_wait   +
                              cpu->cpu_hintr  +
                              cpu->cpu_sintr  +
                              cpu->cpu_steal;

    uint64_t prev_cpu_total = serv->serv_cpu.cpu_user   +
                              serv->serv_cpu.cpu_system +
                              serv->serv_cpu.cpu_nice   +
                              serv->serv_cpu.cpu_idle   +
                              serv->serv_cpu.cpu_wait   +
                              serv->serv_cpu.cpu_hintr  +
                              serv->serv_cpu.cpu_sintr  +
                              serv->serv_cpu.cpu_steal;

    if (prev_cpu_total != 0) {
        uint64_t cpu_total = curr_cpu_total - prev_cpu_total;

        /*
         * Compute the percentage CPU and memory for this process/thread
         */
        *pmem   = proc.proc_resident * 100 / mem.mem_total;
        *putime = (proc.proc_utime - serv->serv_proc.proc_utime) * 100 / cpu_total;
        if (*putime > 100) {
            *putime = 0;
        }
        *pstime = (proc.proc_stime - serv->serv_proc.proc_stime) * 100 / cpu_total;
        if (*pstime > 100) {
            *pstime = 0;
        }
    }

    memcpy(&serv->serv_proc, &proc, sizeof(fh_mon_proc_t));
    memcpy(&serv->serv_cpu,  cpu, sizeof(fh_mon_cpu_t));
}

/*
 * serv_get_status
 *
 * Get the status of a given service
 */
static FH_STATUS serv_get_status(fh_mgmt_serv_t *serv, void *resp)
{
    fh_adm_status_resp_t *status_resp = (fh_adm_status_resp_t *)resp;
    fh_adm_status_req_t   status_req;
    FH_STATUS             rc;

    memset(status_resp, 0, sizeof(fh_adm_status_resp_t));

    if ((serv->serv_flags & FH_MGMT_SERV_RUNNING) &&
        !(serv->serv_flags & FH_MGMT_SERV_STOPPING)) {
        strcpy(status_req.status_service, serv->serv_name);

        rc = serv_reqresp(serv,
                          FH_ADM_CMD_STATUS_REQ,  &status_req, sizeof(status_req),
                          FH_ADM_CMD_STATUS_RESP, status_resp, sizeof(fh_adm_status_resp_t));
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("Failed to send STATUS Req/Resp to service: %s",
                               serv->serv_name));
            return rc;
        }

        /* Start the system monitoring work */
        serv_monitor(serv,
                     status_resp->status_pid,
                     status_resp->status_fp_cpu,
                     status_resp->status_fp_tid,
                     &status_resp->status_pmem,
                     &status_resp->status_putime,
                     &status_resp->status_pstime);
    }
    else {
        strcpy(status_resp->status_service, serv->serv_name);
    }

    status_resp->status_state = serv->serv_flags;

    return FH_OK;
}

/*
 * serv_action
 *
 * Sends a action request to the service.. doesn't wait for any response.
 */
static FH_STATUS serv_action(fh_mgmt_serv_t *serv, uint32_t action_type)
{
    fh_adm_action_req_t  action_req;
    FH_STATUS            rc;

    action_req.action_type = action_type;

    rc = serv_post(serv, FH_ADM_CMD_ACTION_REQ, &action_req, sizeof(action_req));
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to send restart eventto service: %s",
                           serv->serv_name));
        return rc;
    }

    return FH_OK;
}


/*
 * serv_request
 *
 * Processes the service request and handle the response
 */
static FH_STATUS serv_request(fh_mgmt_serv_t *serv, fh_adm_serv_req_t  *serv_req,
                              fh_adm_serv_resp_t *serv_resp, void *ptr)
{
    FH_STATUS rc;

    switch (serv_req->serv_cmd) {
    case FH_ADM_CMD_GETVER_REQ:
        rc = serv_get_version(serv, ptr);

        if (rc == FH_OK) {
            serv_resp->serv_resp_cnt++;
        }
        break;

    case FH_ADM_CMD_STATS_REQ:
        rc = serv_get_stats(serv, ptr);

        if (rc == FH_OK) {
            serv_resp->serv_resp_cnt++;
        }
        break;

    case FH_ADM_CMD_STATUS_REQ:
        rc = serv_get_status(serv, ptr);

        if (rc == FH_OK) {
            serv_resp->serv_resp_cnt++;
        }
        break;

    /*
     * Service control commands
     */
    case FH_MGMT_CL_CTRL_ENABLE:
        rc = fh_mgmt_serv_enable(serv);
        break;

    case FH_MGMT_CL_CTRL_DISABLE:
        rc = fh_mgmt_serv_disable(serv);
        break;

    case FH_MGMT_CL_CTRL_START:
        rc = fh_mgmt_serv_start(serv);
        break;

    case FH_MGMT_CL_CTRL_STOP:
        rc = fh_mgmt_serv_admin_stop(serv);
        break;

    case FH_MGMT_CL_CTRL_RESTART:
        rc = fh_mgmt_serv_restart(serv);
        break;

    case FH_MGMT_CL_CTRL_CLRSTATS:
        rc = fh_mgmt_serv_clrstats(serv);
        break;

    default:
        rc = FH_ERROR;
    }

    return rc;
}

/*
 * fh_mgmt_serv_request
 *
 * Send a request to a downstream service
 */
FH_STATUS fh_mgmt_serv_request(fh_mgmt_serv_t      *serv,
                               fh_adm_serv_req_t   *serv_req,
                               fh_adm_serv_resp_t  *serv_resp, char *ptr)
{
    return serv_request(serv, serv_req, serv_resp, ptr);
}

/*
 * sg_request
 *
 * Make a service group request.
 */
static FH_STATUS sg_request(fh_mgmt_sg_t *sg,
                            fh_adm_serv_req_t   *serv_req,
                            fh_adm_serv_resp_t  *serv_resp, char **ptr)
{
    FH_STATUS       rc;
    char           *tmp_ptr = *ptr;
    fh_mgmt_serv_t *serv;

    TAILQ_FOREACH(serv, &sg->sg_serv_lh, serv_le) {
        if (serv->serv_flags & FH_MGMT_SERV_CLI) {
            /*
             * We only process daemon type services
             */
            rc = serv_request(serv, serv_req, serv_resp, tmp_ptr);
            if (rc != FH_OK) {
                return rc;
            }

            if (serv_resp) {
                if (serv_resp->serv_resp_cnt == FH_MGMT_MAX_SERVICES) {
                    FH_LOG(MGMT, ERR, ("Max number of services exceeded: %d",
                                       FH_MGMT_MAX_SERVICES));
                    return FH_ERROR;
                }

                tmp_ptr += serv_resp->serv_resp_size;
            }
        }
    }

    *ptr = tmp_ptr;

    return FH_OK;
}


/*
 * fh_mgmt_sg_request
 *
 * Make a service group request.
 */
FH_STATUS fh_mgmt_sg_request(fh_mgmt_sg_t *sg,
                             fh_adm_serv_req_t   *serv_req,
                             fh_adm_serv_resp_t  *serv_resp, char *ptr)
{
    return sg_request(sg, serv_req, serv_resp, &ptr);
}

/*
 * fh_mgmt_serv_request_all
 *
 * Send a request to all downstream services
 */
FH_STATUS fh_mgmt_serv_request_all(fh_adm_serv_req_t  *serv_req,
                                   fh_adm_serv_resp_t *serv_resp, char *ptr)
{
    fh_mgmt_sg_t *sg = NULL;
    FH_STATUS rc;

    /*
     * Walk through all the service groups
     */
    TAILQ_FOREACH(sg, &sg_list, sg_le) {
        rc = sg_request(sg, serv_req, serv_resp, &ptr);
        if (rc != FH_OK) {
            return rc;
        }
    }


    return FH_OK;
}

/*
 * fh_mgmt_serv_process
 *
 * Processes a request from a downstream service.
 */
FH_STATUS fh_mgmt_serv_process(fh_mgmt_serv_t *serv)
{
    fh_adm_cmd_t   *cmd = NULL;
    char           *data = NULL;
    fh_mgmt_conn_t *conn = serv->serv_conn;
    FH_STATUS       rc;

    if (!conn->conn_serv) {
        FH_LOG(MGMT, ERR, ("Connection fd:%d has no service attached", conn->conn_fd));
        return FH_ERROR;
    }

    FH_LOG(MGMT, DIAG, ("Service processing - fd:%d - serv:%s", conn->conn_fd,
                        serv->serv_name));

    /*
     * Receive the data from the socket
     */
    rc = fh_adm_recv(conn->conn_fd, &data);
    if (rc != FH_OK) {
        FH_LOG(MGMT, INFO, ("Failed to recv message from service: fd:%d serv:%s",
                            conn->conn_fd, serv->serv_name));
        return rc;
    }

    cmd = (fh_adm_cmd_t *) data;

    FH_LOG(MGMT, DIAG, ("Received command: %s - fd:%d serv:%s",
                        FH_ADM_CMD_NAME(cmd->cmd_type), conn->conn_fd,
                        serv->serv_name));

    /*
     * Handle the service commands (None are expected at this point)
     */
    switch (cmd->cmd_type) {
    default:
        FH_LOG(MGMT, ERR, ("Invalid service command: %d (serv:%s)",
                           cmd->cmd_type, serv->serv_name));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_mgmt_serv_lookup
 *
 * Looks up a service based on service name.
 */
fh_mgmt_serv_t *fh_mgmt_serv_lookup(char *serv_name)
{
    fh_mgmt_sg_t *sg = NULL;

    /*
     * Lookup through all the service groups and service within these service
     * groups. This is completely inefficient, but we will have to create a
     * htable of all the services, but for now, performance is not an issue
     * considering the number of services that we are managing.
     */
    TAILQ_FOREACH(sg, &sg_list, sg_le) {
        fh_mgmt_serv_t *serv;

        TAILQ_FOREACH(serv, &sg->sg_serv_lh, serv_le) {
            if (strcmp(serv->serv_name, serv_name) == 0) {
                return serv;
            }
        }
    }

    return NULL;
}

static int child_rc = 0;

/*
 * serv_child_handler
 *
 * Signal handler for the child process that scp's the listedoptions.txt
 */
static void serv_child_handler(int signal)
{
    int status = 0;

    /* Suppress compiler warning */
    if (signal) {};

    wait(&status);
    child_rc = status;
}


/*
 * fh_mgmt_serv_start
 *
 * Start a given service.
 */
FH_STATUS fh_mgmt_serv_start(fh_mgmt_serv_t *serv)
{
    int status;
    char full_command[1024];

    if (serv->serv_flags & FH_MGMT_SERV_DISABLED) {
        FH_LOG(MGMT, WARN, ("Service '%s' is DISABLED", serv->serv_name));
        return FH_OK;
    }

    if (!(serv->serv_flags & (FH_MGMT_SERV_STOPPED | FH_MGMT_SERV_ADMIN_STOPPED))) {
        FH_LOG(MGMT, WARN, ("Service '%s' not marked STOPPED", serv->serv_name));
        return FH_OK;
    }

    signal(SIGCHLD, serv_child_handler);

    sprintf(full_command, "%s %s", serv->serv_command, serv->serv_args ? : "");

    status = system(full_command);

    if (status != 0) {
        if (errno == ECHILD) {
            status = child_rc;
        }

        if (status != 0) {
            FH_LOG(MGMT, ERR, ("Service '%s' failed with status: %d", serv->serv_name, status));
            return FH_ERROR;
        }
    }

    /*
     * Wait for the child process to finish if the service is configured as
     * WAIT as opposed to SPAWN.
     */
    if (serv->serv_flags & FH_MGMT_SERV_WAIT) {
        FH_LOG(MGMT, STATE, ("Service '%s' started and finished successfully", serv->serv_name));
    }
    else {
        serv->serv_flags &= ~(FH_MGMT_SERV_STOPPED | FH_MGMT_SERV_ADMIN_STOPPED);
        serv->serv_flags |= FH_MGMT_SERV_STARTED;

        FH_LOG(MGMT, STATE, ("Service '%s' state: STARTED", serv->serv_name));
    }


    return FH_OK;
}

/*
 * fh_mgmt_serv_stop
 *
 * Stop a given service
 */
FH_STATUS fh_mgmt_serv_stop(fh_mgmt_serv_t *serv)
{
    FH_STATUS rc;

    if (serv->serv_flags & (FH_MGMT_SERV_STOPPED | FH_MGMT_SERV_ADMIN_STOPPED)) {
        FH_LOG(MGMT, WARN, ("Service '%s' already STOPPED", serv->serv_name));
        return FH_OK;
    }

    if (!(serv->serv_flags & FH_MGMT_SERV_RUNNING)) {
        FH_LOG(MGMT, WARN, ("Service '%s' not RUNNING", serv->serv_name));
        return FH_OK;
    }

    /* send the stop command to the service */
    rc = serv_action(serv, FH_MGMT_CL_CTRL_STOP);
    if (rc != FH_OK) {
        return rc;
    }

    serv->serv_flags |= FH_MGMT_SERV_STOPPING;

    /* force the group that this service belongs to to reload its stats report */
    serv->serv_group->sg_stats_ticks = serv->serv_group->sg_stats_period;

    return FH_OK;
}

/*
 * Administratively (manually, from the command line) stop a given service
 */
FH_STATUS fh_mgmt_serv_admin_stop(fh_mgmt_serv_t *serv)
{
    FH_STATUS rc;

    if (serv->serv_flags & (FH_MGMT_SERV_STOPPED | FH_MGMT_SERV_ADMIN_STOPPED)) {
        FH_LOG(MGMT, WARN, ("Service '%s' already STOPPED", serv->serv_name));
        return FH_OK;
    }

    if (!(serv->serv_flags & FH_MGMT_SERV_RUNNING)) {
        FH_LOG(MGMT, WARN, ("Service '%s' not RUNNING", serv->serv_name));
        return FH_OK;
    }

    /* send the stop command to the service */
    rc = serv_action(serv, FH_MGMT_CL_CTRL_STOP);
    if (rc != FH_OK) {
        return rc;
    }

    serv->serv_flags |= FH_MGMT_SERV_ADMIN_STOPPING;

    /* force the group that this service belongs to to reload its stats report */
    serv->serv_group->sg_stats_ticks = serv->serv_group->sg_stats_period;

    return FH_OK;
}

/*
 * fh_mgmt_serv_restart
 *
 * Stops the service if currently running, and mark the service ready for a
 * restart, when the service is fully stopped.
 */
FH_STATUS fh_mgmt_serv_restart(fh_mgmt_serv_t *serv)
{
    FH_STATUS rc;

    if (serv->serv_flags & (FH_MGMT_SERV_STOPPED | FH_MGMT_SERV_ADMIN_STOPPED)) {
        FH_LOG(MGMT, WARN, ("Service '%s' already STOPPED.. starting it", serv->serv_name));
        return fh_mgmt_serv_start(serv);
    }

    rc = fh_mgmt_serv_stop(serv);
    if (rc != FH_OK) {
        return rc;
    }

    /*
     * Mark the service RESTART, so when it is disconnecting we can restart the
     * service. We have to wait until it is down before starting a new instance
     * of the service.
     */
    serv->serv_flags |= FH_MGMT_SERV_RESTART;

    return FH_OK;
}

/*
 * fh_mgmt_serv_clrstats
 *
 * Clear the service statistics.
 */
FH_STATUS fh_mgmt_serv_clrstats(fh_mgmt_serv_t *serv)
{
    FH_STATUS rc;

    if (!(serv->serv_flags & FH_MGMT_SERV_RUNNING)) {
        FH_LOG(MGMT, WARN, ("Service '%s' not RUNNING", serv->serv_name));
        return FH_OK;
    }

    /* Send the stop command to the service */
    rc = serv_action(serv, FH_MGMT_CL_CTRL_CLRSTATS);
    if (rc != FH_OK) {
        return rc;
    }

    return FH_OK;
}

/*
 * fh_mgmt_serv_disable
 *
 * Disable a given service.
 */
FH_STATUS fh_mgmt_serv_disable(fh_mgmt_serv_t *serv)
{
    fh_mgmt_sg_t *sg = serv->serv_group;

    FH_ASSERT(sg);

    if (sg->sg_flags & FH_MGMT_SG_DISABLED) {
        FH_ASSERT(serv->serv_flags & FH_MGMT_SERV_DISABLED);

        FH_LOG(MGMT, WARN, ("Service '%s' parent group (%s) already DISABLED",
                           serv->serv_name, sg->sg_name));

        return FH_OK;
    }

    if (serv->serv_flags & FH_MGMT_SERV_DISABLED) {
        FH_LOG(MGMT, WARN, ("Service '%s' already DISABLED", serv->serv_name));
        return FH_OK;
    }

    fh_mgmt_serv_stop(serv);

    serv->serv_flags &= ~FH_MGMT_SERV_ENABLED;
    serv->serv_flags |= FH_MGMT_SERV_DISABLED;

    FH_LOG(MGMT, STATE, ("Service '%s' state: DISABLED", serv->serv_name));

    return FH_OK;
}

/*
 * fh_mgmt_serv_enable
 *
 * Enable a given service.
 */
FH_STATUS fh_mgmt_serv_enable(fh_mgmt_serv_t *serv)
{
    fh_mgmt_sg_t *sg = serv->serv_group;

    FH_ASSERT(sg);

    if (sg->sg_flags & FH_MGMT_SG_DISABLED) {
        FH_LOG(MGMT, ERR, ("Service '%s' parent group (%s) is DISABLED",
                           serv->serv_name, sg->sg_name));
        return FH_ERROR;
    }

    if (serv->serv_flags & FH_MGMT_SERV_ENABLED) {
        FH_LOG(MGMT, WARN, ("Service '%s' already ENABLED", serv->serv_name));
        return FH_OK;
    }

    serv->serv_flags &= ~FH_MGMT_SERV_DISABLED;
    serv->serv_flags |= FH_MGMT_SERV_ENABLED;

    FH_LOG(MGMT, STATE, ("Service '%s' state: ENABLED", serv->serv_name));

    return fh_mgmt_serv_start(serv);
}

/*
 * fh_mgmt_serv_attach
 *
 * Attach the connection to the corresponding service.
 */
FH_STATUS fh_mgmt_serv_attach(fh_mgmt_serv_t *serv, fh_mgmt_conn_t *conn)
{
    if (!(serv->serv_flags & FH_MGMT_SERV_STARTED)) {
        if (!(serv->serv_flags & FH_MGMT_SERV_STOPPED)) {
            FH_LOG(MGMT, ERR, ("Service '%s' not marked STARTED", serv->serv_name));
            return FH_ERROR;
        }

        serv->serv_flags &= ~FH_MGMT_SERV_STOPPED;
        serv->serv_flags |= FH_MGMT_SERV_STARTED;

        FH_LOG(MGMT, STATE, ("Service '%s' state: STARTED (Standalone)", serv->serv_name));
    }

    serv->serv_flags &= ~FH_MGMT_SERV_STARTED;
    serv->serv_flags |= FH_MGMT_SERV_RUNNING;

    serv->serv_conn = conn;
    conn->conn_serv = serv;

    FH_LOG(MGMT, STATE, ("Service '%s' state: RUNNING", serv->serv_name));

    return FH_OK;
}

/*
 * fh_mgmt_serv_detach
 *
 * Detach the connection from the corresponding service
 */
FH_STATUS fh_mgmt_serv_detach(fh_mgmt_serv_t *serv)
{
    fh_mgmt_conn_t *conn = serv->serv_conn;

    if (!(serv->serv_flags & FH_MGMT_SERV_RUNNING)) {
        FH_LOG(MGMT, ERR, ("Service '%s' not marked RUNNING", serv->serv_name));
        return FH_ERROR;
    }

    if (serv->serv_flags & FH_MGMT_SERV_ADMIN_STOPPING) {
        serv->serv_flags |= FH_MGMT_SERV_ADMIN_STOPPED;
    }
    else {
        serv->serv_flags |= FH_MGMT_SERV_STOPPED;
    }

    serv->serv_flags &= ~(FH_MGMT_SERV_RUNNING|FH_MGMT_SERV_STOPPING|FH_MGMT_SERV_ADMIN_STOPPING);

    serv->serv_conn = NULL;
    conn->conn_serv = NULL;

    FH_LOG(MGMT, STATE, ("Service '%s' state: STOPPED", serv->serv_name));

    /*
     * If the service was marked for restart, spawn the service right away
     * We also need to clear the RESTART mask.
     */
    if (serv->serv_flags & FH_MGMT_SERV_RESTART) {
        serv->serv_flags &= ~FH_MGMT_SERV_RESTART;
        fh_mgmt_serv_start(serv);
    }

    return FH_OK;
}

/*
 * serv_dump
 *
 * Dump a service configuration.
 */
static void serv_dump(int i, fh_mgmt_serv_t *serv)
{
    FH_LOG_PGEN(DIAG, ("  ------ Service [%d] -------", i));
    FH_LOG_PGEN(DIAG, ("    - Name      : %s", serv->serv_name));
    FH_LOG_PGEN(DIAG, ("    - Mask      : 0x%08X", serv->serv_mask));
    FH_LOG_PGEN(DIAG, ("    - Flags     : 0x%08X", serv->serv_flags));
    FH_LOG_PGEN(DIAG, ("    - Command   : %s", serv->serv_command));
    FH_LOG_PGEN(DIAG, ("    - Args      : %s", serv->serv_args ? : "N/A"));
}

/*
 * sg_stop
 *
 * Stop a service group and all its services.
 */
void sg_stop(fh_mgmt_sg_t *sg)
{
    fh_mgmt_serv_t *serv;

    TAILQ_FOREACH(serv, &sg->sg_serv_lh, serv_le) {
        fh_mgmt_serv_stop(serv);
    }
}

/*
 * sg_restart
 *
 * Restart a service group and all its services.
 */
void sg_restart(fh_mgmt_sg_t *sg)
{
    fh_mgmt_serv_t *serv;

    TAILQ_FOREACH(serv, &sg->sg_serv_lh, serv_le) {
        fh_mgmt_serv_restart(serv);
    }
}


/*
 * sg_dump
 *
 * Dump a service group and all its services.
 */
static void sg_dump(fh_mgmt_sg_t *sg)
{
    fh_mgmt_serv_t *serv;
    int i = 0;

    FH_LOG_PGEN(DIAG, ("-----------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("> Service Group       : %s", sg->sg_name));
    FH_LOG_PGEN(DIAG, ("-----------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, (" - Number of services : %d", sg->sg_serv_count));
    FH_LOG_PGEN(DIAG, (" - Service Up Mask    : 0x%08X", sg->sg_up_mask));
    FH_LOG_PGEN(DIAG, (" - Flags              : 0x%08x", sg->sg_flags));
    FH_LOG_PGEN(DIAG, (" - Stats interval     : %d secs", sg->sg_stats_period));
    FH_LOG_PGEN(DIAG, (" - Restart time       : %d secs after midnight", sg->sg_restart_time));
    FH_LOG_PGEN(DIAG, ("-----------------------------------------------------------"));

    TAILQ_FOREACH(serv, &sg->sg_serv_lh, serv_le) {
        serv_dump(i, serv);
        i++;
    }
}

/*
 * fh_mgmt_sg_dump
 *
 * Dump all service groups
 */
void fh_mgmt_sg_dump()
{
    fh_mgmt_sg_t *sg = NULL;

    /*
     * Walk through all the service groups
     */
    TAILQ_FOREACH(sg, &sg_list, sg_le) {
        sg_dump(sg);
    }
}

/*
 * fh_mgmt_sgt_resp
 *
 * Builds a service group tree response for the CLI.
 */
fh_adm_sgt_resp_t *fh_mgmt_sgt_resp()
{
    fh_mgmt_sg_t      *sg = NULL;
    fh_adm_sgt_resp_t *sgt_resp = NULL;
    int                resp_size = 0;

    /*
     * Walk through all the service groups, and compute the size of the memory
     * area we need to allocate for the service group response. Note that this
     * is the maximum size for the response, we might be skipping some services
     * when going through each service, if the service is not supposed to be
     * managed by the CLI.
     */
    TAILQ_FOREACH(sg, &sg_list, sg_le) {
        resp_size += sizeof(fh_adm_sg_resp_t);
        resp_size += sg->sg_serv_count * sizeof(fh_adm_sg_serv_resp_t);
    }

    /*
     * Allocate the service group response memory
     */
    sgt_resp = (fh_adm_sgt_resp_t *) malloc(resp_size + sizeof(fh_adm_sgt_resp_t));
    if (!sgt_resp) {
        FH_LOG(MGMT, ERR, ("Failed to allocate memory for CLI service group response: size:%d", resp_size));
        return NULL;
    }

    sgt_resp->sgt_size  = 0;
    sgt_resp->sgt_count = 0;

    {
        char *ptr = (char *)(sgt_resp+1);

        /*
         * Walk through all the service groups
         */
        TAILQ_FOREACH(sg, &sg_list, sg_le) {
            fh_adm_sg_resp_t *sg_resp = (fh_adm_sg_resp_t *)ptr;
            fh_mgmt_serv_t *serv;

            sgt_resp->sgt_count++;
            sgt_resp->sgt_size += sizeof(fh_adm_sg_resp_t);

            strcpy(sg_resp->sg_name, sg->sg_name);
            sg_resp->sg_serv_count = 0;
            sg_resp->sg_serv_size  = 0;

            FH_LOG(MGMT, DIAG, ("Add service group to resp: [%d] = %s serv_count:%d",
                                sgt_resp->sgt_count, sg->sg_name, sg->sg_serv_count));

            ptr += sizeof(fh_adm_sg_resp_t);

            /*
             * Walk through all the services from each group
             */
            TAILQ_FOREACH(serv, &sg->sg_serv_lh, serv_le) {
                fh_adm_sg_serv_resp_t *sg_serv_resp = (fh_adm_sg_serv_resp_t *)ptr;

                /* Only send the services that are manageable via CLI */
                if (serv->serv_flags & FH_MGMT_SERV_CLI) {
                    strcpy(sg_serv_resp->serv_name, serv->serv_name);

                    sg_resp->sg_serv_count++;
                    sg_resp->sg_serv_size += sizeof(fh_adm_sg_serv_resp_t);

                    sgt_resp->sgt_size += sizeof(fh_adm_sg_serv_resp_t);

                    ptr += sizeof(fh_adm_sg_serv_resp_t);
                }
            }
        }
    }

    return sgt_resp;
}

/*
 * default_stats_report
 *
 * Default reporting plugin, if no other stats reporting plugin has been defined
 */
void default_stats_report(fh_mgmt_sg_rpt_t *sg_rpt)
{
    if (FH_LL_OK(MGMT, STATS)) {
        uint32_t i;

        FH_LOG_PGEN(DIAG, ("----------------------------------------------------"));
        FH_LOG_PGEN(DIAG, ("> Report: %s", sg_rpt->sg_rpt_name));
        FH_LOG_PGEN(DIAG, ("----------------------------------------------------"));

        FH_LOG_PGEN(DIAG, ("> Global Statistics:"));

        FH_LOG_PGEN(DIAG, ("  - Total Rx Packets    : %lld", LLI(sg_rpt->sg_rpt_total.line_pkt_rx)));
        FH_LOG_PGEN(DIAG, ("  - Total Rx Duplicates : %lld", LLI(sg_rpt->sg_rpt_total.line_pkt_dups)));
        FH_LOG_PGEN(DIAG, ("  - Total Rx Late       : %lld", LLI(sg_rpt->sg_rpt_total.line_pkt_late)));
        FH_LOG_PGEN(DIAG, ("  - Total Rx Bytes      : %lld", LLI(sg_rpt->sg_rpt_total.line_bytes)));
        FH_LOG_PGEN(DIAG, ("  - Total Rx Errors     : %lld", LLI(sg_rpt->sg_rpt_total.line_pkt_errs)));
        FH_LOG_PGEN(DIAG, ("  - Total Rx Messages   : %lld", LLI(sg_rpt->sg_rpt_total.line_msg_rx)));

        FH_LOG_PGEN(DIAG, ("> Line Statistics:"));

        for (i=0; i<sg_rpt->sg_rpt_line_count; i++) {
            fh_adm_line_stats_t *line = &sg_rpt->sg_rpt_lines[i];

            FH_LOG_PGEN(DIAG, ("  # Line: %s", line->line_name));
            FH_LOG_PGEN(DIAG, ("    - Rx Packets        : %lld", LLI(line->line_pkt_rx)));
            FH_LOG_PGEN(DIAG, ("    - Rx Duplicates     : %lld", LLI(line->line_pkt_dups)));
            FH_LOG_PGEN(DIAG, ("    - Rx Late           : %lld", LLI(line->line_pkt_late)));
            FH_LOG_PGEN(DIAG, ("    - Rx Bytes          : %lld", LLI(line->line_bytes)));
            FH_LOG_PGEN(DIAG, ("    - Rx Errors         : %lld", LLI(line->line_pkt_errs)));
            FH_LOG_PGEN(DIAG, ("    - Rx Messages       : %lld", LLI(line->line_msg_rx)));
        }
        FH_LOG_PGEN(DIAG, ("----------------------------------------------------"));
    }
}

/*
 * sg_stats_report
 *
 * Service group statistics report.
 */
static void sg_stats_report(fh_mgmt_sg_t *sg)
{
    fh_mgmt_serv_t     *serv;
    uint32_t            i = 0;
    fh_adm_stats_resp_t stats_resp;
    fh_mgmt_sg_rpt_t    sg_rpt;
    FH_STATUS           rc;

    /*
     * Initialize the service group report
     */
    memset(&sg_rpt, 0, sizeof(sg_rpt));

    strcpy(sg_rpt.sg_rpt_name, sg->sg_report_name);

    /*
     * Walk through all the services from the service group
     */
    TAILQ_FOREACH(serv, &sg->sg_serv_lh, serv_le) {
        /*
         * Skip the services that are not up and running
         */
        if (!(serv->serv_flags & FH_MGMT_SERV_RUNNING) ||
            (serv->serv_flags & FH_MGMT_SERV_STOPPING)) {
            continue;
        }

        rc = serv_get_stats(serv, &stats_resp);
        if (rc != FH_OK) {
            continue;
        }

        if ((sg_rpt.sg_rpt_line_count+stats_resp.stats_line_cnt) > SG_MAX_LINES) {
            FH_LOG(MGMT, ERR, ("Service group: %s has more lines than expected: (%d + %d) > %d",
                               sg->sg_name, sg_rpt.sg_rpt_line_count,
                               stats_resp.stats_line_cnt, SG_MAX_LINES));
            exit(1);
        }

        /*
         * Accumulate the line statistics
         */
        memcpy(&sg_rpt.sg_rpt_lines[sg_rpt.sg_rpt_line_count], &stats_resp.stats_lines[0],
               stats_resp.stats_line_cnt * sizeof(fh_adm_line_stats_t));

        for (i=0; i<stats_resp.stats_line_cnt; i++) {
            fh_adm_line_stats_t *total_line = &sg_rpt.sg_rpt_total;
            fh_adm_line_stats_t *line       = &stats_resp.stats_lines[i];

            total_line->line_pkt_errs += line->line_pkt_errs;
            total_line->line_pkt_rx   += line->line_pkt_rx;
            total_line->line_pkt_dups += line->line_pkt_dups;
            total_line->line_msg_rx   += line->line_msg_rx;
            total_line->line_bytes    += line->line_bytes;

            sg_rpt.sg_rpt_line_count ++;
        }
    }

    fh_mgmt_sg_rpt_save(sg, &sg_rpt);

    if (mgmt_stats_report) {
        mgmt_stats_report(&rc, &sg_rpt);
    }
    else {
        default_stats_report(&sg_rpt);
    }
}

/*
 * sg_serv_monitor
 *
 * Service group monitoring. If any service that is marked 'respawn' and is
 * STOPPED, then we need to start it.
 */
static void sg_serv_monitor(fh_mgmt_sg_t *sg)
{
    fh_mgmt_serv_t *serv = NULL;

    /* if it is time to attempt a respawn */
    if (sg->sg_respawn_ticks >= sg->sg_respawn_period) {
        /* walk through all the services from the service group */
        TAILQ_FOREACH(serv, &sg->sg_serv_lh, serv_le) {
            /* skip the services that are not up and running */
            if ((serv->serv_flags & FH_MGMT_SERV_RESPAWN) &&
                (serv->serv_flags & FH_MGMT_SERV_STOPPED)) {
                (void) fh_mgmt_serv_start(serv);
            }
        }
        sg->sg_respawn_ticks = 0;
    }
    /* otherwise, just increment the number of ticks */
    else {
        sg->sg_respawn_ticks++;
    }
}


/*
 * fh_mgmt_sg_rpt_save
 *
 * Save the current report to the report snapshot structure. Protected by mutex
 * so we can have a consistent view of the report.
 */
void fh_mgmt_sg_rpt_save(fh_mgmt_sg_t *sg, fh_mgmt_sg_rpt_t *sg_rpt)
{
    pthread_mutex_lock(&sg->sg_rpt_lock);
    memcpy(&sg->sg_rpt, sg_rpt, sizeof(fh_mgmt_sg_rpt_t));
    pthread_mutex_unlock(&sg->sg_rpt_lock);
}

/*
 * fh_mgmt_sg_rpt_snapshot
 *
 * Copy the current saved report to the provided structure.
 */
void fh_mgmt_sg_rpt_snapshot(fh_mgmt_sg_t *sg, fh_mgmt_sg_rpt_t *sg_rpt)
{
    pthread_mutex_lock(&sg->sg_rpt_lock);
    memcpy(sg_rpt, &sg->sg_rpt, sizeof(fh_mgmt_sg_rpt_t));
    pthread_mutex_unlock(&sg->sg_rpt_lock);
}

/*
 * fh_mgmt_sg_rpt_reset
 *
 * Reset the current saved report to all zeroes
 */
void fh_mgmt_sg_rpt_reset(fh_mgmt_sg_t *sg)
{
    pthread_mutex_lock(&sg->sg_rpt_lock);
    memset(&sg->sg_rpt, 0, sizeof(fh_mgmt_sg_rpt_t));
    pthread_mutex_unlock(&sg->sg_rpt_lock);
}

/*
 * fh_mgmt_sg_timers
 *
 * Check the service group timers. This function is called every second, so we
 * can decrement the tick values every second.
 */
void fh_mgmt_sg_timers()
{
    fh_mgmt_sg_t *sg = NULL;

    /*
     * Walk through all the service group timers
     */
    TAILQ_FOREACH(sg, &sg_timers, sg_timer_le) {
        /* Check whether some of the services that are marked 'respawn' need to */
        /* be restarted/started */
        if (sg->sg_flags & FH_MGMT_SG_ENABLED) {
            sg_serv_monitor(sg);
        }

        /* Take care of the statistics monitoring timer */
        if (sg->sg_flags & FH_MGMT_SG_STATS) {
            sg->sg_stats_ticks++;

            if (sg->sg_stats_ticks >= sg->sg_stats_period) {
                sg_stats_report(sg);

                sg->sg_stats_ticks = 0;
            }
        }

        /* Take care of the restart timer */
        if (sg->sg_flags & FH_MGMT_SG_RESTART) {
            sg->sg_restart_ticks++;

            if (sg->sg_restart_ticks == sg->sg_restart_deadline) {
                FH_LOG(MGMT, DIAG, ("Service group %s: restart timer expiration",
                                    sg->sg_name));

                sg_restart(sg);

                sg->sg_restart_ticks = 0;
            }
        }
    }
}

/*
 * Perform initialization post-processing to clean up anything changed by a plugin
 */
void fh_mgmt_serv_post_init()
{
    fh_mgmt_sg_t *sg = NULL;

    /* walk through all of the service groups */
    TAILQ_FOREACH(sg, &sg_list, sg_le) {
        /* if stat gathering or restart flags are set, put this SG in the timer list */
        if (sg->sg_flags & FH_MGMT_SG_ENABLED) {
            if (sg->sg_flags & (FH_MGMT_SG_STATS|FH_MGMT_SG_RESTART)) {
                TAILQ_INSERT_TAIL(&sg_timers, sg, sg_timer_le);
            }
        }
    }
}


