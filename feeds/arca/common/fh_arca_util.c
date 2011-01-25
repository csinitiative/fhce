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

//
/*********************************************************************/
/* file: fh_arca_util.c                                              */
/* Usage: create a thread name                                       */
/* Author: Ross Cooperman of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <stdio.h>
#include <string.h>

// Common FH headers
#include "fh_log.h"

// Arca FH headers
#include "fh_arca_cfg.h"

/*! \brief Generate a thread name string
 *
 *  \param base thread name base (Main, Mgmt, etc)
 *  \return a fully formatted thread name string
 */
char *fh_arca_util_thread_name(const char *base)
{
    int      string_chars;
    char    *thread_name = NULL;

    // figure out how many characters we need in the string
    string_chars = strlen(fh_arca_cfg.name) + strlen(base) + 1;

    // allocate space for, generate, and log a "thread started" message for this thread's name
    thread_name = (char *)malloc((string_chars + 1) * sizeof(char));
    FH_ASSERT(thread_name != NULL);
    sprintf(thread_name, "%s_%s", base, fh_arca_cfg.name);
    
    // return generated thread name
    return thread_name;
};
