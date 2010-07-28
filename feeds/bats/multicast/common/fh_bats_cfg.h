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

#ifndef __FH_BATS_CFG_H__
#define __FH_BATS_CFG_H__

/* Get the "processes" node from the config structure  */
fh_cfg_node_t * fh_bats_get_process_node(fh_cfg_node_t *config);

/* Get the specified process node from the config structure provided */
fh_cfg_node_t * fh_bats_get_one_process_node(fh_cfg_node_t *config,const char *process);

/* Get the "lines" node from the config structure  */
fh_cfg_node_t* fh_bats_get_lines_node(fh_cfg_node_t * config);

/* Get the fill gaps max key from the config structure   */
const char *  fh_bats_get_gapmax_str();

/* Get the "fill gaps timeout" search key  */
const char* fh_bats_get_gap_timeout_str();

/* Get the symbol table search Key for bats  */
const char* fh_bats_get_symbol_str();


/* Get the order table search Key   */
const char* fh_bats_get_order_str();

#endif
