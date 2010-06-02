/* system headers */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* common FH headers */
#include "fh_log.h"
#include "fh_mpool.h"
#include "fh_htable.h"

/* shared library headers */
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_symbol.h"
#include "fh_shr_cfg_table.h"

/*
 * Hash a symbol table key
 */
static uint32_t key_hash(fh_shr_lkp_sym_key_t *key, int key_length)
{
    FH_ASSERT(key_length == sizeof(fh_shr_lkp_sym_key_t));
    return jhash2((uint32_t *)key, sizeof(fh_shr_lkp_sym_key_t) / 4, 0);
}

/*
 * Dump a symbol table key
 */
static char *key_dump(fh_shr_lkp_sym_key_t *key, int key_length)
{
    static char stringified_key[256];
     
    FH_ASSERT(key_length == sizeof(fh_shr_lkp_sym_key_t));
    sprintf(stringified_key, "Symbol: %s", key->symbol);
    return(stringified_key);
}

/*
 * Compare two symbol keys
 */
static int key_compare(fh_shr_lkp_sym_key_t *key1, fh_shr_lkp_sym_key_t *key2, int key_length)
{
    FH_ASSERT(key_length == sizeof(fh_shr_lkp_sym_key_t));
    return(memcmp(key1, key2, sizeof(fh_shr_lkp_sym_key_t)) == 0);
}

/*
 * Initialize a symbol table
 */
FH_STATUS fh_shr_lkp_sym_init(fh_shr_cfg_tbl_t *config, fh_shr_lkp_tbl_t *table)
{
    /* initialize the table structure (to make sure that everything is zeroed) */
    memset(table, 0, sizeof(fh_shr_lkp_tbl_t));
    
    /* if the symbol table is enabled */
    if (config->enabled) {
        /* structure of key operations for hash tables */
        static fh_ht_kops_t key_operations = {
            .kops_khash = (fh_ht_khash_t *)key_hash,
            .kops_kcmp  = (fh_ht_kcmp_t *)key_compare,
            .kops_kdump = (fh_ht_kdump_t *)key_dump,
        };

        /* Initialize the (non-growable) memory pool from which symbol entries will be plucked */
        table->mempool = fh_mpool_new("SymbolTable", sizeof(fh_shr_lkp_sym_t), config->size, 0);
        if (!table->mempool) {
            FH_LOG(LH, ERR, ("failed to initialize the symbol table memory pool"));
            return FH_ERROR;
        }

        /* initialize the growable hash table */
        table->hash = fh_ht_new(config->size, 0, &key_operations);
        if (!table->hash) {
            FH_LOG(LH, ERR, ("Failed to initialize the symbol hash table"));
            fh_mpool_free(table->mempool);
            return FH_ERROR;
        }

        FH_LOG(LH, STATE, ("Symbol table initialized: size:%d key size:%d",
                           config->size, sizeof(fh_shr_lkp_sym_key_t)));

        table->size   = config->size;
        table->count  = 0;
    }
    
    /* if we get here, success */
    return FH_OK;
}

/*
 * Fetch an existing symbol table entry or get a new one
 */
FH_STATUS fh_shr_lkp_sym_get(fh_shr_lkp_tbl_t *table, fh_shr_lkp_sym_key_t *key,
                             fh_shr_lkp_sym_t **entry)
{
    FH_STATUS rc;
    
    /* attempt to get the symbol entry from the table and add it if not present */
    rc = fh_ht_get(table->hash, key, sizeof(fh_shr_lkp_sym_key_t), (void **)entry);
    if (rc == FH_ERR_NOTFOUND) {
        /* get a new options entry from the memory pool */
        *entry = (fh_shr_lkp_sym_t *)fh_mpool_get(table->mempool);
        if (*entry == NULL) {
            FH_LOG(LH, ERR, ("failed to get a new symbol table entry"));
            return FH_ERROR;
        }
        
        /* copy the symbol and key into the new entry */
        strcpy((*entry)->symbol, key->symbol);
        memcpy(&(*entry)->key, key, sizeof(fh_shr_lkp_sym_key_t));
        
        /* re-point the key pointer at the copy inside the entry */
        key = &(*entry)->key;

        /* create a new hash table entry */
        if (fh_ht_put(table->hash, key, sizeof(fh_shr_lkp_sym_key_t), *entry) == FH_ERR_DUP) {
            FH_LOG(LH, ERR, ("duplicate symbol in symbol table: %s", key));
            fh_mpool_put(table->mempool, *entry);
            return FH_ERR_DUP;
        }
    
        /* if we get here, increment the table count */
        table->count++;
        
        /* if the table is > 90% capacity produce a warning every 100 new symbols */
        if ((10 * table->count) > (9 * table->size) && (table->count % 100) == 0) {
            FH_LOG(LH, WARN, ("symbol table over 90%% full (count: %d)", table->count));
        }
    }
    /* if there was an error other than ERR_NOTFOUND while getting entry from the hash table */
    else if (rc != FH_OK) {
        FH_LOG(LH, ERR, ("Unknown error fetching entry from symbol table (%d)", rc));
        return rc;
    }

    /* if we have gotten here, success! */
    return FH_OK;
}
