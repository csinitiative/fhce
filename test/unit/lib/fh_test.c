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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wait.h>
#include <sys/mman.h>

// FH test headers
#include "fh_test.h"
#include "fh_test_msg.h"
#include "fh_test_sym.h"
#include "fh_test_output.h"
#include "fh_test_cmdline.h"

// failure, success, assertion, etc. statistics
fh_test_stats_t  *fh_test_stats;
char             *fh_test_msg;


/*! \brief (private) Allocate memory needed for things like statistics, failure messages, etc.
 */
void fh_test_alloc()
{
    int flags = MAP_SHARED | MAP_ANONYMOUS;

    // get some non-file backed, memory mapped space in which to store statistics and failure
    // messages so that all processes (including those forked for each test) will have access
    fh_test_stats = (fh_test_stats_t *)mmap(0, sizeof(fh_test_stats_t), PROT_WRITE, flags, 0, 0);
    if (fh_test_stats == MAP_FAILED) {
        fprintf(stderr, "\nERROR: allocating stats memory: %s (%d)\n", strerror(errno), errno);
        exit(1);
    }
    fh_test_msg = (char *)mmap(0, sizeof(char) * 1024, PROT_WRITE, flags, 0, 0);
    if (fh_test_msg == MAP_FAILED) {
        fprintf(stderr, "\nERROR: allocating message memory: %s (%d)\n", strerror(errno), errno);
        exit(1);
    }
}

/*! \brief (private) Deallocate memory needed for things like statistics, failure messages, etc.
 */
void fh_test_dealloc()
{
    // unmap memory being used for statistics
    if (munmap(fh_test_stats, sizeof(fh_test_stats_t)) == -1) {
        fprintf(stderr, "\nERROR: deallocating stats memory: %s (%d)\n", strerror(errno), errno);
        exit(1);
    }
    // unmap memory being used for keeping track of the current error/failure message
    if (munmap(fh_test_msg, sizeof(char) * 1024) == -1) {
        fprintf(stderr, "\nERROR: deallocating message memory: %s (%d)\n", strerror(errno), errno);
        exit(1);
    }
}

/*! \brief Main function, kick off testing
 *
 *  \param argc argument count
 *  \param argv argument strings
 *  \return process error code
 */
int main(int argc, char **argv)
{
    int                 i;
    pid_t               pid;
    int                 status;
    long                num_tests;
    fh_test_sym_test_t *tests;
    fh_test_cmd_opts_t  options;

    // sanity check, make sure the argv array is big enough
    if (argc < 1) {
        fprintf(stderr, "\nERROR: executed binary file not included in command line arguments\n");
        exit(1);
    }

    // parse command line options
    fh_test_cmd_parse(argc, argv, &options);

    // get an array of test structures to be run
    tests = fh_test_sym_get_tests(argv[0], &num_tests);

    // allocate shared memory
    fh_test_alloc();

    // print the "starting tests" message
    fh_test_out_start(argv[0], options.compact);

    // call each eligible function
    for (i = 0; i < num_tests; i++) {
        // flush output buffers before forking to ensure we don't end up having duplicate buffers
        fflush(stdout);
        fflush(stderr);

        // fork a process for the test (so each test has a clean environment in which to run)
        pid = fork();
        if (pid == -1) {
            fprintf(stderr, "\nERROR: fork test process: %s (%d)\n", strerror(errno), errno);
            exit(1);
        }

        // child process (test being run, run test, count stats, exit)
        if (pid == 0) {
            // close stdin and redirect stdout and stderr to files in /tmp
            close(0);
            stdout = freopen("/tmp/fhtest.out", "a+", stdout);
            stderr = freopen("/tmp/fhtest.err", "a+", stderr);

            // call the test function
            (tests[i].function)();

            // if we get here there are no failed assertions exit with success code
            exit(0);
        }
        // parent process (wait for child to finish and continue)
        else {
            // wait for the child process to stop
            waitpid(pid, &status, 0);

            // if the child exited with a 0 status code
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                fh_test_stats->successes++;
                fh_test_out_test(SUCCESS, NULL, &tests[i], options.compact);
            }
            // if the child exited with a non-zero status code (a failure)
            else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                fh_test_stats->failures++;
                fh_test_out_test(FAILURE, fh_test_msg, &tests[i], options.compact);
            }
            // if the child was signaled to terminate
            else if (WIFSIGNALED(status)) {
                fh_test_stats->errors++;
                sprintf(fh_test_msg, "%s (%d)", sys_siglist[WTERMSIG(status)], WTERMSIG(status));
                fh_test_out_test(ERROR, fh_test_msg, &tests[i], options.compact);
            }
            // if, somehow the child stopped for some other reason
            else {
                fh_test_stats->errors++;
                fh_test_out_test(ERROR, "Unknown error", &tests[i], options.compact);
            }

            // increment the number of completed tests
            fh_test_stats->tests++;
        }
    }

    // print closing information
    fh_test_out_finish(options.compact);

    // deallocate shared memory
    fh_test_dealloc();

    // we have gotten here so, no errors
    return 0;
}
