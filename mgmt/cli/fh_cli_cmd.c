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
#include <ctype.h>

#include "fh_cli.h"
#include "fh_cli_cmd.h"
#include "fh_cli_hist.h"
#include "fh_cli_filter.h"

#define DEBUG (0)

#if DEBUG
# define dprint(x) printf x
#else
# define dprint(x)
#endif

/* ------------------------------------------------------------------- */
/* --- Command Processing Engine ------------------------------------- */
/* ------------------------------------------------------------------- */

static fh_cli_cmd_lh_t cmd_list;
static int             cmd_mode = FH_CLI_CMD_MODE_DEFAULT;

/*
 * cmd_build
 *
 * Set the unique length of a given command compared to the other
 * sibling commands. This allows to put abreviations on the command line.
 */
static int cmd_build(fh_cli_cmd_lh_t *cmd_lh)
{
    fh_cli_cmd_t *c;

    TAILQ_FOREACH(c, cmd_lh, c_next) {
        for (c->c_uniq_len = 1; c->c_uniq_len <= c->c_len; c->c_uniq_len++) {
            int foundmatch = 0;
            fh_cli_cmd_t *tmp_c;

            TAILQ_FOREACH(tmp_c, cmd_lh, c_next) {
                if (c == tmp_c) {
                    continue;
                }

                if (!strncmp(tmp_c->c_name, c->c_name, c->c_uniq_len)) {
                    foundmatch++;
                }
            }

            if (!foundmatch) {
                break;
            }
        }

        if (!TAILQ_EMPTY(&c->c_children)) {
            cmd_build(&c->c_children);
        }
    }

    return 0;
}

/*
 * cmd_register
 *
 * Register a CLI command to the CLI command tree.
 */
fh_cli_cmd_t *fh_cli_cmd_register(fh_cli_cmd_t *parent, char *cmd_name, int mode,
                                  fh_cli_cmd_cb_t *cmd_cb, char *help)
{
    fh_cli_cmd_lh_t *cmd_lh = NULL;
    fh_cli_cmd_t *c;

    if (!cmd_name) {
        return 0;
    }

    if ((c = malloc(sizeof(fh_cli_cmd_t))) == NULL) {
        return 0;
    }

    memset(c, 0, sizeof(fh_cli_cmd_t));

    c->c_callback = cmd_cb;
    c->c_mode     = mode;
    c->c_name     = strdup(cmd_name);
    c->c_len      = strlen(cmd_name);

    dprint((">>> Register: %s help:%s parent:%s\n", cmd_name,
            help ? : "na", parent ? parent->c_name : "na"));

    if (help) {
        c->c_help   = strdup(help);
    }

    TAILQ_INIT(&c->c_children);

    if (parent) {
        c->c_parent = parent;
        cmd_lh = &parent->c_children;
    }
    else {
        c->c_parent = NULL;
        cmd_lh = &cmd_list;
    }

    TAILQ_INSERT_HEAD(cmd_lh, c, c_next);

    cmd_build(cmd_lh);

    return c;
}

/*
 * fh_cli_cmd_unregister
 *
 * Unregisters a command from the cli tree.
 */
int fh_cli_cmd_unregister(fh_cli_cmd_t *c)
{
    fh_cli_cmd_lh_t *lh;
    fh_cli_cmd_t *child_c = NULL;

    if (c->c_parent) {
        lh = &c->c_parent->c_children;
    }
    else {
        lh = &cmd_list;
    }

    TAILQ_REMOVE(lh, c, c_next);

    while ((child_c = TAILQ_FIRST(&c->c_children)) != NULL) {
        fh_cli_cmd_unregister(child_c);
    }

    free(c->c_help);
    free(c->c_name);
    free(c);

    return 0;
}


/*
 * fh_cli_cmd_set_mode
 *
 * Change the global mode.
 */
void fh_cli_cmd_set_mode(int new_mode)
{
    if (cmd_mode == new_mode) {
        return;
    }

    cmd_mode = new_mode;
}

/*
 * fh_cli_cmd_dft_mode
 *
 * Revert back to default mode.
 */
void fh_cli_cmd_dft_mode()
{
    fh_cli_cmd_set_mode(FH_CLI_CMD_MODE_DEFAULT);
}

/*
 * fh_cli_cmd_init
 *
 * Initialize the command tree.
 */
void fh_cli_cmd_init()
{
    TAILQ_INIT(&cmd_list);
}

/*
 * fh_cli_cmd_search
 *
 * Find a specific command in the command tree as a doted-string, where each
 * stanza is a command level in the command tree.
 */
fh_cli_cmd_t *fh_cli_cmd_search(fh_cli_cmd_t *parent, char *command)
{
    fh_cli_cmd_t *c;
    fh_cli_cmd_lh_t *lh;
    char *ptr = strchr(command, '.');
    if (ptr) {
        *ptr = '\0';
        ptr++;
    }

    if (parent) {
        lh = &parent->c_children;
    }
    else {
        lh = &cmd_list;
    }

    TAILQ_FOREACH(c, lh, c_next) {
        if (strcmp(c->c_name, command) == 0) {
            if (ptr == NULL) {
                return c;
            }

            // Recursively go down the command tree
            return fh_cli_cmd_search(c, ptr);
        }
    }

    return NULL;
}

/*
 * fh_cli_cmd_fullname
 *
 * Returns the full name of the command.
 */
char *fh_cli_cmd_fullname(fh_cli_cmd_t *c)
{
    static char cmd_name[1024];
    char *ptr = cmd_name + sizeof(cmd_name);
    int first = 1;

    while (c) {
        ptr -= c->c_len+1;
        sprintf(ptr, "%s", c->c_name);

        if (first) {
            first = 0;
        }
        else {
            ptr[c->c_len] = ' ';
        }

        c = c->c_parent;
    }

    return ptr;
}

/*
 * fh_cli_cmd_print
 *
 * Print a given cmd.
 */
void fh_cli_cmd_print(fh_cli_cmd_t *c, int full)
{
    fh_cli_print(full ? fh_cli_cmd_fullname(c) : c->c_name,
                 c->c_help ? c->c_help : "<No help>");
}

/*
 * fh_cli_cmd_help
 *
 * Display a full help of the command.
 */
int fh_cli_cmd_help(fh_cli_cmd_lh_t *cmd_lh)
{
    fh_cli_cmd_t *c;

    TAILQ_FOREACH(c, cmd_lh, c_next) {
        if (c->c_name && c->c_mode <= cmd_mode) {
            fh_cli_cmd_print(c, 1);
        }
    }

    return 0;
}

/*
 * fh_cli_cmd_parse
 *
 * Parse a given command and return the components of the command.
 */
int fh_cli_cmd_parse(char *cmd, char *args[], int max_args)
{
    int nargs = 0;
    char *p = cmd;
    char *arg_start = cmd;
    int inquote = 0;

    while (nargs < max_args - 1) {
        if (!*p || *p == inquote ||
            (arg_start && !inquote && (isspace(*p) || *p == '|'))) {

            if (arg_start) {
                int len = p - arg_start;

                memcpy(args[nargs] = malloc(len + 1), arg_start, len);
                args[nargs++][len] = 0;
            }

            if (!*p) {
                break;
            }

            if (inquote) {
                p++; /* skip over trailing quote */
            }

            inquote = 0;
            arg_start = 0;
        }
        else if (*p == '"' || *p == '\'') {
            inquote = *p++;
            arg_start = p;
        }
        else {
            if (!arg_start) {
                if (*p == '|') {
                    args[nargs++] = strdup("|");
                }
                else if (!isspace(*p)) {
                    arg_start = p;
                }
            }

            p++;
        }
    }

    return nargs;
}

/*
 * fh_cli_cmd_find
 *
 * Find a command, and run the callback when provided.
 */
int fh_cli_cmd_find(fh_cli_cmd_lh_t *cmd_list, int num_args, char *args[],
                    int arg, int *filters, fh_cli_cmd_t **match_c)
{
    fh_cli_cmd_t *c;
    int       c_args = num_args;
    int       arg_len;
    char     *arg_p = args[arg];

    if (filters && filters[0]) {
        c_args = filters[0];
    }

    /* deal with ? for help */
    if (arg_p == NULL) {
        return -1;
    }

    arg_len = strlen(args[arg]);

    if (arg_p[arg_len-1] == '?') {
        int l = arg_len - 1;
        int match = 0;

        TAILQ_FOREACH(c, cmd_list, c_next) {
            if (strncasecmp(c->c_name, arg_p, l) == 0 && c->c_mode <= cmd_mode) {
                if (match_c == NULL) {
                    fh_cli_cmd_print(c, 0);
                }
                else {
                    *match_c = c;
                    match++;
                }
            }
        }

        if (match_c && match == 0) {
            return -1;
        }

        return match;
    }

    TAILQ_FOREACH(c, cmd_list, c_next) {
        int rc;

        if (strncasecmp(c->c_name, arg_p, c->c_uniq_len)) {
            continue;
        }

        if (strncasecmp(c->c_name, arg_p, arg_len)) {
            continue;
        }

        if (TAILQ_EMPTY(&c->c_children)) {
            /* last word */
            if (!c->c_callback) {
                fh_cli_write("No callback for \"%s\"\n", fh_cli_cmd_fullname(c));
                return -1;
            }
        }
        else {
            if (arg == c_args - 1) {
                fh_cli_write("Incomplete command\n");
                return 0;
            }

            return fh_cli_cmd_find(&c->c_children, num_args, args,
                                   arg + 1, filters, match_c);
        }

        if (filters && c_args != num_args) {
            if ((num_args - c_args) != 3) {
                fh_cli_write("Invalid filter argument count\n");
            }
            else {
                char *filter = args[c_args+1];
                char *regexp = args[c_args+2];

                fh_cli_filter_init(filter, regexp);
            }
        }

        rc = c->c_callback(fh_cli_cmd_fullname(c), args + arg + 1, c_args - arg - 1);

        if (filters) {
            fh_cli_filter_clean();
        }

        if (rc != 0) {
            fh_cli_write("Invalid %s \"%s\"\n",
                         c->c_parent ? "argument" : "command", arg_p);
        }

        return rc;
    }

    return -1;
}

/*
 * fh_cli_cmd_process
 *
 * Command processing.
 * Returns 0 on success, -1 on failure.
 */
int fh_cli_cmd_process(char *cmd)
{
#define MAX_ARGS 128
#define MAX_FILT 128
    char *args[MAX_ARGS];
    int filters[MAX_FILT];
    int num_args;
    int rc = 0;
    int i, f;

    if (!cmd) return -1;

    while (isspace(*cmd)) cmd++;

    if (!*cmd) {
        return 0;
    }

    num_args = fh_cli_cmd_parse(cmd, args, MAX_ARGS);
    if (num_args == 0) {
        return 0;
    }

    for (i = f = 0; i < num_args && f < MAX_FILT-1; i++) {
        if (args[i][0] == '|') {
            filters[f++] = i;
        }
    }

    filters[f] = 0;

    if (num_args) {
        rc = fh_cli_cmd_find(&cmd_list, num_args, args, 0, filters, NULL);
    }
    else {
        rc = -1;
    }

    for (i = 0; i < num_args; i++) {
        free(args[i]);
    }

    return rc;
}

/*
 * fh_cli_cmd_complete
 *
 * Command completion.
 * Returns 0 on success, -1 on failure.
 */
int fh_cli_cmd_complete(char *cmd, int *cmd_len)
{
#define MAX_ARGS 128
#define MAX_FILT 128
    char *args[MAX_ARGS];
    int num_args;
    int rc = 0;
    int i, filter = 0, include = 0, exclude = 0;

    if (!cmd) return -1;

    while (isspace(*cmd)) cmd++;

    if (!*cmd) {
        fh_cli_write("\n\n");
        fh_cli_write("Available Commands:\n");
        fh_cli_write("-------------------\n");
        fh_cli_cmd_help(&cmd_list);
        fh_cli_write("\n");
        return 0;
    }

    cmd[*cmd_len] = '?';
    cmd[(*cmd_len)+1] = 0;

    num_args = fh_cli_cmd_parse(cmd, args, MAX_ARGS);
    if (num_args == 0) {
        goto done;
    }

    /* No completion for filters, yet */
    for (i = 0; i < num_args; i++) {
        int arg_len = strlen(args[i]);

        if (args[i][0] == '|') {
            filter = 1;
        }
        else if (strncmp(args[i], "include", arg_len) == 0) {
            include = 1;
        }
        else if (strncmp(args[i], "exclude", arg_len) == 0) {
            exclude = 1;
        }
        else if (filter) {
            if (args[i][arg_len-1] == '?') {
                arg_len --;
                if (arg_len > 0) {
                    if (strncmp(args[i], "include", arg_len) == 0) {
                        char *cmd_end = "include" + arg_len;

                        sprintf(cmd+*cmd_len, "%s ", cmd_end);

                        fh_cli_write("%s ", cmd_end);

                        *cmd_len += strlen(cmd_end) + 1;

                        rc = 1;
                        goto done;
                    }
                    else if (strncmp(args[i], "exclude", arg_len) == 0) {
                        char *cmd_end = "exclude" + arg_len;

                        sprintf(cmd+*cmd_len, "%s ", cmd_end);

                        fh_cli_write("%s ", cmd_end);

                        *cmd_len += strlen(cmd_end) + 1;

                        rc = 1;
                        goto done;
                    }
                }
            }

            if (include) {
                fh_cli_write("\n\n");
                fh_cli_print("<regex>", "Include regular expression");
                fh_cli_write("\n");
            }
            else if (exclude) {
                fh_cli_write("\n\n");
                fh_cli_print("<regex>", "Exclude regular expression");
                fh_cli_write("\n");
            }
            else {
                fh_cli_write("\n\n");
                fh_cli_write("Available Commands:\n");
                fh_cli_write("-------------------\n");
                fh_cli_print("include", "Include the line matching regex (egrep <regex>)");
                fh_cli_print("exclude", "Exclude the line matching regex (egrep -v <regex>)");
                fh_cli_write("\n");
            }

            goto done;
        }
    }

    if (num_args) {
        fh_cli_cmd_t *match_c = NULL;

        rc = fh_cli_cmd_find(&cmd_list, num_args, args, 0, NULL, &match_c);

        if (rc > 0) {
            char *last_arg = args[num_args-1];
            int last_arg_len = strlen(last_arg) - 1; /* Remove '?' */

            if (rc == 1) {
                char *cmd_end = match_c->c_name+last_arg_len;

                sprintf(cmd+*cmd_len, "%s ", cmd_end);

                fh_cli_write("%s ", cmd_end);

                *cmd_len += strlen(cmd_end) + 1;

                dprint(("\nCmd complete: [ %s : %s ] ==> %s len:%d\n",
                        match_c->c_name, cmd_end, cmd, *cmd_len));
            }
            else {
                if (last_arg_len > 0) {
                    char *cmd_end = match_c->c_name + last_arg_len;
                    int match_len = match_c->c_uniq_len - last_arg_len;

                    snprintf(cmd+*cmd_len, match_len, "%s", cmd_end);

                    *cmd_len += match_len-1;
                }

                fh_cli_write("\n\n");
                fh_cli_write("Available Commands:\n");
                fh_cli_write("-------------------\n");
                rc = fh_cli_cmd_find(&cmd_list, num_args, args, 0, NULL, NULL);
                fh_cli_write("\n");

                rc = 0;
            }
        }
    }
    else {
        rc = -1;
    }

    for (i = 0; i < num_args; i++) {
        free(args[i]);
    }

done:
    cmd[*cmd_len] = 0;
    return rc;
}

