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
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "fh_log.h"
#include "fh_adm_status_resp.h"

/*
 * adm_status_resp_pack
 *
 * Packs a STATUS response before sending it to the client.
 */
FH_STATUS adm_status_resp_pack(void *msg, char *data, int *length)
{
    fh_adm_status_resp_t *m_status = (fh_adm_status_resp_t *) msg;
    fh_adm_status_resp_t *d_status = (fh_adm_status_resp_t *) data;

    *length = sizeof(fh_adm_status_resp_t);

    if (data == NULL) {
        return FH_OK;
    }

    strcpy(d_status->status_service, m_status->status_service);
    d_status->status_state  = htonl(m_status->status_state);
    d_status->status_pid    = htonl(m_status->status_pid);
    d_status->status_fp_cpu = htonl(m_status->status_fp_cpu);
    d_status->status_fp_tid = htonl(m_status->status_fp_tid);
    d_status->status_uptime = htonl(m_status->status_uptime);
    d_status->status_pmem   = htonl(m_status->status_pmem);
    d_status->status_putime = htonl(m_status->status_putime);
    d_status->status_pstime = htonl(m_status->status_pstime);

    return FH_OK;
}

/*
 * adm_status_resp_unpack
 *
 * Unpacks a STATUS response received from the server.
 */
FH_STATUS adm_status_resp_unpack(void *msg, char *data, int length)
{
    fh_adm_status_resp_t *m_status = (fh_adm_status_resp_t *) msg;
    fh_adm_status_resp_t *d_status = (fh_adm_status_resp_t *) data;

    FH_ASSERT(length == sizeof(fh_adm_status_resp_t));

    strcpy(m_status->status_service, d_status->status_service);
    m_status->status_state  = ntohl(d_status->status_state);
    m_status->status_pid    = ntohl(d_status->status_pid);
    m_status->status_fp_cpu = ntohl(d_status->status_fp_cpu);
    m_status->status_fp_tid = ntohl(d_status->status_fp_tid);
    m_status->status_uptime = ntohl(d_status->status_uptime);
    m_status->status_pmem   = ntohl(d_status->status_pmem);
    m_status->status_putime = ntohl(d_status->status_putime);
    m_status->status_pstime = ntohl(d_status->status_pstime);

    return FH_OK;
}

