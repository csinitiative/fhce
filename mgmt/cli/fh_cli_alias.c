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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "fh_cli.h"
#include "fh_cli_cmd.h"
#include "fh_cli_alias.h"
#include "fh_log.h"

/* ------------------------------------------------------------------- */
/* --- Command Alias API --------------------------------------------- */
/* ------------------------------------------------------------------- */

typedef struct {
    char      a_name[64];
    char      a_cmd[64];
    int       a_index;
    fh_cli_cmd_t *a_cmd_ref;
} alias_t;

#define ALIAS_CNT        (100)

static alias_t alias[ALIAS_CNT];
static char    alias_file[128];
static int     alias_cnt = 0;

/*
 * alias_find
 *
 * Find an alias based on alias name.
 */
static alias_t *alias_find(char *name)
{
    int i;
    for (i=0; i<alias_cnt; i++) {
        alias_t *a = &alias[i];
        if (strcmp(name, a->a_name) == 0) {
            return a;
        }
    }
    return NULL;
}

/*
 * alias_exec
 *
 * Execute an alias.
 */
static int alias_exec(char *full_cmd, char **argv, int argc)
{
    alias_t *a = alias_find(full_cmd);

    argv = NULL;
    argc = 0;

    assert(a);

    return fh_cli_cmd_process(a->a_cmd);
}

/*
 * fh_cli_alias_size
 *
 * Returns the number of entries in the aliases stack.
 */
int fh_cli_alias_size()
{
    return alias_cnt;
}

/*
 * fh_cli_alias_load
 *
 * Load the command aliases. History is managed as a ring of N commands.
 */
void fh_cli_alias_load(char *name)
{
    struct stat sb;

    char * home = getenv("HOME");
    if (!home) {
        return;
    }

    sprintf(alias_file, "%s/.%s_aliases", home, name);

    memset(alias, 0, sizeof(alias));

    if (stat(alias_file, &sb) == 0) {
        char line[1024];

        FILE *fp = fopen(alias_file, "r");

        while (alias_cnt < ALIAS_CNT && fgets(line, sizeof(line), fp) != NULL) {
            unsigned int i;
            char *name, *cmd = NULL;

            line[strlen(line)-1] = 0;

            name = line;

            for (i=0; i<strlen(line); i++) {
                if (line[i] == ' ') {
                    line[i] = 0;
                    cmd = &line[i+1];
                    break;
                }
            }

            if (cmd == NULL) {
                break;
            }

            fh_cli_alias_add(name, cmd);
        }

        fclose(fp);
    }
}

/*
 * fh_cli_alias_save
 *
 * Save aliases to file. Most recent is at the bottom.
 */
void fh_cli_alias_save()
{
    int i;

    FILE *fp = fopen(alias_file, "w+");

    if (fp == NULL) {
        perror("fopen failed");
        exit(1);
    }

    for (i=0; i<alias_cnt; i++) {
        alias_t *a = &alias[i];
        fprintf(fp, "%s %s\n", a->a_name, a->a_cmd);
    }

    fclose(fp);
}

/*
 * fh_cli_alias_show
 *
 * Show aliases.
 */
void fh_cli_alias_show()
{
    int i;

    if (alias_cnt == 0) {
        return;
    }

    fh_cli_write("-------------------------------------\n");
    fh_cli_write("%-10s %s\n", "Alias", "Command");
    fh_cli_write("---------- --------------------------\n");

    for (i=0; i<alias_cnt; i++) {
        alias_t *a = &alias[i];
        fh_cli_write("%-10s %s\n", a->a_name, a->a_cmd);
    }
}

/*
 * fh_cli_alias_add
 *
 * Add a command to the aliases.
 */
int fh_cli_alias_add(char *name, char *cmd)
{
    alias_t *a = alias_find(name);
    if (a) {
        fh_cli_write("Alias already exist: %s\n", name);
        return -1;
    }

    if (alias_cnt < ALIAS_CNT) {
        fh_cli_cmd_t *c;

        a = &alias[alias_cnt];

        a->a_index = alias_cnt;

        strcpy(a->a_name, name);
        strcat(a->a_cmd, cmd);

        c = fh_cli_cmd_register(NULL, a->a_name, FH_CLI_CMD_MODE_DEFAULT,
                                alias_exec, a->a_cmd);

        a->a_cmd_ref = c;

        alias_cnt++;
    }
    else {
        fh_cli_write("Reached maximum number of aliases: %d\n", alias_cnt);
        return -1;
    }

    return 0;
}

/*
 * fh_cli_alias_del
 *
 * Delete an alias.
 */
int fh_cli_alias_del(char *name)
{
    alias_t *a;
    int i;

    a = alias_find(name);
    if (!a) {
        fh_cli_write("Alias not found: %s\n", name);
        return -1;
    }

    fh_cli_cmd_unregister(a->a_cmd_ref);

    for (i=a->a_index; i<alias_cnt; i++) {
        memcpy(a, a+1, sizeof(alias_t));
    }
    alias_cnt--;

    return 0;
}

