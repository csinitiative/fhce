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

#ifndef __FH_HTABLE_H__
#define __FH_HTABLE_H__

#include <stdint.h>
#include "missing/jhash.h"
#include "missing/queue.h"
#include "fh_errors.h"
#include "fh_mpool.h"

/*
 * H-Table Configuration flags
 */
#define FH_HT_FL_GROW    (0x00000001)   /* Growable H-Table             */

/*
 * H-Table Key manipulation functions
 */
typedef uint32_t (fh_ht_khash_t) (void *key, int klen);
typedef char *   (fh_ht_kdump_t) (void *key, int klen);
typedef int      (fh_ht_kcmp_t)  (void *key_a, void *key_b, int klen);

/*
 * H-Table element structure
 */
struct _fh_ht_elt;

typedef TAILQ_ENTRY(_fh_ht_elt) he_entry_t;
typedef TAILQ_HEAD(,_fh_ht_elt) he_head_t;

typedef struct _fh_ht_elt {
    he_entry_t   he_next;       /* Collision list                       */
    he_head_t   *he_chead;      /* Reference to the collision head      */
    void        *he_value;      /* Value pointer                        */
    void        *he_key;        /* Pointer to the hash key              */
    int          he_klen;       /* Key length                           */
    uint32_t     he_hashval;    /* Key hash value                       */
} fh_ht_elt_t;

/*
 * Hash key operation structure
 */
typedef struct {
    fh_ht_khash_t *kops_khash;  /* Pointer to key hash function         */
    fh_ht_kdump_t *kops_kdump;  /* Pointer to key dump function         */
    fh_ht_kcmp_t  *kops_kcmp;   /* Pointer to key compare function      */
} fh_ht_kops_t;

/*
 * Hash table structure
 */
typedef struct {
    fh_mpool_t     *ht_mpool;   /* Hash element memory pool             */
    he_head_t      *ht_table;   /* Collision head table                 */
    uint32_t        ht_size;    /* H-Table size                         */
    uint32_t        ht_count;   /* H-Table element count                */
    uint32_t        ht_mask;    /* H-Table mask to find collision head  */
    uint32_t        ht_flags;   /* H-Table configuration flags          */
    fh_ht_kops_t    ht_kops;    /* H-Table key operations               */
} fh_ht_t;

/*
 * H-Table API
 */
fh_ht_t *  fh_ht_new(int size, int flags, fh_ht_kops_t *kops);
void       fh_ht_free(fh_ht_t *ht);
FH_STATUS  fh_ht_put(fh_ht_t *ht, void *key, int klen, void *val);
FH_STATUS  fh_ht_get(fh_ht_t *ht, void *key, int klen, void **val);
FH_STATUS  fh_ht_delete(fh_ht_t *ht, void *key, int klen, void **val);
uint32_t   fh_ht_memuse(fh_ht_t *ht);

#endif /* __FH_HTABLE_H__ */
