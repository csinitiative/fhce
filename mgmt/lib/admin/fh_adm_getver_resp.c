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
#include "fh_adm_getver_resp.h"

/*
 * adm_getver_resp_pack
 *
 * Packs a GETVER response before sending it to the client.
 */
FH_STATUS adm_getver_resp_pack(void *msg, char *data, int *length)
{
    fh_adm_getver_resp_t *m_getver = (fh_adm_getver_resp_t *) msg;
    fh_adm_getver_resp_t *d_getver = (fh_adm_getver_resp_t *) data;

    *length = sizeof(fh_adm_getver_resp_t);

    if (data == NULL) {
        return FH_OK;
    }

    memcpy(d_getver, m_getver, *length);
    d_getver->getver_state = htonl(m_getver->getver_state);

    return FH_OK;
}

/*
 * adm_getver_resp_unpack
 *
 * Unpacks a GETVER response received from the server.
 */
FH_STATUS adm_getver_resp_unpack(void *msg, char *data, int length)
{
    fh_adm_getver_resp_t *m_getver = (fh_adm_getver_resp_t *) msg;
    fh_adm_getver_resp_t *d_getver = (fh_adm_getver_resp_t *) data;

    FH_ASSERT(length == sizeof(fh_adm_getver_resp_t));

    memcpy(m_getver, d_getver, length);
    m_getver->getver_state = ntohl(d_getver->getver_state);

    return FH_OK;
}

