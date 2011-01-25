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

#ifndef __FH_ARCA_UTIL_H__
#define __FH_ARCA_UTIL_H__

/*********************************************************************/
/* file: fh_arca_util.h                                              */
/* Usage: header for arca multicast feed handler                     */
/* Author: Ross Cooperman of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/


/*! \brief Generate a thread name string
 *
 *  \param base thread name base (Main, Mgmt, etc)
 *  \return a fully formatted thread name string
 */
char *fh_arca_util_thread_name(const char *base);

/* convert arca price to csi price format */
static inline uint64_t make_price(uint8_t scale, uint32_t value)
{
    int shift_count=0;
    uint64_t result;

    result = (uint64_t) value;
    shift_count = 6-scale;
    if (shift_count>0)
    {
        switch(shift_count)
        {
        case 1: {result *= 10; break;}
        case 2: {result *=100; break;}
        case 3: {result *=1000; break;}
        case 4: {result *=10000; break;}
        case 5: {result *=100000; break;}
        default: {result *=1000000;}
        }
    }
    return result;
};
#endif /* __FH_ARCA_UTIL_H__ */
