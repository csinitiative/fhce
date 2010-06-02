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

// FH test headers
#include "fh_test.h"


void fh_test_prep_msg(const char *file, int line)
{
    sprintf(fh_test_msg, "%s failed (%s:%d)", fh_test_msg, file, line);
}

/*! \brief Assert that the provided expression is true (used internally by all other assertions)
 *  \param expression expression that is asserted to be true
 */
void fh_test_assert(int expression, const char *file, int line)
{
    // increment the number of assertions
    fh_test_stats->assertions++;
    
    // if the expression is false exit (w/ a non-zero return code)
    if (!expression) {
        fh_test_prep_msg(file, line);
        exit(1);
    }
}
