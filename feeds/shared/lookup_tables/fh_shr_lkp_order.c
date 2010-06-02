/* system headers */
// #include <stdint.h>
#include <stdio.h>
#include <string.h>

/* common FH headers */
#include "fh_log.h"
// #include "fh_mpool.h"
// #include "fh_htable.h"

/* shared library headers */
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_order.h"
#include "fh_shr_cfg_table.h"

/*
 * Hash an order table key
 */
static uint32_t key_hash(fh_shr_lkp_ord_key_t *key, int key_length)
{
    FH_ASSERT(key_length == sizeof(fh_shr_lkp_ord_key_t));
    return jhash2((uint32_t *)key, sizeof(fh_shr_lkp_ord_key_t) / 4, 0);
}

/*
 * Dump an order table key
 */
static char *key_dump(fh_shr_lkp_ord_key_t *key, int key_length)
{
    static char stringified_key[256];
     
    FH_ASSERT(key_length == sizeof(fh_shr_lkp_ord_key_t));
    sprintf(stringified_key, "Order: %lu Order str: %12s", key->order_no,key->order_no_str);
    return(stringified_key);
}

/*
 * Compare two order keys
 */
static int key_compare(fh_shr_lkp_ord_key_t *key1, fh_shr_lkp_ord_key_t *key2, int key_length)
{
    FH_ASSERT(key_length == sizeof(fh_shr_lkp_ord_key_t));
    return (memcmp(key1, key2, sizeof(fh_shr_lkp_ord_key_t)) == 0);
}

/*
 * Dump the contents of an order table entry
 */
void fh_shr_lkp_ord_dump(fh_shr_lkp_ord_t *entry)
{
    printf("order_no : %lu\n", entry->order_no);
    printf("price    : %lu\n", entry->price);
    printf("shares   : %u\n",  entry->shares);
    printf("buy_sell : %c\n",  entry->buy_sell_ind);
    printf("stock    : %s\n",  entry->stock);
}

/*
 * Initialize an order table
 */
FH_STATUS fh_shr_lkp_ord_init(fh_shr_cfg_tbl_t *config, fh_shr_lkp_tbl_t *table)
{
    /* initialize the table structure (to make sure that everything is zeroed) */
    memset(table, 0, sizeof(fh_shr_lkp_tbl_t));
    
    /* if the order table is enabled */
    if (config->enabled) {
        /* structure of key operations for hash tables */
        static fh_ht_kops_t key_operations = {
            .kops_khash = (fh_ht_khash_t *)key_hash,
            .kops_kcmp  = (fh_ht_kcmp_t *)key_compare,
            .kops_kdump = (fh_ht_kdump_t *)key_dump,
        };

        /* Initialize the (non-growable) memory pool from which symbol entries will be plucked */
        table->mempool = fh_mpool_new("OrderTable", sizeof(fh_shr_lkp_ord_t), config->size, 0);
        if (!table->mempool) {
            FH_LOG(LH, ERR, ("failed to initialize the order table memory pool"));
            return FH_ERROR;
        }

        /* initialize the growable hash table */
        table->hash = fh_ht_new(config->size, 0, &key_operations);
        if (!table->hash) {
            FH_LOG(LH, ERR, ("Failed to initialize the order hash table"));
            fh_mpool_free(table->mempool);
            return FH_ERROR;
        }

        FH_LOG(LH, STATE, ("Order table initialized: size:%d key size:%d",
                           config->size, sizeof(fh_shr_lkp_ord_key_t)));

        table->size   = config->size;
        table->count  = 0;
    }
    
    /* if we get here, success */
    return FH_OK;
}

/*
 * Fetch an order, if it exists, from the order table
 */
FH_STATUS fh_shr_lkp_ord_get(fh_shr_lkp_tbl_t *table, fh_shr_lkp_ord_key_t *key,
                             fh_shr_lkp_ord_t **entry)
{
    /* attempt to get the order entry from the table and return whatever status code is returned */
    return fh_ht_get(table->hash, key, sizeof(fh_shr_lkp_ord_key_t), (void **)entry);
}

/*
 * Add an order to the order table
 */
FH_STATUS fh_shr_lkp_ord_add(fh_shr_lkp_tbl_t *table, fh_shr_lkp_ord_t *entry,
                             fh_shr_lkp_ord_t **tblentry)
{
    fh_shr_lkp_ord_t *nentry;
    
    /* get a new options entry from the memory pool */
    nentry = (fh_shr_lkp_ord_t *)fh_mpool_get(table->mempool);
    if (nentry == NULL) {
        FH_LOG(LH, ERR, ("failed to get a new order table entry"));
        return FH_ERROR;
    }
    
    /* set the newly fetched entry with the value passed in */
    memcpy(nentry, entry, sizeof(fh_shr_lkp_ord_t));
    
    /* set the entry's key based on information from the order itself */
    nentry->key.order_no = nentry->order_no;
    memcpy(&nentry->key.order_no_str[0], &nentry->order_no_str[0], 20);

    /* store the new entry in the order table */
    if (fh_ht_put(table->hash, &nentry->key, sizeof(fh_shr_lkp_ord_key_t), nentry) == FH_ERR_DUP) {
        FH_LOG(LH, ERR, ("duplicate order in order table: %ld", nentry->key.order_no));
        fh_mpool_put(table->mempool, nentry);
        return FH_ERR_DUP;
    }

    /* if we get here, increment the table count and assign the tblentry pointer */
    table->count++;
    *tblentry = nentry;
    
    /* if the table is > 90% capacity produce a warning every 100 new symbols */
    if ((10 * table->count) > (9 * table->size) && (table->count % 100) == 0) {
        FH_LOG(LH, WARN, ("order table over 90%% full (count: %d)", table->count));
    }
    
    /* if we have gotten here, success */
    return FH_OK;
}

/*
 * Delete an order from the order table
 */
FH_STATUS fh_shr_lkp_ord_del(fh_shr_lkp_tbl_t *table, fh_shr_lkp_ord_key_t *key,
                             fh_shr_lkp_ord_t **entry)
{
    void        *old_entry;
    FH_STATUS    rc;
    
    /* attempt to delete the entry with the given key from the given table */
    rc = fh_ht_delete(table->hash, key, sizeof(fh_shr_lkp_ord_key_t), &old_entry);
    
    /* if the delete returned FH_OK go ahead and decrement the table count */
    if (rc == FH_OK) {
        fh_mpool_put(table->mempool, old_entry);
        table->count--;
        *entry = (fh_shr_lkp_ord_t *)old_entry;
    }
    else {
        *entry = NULL;
    }
    
    /* return whatever status code the delete returned */
    return rc;
}

