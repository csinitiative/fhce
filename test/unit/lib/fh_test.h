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

#ifndef __FH_TEST_H__
#define __FH_TEST_H__

// System includes
#include <stdint.h>
#include <bfd.h>

// structure to hold testing statistics
typedef struct fh_test_stats {
    uint32_t tests;
    uint32_t successes;
    uint32_t failures;
    uint32_t errors;
    uint32_t assertions;
} fh_test_stats_t;

// statistics and message data that will be available to all modules
extern fh_test_stats_t  *fh_test_stats;
extern char             *fh_test_msg;

#endif  /* __FH_TEST_H__ */
