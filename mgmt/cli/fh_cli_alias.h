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

#ifndef __FH_CLI_ALIAS_H__
#define __FH_CLI_ALIAS_H__

int  fh_cli_alias_size();
void fh_cli_alias_load();
void fh_cli_alias_save();
void fh_cli_alias_show();
int  fh_cli_alias_add(char *name, char *cmd);
int  fh_cli_alias_del(char *name);

#endif /* __FH_CLI_ALIAS_H__ */
