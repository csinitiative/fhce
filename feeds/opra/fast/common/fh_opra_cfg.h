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

#ifndef __FH_OPRA_CFG_H__
#define __FH_OPRA_CFG_H__

#include "fh_errors.h"
#include "fh_opra_topic.h"
#include "fh_opra_lo.h"
#include "fh_config.h"

/*
 * OPRA Processes
 *
 * The OPRA feed is distributed over 4 processes today. And, each process
 * is handling a subset of all the OPRA lines. Each process runs on its own CPU
 * to prevent any context switching while processing data.
 */
#define OPRA_CFG_MAX_PROCS          (8)

/*
 * OPRA process structure
 */
typedef struct {
    uint16_t   op_init;
    uint16_t   op_cpu;
    uint16_t   op_idx;
    uint8_t    op_line_from;
    uint8_t    op_line_to;
} fh_opra_proc_t;

/*
 * OPRA lines configuration
 *
 * The OPRA FH has 48 FT lines, each FT line has a line A and a line B.
 * The FH performs some arbitrage between the lines A and B, and drops
 * the duplicate messages.
 */

#define OPRA_CFG_MAX_FTLINES        (48)

#define OPRA_CFG_LINE_A             (1)
#define OPRA_CFG_LINE_B             (2)

#define OPRA_CFG_LINE(_l)           ((_l) == OPRA_CFG_LINE_A ? "A" : "B")

/*
 * OPRA line structure
 */
typedef struct {
    uint16_t    ol_init;
    uint16_t    ol_enable;
    uint16_t    ol_side;
    uint16_t    ol_port;
    uint32_t    ol_mcaddr;
    char        ol_ifname[16];
} fh_opra_line_t;

/*
 * OPRA Fault-Tolerant (FT) line structure
 */
typedef struct {
    uint32_t         oftl_index;
    fh_opra_line_t   oftl_line_a;
    fh_opra_line_t   oftl_line_b;
} fh_opra_ftline_t;

/*
 * OPRA configuration structure
 */
typedef struct {
    uint16_t            ocfg_proc_id;
    uint8_t             ocfg_num_procs;
    uint8_t             ocfg_num_lines;
    fh_opra_proc_t      ocfg_procs[OPRA_CFG_MAX_PROCS];
    fh_opra_ftline_t    ocfg_lines[OPRA_CFG_MAX_FTLINES];
    void               *ocfg_private;
    fh_opra_topic_fmt_t ocfg_topic_fmt;
    fh_opra_lo_cfg_t    ocfg_lo_config;
    uint32_t            ocfg_table_size;
    uint8_t             ocfg_jitter_stats;
    uint8_t             ocfg_partial_publish;
    uint8_t             ocfg_lo_scp_enable;
    uint8_t             ocfg_line_status_enable;
    uint32_t            ocfg_line_status_period;
    uint32_t            ocfg_wrap_limit_high;
    uint32_t            ocfg_wrap_limit_low;
    uint32_t            ocfg_seq_jump_threshold;
    uint8_t             ocfg_periodic_stats;
    uint8_t             ocfg_periodic_stats_interval;
} fh_opra_cfg_t;

/*
 * Partial publish modes
 *
 * ALL:         means partial publish all fields in the message if changed.
 *
 * VALUE_ADDED: means only the fields that are being computed and, not coming
 *              from the RAW data.
 */

#define OPRA_CFG_PP_ALL         (1)
#define OPRA_CFG_PP_VALUE_ADDED (2)

/*
 * Exported global OPRA configuration
 */
extern fh_opra_cfg_t opra_cfg;

/*
 * OPRA configuration API
 */
FH_STATUS fh_opra_cfg_load(fh_cfg_node_t *config, int proc_id);
void      fh_opra_cfg_dump();

#endif /* __FH_OPRA_CFG_H__ */
