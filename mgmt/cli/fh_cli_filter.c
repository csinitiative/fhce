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
#include <string.h>
#include <regex.h>

#include "fh_cli.h"

static regex_t _regex, *rep = NULL;
static int exclude = 0;

/* ------------------------------------------------------------------- */
/* --- CLI Filter API ------------------------------------------------ */
/* ------------------------------------------------------------------- */

#define REG_FLAGS (REG_NOSUB|REG_EXTENDED|REG_ICASE)

/* ------------------------------------------------------------------- */
/* fh_cli_filter_match - Matches a given filter                        */
/* ------------------------------------------------------------------- */
int fh_cli_filter_match(char *string)
{
    int rc = -1;

    /* If there is no filter configured, then any string is accepted */
    if (rep == NULL) {
        return 1;
    }

    if (!regexec(rep, string, 0, 0, 0)) {
        rc = 0;
    }

    if (exclude) {
        rc = (rc == 0) ? -1 : 0;
    }

    return rc;
}

/* ------------------------------------------------------------------- */
/* fh_cli_filter_init - Initialize a CLI filter                        */
/* ------------------------------------------------------------------- */
int fh_cli_filter_init(char *filter, char *regexp)
{
    int filter_len = filter ? strlen(filter) : 0;

    /* Reset the filter to NULL if not provided as an argument */
    if (filter == NULL) {
        rep = NULL;
        return 0;
    }
    else {
        rep = &_regex;
    }

    if (strncmp(filter, "exclude", filter_len) == 0) {
        exclude = 0;
    }
    else if (strncmp(filter, "include", filter_len) == 0) {
        exclude = 1;
    }
    else {
        rep = NULL;
        fh_cli_write("Invalid filter keyword: %s\n", filter);
        return -1;
    }

    if (regcomp(rep, regexp, REG_FLAGS)) {
        rep = NULL;
        fh_cli_write("Invalid regexp \"%s\"\n", regexp);
        return -1;
    }

    return 0;
}

/* ------------------------------------------------------------------- */
/* fh_cli_filter_clean - Reset the filter                              */
/* ------------------------------------------------------------------- */
void fh_cli_filter_clean()
{
    if (rep) {
        regfree(rep);
        rep = NULL;
    }
}

