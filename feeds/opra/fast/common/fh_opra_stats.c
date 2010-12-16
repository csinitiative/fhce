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

/*
 * OS Header files
 */
#include <stdio.h>
#include <string.h>

/*
 * FH Common Header files
 */
#include "fh_log.h"

/*
 * FH OPRA Header files
 */
#include "fh_opra_stats.h"
#include "fh_opra_lh.h"

/*
 * Global statistics structure
 */
fh_opra_stats_t fh_opra_stats;

static fh_opra_stats_t *opst = &fh_opra_stats;

/*
 * fh_opra_stats_init
 *
 * Initialize OPRA statistics.
 */
void fh_opra_stats_init()
{
    FH_LOG(MGMT, VSTATE, ("Initializing OPRA statistics"));
    memset(opst, 0, sizeof(fh_opra_stats_t));
}

#define LLI(x)  ((long long int) (x))

/*
 * fh_opra_stats_dump
 *
 * Dump all statistics.
 */
void fh_opra_stats_dump()
{
    register int i;

    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("> OPRA statistics:"));
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("  - Rx Packets      : %lld", LLI(opst->opst_pkts)));
    FH_LOG_PGEN(DIAG, ("  - Rx Duplicates   : %lld", LLI(opst->opst_dups)));
    FH_LOG_PGEN(DIAG, ("  - Rx Messages     : %lld", LLI(opst->opst_msgs)));
    FH_LOG_PGEN(DIAG, ("  - Rx Bytes        : %lld", LLI(opst->opst_bytes)));
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));

    for (i=0; i < OPRA_CFG_MAX_FTLINES; i++) {
        fh_opra_ftline_stats_t *flst = &opst->opst_line[i];

        fh_opra_line_stats_t   *a_lst = &flst->flst_a_stats;
        fh_opra_line_stats_t   *b_lst = &flst->flst_b_stats;

        FH_LOG_PGEN(DIAG, ("  ---- FT Line [ %02d ] ----", i));
        FH_LOG_PGEN(DIAG, ("    * A/B Rx Packets      : %lld / %lld",
                           LLI(a_lst->lst_pkt_rx), LLI(b_lst->lst_pkt_rx)));
        FH_LOG_PGEN(DIAG, ("    * A/B Rx Duplicates   : %lld / %lld",
                           LLI(a_lst->lst_pkt_dups), LLI(b_lst->lst_pkt_dups)));
        FH_LOG_PGEN(DIAG, ("    * A/B Rx Messages     : %lld / %lld",
                           LLI(a_lst->lst_msg_rx), LLI(b_lst->lst_msg_rx)));
        FH_LOG_PGEN(DIAG, ("    * A/B Rx Bytes        : %lld / %lld",
                           LLI(a_lst->lst_bytes), LLI(b_lst->lst_bytes)));
    }

    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
}



