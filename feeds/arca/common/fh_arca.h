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

#ifndef __FH_ARCA_H__
#define __FH_ARCA_H__

/*********************************************************************/
/* file: fh_arca.h                                                   */
/* Usage: headers for arca feed handler main process                 */
/* Author: Wally Matthews & Ross Cooperman of                        */
/*   Collaborative Software Initiative                               */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <semaphore.h>

// Arca FH headers
#include "fh_arca_constants.h"
#include "fh_feed_group.h"

// make fh_arca_stopped external so other threads will know if they are supposed to stop
extern int                       fh_arca_stopped;

//TODO - eliminate dependence on old configuration structures
extern struct arca_process_args  fh_arca_proc_args;

// make thread_init available externally so all threads can lock it while initializing
extern sem_t                     fh_arca_thread_init;

/*! \brief Entry point for all Arca feed handlers
 *
 *  \param argc number of command line arguments
 *  \param argv content of command line arguments
 *  \param arca_version Arca feed handler version
 *  \return boolean value indicating success or failure
 */
int fh_arca_main(int argc, char* argv[], int arca_version);

#endif /* __FH_ARCA_H__ */

