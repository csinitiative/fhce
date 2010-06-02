#ifndef __FH_SHR_LKP_ORDER_H__
#define __FH_SHR_LKP_ORDER_H__

/* system headers */
#include <stdint.h>

/* common FH headers */
#include "fh_errors.h"

/* shared FH library headers */
#include "fh_shr_cfg_table.h"

/* some convenience typedefs */
typedef struct fh_shr_lkp_ord        fh_shr_lkp_ord_t;
typedef struct fh_shr_lkp_ord_key    fh_shr_lkp_ord_key_t;

/**
 *  @brief Structure that stores one order entry in the order table
 */
struct fh_shr_lkp_ord {
    uint64_t     order_no;      /**< the unique order number of this order */
    void        *context;       /**< pointer where a plugin can store its context */
};

/**
 *  @brief Structure of an order table key
 */
struct fh_shr_lkp_ord_key {
    uint64_t     order_no;      /**< the unique order number of this order */
};

/**
 *  @brief Initialize an order table
 *
 *  @param config the table configuration being used for initialization
 *  @param table the table being initialized
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_lkp_ord_init(fh_shr_cfg_tbl_t *config, fh_shr_lkp_tbl_t *table);

#endif /* __FH_SHR_LKP_ORDER_H__ */
