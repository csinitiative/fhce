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

#ifndef __FH_ADM_STATS_RESP_H__
#define __FH_ADM_STATS_RESP_H__

#include "fh_errors.h"
#include "fh_mgmt_client.h"

/*
 * Line statistics
 */
typedef struct {
    char       line_name[32];        /* Line name                         */
    uint64_t   line_pkt_rx;          /* Packets received                  */
    uint64_t   line_pkt_dups;        /* Duplicate packets                 */
    uint64_t   line_pkt_errs;        /* Packet errors                     */
    uint64_t   line_pkt_late;        /* Packet late arrival               */
    uint64_t   line_pkt_until_reset; /* Packet until reset                */
    uint64_t   line_pkt_bad_times;   /* Packet received/unexpected times  */
    uint64_t   line_pkt_seq_jump;    /* Larger than 100000 jump in seq no */
    uint64_t   line_pkt_wrap_noreset;/* Wrap condition without a seq reset*/
    uint64_t   line_msg_rx;          /* Messages received                 */
    uint64_t   line_msg_loss;        /* Lost messages                     */
    uint64_t   line_msg_recovered;   /* Recovered messages                */
    uint64_t   line_msg_late;        /* Late messages                     */
    uint64_t   line_bytes;           /* Bytes received                    */
} fh_adm_line_stats_t;

/*
 * Feed handler statistics with a fixed number of lines
 */
typedef struct {
    char                stats_service[16];
    uint32_t            stats_state;
    uint32_t            stats_line_cnt;
    fh_adm_line_stats_t stats_lines[FH_MGMT_MAX_LINES];
} fh_adm_stats_resp_t;

FH_STATUS adm_stats_resp_pack   (void *msg, char *data, int *length);
FH_STATUS adm_stats_resp_unpack (void *msg, char *data, int length);

#endif /* __FH_ADM_STATS_RESP_H__ */
