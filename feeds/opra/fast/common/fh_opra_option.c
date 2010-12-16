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

/*
 * System includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * FH Common includes
 */
#include "fh_log.h"
#include "fh_util.h"
#include "fh_mpool.h"
#include "fh_htable.h"

/*
 * FH OPRA includes
 */
#include "fh_opra_cfg.h"
#include "fh_opra_option.h"
#include "fh_opra_topic.h"
#include "fh_opra_lh.h"
#include "fh_opra_ml.h"

/*
 * Options database
 */
typedef struct {
    fh_mpool_t  *odb_mpool;
    fh_ht_t     *odb_htable;
    uint32_t     odb_count;
    uint32_t     odb_size;
    uint32_t     odb_init;
} opt_db_t;

/*
 * Option DB Key operations
 */
static uint32_t opt_khash (fh_opra_opt_key_t *k, int klen);
static char *   opt_kdump (fh_opra_opt_key_t *k, int klen);
static int      opt_kcmp  (fh_opra_opt_key_t *k_a, fh_opra_opt_key_t *k_b, int klen);

static fh_ht_kops_t odb_kops = {
    .kops_khash = (fh_ht_khash_t *) opt_khash,
    .kops_kcmp  = (fh_ht_kcmp_t  *) opt_kcmp,
    .kops_kdump = (fh_ht_kdump_t *) opt_kdump,
};

static opt_db_t opt_db = { .odb_init = 0 }, *odb = &opt_db;

/*
 * fh_opra_opt_init
 *
 * Initialize the options database.
 */
FH_STATUS fh_opra_opt_init()
{
    FH_ASSERT(odb->odb_init == 0);

    /*
     * Initialize the growable memory pool.
     *
     * Note that the memory pool is not growable, we cannot afford to grow the
     * size of the table w/o incurring some latency outliers due to dynamic
     * memory allocation. It is better to oversize the table by at least 10%, so
     * there is enough room for growth.
     */
    odb->odb_mpool = fh_mpool_new("OptionTable", sizeof(fh_opra_opt_t),
                                  opra_cfg.ocfg_table_size, 0);
    if (!odb->odb_mpool) {
        FH_LOG(LH, ERR, ("Failed to initialize the options mem pool"));
        return FH_ERROR;
    }

    /*
     * Initialize the growable H-Table.
     */
    odb->odb_htable = fh_ht_new(opra_cfg.ocfg_table_size, 0, &odb_kops);
    if (!odb->odb_htable) {
        FH_LOG(LH, ERR, ("Failed to initialize the options H-table"));
        fh_mpool_free(odb->odb_mpool);
        return FH_ERROR;
    }

    FH_LOG(LH, STATE, ("Option DB initialized: size:%d key size:%d",
                       opra_cfg.ocfg_table_size, sizeof(fh_opra_opt_key_t)));

    odb->odb_size   = opra_cfg.ocfg_table_size;
    odb->odb_count  = 0;
    odb->odb_init   = 1;

    return FH_OK;
}

/*
 * fh_opra_opt_memdump
 *
 * Dump the memory usage of the options database.
 */
void fh_opra_opt_memdump()
{
    FH_ASSERT(odb->odb_init);

    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("> Options Database Memory footprint:"));
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("OPTION DB number of options   : %d", odb->odb_count));
    FH_LOG_PGEN(DIAG, ("OPTION DB H-table usage ratio : %d / %d",
                       odb->odb_htable->ht_count, odb->odb_htable->ht_size));
    FH_LOG_PGEN(DIAG, ("OPTION DB H-table memory      : %.2fK bytes",
                       (float) fh_ht_memuse(odb->odb_htable)/1000));
    FH_LOG_PGEN(DIAG, ("OPTION DB Mem pool memory     : %.2fK bytes",
                       (float) fh_mpool_memuse(odb->odb_mpool)/1000));
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
}

/*
 * fh_opra_opt_add
 *
 * Add a new option entry to the option table.
 */
FH_STATUS fh_opra_opt_add(fh_opra_opt_key_t *k, fh_opra_opt_t **optp)
{
    fh_opra_opt_t *opt = NULL;
    FH_STATUS      rc;
    fh_opra_lo_t  *lo = NULL;

    FH_ASSERT(odb->odb_init);

    if (FH_LL_OK(LH, INFO)) {
        FH_LOG_PGEN(INFO, (" > Option Symbol     : %s",    k->k_symbol));
        FH_LOG_PGEN(INFO, (" > Expiration Year   : %.2u",  k->k_year));
        FH_LOG_PGEN(INFO, (" > Expiration Month  : %.2u",  k->k_month));
        FH_LOG_PGEN(INFO, (" > Expiration Day    : %.2u",  k->k_day));
        FH_LOG_PGEN(INFO, (" > Put/Call          : %c",    k->k_putcall));
        FH_LOG_PGEN(INFO, (" > Strike Price      : %u.%u", k->k_decimal, k->k_fraction));
        FH_LOG_PGEN(INFO, (" > Exchange Code     : %c",    k->k_exchid));
    }

    /*
     * First look for the listed option associated with this root symbol
     */
    rc = fh_opra_lo_lookup(k->k_symbol, &lo);
    if (rc != FH_OK) {
        FH_LOG(LH, ERR, ("Failed to get a new options entry"));
        return FH_ERROR;
    }

    /*
     * Get a new options entry from the memory pool.
     */
    opt = (fh_opra_opt_t *) fh_mpool_get(odb->odb_mpool);
    if (!opt) {
        FH_LOG(LH, ERR, ("Failed to get a new options entry"));
        return FH_ERROR;
    }

    /*
     * Copy the option key
     */
    memcpy(&opt->opt_key, k, sizeof(fh_opra_opt_key_t));

    /*
     * Create the OPRA topic
     */
    rc = fh_opra_topic_fmt(&opra_cfg.ocfg_topic_fmt, k, opt->opt_topic, sizeof(opt->opt_topic));
    if (rc != FH_OK) {
        FH_LOG(LH, ERR, ("Failed to format the topic for sec:%s k:%s",
                         lo->lo_sec, opt_kdump(k, sizeof(fh_opra_opt_key_t))));
        fh_mpool_put(odb->odb_mpool, opt);
        return rc;
    }

    /*
     * Reference the listed option in the option
     */
    opt->opt_lo = lo;

    /*
     * Add the option entry to the htable
     */
    rc = fh_ht_put(odb->odb_htable, &opt->opt_key, sizeof(fh_opra_opt_key_t), opt);
    if (rc == FH_ERR_DUP) {
        FH_LOG(LH, ERR, ("Duplicate option in DB: '%s'",
                         opt_kdump(k, sizeof(fh_opra_opt_key_t))));
        fh_mpool_put(odb->odb_mpool, opt);
        return rc;
    }

    if (rc != FH_OK) {
        FH_LOG(LH, ERR, ("Failed to add option to DB (size:%d)",
                         opt_kdump(k, sizeof(fh_opra_opt_key_t)), odb->odb_count));
        fh_mpool_put(odb->odb_mpool, opt);
        return rc;
    }

    /*
     * Add the new option to the line it belongs to.
     */
    fh_opra_lh_add_opt(fh_opra_lh_line_num, opt);

    /*
     * We need to add the initialization of the messaging context
     * to be able to publish on this option.
     */
    rc = fh_opra_ml_opt_add(opt);
    if (rc != FH_OK) {
        FH_LOG(LH, ERR, ("Failed to add option to message layer: %s", opt->opt_topic));
    }

    *optp = opt;

    odb->odb_count++;

    /*
     * If the table grows beyond 90% capacity, then we start putting warnings on
     * every 10k new options
     */
    if ((10 * odb->odb_count > 9 * odb->odb_size) && (odb->odb_count % 10000) == 0) {
        FH_LOG_PGEN(WARN, ("Option Table size over %.2f %% utilization (count: %d)",
                           (float) odb->odb_count * 100 / odb->odb_size, odb->odb_count));

    }

    return FH_OK;
}

/*
 * fh_opra_opt_lookup
 *
 * Lookup a given option in database.
 */
FH_STATUS fh_opra_opt_lookup(fh_opra_opt_key_t *k, fh_opra_opt_t **optp)
{
    FH_STATUS rc;
    void *val = NULL;

    FH_ASSERT(odb->odb_init);

    /*
     * Lookup the root in the H-Table
     */
    rc = fh_ht_get(odb->odb_htable, k, sizeof(fh_opra_opt_key_t), &val);
    if (rc != FH_OK) {
        return rc;
    }

    *optp = (fh_opra_opt_t *) val;

    return FH_OK;
}

/*
 * opt_khash
 *
 * Hash a given option entry.
 */
static uint32_t opt_khash(fh_opra_opt_key_t *k, int klen)
{
    FH_ASSERT(klen == sizeof(fh_opra_opt_key_t));

    return jhash2((uint32_t*)k, FH_OPRA_OPT_KEY_SIZE, 0);
}

/*
 * opt_kdump
 *
 * Dump a given option key.
 */
static char *opt_kdump(fh_opra_opt_key_t *k, int klen)
{
    static char keystr[256];

    FH_ASSERT(klen == sizeof(fh_opra_opt_key_t));

    sprintf(keystr, "Option symbol: %s year: %.2u month: %.2u day: %.2u put/call: %c "
            "strike: %u.%u exch: %c", k->k_symbol, k->k_year, k->k_month, k->k_day,
            k->k_putcall, k->k_decimal, k->k_fraction, k->k_exchid);

    return(keystr);
}

/*
 * opt_kcmp
 *
 * Compare two options keys.
 */
static int opt_kcmp(fh_opra_opt_key_t *key1, fh_opra_opt_key_t *key2, int klen)
{
    FH_ASSERT(klen == sizeof(fh_opra_opt_key_t));
    return(memcmp(key1, key2, sizeof(fh_opra_opt_key_t)) == 0);
}


