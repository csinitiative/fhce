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
#include "fh_adm_reg_resp.h"

/*
 * adm_reg_resp_pack
 *
 * Packs a registration response before sending it to the client.
 */
FH_STATUS adm_reg_resp_pack(void *msg, char *data, int *length)
{
    fh_adm_reg_resp_t *m_reg = (fh_adm_reg_resp_t *) msg;
    fh_adm_reg_resp_t *d_reg = (fh_adm_reg_resp_t *) data;

    *length = sizeof(fh_adm_reg_resp_t);

    if (data == NULL) {
        return FH_OK;
    }

    d_reg->reg_conn_id = htonl(m_reg->reg_conn_id);

    return FH_OK;
}

/*
 * adm_reg_resp_unpack
 *
 * Unpacks a registration response received from the server.
 */
FH_STATUS adm_reg_resp_unpack(void *msg, char *data, int length)
{
    fh_adm_reg_resp_t *m_reg = (fh_adm_reg_resp_t *) msg;
    fh_adm_reg_resp_t *d_reg = (fh_adm_reg_resp_t *) data;

    FH_ASSERT(length == sizeof(fh_adm_reg_resp_t));

    m_reg->reg_conn_id = ntohl(d_reg->reg_conn_id);

    return FH_OK;
}

