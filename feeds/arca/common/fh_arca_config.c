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

//
/*********************************************************************/
/* file: fh_arca_config.c                                            */
/* Usage: configuration for arca feed handler                        */
/* Author: Wally Matthews & Ross Cooperman of                        */
/*   Collaborative Software Initiative                               */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <stdlib.h>
#include <string.h>

// Common FH headers
#include "fh_log.h"
#include "fh_config.h"
#include "fh_net.h"
#include "fh_cpu.h"

// Arca FH headers
#include "fh_arca_constants.h"
#include "fh_arca_headers.h"
#include "fh_feed_group.h"
#include "fh_arca_cfg.h"

// static data
static char source_id[SOURCE_ID_LENGTH];

/*--------------------------------------------------------------------------*/
/* load list of line names from new config structure                        */
/*--------------------------------------------------------------------------*/
static FH_STATUS get_lines(struct process_maps *p_map)
{
    int i;
    
    for (i = 0; i < fh_arca_cfg.num_lines; i++) {
        strcpy(&p_map->line_names[i][0], fh_arca_cfg.lines[i].name);
    }
    p_map->line_count = fh_arca_cfg.num_lines;
    return FH_OK;
}

/*--------------------------------------------------------------------------*/
/* load process config from new config structure                            */
/*--------------------------------------------------------------------------*/
static FH_STATUS get_process_config(struct process_maps *p_map)
{
    get_lines(p_map);

    p_map->feed_cpu     = fh_arca_cfg.cpu;
    p_map->refresh_cpu  = fh_arca_cfg.cpu;
    p_map->retrans_cpu  = fh_arca_cfg.cpu;
    p_map->max_symbols  = fh_arca_cfg.max_symbols;
    p_map->max_sessions = fh_arca_cfg.max_sessions;
    p_map->max_firms    = fh_arca_cfg.max_firms;
    p_map->max_orders   = fh_arca_cfg.max_orders;

    return FH_OK;
}

/*--------------------------------------------------------------------------*/
/* load group configuration from new config structure                       */
/*--------------------------------------------------------------------------*/
static FH_STATUS get_line_config(const int line_index, struct feed_group *group)
{
    fh_arca_cfg_line_t   *line_cfg = &fh_arca_cfg.lines[line_index];
    
    strcpy(&group->primary_mcast_ip_addrs[0], inet_ntoa(line_cfg->primary.address));
    strcpy(&group->primary_mcast_intfc[0], line_cfg->primary.interface);
    if (line_cfg->primary.enabled) {
        group->primary_mcast_port = line_cfg->primary.port;
    }
    else {
        group->primary_mcast_port = 0;
    }

    strcpy(&group->secondary_mcast_ip_addrs[0], inet_ntoa(line_cfg->secondary.address));
    strcpy(&group->secondary_mcast_intfc[0], line_cfg->secondary.interface);
    if (line_cfg->secondary.enabled) {
        group->secondary_mcast_port = line_cfg->secondary.port;
    }
    else {
        group->secondary_mcast_port = 0;
    }
    
    group->fast_mode       = line_cfg->fast;
    
    strcpy(&group->feed_name[0], line_cfg->name);
    strcpy(&group->process_name[0], fh_arca_cfg.name);
    // create notification index
    group->notification_line_id = ((line_index & 0x03) << 30);    

    // make config parameters available for being over-written by customer configs
    group->maximum_sessions = line_cfg->maximum_sessions;
    group->maximum_symbols  = line_cfg->maximum_symbols;
    group->maximum_firms    = line_cfg->maximum_firms;
    group->maximum_orders   = line_cfg->maximum_orders;

    return FH_OK;
}

/*--------------------------------------------------------------------------*/
/*  extract a source id from a unified configuration file                   */
/*--------------------------------------------------------------------------*/
static FH_STATUS get_source_id(const fh_cfg_node_t *config)
{
    const char *src_id = fh_cfg_get_string(config, "arca.source_id");
    
    if (src_id == NULL) {
        FH_LOG(MGMT, ERR, ("config failed to find source id"));
        return FH_ERROR;
    }
    memcpy(&source_id, src_id, strlen(src_id)+1);
    return FH_OK;
}

/*--------------------------------------------------------------------------*/
/*  load process configuration from new config structure                    */
/*--------------------------------------------------------------------------*/
FH_STATUS get_config(const fh_cfg_node_t *config, struct process_maps *p_map)
{
    struct feed_group   *group;
    int                  i   = 0;
    
    if (get_process_config(p_map) != FH_OK) {
        return FH_ERROR;
    }
    if (get_source_id(config) != FH_OK) {
        return FH_ERROR;
    }
    for (i = 0; i < p_map->line_count; i++) {
        group = new_feed_group();
        FH_ASSERT(group != NULL);
        default_config(group);
        strcpy(&group->source_id[0], source_id);
        if (get_line_config(i, group) != FH_OK) {
            return FH_ERROR;
        }
        p_map->groups[i] = group;
    }

    return FH_OK;
}
