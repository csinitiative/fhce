/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 * 
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

/* System headers */
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>

/* FH common headers */
#include "fh_errors.h"
#include "fh_log.h"
#include "fh_config.h"
#include "fh_time.h"
#include "fh_util.h"
#include "fh_info.h"
#include "fh_mgmt_client.h"
#include "fh_mgmt_admin.h"

/* FH shared management thread headers */
#include "fh_shr_mgmt.h"

/* constants controlling how often we wake up and check for management commands */
#define FH_SHR_MGMT_HZ    (1)
#define FH_SHR_MGMT_SLEEP (1000000 / FH_SHR_MGMT_HZ)

/* START ONLY ONE MANAGEMENT THREAD AT A TIME -- this code is not intended to be thread safe */
static pthread_t         mgmt_thread;
static char              proc_name[MAX_PROPERTY_LENGTH];
static fh_mgmt_cl_t      conn_context;
static fh_info_build_t   fh_info;
static fh_shr_mgmt_cb_t  callbacks;
static uint64_t          uptime      = 0;
static int               finished    = 0;

/**
 *  @brief Handle a request for process stats from the management server
 *
 *  @param cmd command structure containing details about the command from the server
 *  @param cmd_buf un-cast command data passed to the command processing callback
 *  @param cmd_len size of the data in cmd_buf
 *  @return status code indicating success or failure
 */
static FH_STATUS fh_shr_mgmt_cmd_stats(fh_adm_cmd_t *cmd, char *cmd_buf, int cmd_len)
{
    fh_adm_stats_req_t       stats_req;
    fh_adm_stats_resp_t      stats_resp;
    FH_STATUS                rc;

    /* check for correct command type and length */
    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_STATS_REQ);
    FH_ASSERT(cmd_len == sizeof(stats_req));

    /* parse the request */
    rc = fh_adm_parse(cmd->cmd_type, cmd_buf, &stats_req);
    if (rc != FH_OK) {
        return rc;
    }

    /* log that a request for stats has been received and fill the response structure */
    FH_LOG(MGMT, DIAG, ("received management request for statistics"));

    /* fetch stats */
    callbacks.getstats(&stats_resp);

    /* send the response */
    rc = fh_adm_send(conn_context.mcl_fd, FH_ADM_CMD_STATS_RESP, cmd->cmd_tid,
                     &stats_resp, sizeof(fh_adm_stats_resp_t));
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to send statistics response to management server"));
        return rc;
    }

    /* log success and return */
    FH_LOG(MGMT, DIAG, ("statistics response sent successfully"));
    return FH_OK;
}


/**
 *  @brief Handle an action request from the management server
 *
 *  @param cmd command structure containing details about the command from the server
 *  @param cmd_buf un-cast command data passed to the command processing callback
 *  @param cmd_len size of the data in cmd_buf
 *  @return status code indicating success or failure
 */
static FH_STATUS fh_shr_mgmt_cmd_action(fh_adm_cmd_t *cmd, char *cmd_buf, int cmd_len)
{
    fh_adm_action_req_t  action_req;
    FH_STATUS            rc;

    /* ensure that command is of the correct type and is the correct size */
    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_ACTION_REQ);
    FH_ASSERT(cmd_len == sizeof(action_req));

    /* parse the request */
    rc = fh_adm_parse(cmd->cmd_type, cmd_buf, &action_req);
    if (rc != FH_OK) {
        return rc;
    }

    /* check to see which action has been requested */
    switch (action_req.action_type) {
    case FH_MGMT_CL_CTRL_CLRSTATS:
        callbacks.clrstats();
        break;
    
    case FH_MGMT_CL_CTRL_STOP:
        callbacks.exit();
        break;

    default:
        FH_LOG(MGMT, ERR, ("unsupported action type: %d", action_req.action_type));
        break;
    }

    return rc;
}

/**
 *  @brief Return feed handler version information to the management server
 *
 *  @param cmd command structure containing details about the command from the server
 *  @param cmd_buf un-cast command data passed to the command processing callback
 *  @param cmd_len size of the data in cmd_buf
 *  @return status code indicating success or failure
 */
static FH_STATUS fh_shr_mgmt_cmd_version(fh_adm_cmd_t *cmd, char *cmd_buf, int cmd_len)
{
    fh_adm_getver_req_t  getver_req;
    fh_adm_getver_resp_t getver_resp;
    FH_STATUS rc;

    /* assert that the command is a "get version" request and that the command is the proper length */
    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_GETVER_REQ);
    FH_ASSERT(cmd_len == sizeof(getver_req));

    /* parse the request */
    rc = fh_adm_parse(cmd->cmd_type, cmd_buf, &getver_req);
    if (rc != FH_OK) {
        return rc;
    }

    /* log receipt of the request and copy requested data into the response structure */
    FH_LOG(MGMT, DIAG, ("get Arca version request for: %s", getver_req.getver_service));
    strcpy(getver_resp.getver_service,    getver_req.getver_service);
    strcpy(getver_resp.getver_build_date, fh_info.build_date);
    strcpy(getver_resp.getver_build_rev,  fh_info.build_svn_rev);

    /* send the response */
    rc = fh_adm_send(conn_context.mcl_fd, FH_ADM_CMD_GETVER_RESP, cmd->cmd_tid, &getver_resp,
                     sizeof(getver_resp));
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to send 'get version' response for %s",
                           getver_req.getver_service));
        return rc;
    }

    /* log success and return success status code */
    FH_LOG(MGMT, DIAG, ("sent 'get version' response for: %s", getver_req.getver_service));
    return FH_OK;
}

/**
 *  @brief Handle a request for process status from the management server
 *
 *  @param cmd command structure containing details about the command from the server
 *  @param cmd_buf un-cast command data passed to the command processing callback
 *  @param cmd_len size of the data in cmd_buf
 *  @return status code indicating success or failure
 */
static FH_STATUS fh_shr_mgmt_cmd_status(fh_adm_cmd_t *cmd, char *cmd_buf, int cmd_len)
{
    fh_adm_status_req_t      status_req;
    fh_adm_status_resp_t     status_resp;
    fh_info_proc_t          *info;
    FH_STATUS rc;

    /* check for the correct command type and length */
    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_STATUS_REQ);
    FH_ASSERT(cmd_len == sizeof(status_req));

    /* parse the request */
    rc = fh_adm_parse(cmd->cmd_type, cmd_buf, &status_req);
    if (rc != FH_OK) {
        return rc;
    }

    /* log receipt of this request */
    FH_LOG(MGMT, DIAG, ("received request for status from the manager service"));

    /* load the response structure with values from this process */
    strcpy(status_resp.status_service, status_req.status_service);
    info = callbacks.getstatus();
    status_resp.status_pid      = info->pid;
    status_resp.status_fp_cpu   = info->cpu;
    status_resp.status_fp_tid   = info->tid;
    status_resp.status_uptime   = info->start_time;

    /* Computed by FH Manager */
    status_resp.status_pmem     = 0;
    status_resp.status_putime   = 0;
    status_resp.status_pstime   = 0;

    /* Send the response */
    rc = fh_adm_send(conn_context.mcl_fd, FH_ADM_CMD_STATUS_RESP, cmd->cmd_tid, &status_resp,
                     sizeof(status_resp));
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to send status response"));
        return rc;
    }

    /* log successful send and return */
    FH_LOG(MGMT, DIAG, ("sent status response"));
    return FH_OK;
}


/**
 *  @brief Callback for management processing
 *
 *  @param cmd_buf command being passed to the callback
 *  @param cmd_len size of the command
 *  @return status code indicating success or failure
 */
static FH_STATUS fh_shr_mgmt_process(char *cmd_buf, int cmd_len)
{
    fh_adm_cmd_t *cmd = (fh_adm_cmd_t *)cmd_buf;
    FH_STATUS     rc  = FH_OK;
    
    switch (cmd->cmd_type) {
    case FH_ADM_CMD_STATS_REQ:
        rc = fh_shr_mgmt_cmd_stats(cmd, cmd_buf, cmd_len);
        break;
    
    case FH_ADM_CMD_STATUS_REQ:
        rc = fh_shr_mgmt_cmd_status(cmd, cmd_buf, cmd_len);
        break;

    case FH_ADM_CMD_GETVER_REQ:
        rc = fh_shr_mgmt_cmd_version(cmd, cmd_buf, cmd_len);
        break;

    case FH_ADM_CMD_ACTION_REQ:
        rc = fh_shr_mgmt_cmd_action(cmd, cmd_buf, cmd_len);
        break;

    default:
        FH_LOG(MGMT, ERR, ("unexpected client command: %d", cmd->cmd_type));
        return FH_ERROR;
    }

    return rc;
}

/**
 *  @brief Body of the management thread
 *
 *  @param arg argument that was passed when this thread was started
 *  @return funtion will always return NULL
 */
#include <stdio.h>
static void *fh_shr_mgmt_run(void *arg)
{
    int         ticks     = 0;
    int         connected = 0;
    uint32_t    dest_addr = inet_addr("127.0.0.1");
    char       *thread_name = NULL;
    
    /* ensure that no data was passed to the thread (none is needed) */
    FH_ASSERT(arg == NULL);
    
    /* allocate space for, generate, and log a "thread started" message for this thread's name */
    thread_name = fh_util_thread_name("Mgmt", proc_name);
    fh_log_thread_start(thread_name);
    
    /* never exit (only when this thread receives SIGINT) */
    while (!finished) {
        ticks++;

        /* if we are in standalone mode, just sleep for FH_MGMT_SLEEP usecs */
        if (conn_context.mcl_fd == -1) {
            usleep(FH_SHR_MGMT_SLEEP);
        }
        /* if we are not currently connected to the fhmgr */
        else if (connected <= 0) {
            /* if attempt to connect to fhmgr fails... */
            if (fh_mgmt_cl_init(&conn_context, fh_shr_mgmt_process, FH_MGMT_CL_SERVICE,
                                dest_addr, FH_MGR_PORT, proc_name, connected) != FH_OK) {
                /* if this is the first time the connection has failed, log the occurance */
                if (connected == 0) {
                    FH_LOG(MGMT, WARN, ("%s failed to establish management connection",
                                        fh_info.name));
                }
                /* increment the number of failed connections and sleep for a bit */
                connected--;
                usleep(FH_SHR_MGMT_SLEEP);
            }
            /* if attempt succeeds */
            else {
                FH_LOG(MGMT, INFO, ("%s management connection established", fh_info.name));
                connected = 1;
            }
        }
        /* otherwise, check for FH manager requests */
        else if (fh_mgmt_cl_process(&conn_context, FH_SHR_MGMT_SLEEP) != FH_OK) {
            FH_LOG(MGMT, WARN, ("%s management connection lost", thread_name));
            connected = 0;
        }
    
        /* every second, dump the rate statistics. */
        if ((ticks % FH_SHR_MGMT_HZ) == 0) {
            callbacks.snapstats();
            callbacks.snaplatency();
        }
    }
    
    /* print a "thread stopped" message and return */
    fh_log_thread_stop(thread_name);
    return NULL;
}

/**
 *  @brief Initialize management functionality
 * 
 *  This function spawns the management thread and establishes a connection to the fhmgr.
 *
 *  @param standalone flag to set management thread to run without a central manager
 *  @param fh name of the feed handler that is being run
 *  @param info feed handler info (version, build, etc)
 *  @param callbacks structure of callback functions that will be called for stats gathering, etc.
 *  @return status code indicating success of failure
 */
FH_STATUS fh_shr_mgmt_start(int standalone, const fh_info_build_t *info, const char *proc,
                            fh_shr_mgmt_cb_t *cb)
{    
    /* mark the startup time  */
    fh_time_get(&uptime);
    
    /* make copies of the feed handler info, process names, and callbacks for later use */
    memcpy(&fh_info, info, sizeof(fh_info_build_t));
    memcpy(&callbacks, cb, sizeof(fh_shr_mgmt_cb_t));
    strcpy(proc_name, proc);
    
    /* starting up in standalone mode */
    if (standalone) { 
        conn_context.mcl_fd = -1;
    }

    /* start the management thread */
    if (pthread_create(&mgmt_thread, NULL, fh_shr_mgmt_run, NULL) < 0) {
        FH_LOG(MGMT, ERR, ("failed to start %s management thread (%s): %s (%d)",
                           fh_info.name, proc_name, strerror(errno), errno));
        return FH_ERROR;
    }
    
    /* if we get here, return OK */
    return FH_OK;
}

/*
 * Block until the management thread has exited
 */
void fh_shr_mgmt_wait()
{
    if (mgmt_thread) {
        pthread_join(mgmt_thread, NULL);
        FH_LOG(MGMT, VSTATE, ("%s management thread (%s) exited", fh_info.name, proc_name));
    }
}

/*
 * Kill the management thread
 */
void fh_shr_mgmt_exit()
{
    finished = 1;
}
