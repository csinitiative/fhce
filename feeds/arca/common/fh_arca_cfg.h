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

#ifndef __FH_ARCA_CFG_H__
#define __FH_ARCA_CFG_H__

/*********************************************************************/
/* file: fh_arca_cfg.h                                               */
/* Usage: configuration support for arca feed handler                */
/* Author: Wally Matthews & Ross Cooperman of                        */
/*   Collaborative Software Initiative                               */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <arpa/inet.h>

// FH common headers
#include "fh_errors.h"
#include "fh_config.h"

// information about a single Arca connection
typedef struct {
    struct in_addr    address;
    uint16_t          port;
    char              interface[16];
    int               enabled;
} fh_arca_cfg_connection_t;

// information about a single Arca line
typedef struct {
    char                         name[MAX_PROPERTY_LENGTH];
    fh_arca_cfg_connection_t     primary;
    fh_arca_cfg_connection_t     secondary;
    int                          fast;
    int                          strict_ordering;
    uint32_t                     maximum_sessions;
    uint32_t                     maximum_symbols;
    uint32_t                     maximum_firms;
    uint32_t                     maximum_orders;
} fh_arca_cfg_line_t;

// information about a single Arca process
typedef struct {
    char                  name[MAX_PROPERTY_LENGTH];
    fh_arca_cfg_line_t   *lines;
    uint16_t              num_lines;
    long                  cpu;
    long                  index;
    long                  max_sessions;
    long                  max_symbols;
    long                  max_firms;
    long                  max_orders;
} fh_arca_cfg_process_t;

// exported global process configuration
extern fh_arca_cfg_process_t fh_arca_cfg;

// Arca configuration API
FH_STATUS fh_arca_cfg_load(const fh_cfg_node_t *config, const char *process);
void      fh_arca_cfg_dump();

#endif /* __FH_OPRA_CFG_H__ */
