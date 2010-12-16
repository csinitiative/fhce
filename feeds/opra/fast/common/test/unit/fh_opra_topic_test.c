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
#include <string.h>

/* unit test headers */
#include "fh_test_assert.h"

/* common FH headers */
#include "fh_errors.h"

/* common OPRA headers */
#include "fh_opra_option.h"
#include "fh_opra_topic.h"

void test_five_character_symbols_produce_correct_topic()
{
    fh_opra_topic_fmt_t format = {
        .tfmt_num_stanzas    = 4,
        .tfmt_stanza_delim   = '.'
    };
    fh_opra_opt_key_t   key = {
        .k_year      = 10,
        .k_month     = 5,
        .k_day       = 10,
        .k_putcall   = 'C',
        .k_decimal   = 12345,
        .k_fraction  = 123,
        .k_exchid    = 'Q'
    };
    char                topic[32];
    FH_STATUS           rc;

    /* initialize the source material for topic generation and formatting */
    format.tfmt_stanza_fmts[0] = "OPRA";
    format.tfmt_stanza_fmts[1] = "$S";
    format.tfmt_stanza_fmts[2] = "$Y$M$D$C$I$F";
    format.tfmt_stanza_fmts[3] = "$X";
    memcpy(key.k_symbol, "ABCDE", sizeof(key.k_symbol));

    rc = fh_opra_topic_fmt(&format, &key, topic, sizeof(topic));
    FH_TEST_ASSERT_STATEQUAL(rc, FH_OK);
    FH_TEST_ASSERT_STREQUAL(topic, "OPRA.ABCDE.100510C12345123.Q");
}
