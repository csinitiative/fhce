%{
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

/* common FH includes */
#include "fh_cfg_parse.h"

/* global data */
int current_line = 1;

/* to remove the warning about yyunput declared but not used... */
static void yyunput(int, char *);
void do_not_call_this() { int c = 0; char *ptr = NULL; yyunput(c, ptr); }

%}

whitespace       [\t ]*
comment          ^[\t ]*#.*
property         [a-zA-Z_0-9]+
unquoted_string  ([^=:,\.\{\}\(\)"\n\t ]+)
quoted_string    (\"(\\\\|\\\"|[^"])*\")
number           (-?[0-9]+(\.[0-9]+)?)

%%

{comment}         |
{whitespace}

{property}        { return PROPERTY; }

{number}          { return NUMBER; }
{unquoted_string} { return UNQUOTED_STRING; }
{quoted_string}   { return QUOTED_STRING; }

\{                { return BRACE_OPEN; }
\}                { return BRACE_CLOSE; }

\(                { return PAREN_OPEN; }
\)                { return PAREN_CLOSE; }

:                 |
=                 { return ASSIGN; }
,                 { return COMMA; }
\.                { return PERIOD; }

\n                { current_line++; }

.                 {
                      fprintf(stderr, "Unparseable character at line %s, column %s.\n", "1", "1");
                      exit(1);
                  }

%%
