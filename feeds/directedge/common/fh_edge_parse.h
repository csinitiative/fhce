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

#ifndef __FH_DIR_EDGE_PARSE_H__
#define __FH_DIR_EDGE_PARSE_H__

FH_STATUS fh_edge_parse_init(fh_shr_lh_proc_t *);
FH_STATUS fh_edge_parse_msg(char *, uint32_t, char,fh_shr_lh_conn_t *,
                                fh_shr_cfg_lh_line_t *, uint64_t *);
FH_STATUS fh_edge_alarm(char *,int, int);

#endif
