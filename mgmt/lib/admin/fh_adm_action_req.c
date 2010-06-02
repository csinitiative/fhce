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
#include "fh_adm_action_req.h"

/*
 * adm_action_req_pack
 *
 * Packs a action request before sending it to the client.
 */
FH_STATUS adm_action_req_pack(void *msg, char *data, int *length)
{
    fh_adm_action_req_t *m_action = (fh_adm_action_req_t *) msg;
    fh_adm_action_req_t *d_action = (fh_adm_action_req_t *) data;

    *length = sizeof(fh_adm_action_req_t);

    if (data == NULL) {
        return FH_OK;
    }

    d_action->action_type = htonl(m_action->action_type);

    return FH_OK;
}

/*
 * adm_action_req_unpack
 *
 * Unpacks a action request received from the server.
 */
FH_STATUS adm_action_req_unpack(void *msg, char *data, int length)
{
    fh_adm_action_req_t *m_action = (fh_adm_action_req_t *) msg;
    fh_adm_action_req_t *d_action = (fh_adm_action_req_t *) data;

    FH_ASSERT(length == sizeof(fh_adm_action_req_t));

    m_action->action_type = ntohl(d_action->action_type);

    return FH_OK;
}

