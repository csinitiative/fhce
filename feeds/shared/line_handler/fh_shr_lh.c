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

/* System headers */
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>

/* FH common headers */
#include "fh_errors.h"
#include "fh_util.h"
#include "fh_log.h"
#include "fh_info.h"
#include "fh_cpu.h"
#include "fh_udp.h"
#include "fh_net.h"
#include "fh_mcast.h"
#include "fh_prof.h"
#include "fh_plugin_internal.h"

/* FH shared component headers */
#include "fh_shr_lh.h"
#include "fh_shr_cfg_lh.h"
#include "fh_shr_lkp_symbol.h"
#include "fh_shr_lkp_order.h"

/* START ONLY ONE LINE HANDLER THREAD AT A TIME -- this code is not intended to be thread safe */
static pthread_t                     lh_thread;     /* line handler thread */
static uint32_t                      lh_tid   = 0;  /* line handler thread id */
static const fh_info_build_t        *lh_info;       /* version, build, etc. information */
static fh_shr_lh_proc_t              lh_process;    /* feed handler process_data */
static fh_shr_lh_cb_t               *lh_callbacks;  /* callbacks for packet parsing, etc */
static int                           lh_init  = 0;  /* indicates that lh init. is complete */
static int                           finished = 0;  /* flag that tells the line handler to exit */

/* cached hook function(s) */
static fh_plugin_hook_t              hook_msg_flush = NULL;

/* profiling declarations for latency measurements */
FH_PROF_DECL(lh_recv_latency, 1000000, 20, 2);
FH_PROF_DECL(lh_proc_latency, 1000000, 20, 2);

/*
 * Initialize line handler tables
 */
void fh_shr_lh_tbl_init()
{
    /* initialize the symbol table */
    fh_shr_lkp_sym_init(&lh_process.config->symbol_table, &lh_process.symbol_table);

    /* initialize the order table */
    fh_shr_lkp_ord_init(&lh_process.config->order_table, &lh_process.order_table);
}

/*
 * Build a socket set (fd_set) from all opened sockets (for use in the select loop)
 */
static int fh_shr_lh_get_fdset(fd_set *socket_set)
{
    int  i;
    int  max = 0;

    /* zero the socket set (initialize all entries to 0) */
    FD_ZERO(socket_set);

    /* loop through every line adding its sockets (if enabled) to the socket set */
    for (i = 0; i < lh_process.config->num_lines; i++) {
        if (lh_process.config->lines[i].primary.enabled) {
            FD_SET(lh_process.lines[i].primary.socket, socket_set);
            max = MAX(lh_process.lines[i].primary.socket, max);
        }
        if (lh_process.config->lines[i].secondary.enabled) {
            FD_SET(lh_process.lines[i].secondary.socket, socket_set);
            max = MAX(lh_process.lines[i].secondary.socket, max);
        }
    }

    /* return the total count of sockets in the socket set */
    return max;
}


/*
 * The actual body of the line handler thread
 */
static void *fh_shr_lh_run(void *arg)
{
    /* variables related to the select() loop (fd sets, etc) */
    struct timeval           wakeup_interval;
    fd_set                   socket_set, read_set;
    int                      max_socket, count;

    /* variables related to doing the UDP read (buffer, bytes returned, etc) */
    int                      num_bytes;
    uint8_t                  buffer[2048];
    struct sockaddr_in       from;
    uint32_t                 ifindex;
    uint32_t                 ifaddr;

    /* "other" variables */
    int                      i;
    int                      rc;
    fh_shr_lh_line_t        *line;
    fh_shr_cfg_lh_proc_t    *config = lh_process.config;
    char                    *thread_name = NULL;

    /* make sure that no arguments were passed */
    FH_ASSERT(arg == NULL);

    /* set the lh_tid variable to this thread's id */
    lh_tid = gettid();

    /* initialize latency measurement structures */
    if (FH_LL_OK(LH, STATS)) {
        FH_PROF_INIT(lh_proc_latency);
        FH_PROF_INIT(lh_recv_latency);
    }

    /* set thread affinity */
    if (config->cpu >= 0 && fh_cpu_setaffinity(CPU(config->cpu)) != FH_OK) {
        FH_LOG(LH, WARN, ("failed to assign CPU affinity %d to LH thread", config->cpu));
    }

    /* allocate space for, generate, and log a "thread started" message for this thread's name */
    thread_name = fh_util_thread_name("LH", config->name);
    fh_log_thread_start(thread_name);

    /* give the message parser a chance to initialize itself */
    lh_callbacks->init(&lh_process);

    /* get a socket set for the (now opened) sockets attached to the process configuration */
    max_socket = fh_shr_lh_get_fdset(&socket_set);

    /* main line handler loop */
    while (!finished) {
        /* set up the wakeup interval (to make sure the line handler will exit, even when idle) */
        wakeup_interval.tv_sec  = 0;
        wakeup_interval.tv_usec = 100000;

        /* reset the select parameters */
        memcpy(&read_set, &socket_set, sizeof(fd_set));

        /* enter the select loop and never time out */
        count = select(max_socket + 1, &read_set, NULL, NULL, &wakeup_interval);

        /* check for a couple of error/unusual cases where select() returns <= 0 */
        if (count == -1) {
          FH_LOG(LH, DIAG, ("line handler select failed: %s (%d)", strerror(errno), errno));
          continue;
        }
        else if (count == 0) {
          continue;
        }

        /* loop through each line looking for the sockets that have data */
        for (i = 0; count > 0 && i < lh_process.config->num_lines; i++) {
            /* Local flag used to decide whether to call flush hook  */
            static int  to_publish = 0;
            /* store a pointer to the current line */
            line = &lh_process.lines[i];

            /* if none of this line's descriptors are set */
            if (!FD_ISSET(line->primary.socket, &read_set) &&
                !FD_ISSET(line->secondary.socket, &read_set)) {
                continue;
            }

            /* if this line's primary descriptor is set... */
            if (line->config->primary.enabled && FD_ISSET(line->primary.socket, &read_set)) {

                /* mark the start of packet reception */
                if (FH_LL_OK(LH, STATS)) {
                    FH_PROF_BEG(lh_recv_latency);
                }

                /* fetch packet data into the buffer */
                FH_LOG(LH, INFO, ("processing packet on line %s (primary)", line->config->name));
                num_bytes = fh_udp_recv(line->primary.socket, buffer, sizeof(buffer), &from,
                                        &ifindex, &ifaddr, &line->primary.last_recv);
                if (num_bytes < 0) {
                    FH_LOG(LH, DIAG, ("read failed on line: %s (primary)", line->config->name));
                    FH_PROF_END(lh_recv_latency);
                    continue;
                }

                /* mark the end of packet reception and the start of packet processing */
                if (FH_LL_OK(LH, STATS)) {
                    FH_PROF_END(lh_recv_latency);
                    FH_PROF_BEG(lh_proc_latency);
                }

                /* Set the publish flag     */
                to_publish = 1;

                /* number_of_packets_on_this_line++ && number_of_packets_remaining-- */
                line->primary.stats.packets++;
                count--;

                /* pass the packet off the the parsing callback */
                lh_callbacks->parse(buffer, num_bytes, &line->primary);

                /* mark the end of packet processing */
                if (FH_LL_OK(LH, STATS)) {
                    FH_PROF_END(lh_proc_latency);
                }
            }

            /* if this line's secondary descriptor is set... */
            if (line->config->secondary.enabled && FD_ISSET(line->secondary.socket, &read_set)) {

                /* mark the start of packet reception */
                if (FH_LL_OK(LH, STATS)) {
                    FH_PROF_BEG(lh_recv_latency);
                }

                /* fetch packet data into the buffer */
                FH_LOG(LH, INFO, ("processing packet on line %s (secondary)", line->config->name));
                num_bytes = fh_udp_recv(line->secondary.socket, buffer, sizeof(buffer), &from,
                                     &ifindex, &ifaddr, &line->secondary.last_recv);
                if (num_bytes < 0) {
                    FH_LOG(LH, DIAG, ("read failed on line: %s (secondary)", line->config->name));
                    FH_PROF_END(lh_recv_latency);
                    continue;
                }

                /* mark the end of packet reception and the start of packet processing */
                if (FH_LL_OK(LH, STATS)) {
                    FH_PROF_END(lh_recv_latency);
                    FH_PROF_BEG(lh_proc_latency);
                }

                /* Set the publish flag     */
                to_publish = 1;

                /* number_of_packets_on_this_line++ && number_of_packets_remaining-- */
                line->secondary.stats.packets++;
                count--;

                /* pass the packet off the the parsing callback */
                lh_callbacks->parse(buffer, num_bytes, &line->secondary);

                /* mark the end of packet processing */
                if (FH_LL_OK(LH, STATS)) {
                    FH_PROF_END(lh_proc_latency);
                }
                /* if a msg flush hook is registered, call it now */
                if (hook_msg_flush && to_publish ) {
                    hook_msg_flush(&rc);
                    to_publish  = 0;
                }

            }
            /* if a msg flush hook is registered, call it now */
            if (hook_msg_flush && to_publish ) {
                hook_msg_flush(&rc);
                to_publish  = 0;
            }

        } /* end for() */

    }

    /* log the thread's exit */
    fh_log_thread_stop(thread_name);

    /* provided execution gets here, return NULL */
    return NULL;
}

/*
 * Initialize the socket for a single connection
 */
static inline FH_STATUS fh_shr_lh_init_conn(fh_shr_lh_line_t *line, fh_shr_lh_conn_t *conn)
{
    FH_STATUS                    rc;
    int                          ifaddr;
    static char                  straddr[32];
    static const int             udp_flags  = FH_UDP_FL_MAX_BUFSZ | FH_UDP_FL_MCAST;
    fh_shr_cfg_lh_conn_t        *config     = conn->config;

    /* if the connection is enabled... */
    if (config->enabled) {
        /* generate an address:port string for errors */
        sprintf(straddr, "%s:%d", fh_net_ntoa(config->address), config->port);

        /* create a socket to listed on the specified port */
        if ((rc = fh_udp_open(config->address, config->port, udp_flags, &conn->socket)) != FH_OK) {
            FH_LOG(LH, ERR, ("failed to create socket for %s (%s)", straddr, line->config->name));
            return rc;
        }

        /* retrieve the interface address of the configured interface */
        if ((ifaddr = fh_net_ifaddr(conn->socket, config->interface)) == 0) {
            FH_LOG(LH, ERR, ("failed to retrieve ip address for %s", config->interface));
            close(conn->socket);
            return FH_ERROR;
        }

        /* join the configured multicast group */
        if ((rc = fh_mcast_join(conn->socket, ifaddr, config->address)) != FH_OK) {
            FH_LOG(LH, ERR, ("multicast group join failed %s (%s)", straddr, line->config->name));
            close(conn->socket);
            return rc;
        }
    }

    /* as long as execution gets here everything is ok */
    return FH_OK;
}

/*
 * Initialize all sockets, join multicast groups, etc.
 */
static FH_STATUS fh_shr_lh_init(fh_shr_cfg_lh_proc_t *config)
{
    FH_STATUS            rc;
    fh_shr_lh_line_t    *line;
    fh_shr_lh_conn_t    *primary, *secondary;
    int                  i;

    /* point the global process data structure at the process configuration */
    lh_process.config = config;

    /* allocate line structures for each of the configured lines **/
    lh_process.lines = (fh_shr_lh_line_t *)malloc(sizeof(fh_shr_lh_line_t) * config->num_lines);
    if (lh_process.lines == NULL) {
        FH_LOG(LH, ERR, ("unable to allocate memory for line data (%s)", config->name));
        return FH_ERROR;
    }
    lh_process.num_lines = config->num_lines;

    /* loop through all of the lines in our process configuration */
    for (i = 0; i < config->num_lines; i++) {
        /* link each line data structure to its config */
        lh_process.lines[i].config = &config->lines[i];

        /* set a pointer to the connections for the line we are currently configuring */
        line        = &lh_process.lines[i];
        primary     = &line->primary;
        secondary   = &line->secondary;

        /* initialize the line's next expected sequence number */
        line->next_seq_no = 1;

        /* link this line's process pointer back to this process */
        line->process = &lh_process;

        /* set up the primary connection */
        primary->config = &line->config->primary;
        if ((rc = fh_shr_lh_init_conn(line, primary)) != FH_OK) {
            return rc;
        }
        primary->line = line;
        strcpy(primary->tag, "primary");

        /* set up the secondary socket */
        secondary->config = &line->config->secondary;
        if ((rc = fh_shr_lh_init_conn(line, secondary)) != FH_OK) {
            return rc;
        }
        secondary->line = line;
        strcpy(secondary->tag, "secondary");
    }

    /* zero all statistics */
    fh_shr_lh_clear_stats();

    /* initialize any tables that the feed handler is going to keep */
    fh_shr_lh_tbl_init();

    /* allow a plugin the change to modify the loaded configuration */
    if (fh_plugin_is_hook_registered(FH_PLUGIN_LH_INIT)) {
        fh_plugin_get_hook(FH_PLUGIN_LH_INIT)(&rc, &lh_process);
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("error occured during plugin line handler init (%d)", rc));
            return rc;
        }
    }

    /* cache any hooks that have been registered and will later be called */
    if (fh_plugin_is_hook_registered(FH_PLUGIN_MSG_FLUSH)) {
        hook_msg_flush = fh_plugin_get_hook(FH_PLUGIN_MSG_FLUSH);
    }

    /* indicate that initialization is complete */
    lh_init = 1;

    /* as long as we get all the way here, success */
    return FH_OK;
}

/**
 *  @brief Creates the line handler thread
 *
 *  @param info build information for this process
 *  @param config this process's configuration
 *  @param callbacks structure of callbacks for "specific" line handler functions
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_lh_start(const fh_info_build_t *info, fh_shr_cfg_lh_proc_t *config,
                          fh_shr_lh_cb_t *callbacks)
{
    FH_STATUS rc;

    /* store references to feed handler info and configuration */
    lh_info         = info;
    lh_callbacks    = callbacks;

    /* initialize all sockets, join multicast groups, etc. */
    if ((rc = fh_shr_lh_init(config)) != FH_OK) {
        FH_LOG(LH, ERR, ("failed to initialize sockets for line handler (%s)", config->name));
        return rc;
    }

    if (pthread_create(&lh_thread, NULL, fh_shr_lh_run, NULL) < 0) {
        FH_LOG(LH, ERR, ("failed to start line handler thread (%s): %s",
                         config->name, strerror(errno)));
        return FH_ERROR;
    }

    /* by the time execution gets here, success! */
    return FH_OK;
}

/*
 * Wait on the completion of the line handler thread (convenience method for the caller of
 * fh_shr_lh_start in case the caller wants to block until the thread has exited)
 */
void fh_shr_lh_wait()
{
    if (lh_thread) {
        pthread_join(lh_thread, NULL);
        FH_LOG(MGMT, VSTATE, ("%s line handler thread (%s) exited", lh_info->name,
                              lh_process.config->name));
    }
}

/*
 * Return the line handler's thread ID
 */
int fh_shr_lh_get_tid()
{
    return lh_tid;
}

/*
 * Kill the line handler thread by sending it a SIGINT signal
 */
void fh_shr_lh_exit()
{
    finished = 1;
}

/*
 * Converts stats from internal line handler representation to the proper structure for
 * return to an FH manager
 */
void fh_shr_lh_get_stats(fh_adm_stats_resp_t *stats_resp)
{
    int                      i;
    fh_shr_lh_line_t        *line;
    fh_adm_line_stats_t     *stat_line;

    /* zero the stats response (avoids the potential for bad numbers if we don't happen */
    /* to populate every statistic) */
    memset(stats_resp, 0, sizeof(fh_adm_stats_resp_t));

    /* set the stats for each line in the structure */
    for (i = 0; i < lh_process.num_lines; i++) {
        /* set up a pointer to the current line (for ease of access) */
        line = &lh_process.lines[i];

        /* populate stats for the primary line (if enabled) */
        if (line->primary.config->enabled) {
            /* set up a pointer to the stats line being populated and give it a name*/
            stat_line = &stats_resp->stats_lines[stats_resp->stats_line_cnt];
            sprintf(stat_line->line_name, "%s_", line->config->name);
            fh_util_ucstring(stat_line->line_name + strlen(stat_line->line_name),
                             line->primary.tag);

            /* populate statistics */
            stat_line->line_pkt_rx            = line->primary.stats.packets;
            stat_line->line_msg_rx            = line->primary.stats.messages;
            stat_line->line_bytes             = line->primary.stats.bytes;
            stat_line->line_pkt_errs          = line->primary.stats.packet_errors;
            stat_line->line_pkt_dups          = line->primary.stats.duplicate_packets;
            stat_line->line_pkt_seq_jump      = line->primary.stats.gaps;
            stat_line->line_msg_loss          = line->primary.stats.lost_messages;
            stat_line->line_msg_recovered     = line->primary.stats.recovered_messages;

            /* increment the stat line count */
            stats_resp->stats_line_cnt++;
        }

        /* populate stats for the secondary line (if enabled) */
        if (line->secondary.config->enabled) {
            /* set up a pointer to the stats line being populated and give it a name*/
            stat_line = &stats_resp->stats_lines[stats_resp->stats_line_cnt];
            sprintf(stat_line->line_name, "%s_", line->config->name);
            fh_util_ucstring(stat_line->line_name + strlen(stat_line->line_name),
                             line->secondary.tag);


            /* populate statistics */
            stat_line->line_pkt_rx            = line->secondary.stats.packets;
            stat_line->line_msg_rx            = line->secondary.stats.messages;
            stat_line->line_bytes             = line->secondary.stats.bytes;
            stat_line->line_pkt_errs          = line->secondary.stats.packet_errors;
            stat_line->line_pkt_dups          = line->secondary.stats.duplicate_packets;
            stat_line->line_pkt_seq_jump      = line->secondary.stats.gaps;
            stat_line->line_msg_loss          = line->secondary.stats.lost_messages;
            stat_line->line_msg_recovered     = line->secondary.stats.recovered_messages;

            /* increment the stat line count */
            stats_resp->stats_line_cnt++;
        }
    }
}

/*
 * Clear the statistics for lines belonging to this process
 */
void fh_shr_lh_clear_stats()
{
    int i;

    /* zero process stats */
    memset(&lh_process.stats, 0, sizeof(fh_info_stats_t));

    /* loop through each of the lines zeroing the line stats and each connection's stats */
    for (i = 0; i < lh_process.num_lines; i++) {
        memset(&lh_process.lines[i].stats, 0, sizeof(fh_info_stats_t));
        memset(&lh_process.lines[i].primary.stats, 0, sizeof(fh_info_stats_t));
        memset(&lh_process.lines[i].secondary.stats, 0, sizeof(fh_info_stats_t));
        memset(&lh_process.lines[i].request.stats, 0, sizeof(fh_info_stats_t));
    }
}

/*
 * Log a snapshot of basic stats since the last time this function was called
 */
void fh_shr_lh_snap_stats()
{
    /* if line handler initialization is not complete, just return */
    if (!lh_init || !FH_LL_OK(LH, XSTATS)) return;

    /* persistent data across calls */
    static uint64_t packets       = 0;
    static uint64_t messages      = 0;
    static uint64_t dups          = 0;
    static uint64_t errors        = 0;

    /* temporary data (just this call) */
    uint64_t        temp_packets  = 0;
    uint64_t        temp_messages = 0;
    uint64_t        temp_dups     = 0;
    uint64_t        temp_errors   = 0;
    int             i             = 0;

    /* loop through each line, counting stats for each connection */
    for (i = 0; i < lh_process.num_lines; i++) {
        temp_packets  += lh_process.lines[i].primary.stats.packets;
        temp_packets  += lh_process.lines[i].secondary.stats.packets;
        temp_packets  += lh_process.lines[i].request.stats.packets;

        temp_messages += lh_process.lines[i].primary.stats.messages;
        temp_messages += lh_process.lines[i].secondary.stats.messages;
        temp_messages += lh_process.lines[i].request.stats.messages;

        temp_dups     += lh_process.lines[i].primary.stats.duplicate_packets;
        temp_dups     += lh_process.lines[i].secondary.stats.duplicate_packets;
        temp_dups     += lh_process.lines[i].request.stats.duplicate_packets;

        temp_errors   += lh_process.lines[i].primary.stats.packet_errors;
        temp_errors   += lh_process.lines[i].secondary.stats.packet_errors;
        temp_errors   += lh_process.lines[i].request.stats.packet_errors;
    }

    /* log the gathered statistics (minus stats from the last call) */
    FH_LOG(LH, XSTATS, ("LH Aggregated Stats: %5lu PPS - %6lu MPS - (dups: %lu errs: %lu)",
                        temp_packets  - packets,
                        temp_messages - messages,
                        temp_dups     - dups,
                        temp_errors   - errors
                       ));

    /* save stats from this call for next time through */
    packets  = temp_packets;
    messages = temp_messages;
    dups     = temp_dups;
    errors   = temp_errors;
}

/*
 * Display collected latency statistics
 */
/*
 * fh_opra_lh_latency
 *
 * Dumps some statistics about latency when enabled.
 */
void fh_shr_lh_latency()
{
    if (FH_LL_OK(LH, STATS)) {
        FH_PROF_PRINT(lh_recv_latency);
        FH_PROF_PRINT(lh_proc_latency);
    }
}

