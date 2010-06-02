%{
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

/* system includes */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* common FH includes */
#include "fh_config.h"

/* External references */
extern char           *yytext;
extern FILE           *yyin;
extern int             current_line;
extern fh_cfg_node_t  *fh_priv_config;

/* static variables */
static fh_cfg_node_t *current = NULL;

/* global variables */
int popcount = 1;

/* declarations needed to suppress gcc warnings */
int yylex();
int yyerror(char *);

/* declarations of internal functions */
void fh_cfg_dequote_string(char *);
void fh_cfg_prop_new(const char *);
void fh_cfg_prop_prev();
void fh_cfg_val_new(const char *);

%}

%token NUMBER PROPERTY UNQUOTED_STRING QUOTED_STRING BRACE_OPEN BRACE_CLOSE PAREN_OPEN PAREN_CLOSE
       COLON ASSIGN COMMA PERIOD

%%

multiple_assignment: /* permit empty files */ | assignment multiple_assignment

assignment:
    lvalue
    ASSIGN
    rvalue { fh_cfg_prop_prev(); }

lvalue:
    prop |
    prop PERIOD lvalue { popcount++; }

prop: PROPERTY { fh_cfg_prop_new(yytext); }

rvalue: value | array_value | nested_assignment

avalue:
    NUMBER |
    PROPERTY |
    UNQUOTED_STRING |
    QUOTED_STRING { fh_cfg_dequote_string(yytext); }

value: avalue { fh_cfg_val_new(yytext); }

value_list: rvalue COMMA value_list | rvalue

array_value: PAREN_OPEN value_list PAREN_CLOSE

nested_assignment: BRACE_OPEN multiple_assignment BRACE_CLOSE

%%

void fh_cfg_dequote_string(char *s)
{
    unsigned int i;

    if (s[strlen(s) - 1] == '"') s[strlen(s) - 1] = '\0';

    if (s[0] == '"') {
        for (i = 0; i < strlen(s) - 1; i++) {
            s[i] = s[i + 1];
        }
        s[strlen(s) - 1] = '\0';
    }
}

void fh_cfg_prop_new(const char *s)
{
    // allocate space for the new property
    fh_cfg_node_t *new;

    // Initialize the current configuration only once
    if (current == NULL) {
        current = fh_priv_config;
    }

    new = (fh_cfg_node_t *)malloc(sizeof(fh_cfg_node_t));
    if (new == NULL) {
        fprintf(stderr, "%s: out of memory (%d)\n", __FILE__, __LINE__);
        exit(1);
    }
    memset(new, 0, sizeof(fh_cfg_node_t));

    // allocate space in the current node's children array for the new child
    current->children = (fh_cfg_node_t **)realloc(current->children,
                        ++current->num_children * sizeof(fh_cfg_node_t *));
    if (current->children == NULL) {
        fprintf(stderr, "%s: out of memory (%d)\n", __FILE__, __LINE__);
        exit(1);
    }

    // copy the name of the new property to the name attribute of the new node, add
    // the new property node to the current node's childre, set the current node as
    // the parent of the new node, and set current to the new node
    strcpy(new->name, s);
    new->parent = current;
    current->children[current->num_children - 1] = new;
    current = new;
}

void fh_cfg_prop_prev()
{
    int i;

    for (i = 0; i < popcount; i++) {
        // set the current node to the current node's parent
        current = current->parent;
    }
    popcount = 1;
}

void fh_cfg_val_new(const char *val)
{
    // allocate space for the new value and copy the contents of val to this
    // new property
    char *new;
    new = (char *)malloc((strlen(val) + 1) * sizeof(char));
    if (new == NULL) {
        fprintf(stderr, "%s: out of memory (%d)\n", __FILE__, __LINE__);
        exit(1);
    }
    strcpy(new, val);

    // allocate space in the current node's value array for the a pointer to the
    // new value and set the pointer appropriately
    current->values = (char **)realloc(current->values, ++current->num_values * sizeof(char *));
    if (current->values == NULL) {
        fprintf(stderr, "%s: out of memory (%d)\n", __FILE__, __LINE__);
        exit(1);
    }
    current->values[current->num_values - 1] = new;
}

int yyerror(char *s)
{
    fprintf(stderr, "ERROR: '%s' at line %d (last token parsed = %s).\n", s, current_line, yytext);
	return -1;
}
