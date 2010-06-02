#ifndef __FH_SHR_LOOKUP_H__
#define __FH_SHR_LOOKUP_H__

/* system headers */
#include <stdint.h>

/* common FH headers */
#include "fh_mpool.h"
#include "fh_htable.h"

/* convenience typedef(s) */
typedef struct fh_shr_lkp_tbl fh_shr_lkp_tbl_t;

/**
 *  @brief Structure that stores table information
 */
struct fh_shr_lkp_tbl {
    fh_mpool_t  *mempool;           /**< memory pool from which new hash table entries come */
    fh_ht_t     *hash;              /**< hash table structure */
    uint32_t     size;              /**< max size (in entries) of this table */
    uint32_t     count;             /**< number of entries in this table */
};

#endif /* __FH_SHR_LOOKUP_H__ */
