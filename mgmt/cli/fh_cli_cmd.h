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

#ifndef __FH_CLI_CMD_H__
#define __FH_CLI_CMD_H__

#include <stdint.h>
#include "queue.h"

/* Command Modes */
#define FH_CLI_CMD_MODE_BASIC      (1)

#define FH_CLI_CMD_MODE_DEFAULT    FH_CLI_CMD_MODE_BASIC

#define FH_CLI_CMD_MODE_NAME(m) (               \
   (m) == FH_CLI_CMD_MODE_BASIC  ? "basic"  :   \
   "MODE_INVALID")

/* Specification components to define an admin message      */
#define FH_CLI_CMD_DEF_MODE       (1)
#define FH_CLI_CMD_DEF_NAME       (2)
#define FH_CLI_CMD_DEF_CB         (3)
#define FH_CLI_CMD_DEF_HELP       (4)
#define FH_CLI_CMD_DEF_CHILDREN   (5)
#define FH_CLI_CMD_DEF_REG        (6)

/* Specification for one admin message type                 */

#define FH_CLI_CMD_DEF(name, mode, cb, help, children) \
  (void *) FH_CLI_CMD_DEF_NAME,     (void *) name,     \
  (void *) FH_CLI_CMD_DEF_MODE,     (void *) mode,     \
  (void *) FH_CLI_CMD_DEF_CB,       (void *) cb,       \
  (void *) FH_CLI_CMD_DEF_HELP,     (void *) help,     \
  (void *) FH_CLI_CMD_DEF_CHILDREN, (void *) children, \
  (void *) FH_CLI_CMD_DEF_REG

/* Macro equivalent to va_start and va_next stdarg API      */
#define FH_CLI_CMD_DEF_START()     int _defs_idx = 0
#define FH_CLI_CMD_DEF_NEXT(type)  (type) defs[_defs_idx++]


typedef int (fh_cli_cmd_cb_t)(char *full_cmd, char **argv, int argc);

struct fh_cli_cmd;

typedef TAILQ_HEAD(,fh_cli_cmd) fh_cli_cmd_lh_t;
typedef TAILQ_ENTRY(fh_cli_cmd) fh_cli_cmd_le_t;

typedef struct fh_cli_cmd {
    fh_cli_cmd_lh_t    c_children;
    fh_cli_cmd_le_t    c_next;
    struct fh_cli_cmd *c_parent;
    char              *c_name;
    uint16_t           c_uniq_len;
    uint16_t           c_len;
    int                c_mode;
    char              *c_help;
    fh_cli_cmd_cb_t   *c_callback;
    struct fh_cli_cmd *c_childs;
} fh_cli_cmd_t;

void          fh_cli_cmd_init();
fh_cli_cmd_t *fh_cli_cmd_search(fh_cli_cmd_t *parent, char *command);
fh_cli_cmd_t *fh_cli_cmd_register(fh_cli_cmd_t *parent, char *cmd_name, int mode,
                                  fh_cli_cmd_cb_t *cmd_cb, char *help);
int           fh_cli_cmd_unregister(fh_cli_cmd_t *c);
int           fh_cli_cmd_complete(char *cmd, int *cmd_len);
int           fh_cli_cmd_process(char *cmd);
void          fh_cli_cmd_set_mode(int new_mode);
void          fh_cli_cmd_dft_mode();

#endif /* __FH_CLI_CMD_H__ */
