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

// System headers
#include <string.h>
#include <stdio.h>

// FH tests headers
#include "fh_test.h"
#include "fh_test_output.h"
#include "fh_test_msg.h"
#include "fh_test_util.h"

/*! \brief Print a "starting tests" message
 *
 *  \param binary the binary from which these tests are being run
 *  \param compact whether or not we are printing compact output
 */
void fh_test_out_start(const char *binary, int compact)
{
    const char *process_name;

    // strip path information from the currently running binary
    process_name = fh_test_util_strip_path(binary);

    // if "compact" output is selected, print a very brief "running tests" header
    if (compact) {
        printf("%s\n", process_name);
    }
    // otherwise, print an expanded "starting tests" message
    else {
        printf("Running tests from %s ...\n\n", process_name);
    }
}

/*! \brief Print data for a single test
 *
 *  \param type success, failure, etc.
 *  \param message the message to print if type != SUCCESS
 *  \param test the test that was run
 *  \param compact whether or not we are printing compact output
 */
void fh_test_out_test(fh_test_msg_type_t type, const char *message, fh_test_sym_test_t *test,
                      int compact)
{
    // if "compact" output is selected, just print nothing for now
    if (compact) {
        printf("%s:", test->name);
        printf("%s:%ld:", test->source_file, test->first_line);
        printf("%s", fh_test_msg_uctypestr(type));
        printf("\n%s\n.\n", (message) ? message : "");

        return;
    }

    // determine what to print/do depending on success, failure, etc.
    switch (type) {
    case SUCCESS:
        printf(".");
        return;

    case FAILURE:
        printf("F");
        break;

    case ERROR:
        printf("E");
        break;

    default:
        printf("E");
    }

    // if we get here, we are expecting a message to be added/printed
    fh_test_msg_add(type, message, test);
}

/*! \brief Print pass/fail summary information and the end of a test run
 *
 *  \param compact whether or not we are printing compact output
 */
void fh_test_out_finish(int compact)
{
    // if "compact" output is selected, print a compact listing of stats
    if (compact) {
        printf(":%d:%d:%d:%d\n", fh_test_stats->tests, fh_test_stats->assertions,
               fh_test_stats->failures, fh_test_stats->errors);
    }
    // otherwise, print a more verbose listing of stats
    else {
        // print a little message that we are done and any messages we have accumulated on the way
        printf(" done!");
        fh_test_msg_print();

        // change the console to the correct color
        if (fh_test_stats->errors > 0) {
            printf("\x1b%s", CONSOLE_COLOR_RED);
        }
        else if (fh_test_stats->failures > 0) {
            printf("\x1b%s", CONSOLE_COLOR_YELLOW);
        }
        else {
            printf("\x1b%s", CONSOLE_COLOR_GREEN);
        }

        // print a summary message
        printf("\n\n===========================================================================");
        printf("\n%d tests",      fh_test_stats->tests);
        printf(", %d assertions", fh_test_stats->assertions);
        printf(", %d failures",   fh_test_stats->failures);
        printf(", %d errors",     fh_test_stats->errors);
        printf("\n\n");

        // reset the console color
        printf("\x1b%s", CONSOLE_RESET);
    }
}
