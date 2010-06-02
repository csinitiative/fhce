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

#include "fh_mgmt_admin.h"
#include "fh_adm_serv_resp.h"

/*
 * adm_serv_resp_pack
 *
 * Packs a GETVER respuest before sending it to the client.
 */
FH_STATUS adm_serv_resp_pack(void *msg, char *data, int *length)
{
    fh_adm_serv_resp_t *m_serv = (fh_adm_serv_resp_t *) msg;
    fh_adm_serv_resp_t *d_serv = (fh_adm_serv_resp_t *) data;
    register uint32_t i;

    *length = sizeof(fh_adm_serv_resp_t) +
              m_serv->serv_resp_size * m_serv->serv_resp_cnt;

    if (data == NULL) {
        return FH_OK;
    }

    d_serv->serv_resp_cmd  = htonl(m_serv->serv_resp_cmd);
    d_serv->serv_resp_size = htonl(m_serv->serv_resp_size);
    d_serv->serv_resp_cnt  = htonl(m_serv->serv_resp_cnt);
    strcpy(d_serv->serv_name, m_serv->serv_name);

    data += sizeof(fh_adm_serv_resp_t);
    msg  += sizeof(fh_adm_serv_resp_t);

    /*
     * For each response, pack the response
     */
    for (i=0; i<m_serv->serv_resp_cnt; i++) {
        FH_STATUS rc = fh_adm_pack(m_serv->serv_resp_cmd,
                                   data, msg, m_serv->serv_resp_size);
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("Failed to pack cmd: %s resp: %d out of %d",
                               FH_ADM_CMD_NAME(m_serv->serv_resp_cmd), i,
                               m_serv->serv_resp_cnt));
            return rc;
        }

        /*
         * Advance data and message pointer
         */
        data += m_serv->serv_resp_size;
        msg  += m_serv->serv_resp_size;
    }

    return FH_OK;
}

/*
 * adm_serv_resp_unpack
 *
 * Unpacks a GETVER respuest received from the server.
 */
FH_STATUS adm_serv_resp_unpack(void *msg, char *data, int length)
{
    fh_adm_serv_resp_t *m_serv = (fh_adm_serv_resp_t *) msg;
    fh_adm_serv_resp_t *d_serv = (fh_adm_serv_resp_t *) data;
    register uint32_t i;
    int exp_len;

    m_serv->serv_resp_cmd  = ntohl(d_serv->serv_resp_cmd);
    m_serv->serv_resp_size = ntohl(d_serv->serv_resp_size);
    m_serv->serv_resp_cnt  = ntohl(d_serv->serv_resp_cnt);
    strcpy(m_serv->serv_name, d_serv->serv_name);

    exp_len = sizeof(fh_adm_serv_resp_t) + m_serv->serv_resp_size *
        m_serv->serv_resp_cnt;

    if (length != exp_len) {
        FH_LOG(MGMT, ERR, ("Invalid service response length: %d != %d "
                           "(resp_size: %d resp_cnt: %d)", length, exp_len,
                           m_serv->serv_resp_size, m_serv->serv_resp_cnt));
        return FH_ERROR;
    }

    data += sizeof(fh_adm_serv_resp_t);
    msg  += sizeof(fh_adm_serv_resp_t);

    /*
     * For each response, pack the response
     */
    for (i=0; i<m_serv->serv_resp_cnt; i++) {
        FH_STATUS rc = fh_adm_unpack(m_serv->serv_resp_cmd,
                                     data, msg, m_serv->serv_resp_size);
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("Failed to pack cmd: %s resp: %d out of %d",
                               FH_ADM_CMD_NAME(m_serv->serv_resp_cmd), i,
                               m_serv->serv_resp_cnt));
            return rc;
        }

        /*
         * Advance data and message pointer
         */
        data += m_serv->serv_resp_size;
        msg  += m_serv->serv_resp_size;
    }


    return FH_OK;
}

