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

/* system headers */
#include <stdlib.h>

/* shared FH component headers */
#include "fh_shr_cfg_table.h"
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_symbol.h"

/* FH unit test framework headers */
#include "fh_test_assert.h"

fh_shr_cfg_tbl_t *valid_config()
{
    static fh_shr_cfg_tbl_t config = {
        .name       = "dummy_config",
        .enabled    = 1,        
        .size       = 100
    };
    
    return &config;
}

fh_shr_lkp_tbl_t *valid_table()
{
    static fh_shr_lkp_tbl_t table;
    
    fh_shr_lkp_sym_init(valid_config(), &table);
    return &table;
}

fh_shr_lkp_sym_key_t *valid_key()
{
    static fh_shr_lkp_sym_key_t key = {
        .symbol = "AAAAA"
    };
    
    return &key;
}

void test_count_responds_to_entry_insertion()
{
    fh_shr_lkp_tbl_t        *table = valid_table();
    fh_shr_lkp_sym_t        *entry;

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_sym_get(table, valid_key(), &entry), (int)FH_OK);
    FH_TEST_ASSERT_EQUAL(table->count, 1);
}

void test_retrieval_of_same_key_twice_leaves_count_alone()
{
    fh_shr_lkp_tbl_t        *table = valid_table();
    fh_shr_lkp_sym_t        *entry;

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_sym_get(table, valid_key(), &entry), (int)FH_OK);
    FH_TEST_ASSERT_EQUAL(table->count, 1);

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_sym_get(table, valid_key(), &entry), (int)FH_OK);
    FH_TEST_ASSERT_EQUAL(table->count, 1);
}

void test_retrieval_of_same_key_twice_preserves_context_pointer()
{
    fh_shr_lkp_tbl_t        *table = valid_table();
    fh_shr_lkp_sym_t        *entry;

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_sym_get(table, valid_key(), &entry), (int)FH_OK);
    entry->context = (void *)"foo";

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_sym_get(table, valid_key(), &entry), (int)FH_OK);
    FH_TEST_ASSERT_STREQUAL(entry->context, "foo");
}

void test_retrieving_entry_with_freed_key_works()
{
    fh_shr_lkp_sym_key_t    *key;
    fh_shr_lkp_sym_t        *entry;
    fh_shr_lkp_tbl_t        *table = valid_table();
    
    key = (fh_shr_lkp_sym_key_t *)malloc(sizeof(fh_shr_lkp_sym_key_t));
    memset(key, 0, sizeof(fh_shr_lkp_sym_key_t));
    strcpy(key->symbol, "AAAAA");
    
    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_sym_get(table, key, &entry), (int)FH_OK);
    free(key);
    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_sym_get(table, valid_key(), &entry), (int)FH_OK);
    
    FH_TEST_ASSERT_EQUAL(table->count, 1);
}

