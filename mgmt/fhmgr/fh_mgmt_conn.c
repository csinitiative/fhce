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
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "fh_util.h"
#include "fh_log.h"
#include "fh_net.h"
#include "fh_tcp.h"
#include "fh_time.h"

#include "fh_mgmt_admin.h"
#include "fh_mgmt_service.h"
#include "fh_mgmt_cli.h"

/*
 * Local connection library variables
 */
static fd_set        rfds;
static int           fdmax = 0;
static conn_lh_t     conns;
static uint32_t      conn_id = 1;

/*
 * fh_mgmt_conn_init
 *
 * Initialize the connection library
 */
void fh_mgmt_conn_init()
{
    TAILQ_INIT(&conns);

    FD_ZERO(&rfds);
}

/*
 * fh_mgmt_conn_new
 *
 * Creates a new connection.
 */
FH_STATUS fh_mgmt_conn_new(int type, int fd, uint32_t addr, uint16_t port)
{
    fh_mgmt_conn_t *conn = malloc(sizeof(fh_mgmt_conn_t));
    if (!conn) {
        FH_LOG(MGMT, ERR, ("Failed to allocate new connection: type:%d fd:%d", type, fd));
        return FH_ERROR;
    }

    memset(conn, 0, sizeof(fh_mgmt_conn_t));

    conn->conn_type = type;
    conn->conn_fd   = fd;
    conn->conn_id   = conn_id++;
    conn->conn_addr = addr;
    conn->conn_port = port;

    fh_time_get(&conn->conn_uptime);

    FD_SET(fd, &rfds);

    if (fd > fdmax) {
        fdmax = fd;
    }

    TAILQ_INSERT_TAIL(&conns, conn, conn_le);

    return FH_OK;
}

/*
 * conn_free
 *
 * Clear a file descriptor to select read list
 */
static void conn_free(fh_mgmt_conn_t *conn)
{
    if (conn->conn_serv) {
        FH_STATUS rc = fh_mgmt_serv_detach(conn->conn_serv);
        FH_ASSERT(rc == FH_OK);
    }

    FH_LOG(MGMT, VSTATE, ("Closing connection from %s:%d (fd:%d)",
                         fh_net_ntoa(conn->conn_addr), conn->conn_port, conn->conn_fd));

    TAILQ_REMOVE(&conns, conn, conn_le);

    close(conn->conn_fd);

    FD_CLR(conn->conn_fd, &rfds);

    free(conn);

    fdmax = 0;

    TAILQ_FOREACH(conn, &conns, conn_le) {
        if (conn->conn_fd > fdmax) {
            fdmax = conn->conn_fd;
        }
    }
}

/*
 * conn_server_process
 *
 * Processes a server connection, accepts the connection of a new client
 * connection, and adds the client connection to the connection list.
 */
static FH_STATUS conn_server_process(fh_mgmt_conn_t *conn)
{
    FH_STATUS rc = FH_OK;
    uint32_t  addr;
    uint16_t  port;
    int       fd;

    /*
     * Accept the new connection
     */
    rc = fh_tcp_accept(conn->conn_fd, &addr, &port, &fd);
    if(rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("FH Manager failed to process incoming connection"));
        return FH_ERROR;
    }

    FH_LOG(MGMT, VSTATE, ("FH Manager accepted connection from %s:%d (fd:%d)",
                         fh_net_ntoa(addr), port, fd));

    rc = fh_mgmt_conn_new(FH_MGMT_CONN_CLIENT, fd, addr, port);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("FH Manager failed to create new connection"));
        close(fd);
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * conn_client_registration
 *
 * Handles a client registration
 */
static FH_STATUS conn_client_registration(fh_mgmt_conn_t *conn, fh_adm_cmd_t *cmd, void *data)
{
    fh_adm_reg_req_t   reg_req;
    fh_adm_reg_resp_t  reg_resp;
    FH_STATUS          rc;

    FH_ASSERT(cmd->cmd_type == FH_ADM_CMD_REG_REQ);

    /*
     * Parse the registration request
     */
    rc = fh_adm_parse(cmd->cmd_type, data, &reg_req);
    if (rc != FH_OK) {
        return rc;
    }

    FH_LOG(MGMT, VSTATE, ("Registration request received from service: %s (pid:%d)",
                         reg_req.reg_srv, reg_req.reg_pid));

    if (conn->conn_port != reg_req.reg_port) {
        FH_LOG(MGMT, WARN, ("Unexpected client port: %d vs. %d",
                            reg_req.reg_port, conn->conn_port));
    }

    /*
     * Store the client type, process ID information, and service name
     */
    conn->conn_cltype = reg_req.reg_type;
    conn->conn_pid    = reg_req.reg_pid;

    if (conn->conn_cltype == FH_MGMT_CL_SERVICE) {

        fh_mgmt_serv_t *serv = fh_mgmt_serv_lookup(reg_req.reg_srv);
        if (!serv) {
            FH_LOG(MGMT, ERR, ("Service not found: %s", reg_req.reg_srv));
            return FH_ERR_NOTFOUND;
        }

        rc = fh_mgmt_serv_attach(serv, conn);
        if (rc != FH_OK) {
            return rc;
        }

        /* force the group that this service belongs to to reload its stats report */
        serv->serv_group->sg_stats_ticks = serv->serv_group->sg_stats_period;
    }

    reg_resp.reg_conn_id  = conn->conn_id;

    /*
     * Send the registration response
     */
    rc = fh_adm_send(conn->conn_fd, FH_ADM_CMD_REG_RESP,
                     cmd->cmd_tid, &reg_resp, sizeof(reg_resp));

    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to send registration response to: %s (%s)",
                           reg_req.reg_srv, fh_net_ntoa(conn->conn_addr)));
        return rc;
    }

    FH_LOG(MGMT, VSTATE, ("Registration response sent to service: %s (pid:%d)",
                         reg_req.reg_srv, reg_req.reg_pid));

    return FH_OK;
}


/*
 * conn_client_process
 *
 * Generic client processing function. At this point, the client has not
 * registered and we don't know yet what kind of client it is.
 */
static FH_STATUS conn_client_process(fh_mgmt_conn_t *conn)
{
    fh_adm_cmd_t *cmd = NULL;
    char         *data = NULL;
    FH_STATUS     rc;

    FH_LOG(MGMT, DIAG, ("CLI processing - fd:%d from:%s", conn->conn_fd,
                        fh_net_ntoa(conn->conn_addr)));

    /*
     * Receive the data from the socket
     */
    rc = fh_adm_recv(conn->conn_fd, &data);
    if (rc != FH_OK) {
        FH_LOG(MGMT, INFO, ("Failed to recv message from CLI: fd:%d", conn->conn_fd));
        return rc;
    }

    cmd = (fh_adm_cmd_t *) data;

    FH_LOG(MGMT, DIAG, ("Received command: %s - fd:%d from:%s",
                        FH_ADM_CMD_NAME(cmd->cmd_type), conn->conn_fd,
                        fh_net_ntoa(conn->conn_addr)));

    /*
     * Handle the client commands
     */
    switch (cmd->cmd_type) {
    case FH_ADM_CMD_REG_REQ:
        rc = conn_client_registration(conn, cmd, data);
        break;

    default:
        FH_LOG(MGMT, ERR, ("Invalid client connection command: %d from %s",
                           cmd->cmd_type, fh_net_ntoa(conn->conn_addr)));
        rc = FH_ERROR;
    }

    return rc;
}

/*
 * conn_process
 *
 * Receive data from given connection
 */
static FH_STATUS conn_process(fh_mgmt_conn_t *conn)
{
    FH_STATUS rc = FH_OK;

    switch (conn->conn_type) {
    case FH_MGMT_CONN_SERVER:
        rc = conn_server_process(conn);
        break;

    case FH_MGMT_CONN_CLIENT:
        switch (conn->conn_cltype) {
        case FH_MGMT_CL_CLI:
            rc = fh_mgmt_cli_process(conn);
            break;

        case FH_MGMT_CL_SERVICE:
            rc = fh_mgmt_serv_process(conn->conn_serv);
            break;

        default:
            rc = conn_client_process(conn);
        }
        break;

    default:
        FH_LOG(MGMT, ERR, ("Invalid connection type: %d from %s fd:%d",
                           conn->conn_type, fh_net_ntoa(conn->conn_addr), conn->conn_fd));
        rc = FH_ERROR;
    }


    return rc;
}

/*
 * fh_mgmt_conn_loop
 *
 * Handles the connection activity and times out after 'usecs' usecs.
 */
void fh_mgmt_conn_loop(uint64_t usecs)
{
    FH_STATUS      rc;
    int            nfd = 0;
    struct timeval tv;


    /*
     * Wait for this message for the requested timeout value
     */
    tv.tv_sec  = (uint32_t)(usecs / 1000000);
    tv.tv_usec = (uint32_t)(usecs % 1000000);

    /*
     * Main connection selection loop
     */

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

        nfd = select(fdmax+1, &tmprfds, NULL, NULL, &tv);
        if (nfd == -1) {
            FH_LOG(MGMT, WARN, ("Connection select failed: %s", strerror(errno)));
            continue;
        }

        if (nfd == 0) {
            FH_LOG(MGMT, INFO, ("Connection processing - select timed-out"));
            break;
        }

        /*
         * Process active connections
         */

        fh_time_get(&ts1);

        if (nfd > 0) {
            fh_mgmt_conn_t *conn;

            for (conn = TAILQ_FIRST(&conns); conn && nfd > 0; ) {
                fh_mgmt_conn_t *conn_next = TAILQ_NEXT(conn, conn_le);

                if (FD_ISSET(conn->conn_fd, &tmprfds)) {
                    rc = conn_process(conn);

                    if (rc != FH_OK) {
                        conn_free(conn);
                    }

                    nfd--;
                }

                conn = conn_next;
            }
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
}
