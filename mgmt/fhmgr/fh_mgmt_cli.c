/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 * 
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "fh_util.h"
#include "fh_log.h"
#include "fh_net.h"
#include "fh_tcp.h"
#include "fh_time.h"

#include "fh_mgmt_cli.h"
#include "fh_mgmt_revision.h"
#include "fh_mgmt_admin.h"
#include "fh_mgmt_service.h"

/*
 * cli_get_version
 *
 * Handles a 'get version' request from the CLI.
 */
static FH_STATUS cli_get_version(fh_mgmt_conn_t *conn, fh_adm_cmd_t *cmd, void *data)
{
    fh_adm_getver_req_t  getver_req;
    fh_adm_getver_resp_t getver_resp;
    FH_STATUS            rc;
    char                *service_name = "FH Manager";

    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_GETVER_REQ);

    /*
     * Parse the request
     */
    rc = fh_adm_parse(cmd->cmd_type, data, &getver_req);
    if (rc != FH_OK) {
        return rc;
    }

    FH_LOG(MGMT, DIAG, ("Get version request for: %s", service_name));

    strcpy(getver_resp.getver_service,    service_name);
    strcpy(getver_resp.getver_build_date, BUILD_DATE);
    strcpy(getver_resp.getver_build_rev,  BUILD_REV);

    /*
     * Send the response
     */
    rc = fh_adm_send(conn->conn_fd, FH_ADM_CMD_GETVER_RESP,
                     cmd->cmd_tid, &getver_resp, sizeof(getver_resp));

    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to send 'get version' response to CLI"));
        return rc;
    }

    FH_LOG(MGMT, DIAG, ("Sent 'get version' response for: %s", service_name));

    return FH_OK;
}

/*
 * cli_sgt_request
 *
 * Process a service group tree request from the CLI.
 */
static FH_STATUS cli_sgt_request(fh_mgmt_conn_t *conn, fh_adm_cmd_t *cmd, void *data)
{
    fh_adm_sgt_req_t   sgt_req;
    fh_adm_sgt_resp_t *sgt_resp;
    FH_STATUS          rc;

    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_SGT_REQ);

    /*
     * Parse the request
     */
    rc = fh_adm_parse(cmd->cmd_type, data, &sgt_req);
    if (rc != FH_OK) {
        return rc;
    }

    FH_LOG(MGMT, DIAG, ("Get service group tree from FH Manager"));

    sgt_resp = fh_mgmt_sgt_resp();
    if (!sgt_resp) {
        return FH_ERROR;
    }

    /*
     * Send the response
     */
    rc = fh_adm_send(conn->conn_fd, FH_ADM_CMD_SGT_RESP,
                     cmd->cmd_tid, sgt_resp, sizeof(*sgt_resp) + sgt_resp->sgt_size);

    free(sgt_resp);

    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to send service group tree response to CLI"));
        return rc;
    }

    FH_LOG(MGMT, DIAG, ("Sent service group tree response to CLI"));

    return FH_OK;
}

/*
 * cli_serv_request
 *
 * Process a service request from the CLI.
 */
static FH_STATUS cli_serv_request(fh_mgmt_conn_t *conn, fh_adm_cmd_t *cmd, void *data)
{
    fh_adm_serv_req_t   serv_req;
    fh_adm_serv_resp_t *serv_resp = NULL;
    FH_STATUS           rc;
    uint32_t            resp_size;
    uint32_t            resp_cmd;
    uint32_t            max_resp_len;
    void               *ptr = NULL;

    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_SERV_REQ);

    /*
     * Parse the request
     */
    rc = fh_adm_parse(cmd->cmd_type, data, &serv_req);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to parse command: %s",
                           FH_ADM_CMD_NAME(cmd->cmd_type)));
        return rc;
    }

    FH_LOG(MGMT, DIAG, ("Service request for: %s (cmd: %s)",
                        serv_req.serv_name, FH_ADM_CMD_NAME(serv_req.serv_cmd)));

    switch (serv_req.serv_cmd) {
    case FH_ADM_CMD_STATUS_REQ:
        resp_cmd  = FH_ADM_CMD_STATUS_RESP;
        resp_size = sizeof(fh_adm_status_resp_t);
        break;

    case FH_ADM_CMD_STATS_REQ:
        resp_cmd  = FH_ADM_CMD_STATS_RESP;
        resp_size = sizeof(fh_adm_stats_resp_t);
        break;

    case FH_ADM_CMD_GETVER_REQ:
        resp_cmd  = FH_ADM_CMD_GETVER_RESP;
        resp_size = sizeof(fh_adm_getver_resp_t);
        break;

    /*
     * Asynchronous events from CLI down to the service
     */
    case FH_MGMT_CL_CTRL_ENABLE:
    case FH_MGMT_CL_CTRL_DISABLE:
    case FH_MGMT_CL_CTRL_START:
    case FH_MGMT_CL_CTRL_STOP:
    case FH_MGMT_CL_CTRL_RESTART:
    case FH_MGMT_CL_CTRL_CLRSTATS:
        resp_cmd  = 0;
        resp_size = 0;
        break;

    default:
        FH_LOG(MGMT, ERR, ("Unexpected server request: %d", serv_req.serv_cmd));
        return FH_ERROR;
    }

    if (resp_size > 0) {
        max_resp_len = sizeof(fh_adm_serv_resp_t) + FH_MGMT_MAX_SERVICES * resp_size;

        /*
         * Allocate the max-size request
         */
        serv_resp = (fh_adm_serv_resp_t *)malloc(max_resp_len);
        FH_ASSERT(serv_resp);

        strcpy(serv_resp->serv_name, serv_req.serv_name);
        serv_resp->serv_resp_cnt  = 0;
        serv_resp->serv_resp_size = resp_size;
        serv_resp->serv_resp_cmd  = resp_cmd;
    }

   
    if (serv_resp) {
        ptr = (void *)(serv_resp+1);
    }

    if (strcmp(serv_req.serv_name, "all") == 0) {
        rc = fh_mgmt_serv_request_all(&serv_req, serv_resp, ptr);
    }
    else {
        // First look for a service group
        fh_mgmt_sg_t *sg = fh_mgmt_sg_lookup(serv_req.serv_name);
        if (sg) {
            rc = fh_mgmt_sg_request(sg, &serv_req, serv_resp, ptr);
        }
        else {
            // If not found, then look for a service
            fh_mgmt_serv_t *serv = fh_mgmt_serv_lookup(serv_req.serv_name);
            if (serv) {
                rc = fh_mgmt_serv_request(serv, &serv_req, serv_resp, ptr);
            }
        }
    }

    if (serv_resp) {
        /*
         * Send the response (in all cases)
         */
        rc = fh_adm_send(conn->conn_fd, FH_ADM_CMD_SERV_RESP, cmd->cmd_tid, serv_resp, 
                         serv_resp->serv_resp_cnt * resp_size + sizeof(fh_adm_serv_resp_t));
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("Failed to send %s response to: %s",
                               FH_ADM_CMD_NAME(resp_cmd), fh_net_ntoa(conn->conn_addr)));
        }
        else {
            FH_LOG(MGMT, DIAG, ("Sent %s response for: %s",
                                FH_ADM_CMD_NAME(resp_cmd), serv_req.serv_name));
        }

        free(serv_resp);
    }

    return rc;
}


/*
 * fh_mgmt_cli_process
 *
 * Processes a CLI request.
 */
FH_STATUS fh_mgmt_cli_process(fh_mgmt_conn_t *conn)
{
    fh_adm_cmd_t *cmd = NULL;
    char         *data = NULL;
    FH_STATUS     rc;

    FH_LOG(MGMT, DIAG, ("CLI processing (fd:%d)", conn->conn_fd));

    /*
     * Receive the data from the socket
     */
    rc = fh_adm_recv(conn->conn_fd, &data);
    if (rc != FH_OK) {
        FH_LOG(MGMT, INFO, ("Failed to recv message from CLI: fd:%d", conn->conn_fd));
        return rc;
    }

    cmd = (fh_adm_cmd_t *) data;

    FH_LOG(MGMT, DIAG, ("Received command: %s - fd:%d", FH_ADM_CMD_NAME(cmd->cmd_type),
                        conn->conn_fd));

    /*
     * Handle the client commands
     */
    switch (cmd->cmd_type) {
    case FH_ADM_CMD_GETVER_REQ:
        rc = cli_get_version(conn, cmd, data);
        break;

    case FH_ADM_CMD_SERV_REQ:
        rc = cli_serv_request(conn, cmd, data);
        break;

    case FH_ADM_CMD_SGT_REQ:
        rc = cli_sgt_request(conn, cmd, data);
        break;
    
    default:
        FH_LOG(MGMT, ERR, ("Invalid CLI command: %d", cmd->cmd_type));
        return FH_ERROR;
    }

    return FH_OK;
}


