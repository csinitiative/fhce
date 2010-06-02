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
#include <stdint.h>
#include <string.h>

// FH headers
#include "fh_cpu.h"
#include "fh_errors.h"

// FH test headers
#include "fh_test_assert.h"


// test that fh_cpu_rdspeed returns a number in the right general range (between 1 GHz and 6 GHz)
void test_cpu_rdspeed_value_is_reasonable()
{
    uint32_t speed = fh_cpu_rdspeed();
    FH_TEST_ASSERT_TRUE(speed > 1000 && speed < 6000);
}

// test that fh_cpu_count returns a number in the right general range (between 1 and 32)
void test_cpu_count_value_is_reasonable()
{
    uint32_t cpus = fh_cpu_count();
    FH_TEST_ASSERT_TRUE(cpus >= 1 && cpus <= 32);
}

// test that fh_cpu_print produces correct output with a couple of representative cpu masks
void test_cpu_print_produces_correct_output()
{
    char buffer[1024];
    
    memset(&buffer, 0, 1024);

    fh_cpu_print(0x1, buffer, 1024);
    FH_TEST_ASSERT_FALSE(strcmp(buffer, "0"));

    fh_cpu_print(0xff, buffer, 1024);
    FH_TEST_ASSERT_FALSE(strcmp(buffer, "0,1,2,3,4,5,6,7"));
}

// test that passing a valid CPU mask to fh_cpu_setaffinity returns FH_OK
void test_cpu_setaffinity_with_valid_mask_returns_ok()
{
    FH_TEST_ASSERT_TRUE(fh_cpu_setaffinity(0x1) == FH_OK);
}

// test that passing an invalid CPU mask to fh_cpu_setaffinity returns FH_ERROR
void test_cpu_setaffinity_with_invalid_mask_returns_error()
{
    FH_TEST_ASSERT_TRUE(fh_cpu_setaffinity(0x10000000) == FH_ERROR);
}

// test that cpu_getaffinity returns FH_OK and sets a 0x0 mask by default
void test_cpu_getaffiniity_behaves_correctly()
{
    int         i;
    int         num_cpus;
    uint32_t    returned_mask;
    uint32_t    correct_mask = 0;

    // set up a cpu mask equal to all cpus in the system
    num_cpus = fh_cpu_count();
    for (i = 0; i < num_cpus; i++) {
        correct_mask |= (1 << i);
    }
    
    FH_TEST_ASSERT_TRUE(fh_cpu_getaffinity(&returned_mask) == FH_OK);
    FH_TEST_ASSERT_LEQUAL((long int)returned_mask, (long int)correct_mask);
}

// test that cpu_getaffinity returns FH_OK and correct mask after a call to cpu_setaffinity
void test_cpu_getaffinity_behaves_correctly_after_setaffinity()
{
    uint32_t returned_mask;
        
    // set affinity to the first CPU
    FH_TEST_ASSERT_TRUE(fh_cpu_setaffinity(0x1) == FH_OK);
    
    // check to make sure that get_affinity is behaving as expected
    FH_TEST_ASSERT_TRUE(fh_cpu_getaffinity(&returned_mask) == FH_OK);
    FH_TEST_ASSERT_LEQUAL((long int)returned_mask, (long int)0x1);
}
