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
#include <string.h>
#include <errno.h>
#include <bfd.h>

// FH test headers
#include "fh_test_sym.h"

// data shared among all functions in this module
bfd      *the_bfd     = NULL;
asymbol **sym_table   = NULL;
long      num_symbols = 0;


/*! \brief Fetch an array of symbols for the specified filename
 *
 *  \param filename name of the file being opened
 */
void fh_test_sym_load_table(const char *filename)
{
    long    sym_table_size;
    
    // initialize the bfd library
    bfd_init();
        
    // get a bfd struct for the specified filename
    the_bfd = bfd_openr(filename, NULL);
    if (the_bfd == NULL) {
        fprintf(stderr, "\nERROR: could not open a BFD (%s)\n", bfd_errmsg(bfd_get_error()));
        exit(1);
    }
        
    // check for the format of the opened BFD (necessary to establish format)
    if (!bfd_check_format(the_bfd, bfd_object)) {
        fprintf(stderr, "\nERROR: incorrect BFD type (%s)\n", bfd_errmsg(bfd_get_error()));
        exit(1);
    }
        
    // get a list of symbols for the current binary
    sym_table_size = bfd_get_symtab_upper_bound(the_bfd);
    if (sym_table_size < 0) {
        fprintf(stderr, "\nERROR: invalid symbol table size (%s)\n", bfd_errmsg(bfd_get_error()));
        exit(1);
    }
        
    // allocate space for the symbol table
    sym_table = malloc(sym_table_size);
    if (sym_table == NULL) {
        fprintf(stderr, "\nERROR: unable to obtain memory for symbol table\n");
        exit(1);
    }
        
    // fetch the symbol table
    num_symbols = bfd_canonicalize_symtab(the_bfd, sym_table);
    if (num_symbols < 0) {
        fprintf(stderr, "\nERROR: invalid symbol table size %s\n", bfd_errmsg(bfd_get_error()));
        exit(1);
    }
}

/*! \brief Generate an array of test structures in the specified file
 *
 *  \param filename file to scan for symbols
 *  \param num_tests pointer in which to store the number of tests (if not NULL)
 *  \return pointer to array of test structures
 */
fh_test_sym_test_t *fh_test_sym_get_tests(const char *filename, long *test_count)
{
    int                  i;
    symbol_info          sym_info;
    const char          *sym_filename;
    unsigned int         sym_lineno;
    fh_test_sym_test_t  *tests = NULL;
    long                 count = 0;
    
    
    // load up the bfd symbol table that we will look through to find the tests that need to be run
    fh_test_sym_load_table(filename);
    
    // call each eligible function
    for (i = 0; i < num_symbols; i++) {
        // if the symbol name begins with fh_test_ and is an ELF function symbol...
        if (strstr(sym_table[i]->name, "test_") == sym_table[i]->name
            && sym_table[i]->flags & BSF_FUNCTION && sym_table[i]->flags & BSF_GLOBAL) {
            
            // fetch the symbol's info (the value component is the address of the function)
            bfd_symbol_info(sym_table[i], &sym_info);

            // fetch information about the source file of the symbol (if not possible, skip symbol)
            if (!bfd_find_line(the_bfd, sym_table, sym_table[i], &sym_filename, &sym_lineno)) {
                continue;
            }
            
            // if the symbol does not come from a file that begins with "fh_", skip symbol
            sym_filename = strrchr(sym_filename, '/') + sizeof(char);
            if (strstr(sym_filename, "fh_") != sym_filename) {
                continue;
            }
            
            // resize the tests array to make room for the new test
            tests = (fh_test_sym_test_t *)realloc(tests, sizeof(fh_test_sym_test_t) * (count + 1));
            if (tests == NULL) {
                fprintf(stderr, "ERROR: new test realloc: %s (%d)", strerror(errno), errno);
                exit(1);
            }
            
            // set the name of the new test
            tests[count].name = (char *)malloc(sizeof(char) * (strlen(sym_info.name) + 1));
            if (tests[count].name == NULL) {
                fprintf(stderr, "ERROR: new test malloc: %s (%d)", strerror(errno), errno);
                exit(1);
            }
            strcpy(tests[count].name, sym_info.name);

            // set the name of the source file
            tests[count].source_file = (char *)malloc(sizeof(char) * (strlen(sym_filename) + 1));
            if (tests[count].source_file == NULL) {
                fprintf(stderr, "ERROR: source filename malloc: %s (%d)", strerror(errno), errno);
                exit(1);
            }
            strcpy(tests[count].source_file, sym_filename);
            
            // set the source line number of the symbol's first line
            tests[count].first_line = sym_lineno;
            
            // set the function pointer to the new test
            tests[count].function = (fh_sym_test_func_t)sym_info.value;
            
            // increment the number of tests
            count++;
        }
    }
    
    // if num_tests parameter != NULL, store number of tests, then return the test array
    if (test_count != NULL) *test_count = count;
    return tests;
}
