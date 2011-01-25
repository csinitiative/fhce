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
#ifndef __ARCA_PROFILING_H_
#define __ARCA_PROFILING_H_
/*********************************************************************/
/* file: profiling.h                                                 */
/* Usage: profiling constants for arca multicast                     */
/* Author: Wally Matthews of Collaborative Software Initiative       */
/* Conception: Jan. 22, 2009                                         */
/* Inherited from Tervela Itch30                                     */
/*********************************************************************/

// only one of below should be a value of 1
// otherwise the instrumentation will be a leading cause
// of bad performance
//#define ARCA_LOOP_PROFILE              (0)  //profile receive loop with select
//#define ARCA_DRAIN_PROFILE             (0)  //profile receive loop wo select
#define ARCA_MESSAGE_PROFILE           (0)  //profile parse message
#define ARCA_ADD_ORDER_PROFILE         (0)  //profile add order msg processing
#define ARCA_MOD_ORDER_PROFILE         (0)  //profile mod order msg processing
#define ARCA_DEL_ORDER_PROFILE         (0)  //profile del order msg processing
#define ARCA_IMBALANCE_PROFILE         (0)  //profile imbalance msg processing
#define ARCA_SYMBOL_MAP_PROFILE        (0)  //profile symbol map processing
#define ARCA_FIRM_MAP_PROFILE          (0)  //profile firm map processing
#define ARCA_IMBALANCE_REFRESH_PROFILE (0)  //profile imbalance refresh processing
#define ARCA_BOOK_REFRESH_PROFILE      (0)  //profile book refresh processing
#endif
