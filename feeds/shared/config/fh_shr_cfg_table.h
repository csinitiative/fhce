/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __FH_SHR_CFG_TABLE_H__
#define __FH_SHR_CFG_TABLE_H__

/* common FH headers */
#include "fh_config.h"

/* some convenience typedefs */
typedef struct fh_shr_cfg_tbl fh_shr_cfg_tbl_t;

/**
 *  @brief Structure that stores configuration elements for lookup tables
 */
struct fh_shr_cfg_tbl {
    char        name[MAX_PROPERTY_LENGTH];      /**< the name of the table */
    uint8_t     enabled;                        /**< whether the table is enabled */
    uint32_t    size;                           /**< the size of the table (in elements) */
};

/**
 *  @brief Load a table configuration for the table with the given name
 *
 *  @param cfg raw configuration structure from which to load the table configuration
 *  @param name the name of the table configuration to load
 *  @param tbl pointer to the table configuration structure being populated
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_cfg_tbl_load(const fh_cfg_node_t *cfg, const char *name, fh_shr_cfg_tbl_t *tbl);

#endif /* __FH_SHR_CFG_TABLE_H__ */
