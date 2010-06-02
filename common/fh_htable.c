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

#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "fh_htable.h"
#include "fh_log.h"

#define FH_HT_PROFILING  (1)

#if FH_HT_PROFILING

#include "fh_prof.h"

static FH_PROF_DECL(HT_PUT,1<<21,25,1);
#endif

/*
 * ht_npower
 *
 * Returns the closest power of 2 greater than `n'.
 */
static int ht_npower(int n)
{
    register int k = 3;

    for (k = 0; k < 32; k++) {
        int size = 1 << k;
        if (size >= n) {
            return size;
        }
    }

    return 0;
}

#define ht_index(ht, hval) ((hval) & ht->ht_mask)

/*
 * fh_ht_new
 *
 * Allocate a new H-Table for the given size. This H-Table can grow or not
 * depending on the H-Table configuration flags.
 */
fh_ht_t *fh_ht_new(int size, int flags, fh_ht_kops_t *kops)
{
    fh_ht_t *ht = NULL;
    register int i;
    int pflags = 0;
    int psize  = size;

    ht = (fh_ht_t *) malloc(sizeof(fh_ht_t));
    if (!ht) {
        FH_LOG(CSI, ERR, ("Failed to allocate memory for H-Table object"));
        return NULL;
    }

    memset(ht, 0, sizeof(fh_ht_t));

    size *= 2;
    size  = ht_npower(size);

    ht->ht_table = (he_head_t *) calloc(size, sizeof(he_head_t));
    if (!ht->ht_table) {
        goto error;
    }

    memset(ht->ht_table, 0, size * sizeof(he_head_t));

    for (i=0; i<size; i++) {
        he_head_t *he_head = &ht->ht_table[i];
        TAILQ_INIT(he_head);
    }

    if (flags & FH_HT_FL_GROW) {
        pflags = FH_MPOOL_FL_GROW;
    }

    ht->ht_mpool = fh_mpool_new("HASH_ELMT", sizeof(fh_ht_elt_t), psize, pflags);
    if (!ht->ht_mpool) {
        goto error;
    }

    ht->ht_size = size;
    ht->ht_mask = size - 1;

    ht->ht_flags = flags;

    memcpy(&ht->ht_kops, kops, sizeof(fh_ht_kops_t));

    return ht;

error:
    FH_LOG(CSI, ERR, ("Couldn't allocate memory for the ht (size=%d)", size));
    fh_ht_free(ht);
    return NULL;
}

/*
 * fh_ht_free
 *
 * Free the H-Table and all internal resources
 */
void fh_ht_free(fh_ht_t *ht)
{
    if (ht) {
        if (ht->ht_table) {
            free(ht->ht_table);
        }
        if (ht->ht_mpool) {
            fh_mpool_free(ht->ht_mpool);
        }
        free(ht);
    }
}

/*
 * ht_grow
 *
 * Grow H-Table by doubling the size.
 */
static FH_STATUS ht_grow(fh_ht_t *ht)
{
    int i, newsize, size = ht->ht_size;
    register fh_ht_elt_t *he;
    he_head_t *oldtable, *newtable;

    newsize = ht_npower(2*size);

    FH_LOG(CSI, VSTATE, ("Grow H-Table from %d to %d", size, newsize));

    newtable = (he_head_t *) calloc(newsize, sizeof(he_head_t));
    if (!newtable) {
        FH_LOG(CSI, ERR, ("Failed to create new hash table with size %d", newsize));
        return FH_ERROR;
    }

    memset(newtable, 0, newsize * sizeof(he_head_t));

    for (i=0; i<newsize; i++) {
        he_head_t *he_head = &newtable[i];
        TAILQ_INIT(he_head);
    }

    oldtable     = ht->ht_table;

    ht->ht_table = newtable;
    ht->ht_size  = newsize;
    ht->ht_mask  = newsize - 1;

    /*
     * Insert the old keys in the new hash table
     */
    for (size--; size >= 0; size--) {
        he_head_t *old_head = &oldtable[size];

        while ((he = TAILQ_FIRST(old_head)) != NULL) {
            he_head_t *new_head;

            TAILQ_REMOVE(old_head, he, he_next);

            new_head = &newtable[ht_index(ht, he->he_hashval)];

            TAILQ_INSERT_HEAD(new_head, he, he_next);

            he->he_chead  = new_head;
        }
    }

    free(oldtable);

    return FH_OK;
}

/*
 * fh_ht_put
 *
 * Insert a new entry to the H-Table.
 */
FH_STATUS fh_ht_put(fh_ht_t *ht, void *key, int klen, void *value)
{
    register uint32_t hashval = 0;
    register int hops = 0;
    fh_ht_elt_t *he = NULL;
    he_head_t *he_head;
    fh_ht_kops_t *kops;
    int index;

#if FH_HT_PROFILING
    FH_PROF_INIT(HT_PUT);
    FH_PROF_BEG(HT_PUT);
#endif

    FH_ASSERT(ht);

    kops = &ht->ht_kops;

    /*
     * Compute the hash key outside the critical section
     */
    hashval = kops->kops_khash(key, klen);
    index   = ht_index(ht, hashval);
    he_head = &ht->ht_table[index];

    /*
     * Look through the collision list to make sure that it is not
     * a duplicate entry.
     */
    TAILQ_FOREACH(he, he_head, he_next) {

        if (he->he_hashval == hashval   &&
            he->he_klen    == klen      &&
            kops->kops_kcmp(he->he_key, key, klen)) {

            FH_LOG(CSI, DIAG, ("Duplicate key '%s' hashval=%u",
                               kops->kops_kdump(key, klen), hashval));
            return FH_ERR_DUP;
        }

        hops++;
    }

    /*
     * For debugging reasons, track the number of collisions that
     * we are seeing.
     */
    if (hops > 0) {
        FH_LOG(CSI, DIAG, ("Hash collisions - hops:%d key:%s",
            hops, kops->kops_kdump(key, klen)));
    }

    /*
     * Get a new hash element from the memory pool.
     */
    he = (fh_ht_elt_t *)fh_mpool_get(ht->ht_mpool);
    if (!he) {
        FH_LOG(CSI, ERR, ("Couldn't get another hash entry for key '%s'",
            kops->kops_kdump(key, klen)));
        return FH_ERROR;
    }

    ht->ht_count++;

    FH_LOG(CSI, INFO, ("Insert key '%s' hashval=%u", kops->kops_kdump(key, klen), hashval));

    /*
     * Grow the pool if necessary:
     *
     * Capping collisions to 5 hops if hash map remains
     * below 80% utilization.
     */
    if (ht->ht_flags & FH_HT_FL_GROW) {
        if (5 * (uint32_t)ht->ht_count > 4 * ht->ht_size) {
            /*
             * If we cannot grow the htable try to continue on,
             * we will try again next time.
             */
            (void) ht_grow(ht);

            index   = ht_index(ht, hashval);
            he_head = &ht->ht_table[index];
        }
    }

    he->he_key     = key;
    he->he_klen    = klen;
    he->he_hashval = hashval;

    /*
     * Add the new hash element at the beginning
     * of the collision list
     */
    TAILQ_INSERT_HEAD(he_head, he, he_next);

    he->he_chead = he_head;
    he->he_value = value;

#if FH_HT_PROFILING
    FH_PROF_END(HT_PUT);
    FH_PROF_PRINT(HT_PUT);
#endif

    return FH_OK;
}

/*
 * ht_lookup
 *
 * Looks up a hash-key in the table, and return the hash-element.
 */
static fh_ht_elt_t *ht_lookup(fh_ht_t *ht, void *key, int klen)
{
    register uint32_t hashval = 0;
    register fh_ht_elt_t *he = NULL;
    he_head_t *he_head;
    fh_ht_kops_t *kops;

    FH_ASSERT(ht);

    kops = &ht->ht_kops;

    /*
     * Compute the hash value outside the critical section
     */
    hashval = kops->kops_khash(key, klen);

    he_head = &ht->ht_table[ht_index(ht, hashval)];

    TAILQ_FOREACH(he, he_head, he_next) {
        if (he->he_hashval == hashval   &&
            he->he_klen    == klen      &&
            kops->kops_kcmp(he->he_key, key, klen)) {
            break;
        }
    }

    return he;
}

/*
 * fh_ht_get
 *
 * Get an entry from the H-Table
 */
FH_STATUS fh_ht_get(fh_ht_t *ht, void *key, int klen, void **value)
{
    fh_ht_elt_t *he;

    FH_ASSERT(ht);

    *value = NULL;

    he = ht_lookup(ht, key, klen);
    if (!he) {
        return FH_ERR_NOTFOUND;
    }

    *value = he->he_value;

    return FH_OK;
}

/*
 * fh_ht_delete
 *
 * Delete a hash entry from the table.
 */
FH_STATUS fh_ht_delete(fh_ht_t *ht, void *key, int klen, void **val)
{
    fh_ht_elt_t *he;

    FH_ASSERT(ht);

    *val = NULL;

    FH_ASSERT(ht && ht->ht_size > 0);

    he = ht_lookup(ht, key, klen);
    if (!he) {
        FH_LOG(CSI, ERR, ("Couldn't find key '%s'", ht->ht_kops.kops_kdump(key, klen)));
        return FH_ERR_NOTFOUND;
    }

    TAILQ_REMOVE(he->he_chead, he, he_next);

    *val = he->he_value;

    fh_mpool_put(ht->ht_mpool, he);

    FH_ASSERT(ht->ht_count > 0);
    ht->ht_count--;

    FH_LOG(CSI, INFO, ("Deleted key '%s' (remaining=%d)",
        ht->ht_kops.kops_kdump(key, klen), ht->ht_count));

    return FH_OK;
}

/*
 * fh_ht_memuse
 *
 * Dump the memory usage of the H-Table.
 */
uint32_t fh_ht_memuse(fh_ht_t *ht)
{
    uint32_t mem = 0;

    mem += sizeof(fh_ht_t);
    mem += ht->ht_size * sizeof(he_head_t);
    mem += fh_mpool_memuse(ht->ht_mpool);

    return mem;
}

