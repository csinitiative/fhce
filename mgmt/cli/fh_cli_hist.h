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

#ifndef __FH_CLI_HIST_H__
#define __FH_CLI_HIST_H__

void  fh_cli_hist_load(char *name);
void  fh_cli_hist_save();
void  fh_cli_hist_show(int n);
char *fh_cli_hist_get(int cmd_idx);
char *fh_cli_hist_call(int cmd_idx);
void  fh_cli_hist_add(char *cmd);
int   fh_cli_hist_size();
int   fh_cli_hist_find(char *cmd_start);
char *fh_cli_hist_match(char *match, int *itr);

#endif /* __FH_CLI_HIST_H__ */
