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

// FH test headers
#include "fh_test_assert.h"

/*
 * Each test file (fh_X_test.c) will be compiled into a separate binary.  Test files *do not*
 * need any kind of main program as the unit test library (libfhtest.a) contains a main program
 * that does all the work.  Any function, in a test file, that is named "test_*" will be run as
 * a unit test by the main function.  Test functions are expected to have a void return type and
 * take no arguments.  Any function that is not of this form (test_*) will be ignored by the main
 * function but can be called from within any test.  This is useful for situations where several
 * tests need to share a common chunk of functionality.  The following assertions are available to
 * all tests and are contained in fh_test_assert.h (included above):
 *
 * FH_TEST_ASSERT_TRUE(int)
 * FH_TEST_ASSERT_FALSE(int)
 * FH_TEST_ASSERT_EQUAL(int, int)
 * FH_TEST_ASSERT_UNEQUAL(int, int)
 * FH_TEST_ASSERT_LEQUAL(long, long)
 * FH_TEST_ASSERT_LUNEQUAL(long, long)
 * FH_TEST_ASSERT_STREQUAL(const char *, const char *)
 * FH_TEST_ASSERT_STRUNEQUAL(const char *, const char *)
 * FH_TEST_ASSERT_NULL(void *)
 * FH_TEST_ASSERT_NOTNULL(void *)
 */
void test_one_is_one()
{
    FH_TEST_ASSERT_EQUAL(1, 1);
}
