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

/* System headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

/* FH common headers */
#include "fh_errors.h"
#include "fh_config.h"
#include "fh_log.h"
#include "fh_plugin_internal.h"

/* FH shared config headers */
#include "fh_shr_cfg_lh.h"
#include "fh_shr_lh.h"


/*
 *  The following are callbacks that allow the line handler code to be generic
 *  and hides the specific details of the feeds within the feed specific code
 *  base.
 */


/* Get the "processes" node from the config structure  */
const fh_cfg_node_t * fh_bats_get_process_node(fh_cfg_node_t *config)
{
    return (fh_cfg_get_node(config, "bats.processes"));
}


/* Get the specified process node from the config structure provided */
const fh_cfg_node_t * fh_bats_get_one_process_node(fh_cfg_node_t *config, const char *process)
{
    char               process_node_name[MAX_PROPERTY_LENGTH];
    sprintf(process_node_name, "bats.processes.%s", process);
    return (fh_cfg_get_node(config, process_node_name));

}

/* Get the "lines" node from the config structure  */
const fh_cfg_node_t* fh_bats_get_lines_node(fh_cfg_node_t * config)
{
    return (fh_cfg_get_node(config,"bats.lines"));
}

/* Get the fill gaps max key from the config structure   */
const char *  fh_bats_get_gapmax_str()
{
    static char * gapmax_str = "bats.fill_gaps.max";
    return gapmax_str;
}


/* Get the "fill gaps timeout" search key  */
const char* fh_bats_get_gap_timeout_str()
{
    static char * gap_timeout_str = "bats.fill_gaps.timeout";
    return gap_timeout_str;
}


/* Get the symbol table search Key for bats  */
const char* fh_bats_get_symbol_str()
{
    static char * symbol_table_str = "bats.symbol_table";
    return symbol_table_str;
}

/* Get the order table search Key   */
const char* fh_bats_get_order_str()
{
    static char * order_table_str = "bats.order_table";
    return order_table_str;
}
