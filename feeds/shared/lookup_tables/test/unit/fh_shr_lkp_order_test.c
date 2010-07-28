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

/* system headers */
#include <stdlib.h>

/* shared FH component headers */
#include "fh_shr_cfg_table.h"
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_order.h"

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

    fh_shr_lkp_ord_init(valid_config(), &table);
    return &table;
}

fh_shr_lkp_ord_t *valid_entry()
{
    static fh_shr_lkp_ord_t entry = {
        .order_no       = 1,
        .price          = 1005000,
        .shares         = 100,
        .buy_sell_ind   = 'B',
        .stock          = "AAPL",
        .sym_entry      = NULL,
        .context        = NULL
    };

    return &entry;
}

fh_shr_lkp_ord_key_t *valid_key()
{
    static fh_shr_lkp_ord_key_t key = {
        .order_no       = 1
    };

    return &key;
}

void test_count_responds_to_entry_insertion()
{
    fh_shr_lkp_tbl_t *table = valid_table();
    fh_shr_lkp_ord_t *tblentry;

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_add(table, valid_entry(), &tblentry), (int)FH_OK);
    FH_TEST_ASSERT_EQUAL(table->count, 1);
}

void test_get_after_insertion_yields_correct_values()
{
    fh_shr_lkp_ord_t        *tblentry;
    fh_shr_lkp_tbl_t        *table = valid_table();
    fh_shr_lkp_ord_t        *entry = valid_entry();

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_add(table, entry, &tblentry), (int)FH_OK);
    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_get(table, valid_key(), &tblentry), (int)FH_OK);

    FH_TEST_ASSERT_LEQUAL(tblentry->order_no, entry->order_no);
    FH_TEST_ASSERT_LEQUAL(tblentry->price, entry->price);
    FH_TEST_ASSERT_EQUAL(tblentry->shares, entry->shares);
    FH_TEST_ASSERT_EQUAL(tblentry->buy_sell_ind, entry->buy_sell_ind);
    FH_TEST_ASSERT_STREQUAL(tblentry->stock, entry->stock);

    FH_TEST_ASSERT_EQUAL(table->count, 1);
}

void test_retrieving_entry_with_freed_key_works()
{
    fh_shr_lkp_ord_key_t    *key;
    fh_shr_lkp_ord_t        *entry;
    fh_shr_lkp_tbl_t        *table = valid_table();

    key = (fh_shr_lkp_ord_key_t *)malloc(sizeof(fh_shr_lkp_ord_key_t));
    memcpy(key, valid_key(), sizeof(fh_shr_lkp_ord_key_t));

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_add(table, valid_entry(), &entry), (int)FH_OK);
    free(key);
    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_get(table, valid_key(), &entry), (int)FH_OK);

    FH_TEST_ASSERT_EQUAL(table->count, 1);
}

void test_modified_entry_persists_after_refetching()
{
    fh_shr_lkp_ord_t        *tblentry;
    fh_shr_lkp_tbl_t        *table = valid_table();
    fh_shr_lkp_ord_t        *entry = valid_entry();

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_add(table, entry, &tblentry), (int)FH_OK);
    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_get(table, valid_key(), &tblentry), (int)FH_OK);

    tblentry->shares = 200;
    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_get(table, valid_key(), &tblentry), (int)FH_OK);

    FH_TEST_ASSERT_LEQUAL(tblentry->order_no, entry->order_no);
    FH_TEST_ASSERT_LEQUAL(tblentry->price, entry->price);
    FH_TEST_ASSERT_EQUAL(tblentry->shares, 200);
    FH_TEST_ASSERT_EQUAL(tblentry->buy_sell_ind, entry->buy_sell_ind);
    FH_TEST_ASSERT_STREQUAL(tblentry->stock, entry->stock);

    FH_TEST_ASSERT_EQUAL(table->count, 1);
}

void test_fetch_returns_same_pointer_as_add()
{
    fh_shr_lkp_ord_t *addentry, *getentry;
    fh_shr_lkp_tbl_t *table = valid_table();

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_add(table, valid_entry(), &addentry), (int)FH_OK);
    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_get(table, valid_key(), &getentry), (int)FH_OK);
    FH_TEST_ASSERT_LEQUAL((unsigned long)addentry, (unsigned long)getentry);

}

void test_deleting_an_existing_order_decrements_count()
{
    fh_shr_lkp_tbl_t *table = valid_table();
    fh_shr_lkp_ord_t *entry;

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_add(table, valid_entry(), &entry), (int)FH_OK);
    FH_TEST_ASSERT_EQUAL(table->count, 1);
    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_del(table, valid_key(), &entry), (int)FH_OK);
    FH_TEST_ASSERT_EQUAL(table->count, 0);
}

void test_deleting_a_nonexistent_order_produces_correct_status()
{
    fh_shr_lkp_tbl_t *table = valid_table();
    fh_shr_lkp_ord_t *entry;

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_del(table, valid_key(), &entry), (int)FH_ERR_NOTFOUND);
    FH_TEST_ASSERT_NULL(entry);
    FH_TEST_ASSERT_EQUAL(table->count, 0);
}

void test_order_is_not_fetchable_after_delete()
{
    fh_shr_lkp_tbl_t *table = valid_table();
    fh_shr_lkp_ord_t *entry;

    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_add(table, valid_entry(), &entry), (int)FH_OK);
    FH_TEST_ASSERT_EQUAL(table->count, 1);
    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_del(table, valid_key(), &entry), (int)FH_OK);
    FH_TEST_ASSERT_EQUAL(table->count, 0);
    FH_TEST_ASSERT_EQUAL((int)fh_shr_lkp_ord_get(table, valid_key(), &entry), (int)FH_ERR_NOTFOUND);
}
