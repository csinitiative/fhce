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

#ifndef __FH_TEST_OUTPUT_H__
#define __FH_TEST_OUTPUT_H__

// FH test headers
#include "fh_test_msg.h"

// console color constants
#define    CONSOLE_COLOR_RED        "[0;31m"
#define    CONSOLE_COLOR_GREEN      "[0;32m"
#define    CONSOLE_COLOR_YELLOW     "[0;33m"
#define    CONSOLE_RESET            "[0m"

/*! \brief Print a "starting tests" message
 *
 *  \param binary the binary from which these tests are being run
 *  \param compact whether or not we are printing compact output
 */
void fh_test_out_start(const char *binary, int compact);

/*! \brief Print data for a single test
 *
 *  \param type success, failure, etc.
 *  \param message the message to print if type != SUCCESS
 *  \param test the test that was run
 */
void fh_test_out_test(fh_test_msg_type_t type, const char *message, fh_test_sym_test_t *test,
                      int compact);

/*! \brief Print pass/fail summary information and the end of a test run
 *
 *  \param compact whether or not we are printing compact output
 */
void fh_test_out_finish(int compact);

#endif  /* __FH_TEST_OUTPUT_H__ */
