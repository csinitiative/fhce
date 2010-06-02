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

#ifndef __FH_ADM_REG_RESP_H__
#define __FH_ADM_REG_RESP_H__

#include "fh_errors.h"

typedef struct {
    uint32_t  reg_conn_id;  /* Client connection ID */
} fh_adm_reg_resp_t;

FH_STATUS adm_reg_resp_pack   (void *msg, char *data, int *length);
FH_STATUS adm_reg_resp_unpack (void *msg, char *data, int length);

#endif /* __FH_ADM_REG_RESP_H__ */
