#ifndef __FH_SHR_LKP_ORDER_H__
#define __FH_SHR_LKP_ORDER_H__

/* common FH headers */
#include "fh_errors.h"

/* shared FH library headers */
#include "fh_shr_cfg_table.h"
#include "fh_shr_lkp_symbol.h"

/* some convenience typedefs */
typedef struct fh_shr_lkp_ord_key    fh_shr_lkp_ord_key_t;
typedef struct fh_shr_lkp_ord        fh_shr_lkp_ord_t;

/**
 *  @brief Structure of an order table key
 */
struct fh_shr_lkp_ord_key {
    uint64_t                 order_no;          /**< unique order reference number */
    char             order_no_str[20];          /**< unique order no in AlplaNumeric*/
};

/**
 *  @brief Structure that stores one order entry in the order table
 */
struct fh_shr_lkp_ord {
    fh_shr_lkp_ord_key_t     key;               /**< order table key for this entry */
    uint64_t                 order_no;          /**< unique order reference number */
    char                     order_no_str[20];  /**< unique AlphaNumeric order ref number */
    uint64_t                 price;             /**< price (in ISE price format) */
    uint32_t                 shares;            /**< number of shares in the order */
    char                     buy_sell_ind;      /**< buy/sell indicator */
    char                     stock[6];          /**< stock symbol */
    fh_shr_lkp_sym_t        *sym_entry;         /**< entry in the symbol table for this symbol */
    void                    *context;           /**< pointer for a plugin to store context */
};

/**
 *  @brief Dump an order table entry (allowing it to be more easily visualized)
 *
 *  @param entry the entry being dumped
 */
void fh_shr_lkp_ord_dump(fh_shr_lkp_ord_t *entry);

/**
 *  @brief Initialize an order table
 *
 *  @param config the table configuration being used for initialization
 *  @param table the table being initialized
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_lkp_ord_init(fh_shr_cfg_tbl_t *config, fh_shr_lkp_tbl_t *table);

/**
 *  @brief Retrieve an entry from the order table (if that entry exists)
 *
 *  @param table the table that we are lookup up the order in
 *  @param key the key that we are looking up
 *  @param location where order table entry will be stored
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_lkp_ord_get(fh_shr_lkp_tbl_t *table, fh_shr_lkp_ord_key_t *key,
                             fh_shr_lkp_ord_t **entry);

/**
 * @brief Add an order to the order table
 *
 * @param table the table that the order is to be added to
 * @param entry the entry being added
 * @return status code indicating success or failure
 */
FH_STATUS fh_shr_lkp_ord_add(fh_shr_lkp_tbl_t *table, fh_shr_lkp_ord_t *entry,
                             fh_shr_lkp_ord_t **tblentry);

/**
 * @brief Delete an order from the order table
 *
 * @param table the table the order is being deleted from
 * @param entry the key of the order being deleted
 * @return status code indicating success or failure
 */
FH_STATUS fh_shr_lkp_ord_del(fh_shr_lkp_tbl_t *table, fh_shr_lkp_ord_key_t *key,
                             fh_shr_lkp_ord_t **entry);

#endif /* __FH_SHR_LKP_ORDER_H__ */
