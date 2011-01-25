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

#ifndef __FH_ARCA_LH_H__
#define __FH_ARCA_LH_H__

/*********************************************************************/
/* file: fh_arca_lh.h                                                */
/* Usage: line handler for arca multicast feed handler               */
/* Author: Ross Cooperman of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// Common FH headers
#include "fh_mgmt_admin.h"

typedef struct fh_arca_lh_stats {
    uint64_t packets;
    uint64_t messages;
    uint64_t bytes;
    uint64_t errors;
} fh_arca_lh_stats_t;

/*! \brief Start the line handler thread (the thread that does all the work) 
 *
 *  \return status code indicating success or failure
 */
FH_STATUS fh_arca_lh_start();

/*! \brief Block until the line handler process has exited
 */
void fh_arca_lh_wait();

/*! \brief Return the thread ID of the line handler process
 *
 *  \return requested thread ID
 */
int fh_arca_lh_get_tid();

/*! \brief Get aggregated statistics for all lines that belong to this process
 *
 *  \param stats_resp response structure for an fh manager stats request
 */
void fh_arca_lh_get_stats(fh_adm_stats_resp_t *stats_resp);

/*! \brief Clear statistics counters for this line handler process
 */
void fh_arca_lh_clr_stats();

/*! \brief Dumps some statistics about message and packet rates (when in debugging mode)
 */
void fh_arca_lh_rates();

#endif /* __FH_ARCA_LH_H__ */
