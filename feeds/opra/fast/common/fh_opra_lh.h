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

#ifndef __FH_OPRA_LH_H__
#define __FH_OPRA_LH_H__

#include "fh_errors.h"
#include "fh_hist.h"
#include "fh_mgmt_admin.h"
#include "fh_opra_cfg.h"
#include "fh_opra_stats.h"
#include "fh_opra_option.h"

extern uint32_t fh_opra_lh_line_num;
extern uint64_t fh_opra_lh_recv_time;
extern uint8_t  fh_opra_lh_publish_stats;

/*
 * The window size should probably be configurable. If the delay between lines A
 * and B is big, we need a bigger window. At 50K MPS per FT line, it gives us 50
 * msgs per millisecond (on average). So, if the lines are apart from each other
 * by 10ms, then we need to have a window size of at least 10 * 50 msgs = 500
 * msgs. Setting the window size to 512 for now. Gives us plenty of growth. Note
 * that 50K MPS is (24 * 50K) = 1.2M MPS
 */
#define WINSZ_BITS  (9)
#define WINSZ       (1<<WINSZ_BITS)
#define WINSZ_MASK  (WINSZ-1)
#define WIN_INIT    ((uint32_t)-1)

/*
 * FH Line definition
 *
 * Each line A and B points to their shared FT line structure
 * This object contains all the information about duplicate detection.
 */
typedef struct lh_ftline {
    fh_opra_opt_lh_t  ftl_options;      /* List of options */
    fh_opra_ftline_t *ftl_config;       /* FT line reference */
    uint32_t          ftl_reset_sn;     /* Reset SN expected */
    uint32_t          ftl_seq_num;      /* Current SN */
    uint32_t          ftl_missing;      /* Missing mesasges */
    uint32_t          ftl_window[WINSZ];/* Line SN window */
    uint32_t          ftl_state;        /* State */
    void             *ftl_priv;         /* Private context for plugin-use only */
    uint32_t          ftl_time;         /* Time in pkt message */
} lh_ftline_t;

/*
 * FT Line State
 */
#define OPRA_FTLINE_STATE_OK         (1)
#define OPRA_FTLINE_STATE_STALE      (2)

#define OPRA_FTLINE_STATE_NAME(ev) (              \
    (ev) == OPRA_FTLINE_STATE_OK     ? "OK"     : \
    (ev) == OPRA_FTLINE_STATE_STALE  ? "STALE"  : \
    "ST_UNKNOWN")

/*
 * FT Line Event
 */
#define OPRA_FTLINE_EVENT_STATE           (1)
#define OPRA_FTLINE_EVENT_SN_RESET        (2)
#define OPRA_FTLINE_EVENT_SN_RESET_JUMP   (3)
#define OPRA_FTLINE_EVENT_SN_RESET_WRAP   (4)
#define OPRA_FTLINE_EVENT_STATS           (5)


#define OPRA_FTLINE_EVENT_NAME(ev) (                            \
    (ev) == OPRA_FTLINE_EVENT_STATE         ? "STATE"         : \
    (ev) == OPRA_FTLINE_EVENT_SN_RESET      ? "SN_RESET"      : \
    (ev) == OPRA_FTLINE_EVENT_SN_RESET_JUMP ? "SN_RESET_JUMP" : \
    (ev) == OPRA_FTLINE_EVENT_SN_RESET_WRAP ? "SN_RESET_WRAP" : \
    (ev) == OPRA_FTLINE_EVENT_STATS         ? "STATS"         : \
    "EV_UNKNOWN")

/*
 * Line definition
 */
typedef struct lh_line {
    char                  l_name[4];    /* Line name            */
    int                   l_index;      /* Line index in LH     */
    int                   l_sock;       /* Line socket          */
    fh_opra_line_t       *l_config;     /* Line configuration   */
    fh_opra_line_stats_t *l_stats;      /* Line statistics      */
    lh_ftline_t          *l_ftline;     /* FT partner line      */
    uint32_t              l_reset;      /* Reset is pending     */
    struct lh_line       *l_peer;       /* A/B peer             */
    fh_hist_t            *l_jitter_hist;/* Jitter statistics    */
    uint32_t              l_tap;        /* Tap the line only    */
    uint32_t              l_seq_num;    /* Line Sequence number */
} lh_line_t;


/*
 * Line-handler API
 */
FH_STATUS fh_opra_lh_start(int record_bytes);
void      fh_opra_lh_wait();
void      fh_opra_lh_get_stats(fh_adm_stats_resp_t *stats_resp);
void      fh_opra_lh_clr_stats();
void      fh_opra_lh_latency();
void      fh_opra_lh_rates(int aggregated);
uint32_t  fh_opra_lh_get_tid();
void      fh_opra_lh_add_opt(uint32_t l_index, fh_opra_opt_t *opt);
void      fh_opra_lh_late_opt(uint32_t l_index, fh_opra_opt_t *opt);

int       fh_opra_lh_is_dup(lh_line_t *l, char msg_cat, char msg_type,
                            uint32_t msg_sn, uint32_t num_msgs, uint32_t msg_time);
int       fh_opra_lh_win_update(lh_line_t *l, uint32_t msg_sn, int num_msgs, uint32_t msg_time);

#endif /* __FH_OPRA_LH_H__ */
