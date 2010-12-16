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

#ifndef __FH_OPRA_STATS_H__
#define __FH_OPRA_STATS_H__

#include <stdint.h>
#include "fh_opra_cfg.h"

/*
 * Line statistics
 */
typedef struct {
    uint64_t   lst_pkt_rx;              /* packets received */
    uint64_t   lst_pkt_errs;            /* packets errors */
    uint64_t   lst_pkt_dups;            /* duplicate packets */
    uint64_t   lst_pkt_late;            /* packets arrived late */
    uint64_t   lst_pkt_until_reset;     /* packets until reset */
    uint64_t   lst_pkt_bad_times;       /* packets received/unexpected time */
    uint64_t   lst_pkt_seq_jump;        /* larger than 100000 jump in seq no */
    uint64_t   lst_pkt_wrap_noreset;    /* wrap condition without a seq reset */
    uint64_t   lst_msg_rx;              /* messages received */
    uint64_t   lst_msg_loss;            /* messages missing after gaps */
    uint64_t   lst_msg_recovered;       /* messages recovered */
    uint64_t   lst_msg_late;            /* messages late */
    uint64_t   lst_bytes;               /* bytes received */
} fh_opra_line_stats_t;

/*
 * Fault-Tolerant line statistics with A and B lines.
 */
typedef struct {
    fh_opra_line_stats_t flst_a_stats;
    fh_opra_line_stats_t flst_b_stats;
} fh_opra_ftline_stats_t;

/*
 * Global OPRA statistics
 */
typedef struct {
    fh_opra_ftline_stats_t  opst_line[OPRA_CFG_MAX_FTLINES];
    uint64_t                opst_pkts;
    uint64_t                opst_dups;
    uint64_t                opst_loss;
    uint64_t                opst_msgs;
    uint64_t                opst_bytes;
} fh_opra_stats_t;

extern fh_opra_stats_t fh_opra_stats;

/*
 * OPRA statistics API
 */
void fh_opra_stats_init();
void fh_opra_stats_dump();

#endif /* __FH_OPRA_STATS_H__ */
