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
 
/* system headers */
#include <stdlib.h>
#include <string.h>
 
/* common FH headers */
#include "fh_errors.h"
#include "fh_log.h"
 
/* shared component headers */
#include "fh_shr_cfg_table.h"

/* 
 * Load a table configuration for the table with the given name
 */
FH_STATUS fh_shr_cfg_tbl_load(const fh_cfg_node_t *cfg, const char *name, fh_shr_cfg_tbl_t *tbl)
{
    const fh_cfg_node_t   *tbl_node;
    
    /* initialize the table configuration */
    memset(tbl, 0, sizeof(fh_shr_cfg_tbl_t));
        
    /* fetch the table node for this table (if it exists) */
    if ((tbl_node = fh_cfg_get_node(cfg, name)) == NULL) {
        return FH_ERR_NOTFOUND;
    }
    
    /* copy just the node name into the table name */
    strcpy(tbl->name, tbl_node->name);
    
    /* determine the size of this table (in elements) */
    switch (fh_cfg_set_uint32(tbl_node, "size", &tbl->size)) {

    case FH_OK:
        break;
        
    case FH_ERR_NOTFOUND:
        FH_LOG(CSI, ERR, ("table %s: size property is required", name));
        return FH_ERR_NOTFOUND;
        
    default:
        FH_LOG(CSI, ERR, ("table %s: size property is invalid (must be an integer)", name));
        return FH_ERROR;
    }
    
    /* if we get to this point, success */
    tbl->enabled = 1;
    return FH_OK;
}
