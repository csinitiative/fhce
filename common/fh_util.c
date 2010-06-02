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

// System headers
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

// FH common headers
#include "fh_log.h"

/*
 * fh_daemonize
 *
 * Make this process a daemon
 */
void fh_daemonize()
{
    int pid;
    int i;

    /*
     * Ignore child terminate signal
     */
    signal(SIGCHLD, SIG_IGN);

    pid = fork();
    if (pid < 0) exit(1); /* fork error */
    if (pid > 0) exit(0); /* parent exits */

    /*
     * Child (daemon) continues
     */

    /*
     * Obtain a new process group
     */
    setsid();

    /*
     * Close all descriptors
     */
    for (i = getdtablesize(); i >= 0; --i) close(i);

    /*
     * Redirect stdin and stdout to /dev/null
     */
    i = open("/dev/null", O_RDWR);
    i = dup(i); /* stdout */
    i = dup(i); /* stderr */
}

/*
 * fh_strsplit_sep
 *
 * Splits a string in chunks
 */
int fh_strsplit_sep(char *string, char *sep, char **fields, size_t size)
{
    size_t i;
    char *ptr;
    char *saveptr;

    i = 0;
    ptr = string;
    saveptr = NULL;

    while ((fields[i] = strtok_r(ptr, sep, &saveptr)) != NULL) {
        ptr = NULL;

        i++;

        if (i >= size) {
            break;
        }
    }

    return (i);
}

/*
 * fh_strsplit
 *
 * Splits a string in chunks
 */
int fh_strsplit(char *string, char **fields, size_t size)
{
    return fh_strsplit_sep(string, " \t", fields, size);
}

/*! \brief Convert a string to lower case
 *
 *  \param str string to be converted
 *  \return a pointer to the same string
 */
char *fh_str_downcase(char *str)
{
    int i;

    for (i = 0; str[i] != '\0'; i++) {
        str[i] = tolower(str[i]);
    }
    return str;
}

/*! \brief Generate a thread name string (such as Main_fhFoo0)
 *
 *  \param base thread name base (Main, Mgmt, etc)
 *  \param proc process for which this name is being generated
 *  \return a fully formatted thread name string
 */
char *fh_util_thread_name(const char *base, const char *proc)
{
    int      string_chars;
    char    *thread_name = NULL;

    // figure out how many characters we need in the string
    string_chars = strlen(proc) + strlen(base) + 1;

    // allocate space for, generate, and log a "thread started" message for this thread's name
    thread_name = (char *)malloc((string_chars + 1) * sizeof(char));
    FH_ASSERT(thread_name != NULL);
    sprintf(thread_name, "%s_%s", base, proc);

    // return generated thread name
    return thread_name;
}

/*
 * Convert a null-terminated string's alphabetic characters to upper case
 */
void fh_util_ucstring(char *target, const char *source)
{
    int  i;

    for (i = 0; source[i] != '\0'; i++) {
        target[i] = toupper(source[i]);
    }
    target[strlen(source)] = '\0';
}

