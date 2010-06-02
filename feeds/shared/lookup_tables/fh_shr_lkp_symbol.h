#ifndef __FH_SHR_LKP_SYMBOL_H__
#define __FH_SHR_LKP_SYMBOL_H__

/* common FH headers */
#include "fh_errors.h"

/* shared FH library headers */
#include "fh_shr_cfg_table.h"

/* some convenience typedefs */
typedef struct fh_shr_lkp_sym_key    fh_shr_lkp_sym_key_t;
typedef struct fh_shr_lkp_sym        fh_shr_lkp_sym_t;

/**
 *  @brief Structure of a symbol table key
 */
struct fh_shr_lkp_sym_key {
    char                     symbol[20];    /**< the symbol belonging to this entry */
};

/**
 *  @brief Structure that stores one symbol entry in the symbol table
 */
struct fh_shr_lkp_sym {
    fh_shr_lkp_sym_key_t     key;
    char                     symbol[20];    /**< a copy of the symbol that this entry represents */
    void                    *context;       /**< pointer where a plugin can store its context */
};

/**
 *  @brief Initialize a symbol table
 *
 *  @param config the table configuration being used for initialization
 *  @param table the table being initialized
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_lkp_sym_init(fh_shr_cfg_tbl_t *config, fh_shr_lkp_tbl_t *table);

/**
 *  @brief Add or retrieve a symbol entry to the symbol table
 *
 *  @param table the table that we are lookup up the symbol in
 *  @param key the key that we are looking up or adding
 *  @param location where new/existing symbol table entry will be stored
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_lkp_sym_get(fh_shr_lkp_tbl_t *table, fh_shr_lkp_sym_key_t *key,
                             fh_shr_lkp_sym_t **entry);

#endif /* __FH_SHR_LKP_SYMBOL_H__ */
