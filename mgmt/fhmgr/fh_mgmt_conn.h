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

#ifndef __FH_MGMT_CONN_H__
#define __FH_MGMT_CONN_H__

#include "queue.h"
#include "fh_errors.h"

/*
 * Connection types
 *
 * Server is the listening TCP server for incoming connections.
 * Client is for all CLI connections and FH services connections.
 */
#define FH_MGMT_CONN_SERVER          (1)
#define FH_MGMT_CONN_CLIENT          (2)

/*--------------------------------------------------------------*/
/* Connection logic                                             */
/*--------------------------------------------------------------*/

struct fh_conn;

struct fh_serv;

typedef TAILQ_ENTRY(fh_conn) conn_le_t;
typedef TAILQ_HEAD(,fh_conn) conn_lh_t;

typedef struct fh_conn {
    conn_le_t       conn_le;       /* Linked-list element      */
    int             conn_fd;       /* Connection FD            */
    int             conn_type;     /* Connection type          */
    int             conn_cltype;   /* Client type (CLI/Service)*/
    uint32_t        conn_id;       /* Connection Identifier    */
    uint32_t        conn_addr;     /* Connection Address       */
    uint16_t        conn_port;     /* Connection Port          */
    uint32_t        conn_req_tid;  /* Connection Request TID   */
    uint32_t        conn_pid;      /* Connection Process ID    */
    uint64_t        conn_uptime;   /* Connection Up-Time       */
    struct fh_serv *conn_serv;     /* Service pointer          */
} fh_mgmt_conn_t;

/*--------------------------------------------------------------*/
/* Connection API                                               */
/*--------------------------------------------------------------*/

void      fh_mgmt_conn_init();
FH_STATUS fh_mgmt_conn_new(int type, int fd, uint32_t addr, uint16_t port);
void      fh_mgmt_conn_loop(uint64_t usecs);

#endif /* __FH_MGMT_CONN_H__ */
