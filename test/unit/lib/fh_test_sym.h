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

#ifndef __FH_TEST_SYM_H__
#define __FH_TEST_SYM_H__

// define a function signature for function pointers to tests
typedef void (*fh_sym_test_func_t)();

// structure to store the needed details for all test symbols that are to be run
typedef struct fh_test_sym_test {
    char                *name;
    char                *source_file;
    long                 first_line;
    fh_sym_test_func_t   function;
} fh_test_sym_test_t;

/*! \brief Generate an array of test structures in the specified file
 *
 *  \param filename file to scan for symbols
 *  \param num_tests pointer in which to store the number of tests (if not NULL)
 *  \return pointer to array of test structures
 */
fh_test_sym_test_t *fh_test_sym_get_tests(const char *filename, long *test_count);

#endif  /* __FH_TEST_SYM_H__ */
