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

#ifndef __FH_ADM_SGT_RESP_H__
#define __FH_ADM_SGT_RESP_H__

typedef struct {
    char      sg_name[16];
    uint32_t  sg_serv_count;
    uint32_t  sg_serv_size;
} fh_adm_sg_resp_t;

typedef struct {
    char      serv_name[16];
} fh_adm_sg_serv_resp_t;

typedef struct {
    uint16_t  sgt_count; /* Number of service groups             */
    uint16_t  sgt_size;  /* Total service group response size    */
} fh_adm_sgt_resp_t;

FH_STATUS adm_sgt_resp_pack   (void *msg, char *data, int *length);
FH_STATUS adm_sgt_resp_unpack (void *msg, char *data, int length);

#endif /* __FH_ADM_SGT_RESP_H__ */
