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

#ifndef __FH_CONIFG_H__
#define __FH_CONIFG_H__

#include "fh_errors.h"

// maximum length of a property string
#define MAX_PROPERTY_LENGTH 255
#define MAX_PROPERTY_DEPTH  255


/**
 * Configuration node
 *
 * One node of configuration.  A node can have any number of child nodes or values but cannot
 * have both children and values.
 */
typedef struct fh_cfg_node {
    char                  name[MAX_PROPERTY_LENGTH];
    struct fh_cfg_node  **children;
    struct fh_cfg_node   *parent;
    char                **values;
    int                   num_children;
    int                   num_values;
} fh_cfg_node_t;


/*
 * Function definitions for YACC/LEX functions
 */
int yyparse();


/*
 * Configuration API
 */
fh_cfg_node_t        *fh_cfg_load(const char *);
void                  fh_cfg_dump(fh_cfg_node_t *);
void                  fh_cfg_free(fh_cfg_node_t *);
const fh_cfg_node_t  *fh_cfg_get_node(const fh_cfg_node_t *, const char *);
const char           *fh_cfg_get_string(const fh_cfg_node_t *, const char *);
const char          **fh_cfg_get_array(const fh_cfg_node_t *, const char *);

/**
 *  @brief Set a yes/no field value
 *
 *  @param config configuration node where the property resides
 *  @param property the property we are looking for
 *  @param value where we are storing the result
 *  @return status code indicating success or failure
 */
FH_STATUS fh_cfg_set_yesno(const fh_cfg_node_t *config, const char *property, uint8_t *value);

/**
 *  @brief Set a uint32_t field value
 *
 *  @param config configuration node where the property resides
 *  @param property the property we are looking for
 *  @param value where we are storing the result
 *  @return status code indicating success or failure
 */
FH_STATUS fh_cfg_set_uint32(const fh_cfg_node_t *config, const char *property, uint32_t *value);

/**
 *  @brief Set a uint16_t field value
 *
 *  @param config configuration node where the property resides
 *  @param property the property we are looking for
 *  @param value where we are storing the result
 *  @return status code indicating success or failure
 */
FH_STATUS fh_cfg_set_uint16(const fh_cfg_node_t *config, const char *property, uint16_t *value);

/**
 *  @brief Set an int field value
 *
 *  @param config configuration node where the property resides
 *  @param property the property we are looking for
 *  @param value where we are storing the result
 */
FH_STATUS fh_cfg_set_int(const fh_cfg_node_t *config, const char *property, int *value);

#endif /* __FH_CONIFG_H__ */
