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

#ifndef __FH_ARCA_MGMT_H__
#define __FH_ARCA_MGMT_H__

/*********************************************************************/
/* file: fh_arca_mgmt.h                                              */
/* Usage: management headers for arca multicast feed handler         */
/* Author: Ross Cooperman of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/


// FH common includes
#include "fh_errors.h"

/*! \brief Initialize management functionality
 * 
 *  This function spawns the management thread and establishes a connection
 *  to the Arca management process.
 *
 *  \param standalone flag to set management processes to run without a central manager
 *  \return status code indicating success of failure
 */
FH_STATUS fh_arca_mgmt_start(int standalone);

/*! \brief Block until the management process has exited
 */
void fh_arca_mgmt_wait();

#endif /* __FH_ARCA_MGMT_H__ */
