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

//
/*********************************************************************/
/* file: fh_arca_mgmt.c                                              */
/* Usage: management thread for arca multicast feed handler          */
/* Author: Ross Cooperman of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// Standard library includes
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <arpa/inet.h>

// FH common includes
#include "fh_log.h"
#include "fh_time.h"
#include "fh_mgmt_client.h"
#include "fh_mgmt_admin.h"

// FH Arca headers
#include "fh_arca.h"
#include "fh_arca_mgmt.h"
#include "fh_arca_cfg.h"
#include "fh_arca_util.h"
#include "fh_arca_lh.h"
#include "fh_arca_headers.h"

// constants controlling how often we wake up and check for management commands
#define FH_ARCA_MGMT_HZ    (1)
#define FH_ARCA_MGMT_SLEEP (1000000 / FH_ARCA_MGMT_HZ)

// static variables
static fh_mgmt_cl_t  arca_mgmt_cl;
static pthread_t     arca_mgmt_thread;
static uint64_t      arca_mgmt_uptime = 0;

/*! \brief Handle a request for process stats from the management server
 *
 *  \param cmd command structure containing details about the command from the server
 *  \param cmd_buf un-cast command data passed to the command processing callback
 *  \param cmd_len size of the data in cmd_buf
 *  \return status code indicating success or failure
 */
static FH_STATUS fh_arca_mgmt_cmd_stats(const fh_adm_cmd_t *cmd, const char *cmd_buf, 
    const int cmd_len)
{
    fh_adm_stats_req_t  stats_req;
    fh_adm_stats_resp_t stats_resp;
    FH_STATUS           rc;

    // check for correct command type and length
    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_STATS_REQ);
    FH_ASSERT(cmd_len == sizeof(stats_req));

    // parse the request
    rc = fh_adm_parse(cmd->cmd_type, (char*) cmd_buf, &stats_req);
    if (rc != FH_OK) {
        return rc;
    }

    // log that a request for stats has been received and fill the response structure
    FH_LOG(MGMT, DIAG, ("received management request for statistics"));
    strcpy(stats_resp.stats_service, stats_req.stats_service);
    fh_arca_lh_get_stats(&stats_resp);

    // send the response
    rc = fh_adm_send(arca_mgmt_cl.mcl_fd, FH_ADM_CMD_STATS_RESP, cmd->cmd_tid, &stats_resp,
                     sizeof(stats_resp));
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to send statistics response to management server"));
        return rc;
    }

    // log success and return
    FH_LOG(MGMT, DIAG, ("statistics response sent successfully"));
    return FH_OK;
}

/*! \brief Handle a request for process status from the management server
 *
 *  \param cmd command structure containing details about the command from the server
 *  \param cmd_buf un-cast command data passed to the command processing callback
 *  \param cmd_len size of the data in cmd_buf
 *  \return status code indicating success or failure
 */
static FH_STATUS fh_arca_mgmt_cmd_status(const fh_adm_cmd_t *cmd, const char *cmd_buf, 
    const int cmd_len)
{
    fh_adm_status_req_t  status_req;
    fh_adm_status_resp_t status_resp;
    FH_STATUS rc;

    // check for the correct command type and length
    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_STATUS_REQ);
    FH_ASSERT(cmd_len == sizeof(status_req));

    // parse the request
    rc = fh_adm_parse(cmd->cmd_type, (char*) cmd_buf, &status_req);
    if (rc != FH_OK) {
        return rc;
    }

    // log receipt of this request
    FH_LOG(MGMT, DIAG, ("received request for status from the fh manager service"));

    // load the response structure with values from this process
    strcpy(status_resp.status_service, status_req.status_service);
    status_resp.status_pid      = getpid();
    status_resp.status_fp_cpu   = fh_arca_cfg.cpu;
    status_resp.status_fp_tid   = fh_arca_lh_get_tid();
    status_resp.status_uptime   = (uint32_t)(arca_mgmt_uptime / 1000000);

    // Computed by FH Manager
    status_resp.status_pmem     = 0;
    status_resp.status_putime   = 0;
    status_resp.status_pstime   = 0;

    // Send the response
    rc = fh_adm_send(arca_mgmt_cl.mcl_fd, FH_ADM_CMD_STATUS_RESP, cmd->cmd_tid, &status_resp,
                     sizeof(status_resp));
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to send status response"));
        return rc;
    }

    // log successful send and return
    FH_LOG(MGMT, DIAG, ("sent status response"));
    return FH_OK;
}

/*! \brief Handle an action request from the management server
 *
 *  \param cmd command structure containing details about the command from the server
 *  \param cmd_buf un-cast command data passed to the command processing callback
 *  \param cmd_len size of the data in cmd_buf
 *  \return status code indicating success or failure
 */
static FH_STATUS fh_arca_mgmt_cmd_action(const fh_adm_cmd_t *cmd, const char *cmd_buf, 
    const int cmd_len)
{
    fh_adm_action_req_t  action_req;
    FH_STATUS            rc;

    // ensure that command is of the correct type and is the correct size
    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_ACTION_REQ);
    FH_ASSERT(cmd_len == sizeof(action_req));

    // parse the request
    rc = fh_adm_parse(cmd->cmd_type, (char*) cmd_buf, &action_req);
    if (rc != FH_OK) {
        return rc;
    }

    // check to see which action has been requested
    switch (action_req.action_type) {
    case FH_MGMT_CL_CTRL_CLRSTATS:
        fh_arca_lh_clr_stats();
        break;
    
    case FH_MGMT_CL_CTRL_STOP:
        fh_arca_stopped = 1;
        break;

    default:
        FH_LOG(MGMT, ERR, ("unsupported action type: %d", action_req.action_type));
        break;
    }

    return rc;
}

/*! \brief Return Arca version information to the management server
 *
 *  \param cmd command structure containing details about the command from the server
 *  \param cmd_buf un-cast command data passed to the command processing callback
 *  \param cmd_len size of the data in cmd_buf
 *  \return status code indicating success or failure
 */
static FH_STATUS fh_arca_mgmt_cmd_version(const fh_adm_cmd_t *cmd, const char *cmd_buf, 
    const int cmd_len)
{
    fh_adm_getver_req_t  getver_req;
    fh_adm_getver_resp_t getver_resp;
    FH_STATUS rc;

    // assert that the command is a "get version" request and that the command is the proper length
    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_GETVER_REQ);
    FH_ASSERT(cmd_len == sizeof(getver_req));

    // parse the request
    rc = fh_adm_parse(cmd->cmd_type, (char*)cmd_buf, &getver_req);
    if (rc != FH_OK) {
        return rc;
    }

    // log receipt of the request and copy requested data into the response structure
    FH_LOG(MGMT, DIAG, ("get Arca version request for: %s", getver_req.getver_service));
    strcpy(getver_resp.getver_service,    getver_req.getver_service);
    strcpy(getver_resp.getver_build_date, get_build_date());
    strcpy(getver_resp.getver_build_rev,  get_build_rev());

    // send the response
    rc = fh_adm_send(arca_mgmt_cl.mcl_fd, FH_ADM_CMD_GETVER_RESP, cmd->cmd_tid, &getver_resp,
                     sizeof(getver_resp));
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to send 'get version' response for %s",
                           getver_req.getver_service));
        return rc;
    }

    // log success and return success status code
    FH_LOG(MGMT, DIAG, ("sent 'get version' response for: %s", getver_req.getver_service));
    return FH_OK;
}

/*! \brief Callback for management processing
 *
 *  \param cmd_buf command being passed to the callback
 *  \param cmd_len size of the command
 *  \return status code indicating success or failure
 */
static FH_STATUS fh_arca_mgmt_process(char *cmd_buf, const int cmd_len)
{
    fh_adm_cmd_t *cmd = (fh_adm_cmd_t *)cmd_buf;
    FH_STATUS     rc  = FH_OK;
    
    switch (cmd->cmd_type) {
    case FH_ADM_CMD_STATS_REQ:
        rc = fh_arca_mgmt_cmd_stats(cmd, cmd_buf, cmd_len);
        break;
    
    case FH_ADM_CMD_STATUS_REQ:
        rc = fh_arca_mgmt_cmd_status(cmd, cmd_buf, cmd_len);
        break;

    case FH_ADM_CMD_GETVER_REQ:
        rc = fh_arca_mgmt_cmd_version(cmd, cmd_buf, cmd_len);
        break;

    case FH_ADM_CMD_ACTION_REQ:
        rc = fh_arca_mgmt_cmd_action(cmd, cmd_buf, cmd_len);
        break;

    default:
        FH_LOG(MGMT, ERR, ("unexpected client command: %d", cmd->cmd_type));
        return FH_ERROR;
    }

    return rc;
}

/*! \brief Body of the management thread
 *
 *  \param arg argument that was passed when this thread was started
 *  \return funtion will always return NULL
 */
static void *fh_arca_mgmt_run(void *arg)
{
    char        *thread_name = NULL;
    int          ticks       = 0;

    // make sure that no arguments were passed in (and suppress unused argument)
    FH_ASSERT(arg == NULL);
    
    // allocate space for, generate, and log a "thread started" message for this thread's name
    thread_name = fh_arca_util_thread_name("Mgmt");
    fh_log_thread_start(thread_name);
    
    // done initializing, unlock the thread init semaphore
    if (sem_post(&fh_arca_thread_init) == -1) {
        FH_LOG(MGMT, ERR, ("unable to unlock init semaphore for MGMT thread: %s", strerror(errno)));
        fh_arca_stopped = 1;
    }

    while (!fh_arca_stopped) {
        ticks++;

        // if we are in standalone mode, just sleep for FH_MGMT_SLEEP usecs
        if (arca_mgmt_cl.mcl_fd == -1) {
            usleep(FH_ARCA_MGMT_SLEEP);
        }
        // otherwise, check for FH manager requests
        else {
            if (fh_mgmt_cl_process(&arca_mgmt_cl, FH_ARCA_MGMT_SLEEP) != FH_OK) {
                FH_LOG(MGMT, ERR, ("%s management connection failed", thread_name));
                fh_arca_stopped = 1;
                break;
            }
        }
    
        // every second, dump the rate statistics.
        if ((ticks % FH_ARCA_MGMT_HZ) == 0) {
            fh_arca_lh_rates();
        }
    }

    // log a "thread stopped" message and return
    fh_log_thread_stop(thread_name);
    return NULL;
}

/*! \brief Initialize management functionality
 * 
 *  This function spawns the management thread and establishes a connection
 *  to the Arca management process.
 *
 *  \param standalone flag to set management processes to run without a central manager
 *  \return status code indicating success of failure
 */
FH_STATUS fh_arca_mgmt_start(const int standalone)
{
    FH_STATUS        rc;
    struct timespec  lock_wait;
    uint32_t         daddr = inet_addr("127.0.0.1");
    
    // lock the thread init semaphore
    memset(&lock_wait, 0, sizeof(struct timespec));
    lock_wait.tv_sec = time(NULL) + 5;
    if (sem_timedwait(&fh_arca_thread_init, &lock_wait) == -1) {
        FH_LOG(MGMT, ERR, ("unable to lock init semaphore for MGMT thread: %s", strerror(errno)));
        return FH_ERROR;
    }
    
    // mark the startup time 
    fh_time_get(&arca_mgmt_uptime);
        
    // starting up in standalone mode
    if (standalone) { 
        arca_mgmt_cl.mcl_fd = -1;
    }
    // connecting to a central manager
    else {
        rc = fh_mgmt_cl_init(&arca_mgmt_cl, fh_arca_mgmt_process, FH_MGMT_CL_SERVICE, daddr,
                             FH_MGR_PORT, fh_arca_cfg.name, 0);
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("%s failed to establish management connection", fh_arca_cfg.name));
            sem_post(&fh_arca_thread_init);
            return FH_ERROR;
        }
    }

    // start the Arca management thread
    if (pthread_create(&arca_mgmt_thread, NULL, fh_arca_mgmt_run, NULL) < 0) {
        FH_LOG(MGMT, ERR, ("Failed to start Arca management thread (%s): %s (%d)",
                           fh_arca_cfg.name, strerror(errno), errno));
        sem_post(&fh_arca_thread_init);
        return FH_ERROR;
    }
    
    // if we get here, return success code
    return FH_OK;
}

/*! \brief Block until the management process has exited
 */
void fh_arca_mgmt_wait()
{
    if (arca_mgmt_thread) {
        pthread_join(arca_mgmt_thread, NULL);
        FH_LOG(MGMT, VSTATE, ("Arca management thread (%s) exited", fh_arca_cfg.name));
    }
    
}

