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

//
/*********************************************************************/
/* file: fh_arca_lh.c                                                */
/* Usage: line handler for arca multicast feed handler               */
/* Author: Ross Cooperman of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <semaphore.h>
#include <time.h>
#include <assert.h>

// Common FH headers
#include "fh_errors.h"
#include "fh_log.h"
#include "fh_cpu.h"
#include "fh_mgmt_admin.h"

// Arca FH headers
#include "fh_arca.h"
#include "fh_arca_lh.h"
#include "fh_arca_cfg.h"
#include "fh_arca_util.h"
#include "fh_arca_headers.h"

// static variables
static pthread_t    arca_lh_thread  = 0;
static int          arca_lh_tid     = 0;

// snapshot of several aggregated statistics (for comparison when doing snapshots)
static fh_arca_lh_stats_t   fh_arca_lh_aggr_stats;

/*! \brief Return the thread ID of the line handler process
 *
 *  \return requested thread ID
 */
int fh_arca_lh_get_tid()
{
    return arca_lh_tid;
}

/*! \brief Line handler thread body
 *
 *  \param arg argument required to satisfy pthread_create signature (must be NULL)
 *  \return this function will always return NULL
 */
static void *fh_arca_lh_run(void *arg)
{
    char    *thread_name = NULL;
    int     cpu_mask     = 1; 
    // store the thread ID of this newly created thread
    arca_lh_tid = gettid();
    
    // generate a thread name string for this thread
    thread_name = fh_arca_util_thread_name("LH");
    fh_log_thread_start(thread_name);
    
    // make sure this function wasn't inadvertantly passed some data (and suppress warning)
    FH_ASSERT(arg == NULL);
    
    // set the proper CPU affinity for this process
    cpu_mask <<= fh_arca_cfg.cpu;
    if ((fh_arca_cfg.cpu > 31) || (fh_cpu_setaffinity(cpu_mask) != FH_OK)) {
        FH_LOG(LH, WARN, ("failed to assign CPU affinity %d to line handler", fh_arca_cfg.cpu));
    }
    
    // we are done initializing, unlock the thread init semaphore
    if (sem_post(&fh_arca_thread_init) == -1) {
        FH_LOG(MGMT, ERR, ("unable to unlock init semaphore for LH thread: %s", strerror(errno)));
        fh_arca_stopped = 1;
    }
    
    //TODO - clean up code past this point (in call graph)
    rcv_loop(fh_arca_proc_args.main_sockets, &fh_arca_stopped);

    // log a "thread stop" message and return
    fh_log_thread_stop(thread_name);
    return NULL;
}

/*! \brief Start the line handler thread (the thread that does all the work) 
 *
 *  \return status code indicating success or failure
 */
FH_STATUS fh_arca_lh_start()
{
    struct timespec  lock_wait;

    // lock the thread init semaphore
    memset(&lock_wait, 0, sizeof(struct timespec));
    lock_wait.tv_sec = time(NULL) + 5;
    if (sem_timedwait(&fh_arca_thread_init, &lock_wait) == -1) {
        FH_LOG(MGMT, ERR, ("unable to lock init semaphore for LH thread: %s", strerror(errno)));
        return FH_ERROR;
    }
    
    if (pthread_create(&arca_lh_thread, NULL, fh_arca_lh_run, NULL) < 0) {
        FH_LOG(LH, ERR, ("Failed to start Arca line handler thread (%s): %s (%d)",
                         fh_arca_cfg.name, strerror(errno), errno));
        return FH_ERROR;
    }
        
    // if we get to this point, success
    return FH_OK;
}

/*! \brief Block until the line handler process has exited
 */
void fh_arca_lh_wait()
{
    if (arca_lh_thread) {
        pthread_join(arca_lh_thread, NULL);
        FH_LOG(LH, VSTATE, ("Arca line handler thread (%s) exited: tid:0x%x",
                            fh_arca_cfg.name, gettid()));
    }
}

/*! \brief Get aggregated statistics for all lines that belong to this process
 *
 *  \param stats_resp response structure for an fh manager stats request
 */
void fh_arca_lh_get_stats(fh_adm_stats_resp_t *stats_resp)
{
    int                  i;
    struct feed_group   *group;
    fh_adm_line_stats_t *line;
    
    // set the correct number of lines in the response structure
    stats_resp->stats_line_cnt = fh_arca_proc_args.main_sockets->socket_count;
    
    // go through every socket in this process's socket set
    for (i = 0; i < fh_arca_proc_args.main_sockets->socket_count; i++) {
        // fetch the feed group and mgmt stats response line for this socket
        group = fh_arca_proc_args.main_sockets->feeds[i];
        line = &stats_resp->stats_lines[i];
    
        // 0 indicates primary, 1 indicates secondary
        if (!fh_arca_proc_args.main_sockets->primary_or_secondary[i]) {
            // generate a line name string out of the line name + PRI
            sprintf(line->line_name, "%s_PRI", group->feed_name);

            // add this line's statistics to the array of line stats
            line->line_pkt_errs      = group->pkt_format_errors_primary;
            line->line_pkt_rx        = group->pckt_rcvd_from_primary;
            line->line_pkt_dups      = group->pkt_duplicate_primary;
            line->line_pkt_late      = 0;
            line->line_msg_rx        = group->msgs_rcvd_from_primary;
            line->line_msg_loss      = 0;
            line->line_msg_recovered = 0;
            line->line_msg_late      = 0;
            line->line_bytes         = group->bytes_rcvd_from_primary;
        }
        else {
            // generate a line name string out of the line name + PRI
            sprintf(line->line_name, "%s_SEC", group->feed_name);

            // add this line's statistics to the array of line stats
            line->line_pkt_errs      = group->pkt_format_errors_secondary;
            line->line_pkt_rx        = group->pckt_rcvd_from_secondary;
            line->line_pkt_dups      = group->pkt_duplicate_secondary;
            line->line_pkt_late      = 0;
            line->line_msg_rx        = group->msgs_rcvd_from_secondary;
            line->line_msg_loss      = 0;
            line->line_msg_recovered = 0;
            line->line_msg_late      = 0;
            line->line_bytes         = group->bytes_rcvd_from_secondary;
        }
    }
}

/*! \brief Clear statistics counters for this line handler process
 */
void fh_arca_lh_clr_stats()
{
    int          i;
    int          bytes_to_zero  = sizeof(uint64_t) * 10;
    uint64_t    *base_address   = NULL;
    
    // go through every socket in this process's socket set
    for (i = 0; i < fh_arca_proc_args.main_sockets->socket_count; i++) {
        // find the memory address of the first stats value in the feed_group structure
        base_address = &fh_arca_proc_args.main_sockets->feeds[i]->pckt_rcvd_from_primary;
        
        // zero the number of bytes that store statistics starting at the base address (
        // computed above)
        memset((void *)base_address, 0, bytes_to_zero);
    }
    
    // zero the aggregated statistics we have been collecting
    memset(&fh_arca_lh_aggr_stats, 0, sizeof(fh_arca_lh_stats_t));
}

/*! \brief Dumps some statistics about message and packet rates (when in debugging mode)
 */
void fh_arca_lh_rates()
{
    if (FH_LL_OK(LH, STATS)) {
        fh_arca_lh_stats_t  prev_stats;
        struct feed_group   tmp_stats;
        register int        i;

        // make a copy of the stats from the last snapshot and zero the totals for this snapshot
        memcpy(&prev_stats, &fh_arca_lh_aggr_stats, sizeof(fh_arca_lh_stats_t));
        memset(&fh_arca_lh_aggr_stats, 0, sizeof(fh_arca_lh_stats_t));
        
        // go through every socket that belongs to this process and...
        for (i = 0; i < fh_arca_proc_args.main_sockets->socket_count; i++) {
            // take a snapshot of the current feed group (to ensure consistent results)
            // from snapshot to snapshot
            memcpy(&tmp_stats, fh_arca_proc_args.main_sockets->feeds[i], sizeof(struct feed_group));
            
            // add this line's numbers into the aggregated totals if this is a primary line
            if (!fh_arca_proc_args.main_sockets->primary_or_secondary[i]) {
                fh_arca_lh_aggr_stats.packets   += tmp_stats.pckt_rcvd_from_primary;
                fh_arca_lh_aggr_stats.messages  += tmp_stats.msgs_rcvd_from_primary;
                fh_arca_lh_aggr_stats.bytes     += tmp_stats.bytes_rcvd_from_primary;
                fh_arca_lh_aggr_stats.errors    += tmp_stats.pkt_format_errors_primary;
                fh_arca_lh_aggr_stats.errors    += tmp_stats.pkt_format_errors_secondary;
            }
        }

        FH_LOG_PGEN(STATS, ("LH Aggregated Stats: %5d PPS - %6d MPS - %7d BPS - (errs: %d)",
                            fh_arca_lh_aggr_stats.packets  - prev_stats.packets,
                            fh_arca_lh_aggr_stats.messages - prev_stats.messages,
                            fh_arca_lh_aggr_stats.bytes    - prev_stats.bytes,
                            fh_arca_lh_aggr_stats.errors   - prev_stats.errors));
    }
}


