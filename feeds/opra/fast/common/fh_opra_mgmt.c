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

/*
 * System includes
 */
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*
 * FH common includes
 */
#include "fh_log.h"
#include "fh_time.h"
#include "fh_plugin.h"
#include "fh_mgmt_admin.h"
#include "fh_mgmt_client.h"

/*
 * OPRA FH includes
 */
#include "fh_opra.h"
#include "fh_opra_cfg.h"
#include "fh_opra_mgmt.h"
#include "fh_opra_lh.h"
#include "fh_opra_stats.h"
#include "fh_opra_lo.h"
#include "fh_opra_revision.h"

static pthread_t    opra_mgmt_tid = 0;
static fh_mgmt_cl_t opra_mgmt_cl;
static uint64_t     opra_mgmt_uptime = 0;

/*
 * Make sure that we wake-up enough to catch up the restart command
 */
#define FH_MGMT_HZ    (1)
#define FH_MGMT_SLEEP (1000000 / FH_MGMT_HZ)

/*
 * opra_mgmt_stats
 *
 * Get the OPRA process statistics
 */
static FH_STATUS opra_mgmt_stats(fh_adm_cmd_t *cmd, char *cmd_buf, int cmd_len)
{
    fh_adm_stats_req_t  stats_req;
    fh_adm_stats_resp_t stats_resp;
    FH_STATUS rc;

    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_STATS_REQ);

    /*
     * Command length includes the full payload, command header + body
     */
    FH_ASSERT(cmd_len == sizeof(stats_req));

    /*
     * Parse the request
     */
    rc = fh_adm_parse(cmd->cmd_type, cmd_buf, &stats_req);
    if (rc != FH_OK) {
        return rc;
    }

    FH_LOG(MGMT, DIAG, ("Get OPRA STATS request"));

    strcpy(stats_resp.stats_service, stats_req.stats_service);

    fh_opra_lh_get_stats(&stats_resp);

    /*
     * Send the response
     */
    rc = fh_adm_send(opra_mgmt_cl.mcl_fd, FH_ADM_CMD_STATS_RESP,
                     cmd->cmd_tid, &stats_resp, sizeof(stats_resp));

    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to send STATS response to"));
        return rc;
    }

    FH_LOG(MGMT, DIAG, ("Sent STATS response"));

    return FH_OK;
}


/*
 * opra_mgmt_status
 *
 * Get the OPRA process status
 */
static FH_STATUS opra_mgmt_status(fh_adm_cmd_t *cmd, char *cmd_buf, int cmd_len)
{
    fh_adm_status_req_t  status_req;
    fh_adm_status_resp_t status_resp;
    FH_STATUS rc;

    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_STATUS_REQ);

    /*
     * Command length includes the full payload, command header + body
     */
    FH_ASSERT(cmd_len == sizeof(status_req));

    /*
     * Parse the request
     */
    rc = fh_adm_parse(cmd->cmd_type, cmd_buf, &status_req);
    if (rc != FH_OK) {
        return rc;
    }

    FH_LOG(MGMT, DIAG, ("Get OPRA STATUS request"));

    strcpy(status_resp.status_service, status_req.status_service);
    status_resp.status_pid      = getpid();
    status_resp.status_fp_cpu   = opra_cfg.ocfg_procs[opra_cfg.ocfg_proc_id].op_cpu;
    status_resp.status_fp_tid   = fh_opra_lh_get_tid();
    status_resp.status_uptime   = (uint32_t)(opra_mgmt_uptime / 1000000);

    // Computed by FH Manager
    status_resp.status_pmem     = 0;
    status_resp.status_putime   = 0;
    status_resp.status_pstime   = 0;

    /*
     * Send the response
     */
    rc = fh_adm_send(opra_mgmt_cl.mcl_fd, FH_ADM_CMD_STATUS_RESP,
                     cmd->cmd_tid, &status_resp, sizeof(status_resp));

    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to send STATUS response to"));
        return rc;
    }

    FH_LOG(MGMT, DIAG, ("Sent STATUS response"));

    return FH_OK;
}

/*
 * opra_mgmt_get_version
 *
 * Get the OPRA version information
 */
static FH_STATUS opra_mgmt_get_version(fh_adm_cmd_t *cmd, char *cmd_buf, int cmd_len)
{
    fh_adm_getver_req_t  getver_req;
    fh_adm_getver_resp_t getver_resp;
    FH_STATUS rc;

    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_GETVER_REQ);

    /*
     * Command length includes the full payload, command header + body
     */
    FH_ASSERT(cmd_len == sizeof(getver_req));

    /*
     * Parse the request
     */
    rc = fh_adm_parse(cmd->cmd_type, cmd_buf, &getver_req);
    if (rc != FH_OK) {
        return rc;
    }

    FH_LOG(MGMT, DIAG, ("Get OPRA version request for: %s",
                        getver_req.getver_service));

    strcpy(getver_resp.getver_service,    getver_req.getver_service);
    strcpy(getver_resp.getver_build_date, BUILD_DATE);
    strcpy(getver_resp.getver_build_rev,  BUILD_REV);

    /*
     * Send the response
     */
    rc = fh_adm_send(opra_mgmt_cl.mcl_fd, FH_ADM_CMD_GETVER_RESP,
                     cmd->cmd_tid, &getver_resp, sizeof(getver_resp));

    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to send 'get version' response to"));
        return rc;
    }

    FH_LOG(MGMT, DIAG, ("Sent 'get version' response for: %s",
                        getver_req.getver_service));

    return FH_OK;
}

/*
 * opra_mgmt_action_req
 *
 * Handles the action request from the FH manager.
 */
static FH_STATUS opra_mgmt_action_req(fh_adm_cmd_t *cmd, char *cmd_buf, int cmd_len)
{
    fh_adm_action_req_t  action_req;
    FH_STATUS rc;

    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_ACTION_REQ);

    /*
     * Command length includes the full payload, command header + body
     */
    FH_ASSERT(cmd_len == sizeof(action_req));

    /*
     * Parse the request
     */
    rc = fh_adm_parse(cmd->cmd_type, cmd_buf, &action_req);
    if (rc != FH_OK) {
        return rc;
    }

    switch (action_req.action_type) {
    case FH_MGMT_CL_CTRL_CLRSTATS:
        fh_opra_lh_clr_stats();
        break;

    case FH_MGMT_CL_CTRL_STOP:
        opra_stopped = 1;
        break;

    default:
        FH_LOG(MGMT, ERR, ("Unsupported action type: %d", action_req.action_type));
        break;
    }

    return rc;
}


/*
 * opra_mgmt_process
 *
 * Callback for management processing.
 */
static FH_STATUS opra_mgmt_process(char *cmd_buf, int cmd_len)
{
    fh_adm_cmd_t *cmd = (fh_adm_cmd_t *) cmd_buf;
    FH_STATUS     rc = FH_OK;

    switch (cmd->cmd_type) {
    case FH_ADM_CMD_STATS_REQ:
        rc = opra_mgmt_stats(cmd, cmd_buf, cmd_len);
        break;

    case FH_ADM_CMD_STATUS_REQ:
        rc = opra_mgmt_status(cmd, cmd_buf, cmd_len);
        break;

    case FH_ADM_CMD_GETVER_REQ:
        rc = opra_mgmt_get_version(cmd, cmd_buf, cmd_len);
        break;

    case FH_ADM_CMD_ACTION_REQ:
        rc = opra_mgmt_action_req(cmd, cmd_buf, cmd_len);
        break;

    default:
        FH_LOG(MGMT, ERR, ("Unexpected client command: %d", cmd->cmd_type));
        return FH_ERROR;
    }

    return rc;
}


/*
 * fh_opra_mgmt_run
 *
 * Management thread main context.
 */
static void *fh_opra_mgmt_run(void *arg)
{
    char      thread_name[16];
    int       ticks = 0;
    FH_STATUS rc;

    FH_ASSERT(arg == NULL);

    sprintf(thread_name,  "OPRA_Mgmt_%d", opra_cfg.ocfg_proc_id);

    fh_log_thread_start(thread_name);

    while (!opra_stopped) {
        ticks++;

        if (opra_mgmt_cl.mcl_fd == -1) {
            usleep(FH_MGMT_SLEEP);
        }
        else {
            /*
             * Main loop for handling FH manager requests
             */
            rc = fh_mgmt_cl_process(&opra_mgmt_cl, FH_MGMT_SLEEP);
            if (rc != FH_OK) {
                FH_LOG(MGMT, ERR, ("%s management connection failed", thread_name));
                opra_stopped = 1;
                break;
            }
        }

        /*
         * Every second, dump the rate and latency statistics.
         */
        if ((ticks % FH_MGMT_HZ) == 0) {
            fh_opra_lh_rates(1);
            fh_opra_lh_latency();
        }

        /* if ticks is a multiple of periodic statistics interval, tell the line handler */
        if (opra_cfg.ocfg_periodic_stats && (ticks % opra_cfg.ocfg_periodic_stats_interval) == 0) {
            fh_opra_lh_publish_stats = 1;
        }
    }

    fh_log_thread_stop(thread_name);

    return NULL;
}

/*
 * fh_opra_mgmt_start
 *
 * Initialize the management module. The management module includes
 * the communication/coordination with the opra management process.
 *
 * This also spawns the management thread.
 */
FH_STATUS fh_opra_mgmt_start(char *lo_file, int standalone)
{
    uint32_t  daddr = inet_addr("127.0.0.1");
    char      service_name[16];
    FH_STATUS rc;

    fh_time_get(&opra_mgmt_uptime);

    /*
     * Load the listed options
     */
    rc = fh_opra_lo_init(lo_file);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to load listed options"));
        return rc;
    }

    sprintf(service_name, "fhFastOpra%d", opra_cfg.ocfg_proc_id);

    /*
     * Check if we have to start in standalone mode or we have to connect to the
     * FH manager process.
     */
    if (standalone) {
        opra_mgmt_cl.mcl_fd = -1;
    }
    else {
        /*
         * Initialize the management connection
         */
        rc = fh_mgmt_cl_init(&opra_mgmt_cl, opra_mgmt_process,
                             FH_MGMT_CL_SERVICE, daddr, FH_MGR_PORT, service_name, 0);
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("%s failed to establish management connection", service_name));
            return FH_ERROR;
        }
    }

    /*
     * Start the OPRA management thread
     */
    if (pthread_create(&opra_mgmt_tid, NULL, fh_opra_mgmt_run, NULL) < 0) {
        FH_LOG(MGMT, ERR, ("Failed to start OPRA management thread (id:%d): %s (%d)",
                           opra_cfg.ocfg_proc_id, strerror(errno), errno));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_opra_mgmt_wait
 *
 * Wait for the management thread to exit.
 */
void fh_opra_mgmt_wait()
{
    if (opra_mgmt_tid) {
        pthread_join(opra_mgmt_tid, NULL);

        FH_LOG(MGMT, VSTATE, ("OPRA management thread exited: id:%d tid:0x%x",
                              opra_cfg.ocfg_proc_id, opra_mgmt_tid));
    }
}
