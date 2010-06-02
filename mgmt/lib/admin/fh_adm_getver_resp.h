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

#ifndef __FH_ADM_GETVER_RESP_H__
#define __FH_ADM_GETVER_RESP_H__

#include "fh_errors.h"

typedef struct {
    char     getver_service[16];      /* Service name         */
    uint32_t getver_state;            /* Service state        */
    char     getver_build_date[24];   /* Build date           */
    char     getver_build_rev[8];     /* Build Git revision   */
} fh_adm_getver_resp_t;

FH_STATUS adm_getver_resp_pack   (void *msg, char *data, int *length);
FH_STATUS adm_getver_resp_unpack (void *msg, char *data, int length);

#endif /* __FH_ADM_GETVER_RESP_H__ */
