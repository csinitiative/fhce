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
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <signal.h>
#include <asm/ioctls.h>
#include <linux/ioctl.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>

#include "fh_cli_hist.h"
#include "fh_cli.h"
#include "fh_cli_cmd.h"
#include "fh_cli_alias.h"
#include "fh_cli_filter.h"

#define DEBUG (0)

#if DEBUG
# define dprint(x) fh_cli_write x
#else
# define dprint(x)
#endif

/* --- Local defines ------------------------------------------------- */

#define REV_SEARCH_PROMPT  "(reverse-i-search)"

static char *prompt       = NULL;
static char  prompt_char  = '>';
static char *mode         = NULL;
static char *tmp_buf      = NULL;

static int   prev_prompt_len = 0;

static char *saved_prompt      = NULL;
static char *saved_mode        = NULL;
static char  saved_prompt_char = 0;

static int   hist_pos = 0;
static struct termios saved_tio;

/* ------------------------------------------------------------------- */
/* --- Command Line Interface (CLI) API ------------------------------ */
/* ------------------------------------------------------------------- */

/*
 * fh_cli_print
 */
void fh_cli_print(char *cmd, char *help)
{
    fh_cli_write("  %-15s - %s\n", cmd, help);
}

/*
 * fh_cli_write
 */
void fh_cli_write(char *fmt, ...)
{
    static char buffer[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (fh_cli_filter_match(buffer)) {
        fputs(buffer, stdout);
    }
}


/*
 * quit_cb
 *
 * Exit the current mode.
 */
static int quit_cb(char *full_cmd, char **argv, int argc)
{
    char *last_arg = argv[argc-1];

    assert(full_cmd);

    if (last_arg[strlen(last_arg)-1] == '?') {
        fh_cli_write("\n");
        fh_cli_print("<Enter>", "Exit the current mode");
        return 0;
    }

    if (mode) {
        fh_cli_write("Exiting %s mode...\n", mode);
        fh_cli_cmd_dft_mode();
        fh_cli_set_mode(NULL);
        fh_cli_set_prompt_char('>');
    }
    else {
        fh_cli_quit();
    }

    return 0;
}

/*
 * help_cb
 *
 * Display help.
 */
static int help_cb(char *full_cmd, char **argv, int argc)
{
    char *last_arg = argv[argc-1];

    assert(full_cmd);

    if (last_arg[strlen(last_arg)-1] == '?') {
        fh_cli_write("\n");
        fh_cli_print("<Enter>", "Display the CLI help");
        return 0;
    }

    fh_cli_help();
    return 0;
}

/*
 * hist_cb
 *
 * Display history.
 */
static int hist_cb(char *full_cmd, char **argv, int argc)
{
    int n = -1;
    char *last_arg = argv[argc-1];

    assert(full_cmd);

    if (last_arg[strlen(last_arg)-1] == '?') {
        fh_cli_write("\n");
        fh_cli_print("<Enter>", "Get the last 10 commands");
        fh_cli_print("<Number>", "Get the last 'Number' commands");
        return 0;
    }

    if (argc == 1) {
        n = atoi(argv[0]);
    }

    fh_cli_hist_show(n);

    return 0;
}

/*
 * alias_add_cb
 *
 * Add alias.
 */
static int alias_add_cb(char *full_cmd, char **argv, int argc)
{
    char *last_arg = argv[argc-1];

    if (argc > 2) {
        fh_cli_write("Usage: %s <alias> \"<cmd>\"\n", full_cmd);
        return 0;
    }

    if (last_arg[strlen(last_arg)-1] == '?') {
        fh_cli_write("\n");

        if (argc == 1) {
            fh_cli_print("<Alias>", "Define an alias");
        }
        fh_cli_print("<cmd>",   "Command corresponding to your alias");

        return 0;
    }
    else if (argc != 2) {
        fh_cli_write("Incomplete command: %s\n", full_cmd);
        return 0;
    }

    if (fh_cli_alias_add(argv[0], argv[1]) == 0) {
        fh_cli_write("Adding alias: %s -> %s\n", argv[0], argv[1]);
        fh_cli_alias_save();
    }

    return 0;
}

/*
 * alias_del_cb
 *
 * Delete alias.
 */
static int alias_del_cb(char *full_cmd, char **argv, int argc)
{
    char *last_arg = argv[argc-1];

    if (argc > 1) {
        fh_cli_write("Usage: %s <alias>\n", full_cmd);
        return 0;
    }

    if (last_arg[strlen(last_arg)-1] == '?') {
        fh_cli_write("\n");
        fh_cli_print("<Alias>", "Alias to delete");
        return 0;
    }
    else if (argc != 1) {
        fh_cli_write("Incomplete command: %s\n", full_cmd);
        return 0;
    }

    if (fh_cli_alias_del(argv[0]) == 0) {
        fh_cli_write("Deleting alias: %s\n", argv[0]);
        fh_cli_alias_save();
    }

    return 0;
}


/*
 * alias_ls_cb
 *
 * Show all the aliases.
 */
static int alias_ls_cb(char *full_cmd, char **argv, int argc)
{
    char *last_arg = argv[argc-1];

    if (last_arg[strlen(last_arg)-1] == '?') {
        fh_cli_write("\n");
        fh_cli_print("<Enter>", "List all your aliases");
        return 0;
    }

    if (argc > 1) {
        fh_cli_write("Usage: %s\n", full_cmd);
        return 0;
    }

    fh_cli_alias_show();
    return 0;
}

/*
 * fh_cli_init
 *
 * Initialize the CLI.
 */
void fh_cli_init(char *name)
{
    fh_cli_cmd_t *alias_cmd;

    /* --- Initialize CLI Command tree ---------------------------------- */

    fh_cli_cmd_init();

    fh_cli_cmd_register(NULL, "quit",   FH_CLI_CMD_MODE_DEFAULT, quit_cb  , "Exit the current mode");
    fh_cli_cmd_register(NULL, "history",FH_CLI_CMD_MODE_DEFAULT, hist_cb  , "Display history");
    fh_cli_cmd_register(NULL, "help",   FH_CLI_CMD_MODE_DEFAULT, help_cb  , "Display the CLI help");

    alias_cmd = fh_cli_cmd_register(NULL, "alias",  FH_CLI_CMD_MODE_DEFAULT, NULL , "Alias management");

    fh_cli_cmd_register(alias_cmd, "add",  FH_CLI_CMD_MODE_DEFAULT, alias_add_cb, "Create a command alias");
    fh_cli_cmd_register(alias_cmd, "del",  FH_CLI_CMD_MODE_DEFAULT, alias_del_cb, "Delete a command alias");
    fh_cli_cmd_register(alias_cmd, "ls", FH_CLI_CMD_MODE_DEFAULT, alias_ls_cb, "List all your aliases");

    /* --- Load command history/aliases from file ----------------------- */

    fh_cli_hist_load(name);
    fh_cli_alias_load(name);

    /* --- Take care of terminal settings ------------------------------- */ 

    fh_cli_term_config();
}

/*
 * fh_cli_exit
 *
 * Exit the CLI.
 */
void fh_cli_exit()
{
    fh_cli_quit();
}

/*
 * fh_cli_set_mode
 *
 * Set the mode string that we are in.
 */
void fh_cli_set_mode(char *mode_str)
{
    if (mode) {
        free(mode);
        mode = NULL;
    }

    if (mode_str) {
        mode = strdup(mode_str);
    }
}

/*
 * fh_cli_help
 *
 * Display the help on how to navigate the CLI.
 */
void fh_cli_help()
{
    fh_cli_write("\n");
    fh_cli_write("+--- Key controls ------------------------------------------+\n");
    fh_cli_write("|                                                           |\n");
    fh_cli_write("| Ctrl-A                :  Go to the start of the line      |\n");
    fh_cli_write("| Ctrl-E                :  Go to the end of the line        |\n");
    fh_cli_write("| Ctrl-K                :  Delete end of the line           |\n");
    fh_cli_write("| Ctrl-Q                :  Quit the CLI session             |\n");
    fh_cli_write("| Ctrl-Y                :  Paste a buffer previous yank'ed  |\n");
    fh_cli_write("| Ctrl-U                :  Clear complete line              |\n");
    fh_cli_write("| Esc-Backspace         :  Delete a full word before cursor |\n");
    fh_cli_write("| Left                  :  Go left on the command line      |\n");
    fh_cli_write("| Right                 :  Go Right on the command line     |\n");
    fh_cli_write("| Up                    :  Go up in the history stack       |\n");
    fh_cli_write("| Down                  :  Go down in the history stack     |\n");
    fh_cli_write("|                                                           |\n");
    fh_cli_write("+--- Command History ---------------------------------------+\n");
    fh_cli_write("|                                                           |\n");
    fh_cli_write("| # hi[story] [ Count ] :  Show last 'Count' from history   |\n");
    fh_cli_write("| # !<N>                :  Call history entry 'N'           |\n");
    fh_cli_write("| # !!                  :  Call last history entry          |\n");
    fh_cli_write("| # !<cmd>              :  Call history starting with <cmd> |\n");
    fh_cli_write("| Ctrl-R                :  Reverse history search           |\n");
    fh_cli_write("|                                                           |\n");
    fh_cli_write("+--- Command filtering -------------------------------------+\n");
    fh_cli_write("|                                                           |\n");
    fh_cli_write("| > Include matching lines (equivalent to 'egrep <RE>')     |\n");
    fh_cli_write("| # <cmd> | include \"<Regex>\"                               |\n");
    fh_cli_write("|                                                           |\n");
    fh_cli_write("| > Exclude matching lines (equivalent to 'egrep -v <RE>')  |\n");
    fh_cli_write("| # <cmd> | exclude \"<Regex>\"                               |\n");
    fh_cli_write("|                                                           |\n");
    fh_cli_write("+-----------------------------------------------------------+\n");
    fh_cli_write("\n");
}

/*
 * fh_cli_beep
 *
 * Sends a bip to the CLI.
 */
void fh_cli_beep()
{
    fh_cli_write("\7");
}

/*
 * fh_cli_prompt
 *
 * Displays the CLI prompt.
 */
void fh_cli_prompt()
{
    char tmp_prompt[64];

    int tmp_len = sprintf(tmp_prompt, "%s%s%c ",
                          prompt ? prompt : "",
                          mode   ? mode   : "", 
                          prompt_char);

    if (prev_prompt_len > tmp_len) {
        int diff = prev_prompt_len - tmp_len;
        int i;

        for (i=0; i<diff; i++) {
            fh_cli_write("");
        }

        for (i=0; i<diff; i++) {
            fh_cli_write(" ");
        }

        prev_prompt_len = tmp_len;
    }

    fh_cli_write("\r%s", tmp_prompt);
}

/*
 * fh_cli_set_prompt
 *
 * Reconfigure the prompt.
 */
void fh_cli_set_prompt(char *str)
{
    if (prompt) {
        free(prompt);
        prompt = NULL;
    }
    if (str) {
        prompt = strdup(str);
    }
}

/*
 * fh_cli_save_prompt
 *
 * Save current prompt configuration.
 */
void fh_cli_save_prompt()
{
    if (saved_prompt == NULL) {
        saved_prompt_char = prompt_char;
        saved_prompt      = prompt;
        saved_mode        = mode;

        prompt      = NULL;
        mode        = NULL;
        prompt_char = 0;
    }
}

/*
 * fh_cli_restore_prompt
 *
 * Save current prompt configuration.
 */
void fh_cli_restore_prompt()
{
    prev_prompt_len = 2;
    if (prompt && prompt != saved_prompt) {
        prev_prompt_len = strlen(prompt);
        free(prompt);
    }
    if (mode && mode != saved_mode) {
        prev_prompt_len += strlen(mode);
        free(mode);
    }

    mode        = saved_mode;
    prompt_char = saved_prompt_char;
    prompt      = saved_prompt;

    saved_prompt_char = 0;
    saved_prompt      = NULL;
    saved_mode        = NULL;
}

/*
 * fh_cli_set_prompt_char
 *
 * Reconfigure the prompt character.
 */
void fh_cli_set_prompt_char(char c)
{
    prompt_char = c;
}

/*
 * fh_cli_reset
 *
 * Clears the line and put back the command and the cursor to where we
 * were.
 */
void fh_cli_reset(char *cmd, int cmd_len, int pos)
{
    int tmp_pos = cmd_len;

    fh_cli_prompt();

    fh_cli_write("%s", cmd);

    fh_cli_lmove(&tmp_pos, cmd_len - pos);
}

/*
 * fh_cli_word_del
 *
 * Delete a word on the left of the current position.
 */
void fh_cli_word_del(char *cmd, int *cmd_len, int *pos)
{
    int i, del_space = 1;

    if (*pos == 0) {
        return;
    }

    for (i=(*pos)-1; i>=0; i--) {
        if (!del_space && cmd[i] == ' ') {
            break;
        }

        if (del_space && cmd[i] != ' ') {
            del_space = 0;
        }

        fh_cli_erase(cmd, cmd_len, pos);
    }
}

/*
 * fh_cli_lmove
 *
 * Move 'n' bytes left if possible.
 */
void fh_cli_lmove(int *pos, int n)
{
    int i;
    for (i=0; i<n; i++) {
        if (*pos > 0) {
            fh_cli_write("");
            (*pos)--;
        }
        else {
            break;
        }
    }
}

/*
 * fh_cli_rmove
 *
 * Move one byte right if possible.
 */
void fh_cli_rmove(char *cmd, int cmd_len, int *pos, int n)
{
    int i;
    for (i=0; i<n; i++) {
        if (*pos < cmd_len) {
            fh_cli_write("%c", cmd[*pos]);
            (*pos)++;
        }
        else {
            break;
        }
    }
}

/*
 * fh_cli_kill
 *
 * Kill the line from pos to the end.
 */
void fh_cli_kill(char *cmd, int *cmd_len, int *pos)
{
    if (*pos <= *cmd_len) {
        int i, cnt = 0;

        if (tmp_buf) {
            free(tmp_buf);
        }

        tmp_buf = strdup(cmd+*pos);

        for (i=*pos; i<*cmd_len; i++) {
            fh_cli_write(" ");
            (*pos)++;
            cnt++;
        }

        *cmd_len -= cnt;
        fh_cli_lmove(pos, cnt);
        cmd[*pos] = 0;
    }
}

/*
 * fh_cli_paste
 *
 * Pase the yank'ed temporary buffer as a result of a kill.
 */
void fh_cli_paste(char *cmd, int *cmd_len, int *pos)
{
    if (tmp_buf) {
        unsigned int i;

        for (i=0; i<strlen(tmp_buf); i++) {
            fh_cli_write("%c", tmp_buf[i]);
            fh_cli_insert(cmd, cmd_len, pos, tmp_buf[i]);
        }
    }
}

/*
 * fh_cli_insert
 *
 * Insert a character and shift the command right.
 */
void fh_cli_insert(char *cmd, int *cmd_len, int *pos, int c)
{
    int i, cnt = 0;

    /* Shift the command by one byte					*/
    for (i=*cmd_len; i>*pos; i--) {
        cmd[i] = cmd[i-1];
    }

    cmd[*pos] = c;
    (*pos)++;
    (*cmd_len)++;

    cmd[*cmd_len] = 0;

    /* Print the end of the command					    */
    for (i=(*pos); i<(*cmd_len); i++) {
        fh_cli_write("%c", cmd[i]);
        cnt++;
        (*pos)++;
    }

    fh_cli_lmove(pos, cnt);
}

/*
 * fh_cli_erase
 *
 * Erase a character at position, and shift command left.
 */
void fh_cli_erase(char *cmd, int *cmd_len, int *pos)
{
    if (*pos > 0) {
        int i, cnt = 0;

        fh_cli_write("");

        for (i=*pos; i<*cmd_len; i++) {
            cmd[i-1] = cmd[i];
            fh_cli_write("%c", cmd[i]);
            (*pos)++;
            cnt++;
        }

        (*pos)--;
        (*cmd_len)--;

        cmd[*pos] = 0;

        fh_cli_write(" ");
        (*pos)++;
        cnt++;

        fh_cli_lmove(pos, cnt);
    }
}

/*
 * fh_cli_set_cmd
 *
 * Set the current command, and position to the history entry 'n'.
 */
static void fh_cli_set_cmd(int n, char *cmd, int *cmd_len, int *pos)
{
    char *cmd_ptr = fh_cli_hist_get(n);

    /* Move back to the beginning of the prompt */
    fh_cli_lmove(pos, *pos);
    fh_cli_kill(cmd, cmd_len, pos);

    /* Display the new command */
    fh_cli_write(cmd_ptr);

    /* Update the command context */
    strcpy(cmd, cmd_ptr);
    *cmd_len = strlen(cmd_ptr);
    *pos     = *cmd_len;
}

/*
 * fh_cli_hist_up
 *
 * Go up in the history stack.
 */
void fh_cli_hist_up(char *cmd, int *cmd_len, int *pos)
{
    if (hist_pos < fh_cli_hist_size()) {
        hist_pos++;
        fh_cli_set_cmd(hist_pos, cmd, cmd_len, pos);
    }
}

/*
 * fh_cli_hist_down
 *
 * Go down in the history stack.
 */
void fh_cli_hist_down(char *cmd, int *cmd_len, int *pos)
{
    if (hist_pos > 0) {
        hist_pos--;
        fh_cli_set_cmd(hist_pos, cmd, cmd_len, pos);
    }
}

/*
 * fh_cli_quit
 *
 * Quit the CLI.
 */
void fh_cli_quit()
{
    fh_cli_write("Terminating CLI session...\n");
    fh_cli_term_restore();
    exit(0);
}

/*
 * fh_cli_register
 *
 * Register the command and its chidldren
 */
void fh_cli_register(fh_cli_cmd_t *parent, void **defs)
{
    int          def_cmd = 0;

    char        *c_name     = NULL;
    long         c_mode     = 0;
    fh_cli_cmd_cb_t *c_cb       = NULL;
    char        *c_help     = NULL;
    void       **c_children = NULL;

    FH_CLI_CMD_DEF_START();

    while ((def_cmd = FH_CLI_CMD_DEF_NEXT(long)) != 0) {
        switch (def_cmd) {
        case FH_CLI_CMD_DEF_NAME:
            assert(c_name == NULL);
            c_name = FH_CLI_CMD_DEF_NEXT(char *);
            break;

        case FH_CLI_CMD_DEF_MODE:
            assert(c_mode == 0);
            c_mode = FH_CLI_CMD_DEF_NEXT(long);
            break;

        case FH_CLI_CMD_DEF_CB:
            assert(c_cb == NULL);
            c_cb = FH_CLI_CMD_DEF_NEXT(fh_cli_cmd_cb_t *);
            break;

        case FH_CLI_CMD_DEF_HELP:
            assert(c_help == NULL);
            c_help = FH_CLI_CMD_DEF_NEXT(char *);
            break;

        case FH_CLI_CMD_DEF_CHILDREN:
            assert(c_children == NULL);
            c_children = FH_CLI_CMD_DEF_NEXT(void **);
            break;

        case FH_CLI_CMD_DEF_REG:
        {
            fh_cli_cmd_t *c;
            
            c = fh_cli_cmd_register(parent, c_name, c_mode, c_cb, c_help);

            if (c_children) {
                fh_cli_register(c, c_children);
            }

            /* Reset the data for the next sibling command tree */
            c_mode     = 0;
            c_name     = NULL;
            c_help     = NULL;
            c_cb       = NULL;
            c_children = NULL;

            break;
        }

        default:
            fh_cli_write("FATAL: invalid cmd:%d\n", def_cmd);
            exit(1);
        }
    }
}

/*
 * fh_cli_tree
 *
 * Loads the provided cli tree.
 */
void fh_cli_tree(void **cmd_tree)
{
    fh_cli_register(NULL, cmd_tree);
}

/*
 * fh_cli_loop
 *
 * Main loop that gather the command and runs them when completed.
 */
void fh_cli_loop()
{
    char cmd[1024];
    int cmd_len    = 0;
    int pos        = 0;
    int arrow      = 0;
    int esc        = 0;
    int new_line   = 1;

    /* Reserse search definitions */
    char rev_search_cmd[1024];
    int rev_search_cmd_len = 0;
    int rev_search_itr     = 0;
    int rev_search         = 0;

    memset(cmd, 0, sizeof(cmd));

    while (1) {
        int c;

        /* --- Prompt display ---------------------------------------------- */ 

        if (new_line == 1) {
            if (rev_search) {
                fh_cli_restore_prompt();
                rev_search         = 0;
                rev_search_cmd_len = 0;
                rev_search_itr     = 0;
            }
            fh_cli_prompt();
            new_line = 0;
        }

        /* --- Get new character ------------------------------------------- */ 

        c = fgetc(stdin);
        if (c == EOF) {
            continue;
        }

        /* --- Backspace handling ------------------------------------------ */ 

        if ((c == '') || (c == 0x08) || (c == 0x7F) || (c == '\177')) {
            if (esc) {
                fh_cli_word_del(cmd, &cmd_len, &pos);
                esc = 0;
                continue;
            }

            if (rev_search) {
                char tmp_mode[1024];

                rev_search_itr = 0;
                rev_search_cmd_len--;
                rev_search_cmd[rev_search_cmd_len] = 0;

                sprintf(tmp_mode, "`%s'", rev_search_cmd);
                fh_cli_set_mode(tmp_mode);

                fh_cli_reset(cmd, cmd_len, pos);
            }
            else {
                fh_cli_erase(cmd, &cmd_len, &pos);
            }
            continue;
        }

        /* --- Jump to beginning of the line ------------------------------- */

        if (c == '\1') {
            fh_cli_lmove(&pos, pos);
            continue;
        }

        /* --- Jump to end of the line ------------------------------------- */ 

        if (c == '\5') {
            fh_cli_rmove(cmd, cmd_len, &pos, cmd_len);
            continue;
        }

        /* --- End session ------------------------------------------------- */ 

        if (c == '\4') {
            fh_cli_write("\n");
            fh_cli_cmd_process("quit");
            fh_cli_reset(cmd, cmd_len, pos);
            continue;
        }

        /* --- Kill line handling ------------------------------------------ */ 

        if (c == '\v') {
            fh_cli_kill(cmd, &cmd_len, &pos);
            continue;
        }

        /* --- Paste Yank'ed data ------------------------------------------ */ 

        if (c == '\31') {
            fh_cli_paste(cmd, &cmd_len, &pos);
            continue;
        }

        /* --- Clear line -------------------------------------------------- */

        if (c == '\25') {
            fh_cli_lmove(&pos, pos);
            fh_cli_kill(cmd, &cmd_len, &pos);
            continue;
        }

        /* --- Reverse history search -------------------------------------- */

        if (c == '\22') {
            if (rev_search == 0) {
                fh_cli_save_prompt();
                fh_cli_set_prompt(REV_SEARCH_PROMPT);
                fh_cli_set_prompt_char(':');
                fh_cli_set_mode("`'");
                fh_cli_reset(cmd, cmd_len, pos);
                rev_search = 1;
            }
            else if (rev_search_cmd_len > 0) {
                char *cmd_ptr;

                cmd_ptr = fh_cli_hist_match(rev_search_cmd, &rev_search_itr);
                if(cmd_ptr == NULL) {
                    fh_cli_beep();

                    /* Revert search string last character */
                    rev_search_cmd_len--;
                    rev_search_cmd[rev_search_cmd_len] = 0;

                    continue;
                }

                fh_cli_kill(cmd, &cmd_len, &pos);

                cmd_len = sprintf(cmd, "%s", cmd_ptr);

                pos = 0;

                fh_cli_reset(cmd, cmd_len, pos);
            }
            continue;
        }

        /* --- Clear Screen handling --------------------------------------- */ 

        if (c == '\f') {
            fh_cli_write("\n");
            fh_cli_reset(cmd, cmd_len, pos);
            continue;
        }

        /* --- Command Completion handling -------------------------------- */

        if (c == '\t' || c == '?') {
            int rc;

            if (pos != cmd_len) {
                continue;
            }

            rc = fh_cli_cmd_complete(cmd, &cmd_len);

            if (rc == 0) {
                pos = cmd_len;
                fh_cli_reset(cmd, cmd_len, pos);
            }
            else if (rc == 1) {
                /* Perfect match */
                pos = cmd_len;
            }
            else {
                fh_cli_write("\nInvalid command: '%s'\n", cmd);
                fh_cli_reset(cmd, cmd_len, pos);
            }

            continue;
        }

        /* --- Control handling (Up/Down/Left/Right) ---------------------- */ 

        if (c == '\33') {
            if (rev_search) {
                int saved_pos = pos;

                fh_cli_lmove(&pos, pos);
                fh_cli_kill(cmd, &cmd_len, &pos);
                fh_cli_restore_prompt();
                fh_cli_reset(cmd, cmd_len, pos);
                fh_cli_paste(cmd, &cmd_len, &pos);

                if (saved_pos != cmd_len) {
                    fh_cli_lmove(&pos, cmd_len-saved_pos);
                }

                rev_search_cmd_len = 0;
                rev_search         = 0;
            }

            esc = 1;
            continue;
        }

        if (esc) {
            if (c == '[') {
                arrow = 1;
                continue;
            }
            esc = 0;
        }

        if (arrow) {
            switch (c) {
            case 'A': /* UP    */
                fh_cli_hist_up(cmd, &cmd_len, &pos);
                continue;

            case 'B': /* DOWN  */
                fh_cli_hist_down(cmd, &cmd_len, &pos);
                continue;

            case 'C': /* RIGHT */
                fh_cli_rmove(cmd, cmd_len, &pos, 1);
                continue;

            case 'D': /* LEFT  */
                fh_cli_lmove(&pos, 1);
                continue;
            }
            arrow = 0;
        }

        /* --- Command parsing -------------------------------------------- */

        if (rev_search == 0 || c == '\n') {
            fh_cli_write("%c", c);
        }

        if (c == '\n') {
            /* -- Command history handling ---------------------------------- */
            if (cmd[0] == '!') {
                int cmd_idx;
                char *cmd_ptr;

                if (cmd[1] == '!') {
                    cmd_idx = 0;
                }
                else if (isdigit(cmd[1])) {
                    cmd_idx = atoi(&cmd[1]);
                }
                else {
                    cmd_idx = fh_cli_hist_find(cmd+1);
                    if (cmd_idx == -1) {
                        fh_cli_write("No historical command starting with: %s\n",cmd+1);
                    }
                }

                if (cmd_idx != -1 && (cmd_ptr = fh_cli_hist_call(cmd_idx)) != NULL) {
                    fh_cli_write("> %s\n", cmd_ptr);
                    fh_cli_hist_add(cmd_ptr);
                    if (fh_cli_cmd_process(cmd_ptr) != 0) {
                        fh_cli_write("Invalid command: '%s'\n", cmd_ptr);
                    }
                }
            }

            /* -- User-defined Command handling ----------------------------- */

            else if (cmd_len > 0) {
                if (fh_cli_cmd_process(cmd) != 0) {
                    fh_cli_write("Invalid command: '%s'\n", cmd);
                }

                fh_cli_hist_add(cmd);
                fh_cli_hist_save();
            }

            new_line = 1;
            cmd_len  = 0;
            pos = 0;
            hist_pos = 0;
        }
        else if (isprint(c)) {
            if (rev_search) {
                char tmp_mode[1024], *cmd_ptr;

                rev_search_cmd[rev_search_cmd_len++] = c;
                rev_search_cmd[rev_search_cmd_len] = 0;

                cmd_ptr = fh_cli_hist_match(rev_search_cmd, &rev_search_itr);

                if(cmd_ptr == NULL) {
                    fh_cli_beep();

                    /* Revert search string last character */
                    rev_search_cmd_len--;
                    rev_search_cmd[rev_search_cmd_len] = 0;

                    continue;
                }

                fh_cli_kill(cmd, &cmd_len, &pos);

                sprintf(tmp_mode, "`%s'", rev_search_cmd);
                fh_cli_set_mode(tmp_mode);

                cmd_len = sprintf(cmd, "%s", cmd_ptr);

                pos = 0;

                fh_cli_reset(cmd, cmd_len, pos);
            }
            else {
                /* --- Insert							*/
                if (pos != cmd_len) {
                    fh_cli_insert(cmd, &cmd_len, &pos, c);
                }

                /* --- Append							*/
                else {
                    cmd[cmd_len++] = c;
                    pos++;
                }
            }
        }

        cmd[cmd_len] = 0;
    }
}

/*
 * fh_cli_term_config
 *
 * Configures the terminal to read character by character.
 */
void fh_cli_term_config()
{
    struct termios tio;

    ioctl(0, TCGETS, &tio);

    memcpy(&saved_tio, &tio, sizeof(tio));

    tio.c_iflag &= ~INLCR;
    tio.c_lflag &= ~(ICANON|ECHO);

    tio.c_cc[VMIN]   = 1;
    tio.c_cc[VTIME]  = 0;

    ioctl(0, TCSETSW, &tio);

    setbuf(stdout, NULL);
}

/*
 * fh_cli_term_restore
 *
 * Restore initial terminal settings.
 */
void fh_cli_term_restore()
{
    ioctl(0, TCSETSW, &saved_tio);
}


