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

#ifndef __FH_CLI_H__
#define __FH_CLI_H__

void fh_cli_init(char *name);
void fh_cli_exit();
void fh_cli_help();
void fh_cli_quit();
void fh_cli_tree(void **cmd_tree);

void fh_cli_loop();
void fh_cli_print(char *cmd, char *help);

void fh_cli_reset(char *cmd, int cmd_len, int pos);
void fh_cli_lmove(int *pos, int n);
void fh_cli_rmove(char *cmd, int cmd_len, int *pos, int n);
void fh_cli_kill(char *cmd, int *cmd_len, int *pos);
void fh_cli_insert(char *cmd, int *cmd_len, int *pos, int c);
void fh_cli_erase(char *cmd, int *cmd_len, int *pos);

void fh_cli_hist_up(char *cmd, int *cmd_len, int *pos);
void fh_cli_hist_down(char *cmd, int *cmd_len, int *pos);

void fh_cli_set_prompt_char(char c);
void fh_cli_set_prompt(char *prompt);
void fh_cli_save_prompt();
void fh_cli_restore_prompt();
void fh_cli_set_mode(char *mode_str);

void fh_cli_term_config();
void fh_cli_term_restore();
void fh_cli_beep();
void fh_cli_write(char *fmt, ...);

#endif /* __FH_CLI_H__ */
