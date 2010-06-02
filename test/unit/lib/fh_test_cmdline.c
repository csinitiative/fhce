/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 * 
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

// System headers
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

// FH test headers
#include "fh_test_cmdline.h"
#include "fh_test_util.h"

/*! \brief Print the usage message and exit
 *
 *  \param process_name the name of the currently executing process
 */
void fh_test_cmd_helpmsg(const char *process_name)
{
    fprintf(stderr,
            "Usage: %s [OPTION]...\n"
            "  -h, -?       Display this message\n"
            "  -c           Print results in a more compact format\n"
            "\n",
            process_name);
    exit(1);
}

/*! \brief (private) Set command line arguments to their defaults
 *
 *  \param options structure of options that will be set
 */
void fh_test_cmd_defaults(fh_test_cmd_opts_t *options)
{
    // by default, print in expanded, non-compact format
    options->compact = 0;
}

/*! \brief Parse command line options
 *
 *  \param argc command line argument count
 *  \param argv array of command line arguments
 *  \param options structure where command line arguments will be stored
 */
void fh_test_cmd_parse(int argc, char **argv, fh_test_cmd_opts_t *options)
{
    int            op;
    extern char   *optarg;
    const char    *process_name;
        
    // set defaults
    fh_test_cmd_defaults(options);
    
    // set the process name
    process_name = fh_test_util_strip_path(argv[0]);

    // parse command line options
    while((op = getopt(argc, argv, "?hc")) != EOF) {
        switch (op) {
        case '?':
        case 'h':
            fh_test_cmd_helpmsg(process_name);
            break;
            
        case 'c':
            options->compact = 1;
            break;

        default:
            fprintf(stderr, "ERROR: invalid option '%c'\n\n", op);
            fh_test_cmd_helpmsg(process_name);
            break;
        }
    }
}
