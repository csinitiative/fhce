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

/* system headers */
#include <stdint.h>
#include <string.h>
#include <errno.h>

/* common FH headers */
#include "fh_log.h"
#include "fh_mpool.h"

/* shared FH component headers */
#include "fh_shr_gap_fill.h"

/*
 *  Allocate a new gap list structure of the specified size and return a pointer to it
 */
fh_shr_gap_fill_list_t *fh_shr_gap_fill_new(uint32_t max, uint32_t timeout)
{
    fh_shr_gap_fill_list_t *list;

    /* allocate memory for the list structure (return NULL if ) */
    list = (fh_shr_gap_fill_list_t *)malloc(sizeof(fh_shr_gap_fill_list_t));
    if (list == NULL) {
        FH_LOG(CSI, ERR, ("failed to allocate memory for gap fill list (%d)", errno));
        return NULL;
    }

    /* initialize the newly allocated memory */
    memset(list, 0, sizeof(fh_shr_gap_fill_list_t));
    list->max = max;
    list->timeout = timeout;

    /* allocate a memory pool from which entries will come */
    list->data = fh_mpool_new("gap_list", sizeof(fh_shr_gap_fill_node_t), list->max, 0);

    /* return the initialized list */
    return list;
}
