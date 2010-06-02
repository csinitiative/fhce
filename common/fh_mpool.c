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
#include <pthread.h>
#include "fh_log.h"
#include "fh_mpool.h"

/*
 * fh_mpool_memuse
 *
 * Return the number of bytes currently allocated for the memory pool.
 */
uint32_t fh_mpool_memuse(fh_mpool_t *mp)
{
    int chunk_size, block_size;

    FH_ASSERT(mp);

    chunk_size = sizeof(fh_mchunk_t) + mp->mp_chunksize;
    block_size = sizeof(fh_mblock_t) + mp->mp_nchunks * chunk_size;

    return mp->mp_nblocks * block_size;
}

/*
 * fh_mpool_stats
 *
 * Dump some memory pool statistics.
 */
void fh_mpool_stats(fh_mpool_t *mp)
{
    FH_ASSERT(mp);

    FH_LOG_PGEN(DIAG, ("---------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("------------ Memory Pool Stats --------------"));
    FH_LOG_PGEN(DIAG, ("---------------------------------------------"));
    FH_LOG_PGEN(DIAG, (" > Memory pool name     : %s", mp->mp_name));
    FH_LOG_PGEN(DIAG, (" > Configuration flags  : 0x%08X", mp->mp_flags));
    FH_LOG_PGEN(DIAG, (" > Chunk size           : %10d", mp->mp_chunksize));
    FH_LOG_PGEN(DIAG, (" > Cum chunks per block : %10d", mp->mp_nchunks));
    FH_LOG_PGEN(DIAG, (" > Cum blocks           : %10d", mp->mp_nblocks));
    FH_LOG_PGEN(DIAG, (" > Cum free chunks      : %10d", mp->mp_free));
    FH_LOG_PGEN(DIAG, (" > Cum inuse chunks     : %10d", mp->mp_inuse));
}

/*
 * mpool_grow
 *
 * Grow the memory pool.
 */
static FH_STATUS mpool_grow(fh_mpool_t *mp)
{
    int chunk_size, block_size;
    uint32_t i;
    char     *arena = NULL;
    fh_mblock_t *mb    = NULL;
    fh_mchunk_t *mc    = NULL;

    FH_ASSERT(mp);

    chunk_size = sizeof(fh_mchunk_t) + mp->mp_chunksize;
    block_size = sizeof(fh_mblock_t) + mp->mp_nchunks * chunk_size;

    arena = (char *) malloc(block_size);
    if (!arena) {
        FH_LOG(CSI, ERR, ("Failed to allocate new block: %d bytes",
                          mp->mp_name, block_size));
        fh_mpool_stats(mp);
        return FH_ERROR;
    }

    memset(arena, 0, block_size);

    mb = (fh_mblock_t *) arena;

    mb->mb_next = mp->mp_mblocks;
    mp->mp_mblocks = mb;
    mp->mp_nblocks++;

    arena = (char *)(mb+1);

    for (i=0; i<mp->mp_nchunks; i++) {
        mc = (fh_mchunk_t *) (arena + i * chunk_size);

        mc->mc_next = mp->mp_mchunks;
        mp->mp_mchunks = mc;

        mp->mp_free++;
    }

    FH_LOG(CSI, INFO, ("MPOOL_GROW: pool '%s' nblocks %d nchunks %d free %d inuse %d",
         mp->mp_name, mp->mp_nblocks, mp->mp_nchunks, mp->mp_free, mp->mp_inuse));

    return FH_OK;
}

/*
 * fh_mpool_new
 *
 * Allocate and initialize a new memory pool of element of size 'chunk_size'.
 * The memory pool can be created to be lock-protected, growable, and to clear
 * memory when chunks are returned to pool.
 */
fh_mpool_t *fh_mpool_new(char *name, int chunk_size, int nchunks, int flags)
{
    fh_mpool_t *mp = (fh_mpool_t *) malloc(sizeof(fh_mpool_t));
    if (!mp) {
        FH_LOG(CSI, ERR, ("Couldn't allocate memory for the pool"));
        return NULL;
    }

    memset(mp, 0, sizeof(fh_mpool_t));

    mp->mp_chunksize = chunk_size;
    mp->mp_nchunks   = nchunks;
    mp->mp_flags     = flags;
    mp->mp_mblocks   = NULL;
    mp->mp_mchunks   = NULL;

    if (mp->mp_flags & FH_MPOOL_FL_LOCK) {
        pthread_mutex_init(&mp->mp_lock, NULL);
    }

    mp->mp_name = strdup(name);
    if (!mp->mp_name) {
        FH_LOG(CSI, ERR, ("Couldn't allocate memory for pool name: %s", name));
        fh_mpool_free(mp);
        return NULL;
    }

    mp->mp_inuse = 0;
    mp->mp_free  = 0;

    if (mpool_grow(mp) != FH_OK) {
        FH_LOG(CSI, ERR, ("Couldn't allocate memory chunks"));
        fh_mpool_free(mp);
        return NULL;
    }

    return mp;
}

/*
 * fh_mpool_free
 *
 * Free the all pool resources.
 */
void fh_mpool_free(fh_mpool_t *mp)
{
    fh_mblock_t *mb = NULL;

    FH_ASSERT(mp);

    if (mp->mp_flags & FH_MPOOL_FL_LOCK) {
        pthread_mutex_lock(&mp->mp_lock);
    }

    mb = mp->mp_mblocks;

    while (mb != NULL) {
        fh_mblock_t *nextmb = mb->mb_next;
        free(mb);
        mb = nextmb;
    }

    if (mp->mp_flags & FH_MPOOL_FL_LOCK) {
        pthread_mutex_unlock(&mp->mp_lock);
        pthread_mutex_destroy(&mp->mp_lock);
    }

    if (mp->mp_name) {
        free(mp->mp_name);
    }

    FH_LOG(CSI, INFO, ("MPOOL_FREE: pool='%s': free pool: free %d, inuse %d",
        mp->mp_name, mp->mp_free, mp->mp_inuse));

    free(mp);
}

/*
 * fh_mpool_get
 *
 * Get an element from the memory pool.
 */
void *fh_mpool_get(fh_mpool_t *mp)
{
    fh_mchunk_t *mc;
    void *e = NULL;

    FH_ASSERT(mp);

    if (mp->mp_flags & FH_MPOOL_FL_LOCK) {
        pthread_mutex_lock(&mp->mp_lock);
    }

    if (mp->mp_free == 0) {
        if (!(mp->mp_flags & FH_MPOOL_FL_GROW)) {
            fh_mpool_stats(mp);

            FH_LOG(CSI, ERR, ("pool '%s' is not growable: %d free elments",
                mp->mp_name, mp->mp_free));

            goto end;
        }

        if (mpool_grow(mp) != FH_OK) {
            FH_LOG(CSI, ERR, ("ran out of chunks and couldn't grow the memory pool"));
            goto end;
        }
    }

    if (mp->mp_mchunks == NULL) {
        FH_LOG(CSI, ERR, ("MPOOL_GET: pool='%s': ran out of chunks: inuse %d free %d",
            mp->mp_name, mp->mp_inuse, mp->mp_free));
        FH_ASSERT(1);
    }

    mc = mp->mp_mchunks;
    mp->mp_mchunks = mc->mc_next;

    mp->mp_inuse++;
    mp->mp_free--;

    if (mp->mp_flags & FH_MPOOL_FL_LOCK) {
        pthread_mutex_unlock(&mp->mp_lock);
    }

    e = (char *)mc + sizeof(fh_mchunk_t);

    FH_LOG(CSI, INFO, ("MPOOL_GET: pool='%s': chunk=%p inuse %d free %d",
        mp->mp_name, e, mp->mp_inuse, mp->mp_free));

end:
    if (mp->mp_flags & FH_MPOOL_FL_LOCK) {
        pthread_mutex_unlock(&mp->mp_lock);
    }
    return e;
}

/*
 * fh_mpool_put
 *
 * Return an element to the memory pool.
 */
FH_STATUS fh_mpool_put(fh_mpool_t *mp, void *e)
{
    fh_mchunk_t *mc = NULL;

    FH_ASSERT(mp && e);

    mc = (fh_mchunk_t *) ((char *)e - sizeof(fh_mchunk_t));

    if (mp->mp_flags & FH_MPOOL_FL_LOCK) {
        pthread_mutex_lock(&mp->mp_lock);
    }

    if (mp->mp_flags & FH_MPOOL_FL_CLEAR) {
        memset(e, 0, mp->mp_chunksize);
    }

    mc->mc_next = mp->mp_mchunks;
    mp->mp_mchunks = mc;

    mp->mp_inuse--;
    mp->mp_free++;

    if (mp->mp_flags & FH_MPOOL_FL_LOCK) {
        pthread_mutex_unlock(&mp->mp_lock);
    }

    FH_LOG(CSI, INFO, ("MPOOL_PUT: pool='%s': chunk=%p inuse %d free %d",
        mp->mp_name, e, mp->mp_inuse, mp->mp_free));

    return FH_OK;
}

