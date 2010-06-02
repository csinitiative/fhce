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
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "fh_log.h"
#include "fh_net.h"
#include "fh_sock.h"
#include "fh_tcp.h"
#include "fh_time.h"

#include "fh_mgmt_client.h"
#include "fh_mgmt_admin.h"

static uint32_t req_tid = 0;

/*
 * fh_mgmt_cl_init
 *
 * Initialize the management client handle.
 */
FH_STATUS fh_mgmt_cl_init(fh_mgmt_cl_t *mcl, fh_mgmt_cl_cb_t *process, uint32_t client_type,
                          uint32_t serv_addr, uint16_t serv_port, char *service, int quiet)
{
    FH_STATUS         rc;
    int               sock;
    uint32_t          saddr;
    uint16_t          sport;
    fh_adm_reg_req_t  reg_req;
    fh_adm_reg_resp_t reg_resp;

    FH_ASSERT(mcl);

    /*
     * Initialize the admin message library
     */
    rc = fh_adm_init();
    if (rc != FH_OK) {
        return rc;
    }

    /*
     * Connect to the management server
     */
    rc = fh_tcp_client(serv_addr, serv_port, &sock, quiet);
    if (rc != FH_OK) {
        if (!quiet) {
            FH_LOG(MGMT, ERR, ("Client failed to connect management server: %s:%d",
                               fh_net_ntoa(serv_addr), serv_port));
        }
        return rc;
    }

    mcl->mcl_fd = sock;

    /*
     * Retrieve the local TCP source address and port.
     */
    rc = fh_sock_getsrc(sock, &saddr, &sport);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Client failed to retrieve source address and port"));
        close(sock);
        mcl->mcl_fd = -1;
        return rc;
    }

    /*
     * Send the registration request and wait for the response
     */
    reg_req.reg_type = client_type;
    reg_req.reg_port = sport;
    reg_req.reg_addr = saddr;
    reg_req.reg_pid  = getpid();

    strncpy(reg_req.reg_srv, service, sizeof(reg_req.reg_srv));

    rc = fh_mgmt_cl_reqresp(mcl, FH_ADM_CMD_REG_REQ, &reg_req, sizeof(reg_req),
                            FH_ADM_CMD_REG_RESP, &reg_resp, sizeof(reg_resp));
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Client failed to register"));
        close(sock);
        mcl->mcl_fd = -1;
        return rc;
    }

    mcl->mcl_conn_id = reg_resp.reg_conn_id;
    mcl->mcl_process = process;

    return FH_OK;
}

/*
 * fh_mgmt_cl_process
 *
 * Process the client commands for a fixed amount of time in microseconds.
 */
FH_STATUS fh_mgmt_cl_process(fh_mgmt_cl_t *mcl, uint64_t usecs)
{
    FH_STATUS      rc;
    fd_set         rfds;
    int            nfds;
    int            n = 0;
    struct timeval tv;
    char          *data = NULL;
    fh_adm_cmd_t  *cmd = NULL;

    FD_ZERO(&rfds);
    FD_SET(mcl->mcl_fd, &rfds);

    nfds = mcl->mcl_fd + 1;

    /*
     * Wait for this message for the requested timeout value
     */
    tv.tv_sec  = (uint32_t)(usecs / 1000000);
    tv.tv_usec = (uint32_t)(usecs % 1000000);

    while (1) {
        uint64_t ts1, ts2, elapsed, remaining;
        fd_set tmprfds;

        /*
         * Grab the new read FD_SET
         */

        memcpy(&tmprfds, &rfds, sizeof(fd_set));

        /*
         * Start the select process
         */

        n = select(nfds, &tmprfds, NULL, NULL, &tv);
        if (n < 0) {
            FH_LOG(MGMT, ERR, ("select failed: fd: %d", mcl->mcl_fd));
            return FH_ERROR;
        }

        if (n == 0) {
            FH_LOG(MGMT, DIAG, ("Client processing - select timed-out: fd: %d", mcl->mcl_fd));
            break;
        }

        fh_time_get(&ts1);

        if (FD_ISSET(mcl->mcl_fd, &tmprfds)) {
            /*
             * Receive the data from the socket
             */
            rc = fh_adm_recv(mcl->mcl_fd, &data);
            if (rc != FH_OK) {
                FH_LOG(MGMT, ERR, ("Failed to recv message from manager: fd: %d", mcl->mcl_fd));
                return rc;
            }

            cmd = (fh_adm_cmd_t *) data;

            /*
             * Call back the client processing engine
             */
            rc = mcl->mcl_process(data, cmd->cmd_len);
            if (rc != FH_OK) {
                FH_LOG(MGMT, ERR, ("Client processing callback failed"));
            }

            free(data);
        }

        fh_time_get(&ts2);
        elapsed = ts2 - ts1;

        remaining = (uint64_t) tv.tv_sec * 1000000 + tv.tv_usec;
        if (elapsed > remaining) {
            break;
        }

        /*
         * Adjust the timeout value and go back in the select
         */
        remaining -= elapsed;

        tv.tv_sec  = (uint32_t)(remaining / 1000000);
        tv.tv_usec = (uint32_t)(remaining % 1000000);
    }

    return FH_OK;
}

/*
 * fh_mgmt_cl_reqresp
 *
 * Make a request and wait for the response back from the FH manager.
 */
FH_STATUS fh_mgmt_cl_reqresp(fh_mgmt_cl_t *mcl,
                             int req_cmd,  void *req_data,  int req_len,
                             int resp_cmd, void *resp_data, int resp_len)
{
    FH_STATUS rc;

    req_tid++;

    if (mcl->mcl_fd == -1) {
        FH_LOG(MGMT, ERR, ("Client not connected to FH Manager"));
        return FH_ERROR;
    }

    /*
     * Send the request
     */
    rc = fh_adm_send(mcl->mcl_fd, req_cmd, req_tid, req_data, req_len);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to send %s request", FH_ADM_CMD_NAME(req_cmd)));
        return rc;
    }

    /*
     * Wait for the response
     */
    rc = fh_adm_expect(mcl->mcl_fd, resp_cmd, req_tid, resp_data, resp_len);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to receive %s response", FH_ADM_CMD_NAME(resp_cmd)));
        return rc;
    }

    return FH_OK;
}

/*
 * fh_mgmt_cl_post
 *
 * Posts an event/request to the FH manager. And, don't wait for any response
 */
FH_STATUS fh_mgmt_cl_post(fh_mgmt_cl_t *mcl,
                          int req_cmd,  void *req_data,  int req_len)
{
    FH_STATUS rc;

    req_tid++;

    if (mcl->mcl_fd == -1) {
        FH_LOG(MGMT, ERR, ("Client not connected to FH Manager"));
        return FH_ERROR;
    }

    /*
     * Send the request
     */
    rc = fh_adm_send(mcl->mcl_fd, req_cmd, req_tid, req_data, req_len);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to send %s request", FH_ADM_CMD_NAME(req_cmd)));
        return rc;
    }

    return FH_OK;
}


