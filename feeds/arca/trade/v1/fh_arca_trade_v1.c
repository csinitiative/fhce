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

// System header(s)
#include <stdio.h>

// Arca FH header(s)
#include "fh_arca.h"
#include "fh_arca_constants.h"
#include "fh_arca_headers.h"

/*! \brief Main entry point for ArcaBook Multicast feed handler
 *
 *  \param argc number of command line arguments
 *  \param argv two dimension array of command line arguments
 *  \return 0 on success, non-zero on failure
 */
int main(int argc, char *argv[])
{ 

// if we are compiling with unit tests in
#ifdef __UNIT_TEST__
    // suppress warnings about unused arguments
    if (argc || argv) {}
    
    // perform unit tests
    int rc = unit_test_main();
    if (rc != 0) {
        printf("ERROR: non-zero return value from unit_test_main()\n");
    } else {
        printf("SUCCESS: zero return value from unit_test_main()\n");
    }
    
    // return status code from unit tests
    return rc;
#else
    // main Arca entry point function
    return fh_arca_main(argc, argv, 1);
#endif
};
