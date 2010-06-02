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

// System headers
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/stat.h>

// FH common headers
#include "fh_config.h"
#include "fh_util.h"

// Shared configuration headers
#include "fh_shr_cfg_cmdline.h"
#include "fh_shr_config.h"

/*! \brief Print the usage message for a feed handler
 *
 *  \param program_name name of the program (or the name by which it was executed)
 *  \param fh_name the name of the feed handler being run
 */
static void fh_shr_cfg_cmd_usage(const char *program_name, const char *fh_name)
{
    fprintf(stderr,
            "Usage: %s [OPTION]...\n"
            "Start the %s feed handler\n\n"
            " -h, -?            Print this usage message\n"
            " -d                Debugging mode (prints all log messages to console)\n"
            " -s                Standalone mode (do not attach to central FH manager)\n"
            " -p <process>      Process configuration to use\n"
            " -v                Display the version information\n"
            "\n",
            program_name, fh_name);
    exit(1);
}

/*! \brief Validate command line arguments
 *
 *  \param options default and command line options to this program
 */
static void fh_shr_cfg_cmd_validate(fh_shr_cfg_options_t *options)
{
    struct stat stat_buf;

    if (stat(options->config_file, &stat_buf) < 0) {
        fprintf(stderr, "ERROR: configuration file doesn't exist: %s\n", options->config_file);
        exit(1);
    }
}

/*! \brief Parse general feed handler command line arguments
 *
 *  \param argc argument count (generally passed from main function)
 *  \param argv argument array (generally passed from main function)
 *  \param name the name of the feed handler we parsing command line options for
 *  \param options structure that will hold the options found on the command line
 */
void fh_shr_cfg_cmd_parse(int argc, char **argv, const char *name, fh_shr_cfg_options_t *options)
{
    int          op;
    extern char *optarg;
    
    // get the program name by removing any path information
    FH_PNAME_GET(options->program_name, argv);

    // loop through all command line arguments
    while ((op = getopt(argc, argv, "h?dsp:v")) != EOF) {
        switch (op) {
        case 'h':
        case '?':
            fh_shr_cfg_cmd_usage(options->program_name, name);
            break;

        case 'd':
            options->debug_level++;
            break;

        case 's':
            options->standalone = 1;
            break;
            
        case 'p':
            strcpy(options->process, optarg);
            break;
            
        case 'v':
            options->display_version = 1;
            break;
        }
    }
    
    if (!options->display_version) {
        fh_shr_cfg_cmd_validate(options);
    }
}

