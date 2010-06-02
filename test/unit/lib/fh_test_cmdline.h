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

#ifndef __FH_TEST_CMDLINE_H__
#define __FH_TEST_CMDLINE_H__

// structure to hold command line options, once parsed
typedef struct fh_test_cmd_opts {
    int compact;
} fh_test_cmd_opts_t;

/*! \brief Print the usage message and exit
 *
 *  \param process_name the name of the currently executing process
 */
void fh_test_cmd_helpmsg(const char *process_name);

/*! \brief Parse command line options
 *
 *  \param argc command line argument count
 *  \param argv array of command line arguments
 *  \param options structure where command line arguments will be stored
 */
void fh_test_cmd_parse(int argc, char **argv, fh_test_cmd_opts_t *options);

#endif  /* __FH_TEST_CMDLINE_H__ */
