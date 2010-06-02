/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 * 
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>

#include "fh_cli.h"

/* ------------------------------------------------------------------- */
/* --- Command History API ------------------------------------------- */
/* ------------------------------------------------------------------- */

/* This should go in the configuration file: ~/.cldctl_config */
#define HIST_CNT        (1000)
#define HIST_SHOW_CNT   (10)

static char hist[1024][HIST_CNT];
static char hist_file[128];
static int  hist_idx = 0;
static int  hist_cnt = 0;
static int  hist_nodups = 1;

/*
 * fh_cli_hist_size
 *
 * Returns the number of entries in the history stack.
 */
int fh_cli_hist_size()
{
    return hist_cnt;
}

/*
 * fh_cli_hist_load
 *
 * Load the command history. History is managed as a ring of N commands.
 */
void fh_cli_hist_load(char *name)
{
    struct stat sb;

    char * home = getenv("HOME");
    if (!home) {
        return;
    }

    sprintf(hist_file, "%s/.%s_history", home, name);

    memset(hist, 0, sizeof(hist));

    if (stat(hist_file, &sb) == 0) {
        char line[1024];

        FILE *fp = fopen(hist_file, "r");
        assert(fp);

        while (hist_cnt < HIST_CNT && fgets(line, sizeof(line), fp) != NULL) {
            line[strlen(line)-1] = 0;
            strcpy(hist[hist_cnt], line);
            hist_cnt++;
            hist_idx = (hist_idx + 1) % HIST_CNT;
        }

        fclose(fp);
    }
}

/*
 * fh_cli_hist_save
 *
 * Save history to file. Most recent is at the bottom.
 */
void fh_cli_hist_save()
{
    if (hist_cnt > 0) {
        int i;

        FILE *fp = fopen(hist_file, "w+");

        if (fp == NULL) {
            perror("fopen failed");
            exit(1);
        }

        for (i=0; i<hist_cnt; i++) {
            char * cmd_ptr = hist[(hist_idx+HIST_CNT-hist_cnt+i) % HIST_CNT];
            fprintf(fp, "%s\n", cmd_ptr);
        }

        fclose(fp);
    }
}

/*
 * fh_cli_hist_show
 *
 * Show history.
 */
void fh_cli_hist_show(int n)
{
    int i;

    if (n == -1) {
        n = HIST_SHOW_CNT;
    }

    /* Cannot ask for more than what is stored in the current history stack */
    if (n > hist_cnt) {
        n = hist_cnt;
    }

    fh_cli_write("    # Command History\n");

    for (i=0; i<n; i++) {
        char *cmd_ptr = hist[(hist_idx+HIST_CNT-n+i) % HIST_CNT];
        if (*cmd_ptr != 0) {
            fh_cli_write(" %4d %s\n", hist_cnt-n+i, cmd_ptr);
        }
    }
}

/*
 * fh_cli_hist_find
 *
 * Find a command starting with the provided command start.
 */
int fh_cli_hist_find(char *cmd_start)
{
    int i, match_len = strlen(cmd_start);

    for (i=0; i<hist_cnt; i++) {
        char *cmd_ptr = hist[(hist_idx+HIST_CNT-1-i) % HIST_CNT];

        if (strncmp(cmd_ptr, cmd_start, match_len) == 0) {
            return hist_cnt-1-i;
        }
    }

    return -1;
}

/*
 * fh_cli_hist_match
 *
 * Find a command matching the provided string.
 */
char *fh_cli_hist_match(char *match, int *itr)
{
    int i;

    /* Wrap the iterator back to the top of the history stack */
    if (*itr == hist_cnt) {
        *itr = 0;
    }

    for (i=*itr; i<hist_cnt; i++) {
        char *cmd_ptr = hist[(hist_idx+HIST_CNT-1-i) % HIST_CNT];

        if (strstr(cmd_ptr, match) != NULL) {
            *itr = i+1;
            return cmd_ptr;
        }
    }

    return NULL;
}


/*
 * fh_cli_hist_get
 *
 * Return the command of a given command index.
 */
char *fh_cli_hist_get(int cmd_idx)
{
    return hist[(hist_idx+HIST_CNT-cmd_idx) % HIST_CNT];
}

/*
 * fh_cli_hist_call
 *
 * Call a specific command from the history.
 */
char *fh_cli_hist_call(int cmd_idx)
{
    char *cmd_ptr;
  
    if (cmd_idx < 0) {
        return NULL;
    }

    /* Last history entry */
    if (cmd_idx == 0) {
        cmd_idx = hist_cnt-1;
    }

    cmd_ptr = fh_cli_hist_get(hist_cnt - cmd_idx);
    if (*cmd_ptr == 0) {
        cmd_ptr = NULL;
    }

    return cmd_ptr;
}

/*
 * fh_cli_hist_add
 *
 * Add a command to the history.
 */
void fh_cli_hist_add(char *cmd)
{
    // Skip identical commands from history
    if (hist_nodups && (hist_cnt > 0) &&
        (strcmp(hist[(hist_idx + HIST_CNT - 1) % HIST_CNT], cmd) == 0)) {
        return;
    }

    strcpy(hist[hist_idx], cmd);

    hist_idx = (hist_idx + 1) % HIST_CNT;

    if (hist_cnt < HIST_CNT) {
        hist_cnt++;
    }
}

