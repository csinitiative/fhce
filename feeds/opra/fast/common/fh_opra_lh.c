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
 * System includes
 */
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>

/*
 * FH Common includes
 */
#include "fh_log.h"
#include "fh_time.h"
#include "fh_cpu.h"
#include "fh_util.h"
#include "fh_net.h"
#include "fh_udp.h"
#include "fh_mcast.h"
#include "fh_prof.h"
#include "fh_hist.h"
#include "fh_plugin.h"

/*
 * FH OPRA includes
 */
#include "fh_opra.h"
#include "fh_opra_cfg.h"
#include "fh_opra_lh.h"
#include "fh_opra_lh_tap.h"
#include "fh_opra_option.h"
#include "fh_opra_stats.h"
#include "fh_opra_ml.h"

/*
 * Fast API
 */
#include "fast_wrapper.h"
#include "fast_process.h"

/*
 * External definitions
 */
extern FH_STATUS fh_opra_pkt_process(Fast *fast, lh_line_t *l, uint8_t *buffer, uint32_t len);

uint32_t fh_opra_lh_line_num      = 0;
uint64_t fh_opra_lh_recv_time     = 0;
uint8_t  fh_opra_lh_publish_stats = 0;


/*
 * Static variables
 */
static pthread_t opra_lh_thread = 0;
static uint32_t  opra_lh_tid = 0;

static fh_plugin_hook_t opra_lh_ftline_notify  = NULL;
static fh_plugin_hook_t opra_lh_periodic_stats = NULL;

/*
 * Profiling declarations for Latency measurements
 */
FH_PROF_DECL(opra_recv_latency, 1000000, 20, 2);
FH_PROF_DECL(opra_proc_latency, 1000000, 20, 2);

/*
 * Static variables
 */
static lh_line_t   line_table[OPRA_CFG_MAX_FTLINES * 2];
static lh_ftline_t ftline_table[OPRA_CFG_MAX_FTLINES];
static int         line_count = 0;
static fd_set      lh_fdset;
static int         lh_fdmax;

/*
 * Line statistics for rate computation
 */
static fh_opra_line_stats_t line_stats[OPRA_CFG_MAX_FTLINES * 2];

/*
 * fh_opra_lh_get_stats
 *
 * Get the statistics for all the lines of this OPRA process
 */
void fh_opra_lh_get_stats(fh_adm_stats_resp_t *stats_resp)
{
    int i, idx;

    stats_resp->stats_line_cnt = line_count;

    for (i = 0; i < line_count; i++) {
         /*
          * The return is organized as A1 B1, A2 B2, ......A24 B24
          */
        idx = (i >= (line_count / 2)) ? (2 * (i - (line_count / 2))) + 1 : i * 2;
        fh_adm_line_stats_t *line = &stats_resp->stats_lines[idx];
        lh_line_t *l              = &line_table[i];

        strcpy(line->line_name, l->l_name);

        line->line_pkt_errs           = l->l_stats->lst_pkt_errs;
        line->line_pkt_rx             = l->l_stats->lst_pkt_rx;
        line->line_pkt_dups           = l->l_stats->lst_pkt_dups;
        line->line_pkt_late           = l->l_stats->lst_pkt_late;
        line->line_pkt_until_reset    = l->l_stats->lst_pkt_until_reset;
        line->line_pkt_bad_times      = l->l_stats->lst_pkt_bad_times;
        line->line_pkt_seq_jump       = l->l_stats->lst_pkt_seq_jump;
        line->line_pkt_wrap_noreset   = l->l_stats->lst_pkt_wrap_noreset;
        line->line_msg_rx             = l->l_stats->lst_msg_rx;
        line->line_msg_loss           = l->l_stats->lst_msg_loss;
        line->line_msg_recovered      = l->l_stats->lst_msg_recovered;
        line->line_msg_late           = l->l_stats->lst_msg_late;
        line->line_bytes              = l->l_stats->lst_bytes;
    }
}

/*
 * fh_opra_lh_clr_stats
 *
 * Clear the line-handler statistics.
 */
void fh_opra_lh_clr_stats()
{
    int i;

    for (i = 0; i < line_count; i++) {
        lh_line_t *l = &line_table[i];

        memset(l->l_stats, 0, sizeof(fh_opra_line_stats_t));
    }
}

/*
 * fh_opra_lh_get_tid
 *
 * Get the line-handler FP thread ID.
 */
uint32_t fh_opra_lh_get_tid()
{
    return opra_lh_tid;
}

/*
 * lh_line_add
 *
 * Add the line to the line-handler line table.
 */
static void lh_line_add(lh_line_t *l)
{
    /*
     * Configure the select fd map
     */
    FD_SET(l->l_sock, &lh_fdset);
    lh_fdmax = MAX(lh_fdmax, l->l_sock);

    line_count++;

    FH_ASSERT(line_count <= FH_MGMT_MAX_LINES);
}

/*
 * fh_opra_lh_late_opt
 *
 * Count the number of late messages that were recovered that were actually for
 * option that were updated by newer messages.
 */
void fh_opra_lh_late_opt(uint32_t l_index, fh_opra_opt_t *opt)
{
    lh_line_t *l = &line_table[l_index];

    FH_LOG(LH, VSTATE, ("Line %s received late message: %s",
                        l->l_name, opt->opt_topic));

    l->l_stats->lst_msg_late ++;
}

/*
 * fh_opra_lh_add_opt
 *
 * Add an option to a given line
 */
void fh_opra_lh_add_opt(uint32_t l_index, fh_opra_opt_t *opt)
{
    lh_line_t   *l   = &line_table[l_index];
    lh_ftline_t *ftl = l->l_ftline;

    // Add the option to the option list for this FT line
    TAILQ_INSERT_TAIL(&ftl->ftl_options, opt, opt_line_le);

    // Save the FT line index (1-24) in the option entry
    opt->opt_ftline_idx = ftl->ftl_config->oftl_index;
}

/*
 * lh_line_init
 *
 * Initialize a line and add it to the LH ingress scheduler.
 */
static FH_STATUS lh_line_init(fh_opra_ftline_t      *oftl,
                              fh_opra_line_t        *ol,
                              lh_ftline_t           *ftl,
                              fh_opra_line_stats_t  *line_stats)
{
    lh_line_t *l = &line_table[line_count];
    int        udp_flags = FH_UDP_FL_MAX_BUFSZ|FH_UDP_FL_MCAST;
    FH_STATUS  rc;
    int        sock;
    uint32_t   ifaddr = 0;

    memset(line_stats, 0, sizeof(fh_opra_line_stats_t));

    l->l_index  = line_count;
    l->l_config = ol;
    l->l_stats  = line_stats;
    l->l_ftline = ftl;

    /*
     * Save the FT line configuration, and if it is already set, make sure that
     * it is the correct configuration}.
     */
    if (ftl->ftl_config == NULL) {
        ftl->ftl_config = oftl;
    }
    else {
        FH_ASSERT(ftl->ftl_config == oftl);
    }

    if (l->l_config->ol_side == 0) {
        FH_LOG(LH, WARN, ("Skipping line - no A/B side specified"));
        return FH_OK;
    }

    /*
     * Set the line name based on the FT side (A/B) and the line number
     */
    sprintf(l->l_name, "%s%d", OPRA_CFG_LINE(l->l_config->ol_side),
            ftl->ftl_config->oftl_index);

    if (opra_cfg.ocfg_jitter_stats) {
        char hist_key[64];

        sprintf(hist_key, "Line %s Jitter Stats", l->l_name);

        l->l_jitter_hist = fh_hist_new(hist_key, 20, 2);
        FH_ASSERT(l->l_jitter_hist);
    }

    /*
     * Create the UDP multicast socket
     */
    rc = fh_udp_open(INADDR_ANY, l->l_config->ol_port, udp_flags, &sock);
    if (rc != FH_OK) {
        FH_LOG(LH, ERR, ("Failed to create multicast socket for line: %s", l->l_name));
        return rc;
    }

    /*
     * Check the interface IP address where to join the multicast group.
     */
    ifaddr = fh_net_ifaddr(sock, l->l_config->ol_ifname);
    if (ifaddr == 0) {
        FH_LOG(LH, ERR, ("Failed to retrieve interface '%s' IP address",
                         l->l_config->ol_ifname));
        close(sock);
        return FH_ERROR;
    }

    /*
     * Join the multicast group
     */
    rc = fh_mcast_join(sock, ifaddr, l->l_config->ol_mcaddr);
    if (rc != FH_OK) {
        FH_LOG(LH, ERR, ("Failed to join multicast group: %s (line %s)",
                         fh_net_ntoa(l->l_config->ol_mcaddr), l->l_name));
        close(sock);
        return rc;
    }

    l->l_sock = sock;

    /*
     * Add the line to the ingress loop.
     */
    lh_line_add(l);

    FH_LOG(LH, DIAG, ("OPRA line %s initialized", l->l_name));

    return FH_OK;
}

/*
 * lh_win_init
 *
 * Initialize the window as if the sequence numbers before the current base_sn
 * had arrived in order.
 */
static inline void lh_win_init(uint32_t *win, uint32_t base_sn)
{
    uint32_t base_idx = base_sn & WINSZ_MASK;
    uint32_t i;

    base_sn -= base_idx;

    for (i=0; i<=base_idx; i++) {
        win[i] = (uint32_t)(base_sn + i);
    }

    for (i=base_idx+1; i<WINSZ; i++) {
        win[i] = (uint32_t)(base_sn + i - WINSZ);
    }
}

/*
 * lh_ftline_notify
 *
 * Notify the world about the FT line event.
 */
static inline void lh_ftline_notify(lh_ftline_t *ftl, uint32_t ftline_event)
{
    FH_STATUS rc;

    if (opra_lh_ftline_notify) {
        opra_lh_ftline_notify(&rc, ftl, ftline_event);
    }
    else {
        FH_LOG(LH, STATE, ("FT Line %d event: %s (state:%s sn:%d missing:%d)",
                           ftl->ftl_config->oftl_index,
                           OPRA_FTLINE_EVENT_NAME(ftline_event),
                           OPRA_FTLINE_STATE_NAME(ftl->ftl_state),
                           ftl->ftl_seq_num, ftl->ftl_missing));
    }
}

/*
 * lh_line_reset
 *
 * Reset the line after a line sequence number reset.
 */
static void lh_line_reset(lh_line_t *l, uint32_t reset_sn)
{
    lh_ftline_t   *ftl = l->l_ftline;

    FH_LOG(LH, STATE, ("Reset on OPRA line %s with sequence num: %d", l->l_name, reset_sn));

    /* this line's reset flag is set -- other line has been reset and this one is now too */
    if (l->l_reset) {
        l->l_reset = 0;
        return;
    }

    /* set the partner in reset mode */
    l->l_peer->l_reset = 1;

    /* save the current reset SN */
    ftl->ftl_reset_sn = reset_sn;
    ftl->ftl_seq_num  = reset_sn;
    l->l_seq_num      = reset_sn;

    // Reset the duplicate detection window
    lh_win_init(ftl->ftl_window, reset_sn);

    /* there were some missing sequence numbers, line remains in a stale state until reset */
    if (ftl->ftl_missing > 0) {
        if (ftl->ftl_state != OPRA_FTLINE_STATE_STALE) {
            FH_LOG(LH, STATE, ("Line %s State: LOSS -> STALE: line_sn:%d (missing:%d)",
                               l->l_name, ftl->ftl_seq_num, ftl->ftl_missing));

            ftl->ftl_state = OPRA_FTLINE_STATE_STALE;
            lh_ftline_notify(ftl, OPRA_FTLINE_EVENT_STATE);
        }

        ftl->ftl_missing = 0;
    }

    /* after an explicit reset all messages are considered to be from the future */
    ftl->ftl_time = 0;
}

/*
 * fh_opra_lh_win_update
 *
 * Update the duplicate detection window.
 */
int fh_opra_lh_win_update(lh_line_t *l, uint32_t msg_sn, int num_msgs, uint32_t msg_time)
{
    lh_ftline_t *ftl = l->l_ftline;
    int          i;
    int          drop = 0;

    //RC - suppress unused parameter warning
    (void)msg_time;

    /*
     * Run the same logic all messages
     */
    for (i=0; i<num_msgs; i++, msg_sn++) {
        uint32_t idx    = msg_sn & WINSZ_MASK;
        uint32_t win_sn = ftl->ftl_window[idx];

        drop = 0;

        // First if we received the expected line sequence number
        if (msg_sn == (ftl->ftl_seq_num + 1)) {
            ftl->ftl_seq_num = msg_sn;
        }

        // The received sequence number is behind the current line SN
        else if (msg_sn <= ftl->ftl_seq_num) {
            if (win_sn == msg_sn) {
                FH_LOG(LH, DIAG, ("Line %s received duplicate SN: sn:%d line_sn:%d",
                                  l->l_name, msg_sn, ftl->ftl_seq_num));

                l->l_stats->lst_pkt_dups++;

                drop = 1;
                break;
            }
            else if (msg_sn == (win_sn + WINSZ)) {
                l->l_stats->lst_msg_recovered++;

                if (ftl->ftl_missing > 0) {
                    FH_LOG(LH, DIAG, ("Line %s recovered missing SN: sn:%d line_sn:%d",
                                      l->l_name, msg_sn, ftl->ftl_seq_num));

                    ftl->ftl_missing--;

                    if (ftl->ftl_missing == 0) {
                        FH_LOG(LH, VSTATE, ("Line %s State: LOSS -> OK: sn:%d line_sn:%d",
                                            l->l_name, msg_sn, ftl->ftl_seq_num));
                    }
                }
                else {
                    FH_LOG(LH, WARN, ("Line %s received startup out-of-order SN: sn:%d line_sn:%d",
                                      l->l_name, msg_sn, ftl->ftl_seq_num));
                }
            }
            else {
                FH_LOG(LH, DIAG, ("Line %s received late SN: sn:%d win_sn:%d line_sn:%d",
                                  l->l_name, msg_sn, win_sn, ftl->ftl_seq_num));

                l->l_stats->lst_pkt_late++;

                drop = 1;
                break;
            }
        }

        // The received sequence number is beyond the line SN (gap detected)
        else {
            if (ftl->ftl_seq_num != 0) {
                uint32_t gap = msg_sn - (ftl->ftl_seq_num+1);

                FH_LOG(LH, DIAG, ("Line %s SN gap detected: sn:%d gap:%d line_sn:%d",
                                  l->l_name, msg_sn, gap, ftl->ftl_seq_num));

                if (ftl->ftl_missing == 0) {
                    FH_LOG(LH, VSTATE, ("Line %s State: OK -> LOSS : sn:%d line_sn:%d",
                                        l->l_name, msg_sn, ftl->ftl_seq_num));
                }

                ftl->ftl_missing += gap;

                l->l_stats->lst_msg_loss += gap;
            }

            // Update the current line SN
            ftl->ftl_seq_num = msg_sn;
        }

        // Update the window if we don't drop the mesage
        if (!drop) {

            /*
             * If the current window SN is not the previous expected sequence number
             * for that window index, then we know that there was some unrecoverable
             * loss, and therefore, the missing packet will either come late (beyond
             * the reliability window size), or never show up (packet was lost on both
             * A and B lines). But, in either case, the line is now STALE, because there
             * are messages that will never be processed.
             */
            if (msg_sn != (win_sn + WINSZ)) {
                /*
                 * Notify that the line is STALE, and from this point in time,
                 * the line will never get out of stale state.
                 */
                if (ftl->ftl_state != OPRA_FTLINE_STATE_STALE) {
                    FH_LOG(LH, STATE, ("Line %s State: %s -> STALE: sn:%d line_sn:%d",
                                       l->l_name, ftl->ftl_missing ? "LOSS" : "OK",
                                       msg_sn, ftl->ftl_seq_num));

                    ftl->ftl_state = OPRA_FTLINE_STATE_STALE;
                    lh_ftline_notify(ftl, OPRA_FTLINE_EVENT_STATE);
                }
            }

            ftl->ftl_window[idx] = msg_sn;
        }
    }
    return drop;
}

/**
 *  @brief Convert an OPRA timestamp (numeric with digits laid out "HHMMssmmm") to a pure
 *         millisecond timestamp
 *
 *  @param opra_time timestamp being converted
 *  @return millisecond timestamp
 */
static inline uint32_t fh_opra_convert_time(uint32_t opra_time)
{
    uint32_t result = 0;
    result += (opra_time % 1000) ;                                        /* milliseconds */
    result += ( ((opra_time % 100000 ) / 1000) * 1000);                   /* seconds */
    result += ( ((opra_time % 10000000) / 100000) * 60000);               /* minutes */
    result += ( ((opra_time % 1000000000) / 10000000) * 60 * 60 * 1000);  /* hours   */
    return result;
}

/*
 * fh_opra_lh_is_dup
 *
 * Check whether a given OPRA packet is a duplicate
 */
int fh_opra_lh_is_dup(lh_line_t *l, char msg_cat, char msg_type,
                      uint32_t msg_sn, uint32_t num_msgs, uint32_t msg_time)
{
    lh_ftline_t *ftl = l->l_ftline;

    FH_LOG(LH, DIAG, ("Line %s msg_sn:%u line_sn:%u msg_time:%u line_time:%u",
                      l->l_name, msg_sn, ftl->ftl_seq_num, msg_time, ftl->ftl_time));

    /* when the FT line starts it first gets assigned a stale state, until it receives the start
       of day message. If the FH is started in the middle of the day, the FH will never get out
       of STALE state, which is the correct behavior, because there is no way to figure out what
       has been missed. */
    if (ftl->ftl_state == 0) {
        ftl->ftl_state = OPRA_FTLINE_STATE_STALE;
        lh_ftline_notify(ftl, OPRA_FTLINE_EVENT_STATE);
    }

    /* make sure that if the feed handler is restarted in the middle of the day it accepts the
       feed data and treats it as a large jump in sequence number */
    if (ftl->ftl_time == 0 || msg_time > ftl->ftl_time) {
        ftl->ftl_time = msg_time;
    }

    if (msg_cat == 'H') {
        switch (msg_type) {
        case 'C':
            // Start of day message, we are going to reset the line state to OK
            // It contains the SN of the previous message, so we don't go through
            // duplicate detection

            FH_LOG(LH, STATE, ("Line %s start of day - msg_sn:%d line_sn:%d",
                               l->l_name, msg_sn, ftl->ftl_seq_num));

            ftl->ftl_state = OPRA_FTLINE_STATE_OK;
            lh_ftline_notify(ftl, OPRA_FTLINE_EVENT_STATE);

            lh_line_reset(l, msg_sn);
            return 0;

        case 'K':
            FH_LOG(LH, STATE, ("Line %s reset - msg_sn:%d line_sn:%d",
                               l->l_name, msg_sn, ftl->ftl_seq_num));

            lh_line_reset(l, msg_sn);

            lh_ftline_notify(ftl, OPRA_FTLINE_EVENT_SN_RESET);
            break;

        /* start of test cycle - pretend it is a start of day, except do not put the feed
           handler in the OK state (that is reserved for start of day messages) */
        case 'A':
            FH_LOG(LH, STATE, ("Line %s start of test cycle - msg_sn:%d line_sn:%d",
                               l->l_name, msg_sn, ftl->ftl_seq_num));
            lh_line_reset(l, msg_sn);
            break;

        /* end of test cycle - just log it and continue processing (should increment the
           sequence number) */
        case 'B':
            FH_LOG(LH, STATE, ("Line %s end of test cycle - msg_sn:%d line_sn:%d",
                               l->l_name, msg_sn, ftl->ftl_seq_num));
            break;

        /* line integrity message - log it and move on (nothing to do here) */
        case 'N':
            FH_LOG(LH, DIAG, ("Line %s integrity - msg_sn:%u line_sn:%u msg_time:%u line_time:%u",
                              l->l_name, msg_sn, ftl->ftl_seq_num, msg_time, ftl->ftl_time));
            return 0;

        /* any other type H - there are a few other valid type H messages, but none we
           care about */
        default:
            FH_LOG(LH, DIAG, ("Line %s category 'H' - type:%c msg_sn:%u line_sn:%u msg_time:%u "
                              "line_time:%u", l->l_name, msg_type, msg_sn, ftl->ftl_seq_num,
                              msg_time, ftl->ftl_time));
        }
    }

    /* category C (administrative) message - log and ignore */
    else if (msg_cat == 'C') {
        FH_LOG(LH, DIAG, ("Line %s category 'C' - msg_sn:%u line_sn:%u msg_time:%u line_time:%u",
                          l->l_name, msg_sn, ftl->ftl_seq_num, msg_time, ftl->ftl_time));
        return 0;
    }

    /* there is a seqno wrap without a "reset sequence number" being received */
    /* THIS SHOULD BE EXCEEDINGLY UNLIKELY WITH BIGGER SEQ NOS -- ONLY LIKELY TO CAUSE PROBLEMS
    if ((msg_sn >= 1 && msg_sn < opra_cfg.ocfg_wrap_limit_low) &&
        (l->l_seq_num > (opra_cfg.ocfg_wrap_limit_high - 99))) {

        FH_LOG(LH, STATE, ("Line %s Sequence Number Wrap Detected - msg_sn:%d line_sn:%d",
                           l->l_name, msg_sn, l->l_seq_num));

        lh_line_reset(l, msg_sn-1);

        lh_ftline_notify(ftl, OPRA_FTLINE_EVENT_SN_RESET_WRAP);
        l->l_stats->lst_pkt_wrap_noreset++;

    }
    */
    /* forward jump in sequence number by a large amount (i.e. mid-day start) */
    if (msg_sn >= (l->l_seq_num + opra_cfg.ocfg_seq_jump_threshold)) {
        FH_LOG(LH, STATE, ("large sequence number gap on line %s: msg_sn %d, line_sn %d",
                           l->l_name, msg_sn, l->l_seq_num));
        lh_line_reset(l, msg_sn - 1);
        lh_ftline_notify(ftl, OPRA_FTLINE_EVENT_SN_RESET_JUMP);
        l->l_stats->lst_pkt_seq_jump++;
    }

    /* set the correct line sequence number in the line table */
    if (msg_sn > l->l_seq_num) {
        l->l_seq_num = msg_sn + (num_msgs - 1);
        /* NOT SUPPORTING THIS ARTIFICIAL WRAPPING
        if (l->l_seq_num > opra_cfg.ocfg_wrap_limit_high) {
            l->l_seq_num -= opra_cfg.ocfg_wrap_limit_high;
        }
        */
    }

    /* expecting a reset on this line; drop all packets until that reset is received. */
    if (l->l_reset) {
        FH_LOG(LH, DIAG, ("Line %s waiting for reset: drop sn:%d line_sn:%d",
                          l->l_name, msg_sn, ftl->ftl_seq_num));
        l->l_stats->lst_pkt_until_reset++;
        return 1;
    }

    return fh_opra_lh_win_update(l, msg_sn, num_msgs, msg_time);

}

/*
 * fh_opra_lh_run
 *
 * Line-Handler thread main loop.
 */
static void *fh_opra_lh_run(void *arg)
{
    fh_opra_proc_t 		*op = &opra_cfg.ocfg_procs[opra_cfg.ocfg_proc_id];
	fh_adm_stats_resp_t  periodic_stats;
    char            	 thread_name[16];
    FH_STATUS       	 rc;

    /* Select variables */
    fd_set          rdfds;
    int             fdmax = -1;
    register int    i = 0;
    int             nfd;
    struct timeval  tv;

    /* Receive variables */
    int                len;
    uint8_t            data[2048];
    struct sockaddr_in from;
    uint32_t           ifindex;
    uint32_t           ifaddr;
    uint64_t           rx_time;

    /* OPRA Fast initialization */
    Fast        fast;

	/* make sure we were not passed any arguments */
    FH_ASSERT(arg == NULL);

	/* initialize the FAST parser */
    fast_opra_init(&fast);

	/* store this thread's ID */
    opra_lh_tid = gettid();

	/* initialize the memory where periodic stats will be stored */
	memset(&periodic_stats, 0, sizeof(fh_adm_stats_resp_t));

	/* if we are collecting stats, initialize the stats histograms */
    if (FH_LL_OK(LH, STATS)) {
        FH_PROF_INIT(opra_proc_latency);
        FH_PROF_INIT(opra_recv_latency);
    }

    /* set thread affinity */
    rc = fh_cpu_setaffinity(CPU(op->op_cpu));
    if (rc != FH_OK) {
        FH_LOG(LH, WARN, ("Failed to assign CPU affinity %d to OPRA LH", op->op_cpu));
    }

    /* log the thread along with thread ID and CPU affnity settings */
    sprintf(thread_name, "OPRA_LH_%d", opra_cfg.ocfg_proc_id);
    fh_log_thread_start(thread_name);

    /* start the main loop */
    while (!opra_stopped) {

        /* wake-up every 100ms if idle to figure out whether we have to exit or not */
        tv.tv_sec  = 0;
        tv.tv_usec = 100000;

        /* reset the select parameters */
        memcpy(&rdfds, &lh_fdset, sizeof(fd_set));
        fdmax = lh_fdmax;

        /* enter the select loop and never time out */
        nfd = select((int)(fdmax + 1), &rdfds, NULL, NULL, &tv);

        /* if it is time to publish periodic stats (and periodic stats is on), do so */
        if (fh_opra_lh_publish_stats && opra_lh_periodic_stats) {
			fh_opra_lh_get_stats(&periodic_stats);
            opra_lh_periodic_stats(&rc, &periodic_stats);
            fh_opra_lh_publish_stats = 0;
        }

        if (nfd == -1)
        {
          FH_LOG(LH, DIAG, ("OPRA LH select failed: %s", strerror(errno)));
          continue;
        }

        if (nfd == 0) {
          continue;
        }

        /*
         * Loop through all lines to see whether we detected some activity
         */
        for (i = 0; i < line_count && nfd > 0; i++) {
            lh_line_t *l = &line_table[i];
            //  loopcount++;
            /*
             * Check whether the filedescriptor is set in the return select fdset
             */
            if (!FD_ISSET(l->l_sock, &rdfds)) {
                continue;
            }

            if (FH_LL_OK(LH,STATS)) {
                FH_PROF_BEG(opra_recv_latency);
            }

            FH_LOG(LH, INFO, ("Processing packet on line: %s", l->l_name));
            nfd--;

            /*
             * Perform the UDP receive
             */
            len = fh_udp_recv(l->l_sock, data, sizeof(data), &from,
                              &ifindex, &ifaddr, &rx_time);
            if (len < 0) {
                FH_LOG(LH, DIAG, ("Failed to read UDP packet on line: %s", l->l_name));
                continue;
            }

            /*
             * Jitter statistics
             */
            if (opra_cfg.ocfg_jitter_stats) {
                uint64_t now, jitter;
                fh_time_get(&now);

                jitter = now - rx_time;

                fh_hist_add(l->l_jitter_hist, jitter);

                if ((l->l_jitter_hist->hg_total % 100000) == 0) {
                    fh_hist_print(l->l_jitter_hist);
                    fh_hist_reset(l->l_jitter_hist);
                }
            }

            FH_LOG(LH, DIAG, ("Processing packet on line: %s from %s to %s at %d.%d secs",
                              l->l_name, fh_net_ntoa(from.sin_addr.s_addr), fh_net_ntoa(ifaddr),
                              (uint32_t) (rx_time/1000000), (uint32_t) (rx_time%1000000)));

            if (FH_LL_OK(LH,STATS)) {
                FH_PROF_END(opra_recv_latency);
                FH_PROF_BEG(opra_proc_latency);
            }

            /* account for the packet and bytes even if the packets is corrupt or a duplicate */
            l->l_stats->lst_pkt_rx++;
            l->l_stats->lst_bytes += len;

            /*
             * First validate that it is a correct OPRA packet
             */
            if (data[0] != SOH) {
                l->l_stats->lst_pkt_errs++;
                continue;
            }

            /*
             * Save the current global OPRA line number, and the current packet receive time.
             * These values will be available to the downstream message processing logic.
             */
            fh_opra_lh_line_num  = l->l_index;
            fh_opra_lh_recv_time = rx_time;

            /*
             * Process the OPRA packet and perform duplicate detection
             */
            if (fh_opra_pkt_process(&fast, l, data, len) != FH_OK) {
                continue;
            }

            if (FH_LL_OK(LH,STATS)) {
                FH_PROF_END(opra_proc_latency);
            }
        }

        /* flush the pending messages to the fabric */
        fh_opra_ml_flush();
    }

    fh_log_thread_stop(thread_name);

    return NULL;
}

/*
 * Initialize the FT line
 */
static void fh_ftline_init()
{
    uint32_t i;

    memset(ftline_table, 0, sizeof(ftline_table));

    for (i = 0; i < OPRA_CFG_MAX_FTLINES; i++) {
        lh_ftline_t *ftl = &ftline_table[i];
        TAILQ_INIT(&ftl->ftl_options);
        lh_win_init(ftl->ftl_window, 0);
        ftl->ftl_state = 0;
        ftl->ftl_time  = 0;
        ftl->ftl_priv  = NULL;
    }
}

/*
 * fh_opra_lh_init
 *
 * Initialize all the lines, and join all the multicast groups.
 */
static FH_STATUS fh_opra_lh_init(int record_bytes)
{
    fh_opra_proc_t *op = &opra_cfg.ocfg_procs[opra_cfg.ocfg_proc_id];
    int i, ftidx, num_ftlines;
    FH_STATUS rc;

    /*
     * Initialize the select fd set.
     */
    FD_ZERO(&lh_fdset);
    lh_fdmax = 0;

    fh_ftline_init();

    /*
     * If we are tapping the lines, then we need to create the files upfront.
     */
    if (record_bytes) {
        int line_cnt = 0;

        fh_opra_lh_tap_init(record_bytes);

        for (i=op->op_line_from; i<=op->op_line_to; i++) {
            fh_opra_lh_tap_open('A', i, line_cnt);
            line_cnt++;
        }
        for (i=op->op_line_from; i<=op->op_line_to; i++) {
            fh_opra_lh_tap_open('B', i, line_cnt);
            line_cnt++;
        }
    }

    /*
     * Initialize all the A lines first. We are making sure that A lines are
     * processed first in the loop because they will most likely have data that
     * is coming faster than on the B lines.
     */
    for (i=op->op_line_from, ftidx = 0; i<=op->op_line_to; i++, ftidx++) {
        fh_opra_ftline_t *oftl = &opra_cfg.ocfg_lines[i];

        if (oftl->oftl_line_a.ol_enable) {
            rc = lh_line_init(oftl, &oftl->oftl_line_a, &ftline_table[ftidx],
                              &fh_opra_stats.opst_line[i].flst_a_stats);
            if (rc != FH_OK) {
                return rc;
            }
        }
    }

    /*
     * Initialize all the B lines
     */
    for (i=op->op_line_from, ftidx = 0; i<=op->op_line_to; i++, ftidx++) {
        fh_opra_ftline_t *oftl = &opra_cfg.ocfg_lines[i];

        if (oftl->oftl_line_b.ol_enable) {
            rc = lh_line_init(oftl, &oftl->oftl_line_b, &ftline_table[ftidx],
                              &fh_opra_stats.opst_line[i].flst_b_stats);
            if (rc != FH_OK) {
                return rc;
            }
        }
    }

    /*
     * Associate A lines with B lines
     */
    num_ftlines = line_count / 2;
    for (i=0; i<num_ftlines; i++) {
        lh_line_t *a_l = &line_table[i];
        lh_line_t *b_l = &line_table[num_ftlines+i];

        a_l->l_peer = b_l;
        b_l->l_peer = a_l;

        if (record_bytes) {
            a_l->l_tap = 1;
            b_l->l_tap = 1;
        }
        else {
            a_l->l_tap = 0;
            b_l->l_tap = 0;
        }
    }

    return FH_OK;
}

/*
 * fh_opra_lh_start
 *
 * Start the Line-Handler component. This includes the configuration of all
 * the lines that the process is supposed to join, and spawning a LH thread
 * that blocks on a select loop until it is notified to exit.
 */
FH_STATUS fh_opra_lh_start(int record_bytes)
{
    FH_STATUS rc;

    /* load the OPRA line status notification hook */
    opra_lh_ftline_notify = fh_plugin_get_hook(FH_PLUGIN_OPRA_FTLINE_EVENT);
    if (opra_lh_ftline_notify) {
        FH_LOG(MGMT, STATE, ("FH Plugin loaded: OPRA line event notification"));
    }

    /* cache the OPRA periodic statistics publication hook (if one is registered) */
    opra_lh_periodic_stats = fh_plugin_get_hook(FH_PLUGIN_PERIODIC_STATS);
    if (opra_lh_periodic_stats) {
        FH_LOG(MGMT, STATE, ("FH Plugin loaded: OPRA periodic statistics publication"));
    }

    /*
     * Initialization of the option table
     */
    rc = fh_opra_opt_init();
    if (rc != FH_OK) {
        FH_LOG(LH, ERR, ("Failed to iniatize the OPRA Option table"));
        return rc;
    }

    /*
     * Initialization of the lines and join all the multicast groups
     */
    rc = fh_opra_lh_init(record_bytes);
    if (rc != FH_OK) {
        FH_LOG(LH, ERR, ("Failed to iniatize the OPRA LH sub-system"));
        return rc;
    }

    /*
     * Start the OPRA line-handler thread
     */
    if (pthread_create(&opra_lh_thread, NULL, fh_opra_lh_run, NULL) < 0) {
        FH_LOG(LH, ERR, ("Failed to start OPRA line-handler thread (id:%d): %s (%d)",
                           opra_cfg.ocfg_proc_id, strerror(errno), errno));
        return FH_ERROR;
    }


    return FH_OK;
}

/*
 * fh_opra_lh_wait
 *
 * Wait for the termination of the OPRA Line-Handler thread.
 */
void fh_opra_lh_wait()
{
    if (opra_lh_tid) {
        pthread_join(opra_lh_thread, NULL);
        FH_LOG(LH, VSTATE, ("OPRA line-handler thread exited: id:%d tid:0x%x",
                              opra_cfg.ocfg_proc_id, opra_lh_tid));
    }
}

/*
 * fh_opra_lh_rates
 *
 * Dumps some statistics about message and packet rates when enabled.
 */
void fh_opra_lh_rates(int aggregated)
{
    if (FH_LL_OK(LH, STATS)) {
        fh_opra_line_stats_t tmp_lst;
        fh_opra_line_stats_t total_lst;
        register int i;

        memset(&total_lst, 0, sizeof(total_lst));

        for (i = 0; i < line_count/2; i++) {
            fh_opra_line_stats_t *lst      = line_table[i].l_stats;
            fh_opra_line_stats_t *prev_lst = &line_stats[i];

            /*
             * Take a snapshot of the line statistics to get consistent
             * counters from one cycle to another
             */
            memcpy(&tmp_lst, lst, sizeof(tmp_lst));

            if (aggregated) {
                total_lst.lst_pkt_rx   += tmp_lst.lst_pkt_rx   - prev_lst->lst_pkt_rx;
                total_lst.lst_pkt_dups += tmp_lst.lst_pkt_dups - prev_lst->lst_pkt_dups;
                total_lst.lst_pkt_errs += tmp_lst.lst_pkt_errs - prev_lst->lst_pkt_errs;
                total_lst.lst_msg_rx   += tmp_lst.lst_msg_rx   - prev_lst->lst_msg_rx;
            }
            else {
                FH_LOG(LH,VSTATE, ("Line Stats: %-4s: %5d PPS - %6d MPS - (dups: %d errs: %d)",
                                    tmp_lst.lst_pkt_rx   - prev_lst->lst_pkt_rx,
                                    tmp_lst.lst_msg_rx   - prev_lst->lst_msg_rx,
                                    tmp_lst.lst_pkt_dups - prev_lst->lst_pkt_dups,
                                    tmp_lst.lst_pkt_errs - prev_lst->lst_pkt_errs));
            }

            /*
             * Save the current stats for the next cycle
             */
            memcpy(&line_stats[i], &tmp_lst, sizeof(tmp_lst));
        }

        if (aggregated) {
            FH_LOG(LH, XSTATS, ("LH Aggregated Stats: %5d PPS - %6d MPS - (dups: %d errs: %d)",
                                total_lst.lst_pkt_rx,   total_lst.lst_msg_rx,
                                total_lst.lst_pkt_dups, total_lst.lst_pkt_errs));
        }
    }
}

/*
 * fh_opra_lh_latency
 *
 * Dumps some statistics about latency when enabled.
 */
void fh_opra_lh_latency()
{
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_PRINT(opra_recv_latency);
        FH_PROF_PRINT(opra_proc_latency);
    }
}

