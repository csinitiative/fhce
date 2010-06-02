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
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "fh_log.h"
#include "fh_adm_sgt_resp.h"

/*
 * adm_sg_resp_pack
 *
 * Packs a service group response before sending it to the client.
 */
FH_STATUS adm_sgt_resp_pack(void *msg, char *data, int *length)
{
    fh_adm_sgt_resp_t *m_sgt = (fh_adm_sgt_resp_t *) msg;
    fh_adm_sgt_resp_t *d_sgt = (fh_adm_sgt_resp_t *) data;
    int i;

    *length = sizeof(fh_adm_sgt_resp_t) + m_sgt->sgt_size;

    if (data == NULL) {
        return FH_OK;
    }

    d_sgt->sgt_count = htons(m_sgt->sgt_count);
    d_sgt->sgt_size  = htons(m_sgt->sgt_size);

    if (m_sgt->sgt_count > 0) {
        char *m_ptr = (char *)(m_sgt+1);
        char *d_ptr = (char *)(d_sgt+1);

        for (i=0; i<m_sgt->sgt_count; i++) {
            fh_adm_sg_resp_t *m_sg = (fh_adm_sg_resp_t *)m_ptr;
            fh_adm_sg_resp_t *d_sg = (fh_adm_sg_resp_t *)d_ptr;

            strcpy(d_sg->sg_name, m_sg->sg_name);
            d_sg->sg_serv_count = htonl(m_sg->sg_serv_count);
            d_sg->sg_serv_size  = htonl(m_sg->sg_serv_size);

            if (m_sg->sg_serv_count > 0) {
                char *m_serv_ptr = (char *)(m_sg+1);
                char *d_serv_ptr = (char *)(d_sg+1);

                // Copy everything as a single block, since there are only
                // strings.

                memcpy(d_serv_ptr, m_serv_ptr, m_sg->sg_serv_count *
                       sizeof(fh_adm_sg_serv_resp_t));
            }

            m_ptr += sizeof(fh_adm_sg_resp_t) + m_sg->sg_serv_size;
            d_ptr += sizeof(fh_adm_sg_resp_t) + m_sg->sg_serv_size;
        }
    }

    return FH_OK;
}

/*
 * adm_sgt_resp_unpack
 *
 * Unpacks a service group tree response received from the server.
 */
FH_STATUS adm_sgt_resp_unpack(void *msg, char *data, int length)
{
    fh_adm_sgt_resp_t *m_sgt = (fh_adm_sgt_resp_t *) msg;
    fh_adm_sgt_resp_t *d_sgt = (fh_adm_sgt_resp_t *) data;
    int i;

    m_sgt->sgt_count = ntohs(d_sgt->sgt_count);
    m_sgt->sgt_size  = ntohs(d_sgt->sgt_size);

    FH_ASSERT((uint32_t)length == (sizeof(fh_adm_sgt_resp_t) + m_sgt->sgt_size));

    if (m_sgt->sgt_count > 0) {
        char *m_ptr = (char *)(m_sgt+1);
        char *d_ptr = (char *)(d_sgt+1);

        for (i=0; i<m_sgt->sgt_count; i++) {
            fh_adm_sg_resp_t *m_sg = (fh_adm_sg_resp_t *)m_ptr;
            fh_adm_sg_resp_t *d_sg = (fh_adm_sg_resp_t *)d_ptr;

            strcpy(m_sg->sg_name, d_sg->sg_name);
            m_sg->sg_serv_count = ntohl(d_sg->sg_serv_count);
            m_sg->sg_serv_size  = ntohl(d_sg->sg_serv_size);

            if (m_sg->sg_serv_count > 0) {
                char *m_serv_ptr = (char *)(m_sg+1);
                char *d_serv_ptr = (char *)(d_sg+1);

                // Copy everything as a single block, since there are only
                // strings.

                memcpy(m_serv_ptr, d_serv_ptr, m_sg->sg_serv_count *
                       sizeof(fh_adm_sg_serv_resp_t));
            }

            m_ptr += sizeof(fh_adm_sg_resp_t) + m_sg->sg_serv_size;
            d_ptr += sizeof(fh_adm_sg_resp_t) + m_sg->sg_serv_size;
        }
    }

    return FH_OK;
}

