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

#ifndef __FH_MPOOL_H__
#define __FH_MPOOL_H__

#include <stdint.h>
#include <pthread.h>
#include "fh_errors.h"

/*
 * Memory pool flags
 */
#define FH_MPOOL_FL_GROW    (0x0000001)     /* Growable memory pool         */
#define FH_MPOOL_FL_LOCK    (0x0000002)     /* Lock-protected pool          */
#define FH_MPOOL_FL_CLEAR   (0x0000004)     /* Clear memory on free         */

/*
 * Memory block
 */
typedef struct _mp_mblock {
    struct _mp_mblock *mb_next;
} fh_mblock_t;

/*
 * Memory chunk
 */
typedef struct _mp_mchunk {
    struct _mp_mchunk *mc_next;
} fh_mchunk_t;


/*
 * Memory pool structure
 */
typedef struct {
    char             *mp_name;              /* Memory Pool name             */
    fh_mblock_t      *mp_mblocks;           /* List of memory blocks        */
    fh_mchunk_t      *mp_mchunks;           /* List of memory chunks        */
    uint32_t          mp_chunksize;         /* Chunck size                  */
    uint32_t          mp_nchunks;           /* Total number of chunks       */
    uint32_t          mp_nblocks;           /* Total number of blocks       */
    uint32_t          mp_flags;             /* Pool flags                   */
    uint32_t          mp_free;              /* Number of free chunks        */
    uint32_t          mp_inuse;             /* Number of inuse chunks       */
    pthread_mutex_t   mp_lock;              /* Pool lock                    */
} fh_mpool_t;

/*
 * Memory pool API
 */
fh_mpool_t *fh_mpool_new(char *name, int chunk_size, int nchunks, int flags);
void        fh_mpool_free(fh_mpool_t *mp);
void       *fh_mpool_get(fh_mpool_t *mp);
FH_STATUS   fh_mpool_put(fh_mpool_t *mp, void *e);
uint32_t    fh_mpool_memuse(fh_mpool_t *mp);
void        fh_mpool_stats(fh_mpool_t *mp);

#endif /* __FH_MPOOL_H__ */
