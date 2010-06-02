/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 * 
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FH_SHR_MGMT_H__
#define __FH_SHR_MGMT_H__

/* FH common includes */
#include "fh_errors.h"
#include "fh_info.h"

/* FH mgmt headers */
#include "fh_adm_stats_resp.h"

/* type definitions for callbacks that are necessary for gathering stats, etc. */
typedef void                     (fh_shr_mgmt_exit_cb_t)();
typedef void                     (fh_shr_mgmt_clrstats_cb_t)();
typedef void                     (fh_shr_mgmt_getstats_cb_t)(fh_adm_stats_resp_t *);
typedef void                     (fh_shr_mgmt_snapstats_cb_t)();
typedef void                     (fh_shr_mgmt_snaplatency_cb_t)();
typedef fh_info_proc_t          *(fh_shr_mgmt_getstatus_cb_t)();

/* structure used to pass necessary callbacks to the management thread "start" function */
typedef struct {
    fh_shr_mgmt_exit_cb_t         *exit;
    fh_shr_mgmt_getstats_cb_t     *getstats;
    fh_shr_mgmt_clrstats_cb_t     *clrstats;
    fh_shr_mgmt_snapstats_cb_t    *snapstats;
    fh_shr_mgmt_snaplatency_cb_t  *snaplatency;
    fh_shr_mgmt_getstatus_cb_t    *getstatus;
} fh_shr_mgmt_cb_t;

/**
 *  @brief Initialize management functionality
 * 
 *  This function spawns the management thread and establishes a connection to the fhmgr.
 *
 *  @param standalone flag to set management thread to run without a central manager
 *  @param fh name of the feed handler that is being run
 *  @param info feed handler info (version, build, etc)
 *  @param callbacks structure of callback functions that will be called for stats gathering, etc.
 *  @return status code indicating success of failure
 */
FH_STATUS fh_shr_mgmt_start(int standalone, const fh_info_build_t *info, const char *proc,
                            fh_shr_mgmt_cb_t *callbacks);

/** 
 *  @brief Block until the management thread has exited
 */
void fh_shr_mgmt_wait();

/**
 *  @brief Block until the management thread has exited
 */
void fh_shr_mgmt_exit();

#endif  /* __FH_SHR_MGMT_H__ */
