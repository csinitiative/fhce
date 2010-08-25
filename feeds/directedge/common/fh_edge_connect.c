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

/* System headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

/* FH common headers */
#include "fh_errors.h"
#include "fh_config.h"
#include "fh_log.h"
#include "fh_plugin_internal.h"
#include "fh_tcp.h"

/* FH shared config headers */
#include "fh_shr_cfg_lh.h"
#include "fh_shr_tcp_lh.h"

/* FH DirectEdge specific includes  */
#include "fh_edge_login.h"
#include "fh_edge_connect.h"


/*
 * Routine to create a TCP socket and then login in the server
 */

inline FH_STATUS fh_edge_conn_login(fh_shr_lh_proc_t *lh_process,
                                        fh_shr_cfg_lh_line_t *  line,uint64_t *seq)
{
    FH_STATUS                   rc;
    int *socketp  = &lh_process->lines[0].primary.socket;
    static  int   count   = 0;

    uint32_t primary_addr = lh_process->lines[0].primary.config->address;
    uint16_t primary_port = lh_process->lines[0].primary.config->port;

    /* get a socket and connect as a client to the server */
    rc = fh_tcp_client(primary_addr, primary_port, socketp, 5000000);
    if ( rc != FH_OK) {
        if( count == 0){
            FH_LOG(LH, ERR, ("failed to connect to the TCP server for %s ", line->process->name));
            count = 1;
        }
        return rc;
    }

    count = 0;
    /* Now we have the connection achieved, we can login and start receiving traffic */
    if ( (( rc = login_tcp_server(*socketp, line, seq)) != FH_OK)) {
        close(*socketp);
        return FH_ERROR; /* The caller will retry  */
    }
    return FH_OK;
}
