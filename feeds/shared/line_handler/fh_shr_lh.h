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

#ifndef __FH_SHR_LH_H__
#define __FH_SHR_LH_H__

/**
 *  @defgroup SharedLineHandler
 *  @{
 */

/* FH common headers */
#include "fh_errors.h"
#include "fh_log.h"
#include "fh_info.h"

/* FH mgmt headers */
#include "fh_adm_stats_resp.h"

/* FH shared module headers */
#include "fh_shr_cfg_lh.h"
#include "fh_shr_lookup.h"

/* some convenience typedefs for simplified use of several structs */
typedef struct fh_shr_lh_conn fh_shr_lh_conn_t;
typedef struct fh_shr_lh_line fh_shr_lh_line_t;
typedef struct fh_shr_lh_proc fh_shr_lh_proc_t;

/**
 *  @brief Structure that holds connection information
 */
struct fh_shr_lh_conn {
    fh_shr_lh_line_t        *line;          /**< pointer to this connection's line info */
    int                      socket;        /**< network socket for this connection */
    fh_shr_cfg_lh_conn_t    *config;        /**< pointer to the configration data for this conn */
    char                     tag[10];       /**< the "name" of this connection */
    uint64_t                 timestamp;     /**< timestamp (units/reference pt. vary by feed) */
    uint64_t                 last_recv;     /**< timestamp of last udp_recv on this connection */
    fh_info_stats_t          stats;         /**< statistics counters for this connection */
    void                    *context;       /**< pointer where a plugin can store its context */
};

/**
 *  @brief Structure that holds line information
 */
struct fh_shr_lh_line {
    fh_shr_lh_proc_t        *process;       /**< pointer to this line's process info */
    fh_shr_lh_conn_t         primary;       /**< this line's primary connection */
    fh_shr_lh_conn_t         secondary;     /**< this line's secondary connection */
    fh_shr_lh_conn_t         request;       /**< this line's request (for retrans.) connection */
    fh_shr_cfg_lh_line_t    *config;        /**< pointer to the configuration data for this line */
    uint64_t                 next_seq_no;   /**< last sequence number seen on this line */
    uint64_t                 timestamp;     /**< timestamp (units/reference pt. vary by feed) */
    fh_info_stats_t          stats;         /**< statistics counters for this line */
    void                    *context;       /**< pointer where a plugin can store its context */
};

/**
 *  @brief Structure that holds process information
 */
struct fh_shr_lh_proc {
    fh_shr_lh_line_t        *lines;         /**< array of line structures */
    int                      num_lines;     /**< number of line structures in the array */
    fh_shr_cfg_lh_proc_t    *config;        /**< pointer to the configuration data for this proc */
    fh_info_stats_t          stats;         /**< statistics counters for this process */
    fh_shr_lkp_tbl_t         symbol_table;  /**< symbol table structure */
    fh_shr_lkp_tbl_t         order_table;   /**< symbol table structure */
    void                    *context;       /**< pointer where a plugin can store its context */
};

/* type definitions for callbacks that are necessary for parsing packets, etc */
typedef FH_STATUS (fh_shr_lh_parse_cb_t)(uint8_t *, int, fh_shr_lh_conn_t *);
typedef FH_STATUS (fh_shr_lh_init_cb_t)(fh_shr_lh_proc_t *);

/* structure used to pass necessary callbacks to the line handler thread "start" function */
typedef struct {
    fh_shr_lh_init_cb_t  *init;
    fh_shr_lh_parse_cb_t *parse;
} fh_shr_lh_cb_t;

/**
 *  @brief Start the line handler thread (the thread that does all the work)
 *
 *  @param info feed handler information (version, build info, etc.)
 *  @param config feed handler configuration options
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_lh_start(const fh_info_build_t *info, fh_shr_cfg_lh_proc_t *config,
                          fh_shr_lh_cb_t *callbacks);

/**
 *  @brief Block until the line handler process has exited
 */
void fh_shr_lh_wait();

/**
 *  @brief Kill the line handler thread by sending it a SIGINT signal
 */
void fh_shr_lh_exit();

/**
 *  @brief Return the line handler's thread ID
 *
 *  @return the thread ID of the line handler thread (or -1 if the thread is not yet running)
 */
int fh_shr_lh_get_tid();


/**
 *  @brief Return statistics for this process in the appropriate structure for transmission to an
 *         fhmgr
 *
 *  @param stats_resp stats response structure to be populated
 */
void fh_shr_lh_get_stats(fh_adm_stats_resp_t *stats_resp);

/**
 *  @brief Clear statistics for this process
 */
void fh_shr_lh_clear_stats();

/**
 *  @brief Log a snapshot of statistics since the last snapshot
 */
void fh_shr_lh_snap_stats();

/**
 *  @brief Display collected latency statistics
 */
void fh_shr_lh_latency();

/** @} */

#endif  /* __FH_SHR_LH_H__ */
