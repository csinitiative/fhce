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
/* file: fh_feed_group.c                                             */
/* Usage:Implement data model for a line of arca multicast feed      */
/* Author: Wally Matthews of  Collaborative Software Initiative      */
/* Conception: Nov. 9, 2008                                          */
/* Inherited from Tervela Arca Multicast feed handler                */
/*   with mods to remove Tervela specific data or obsolete members   */
/*********************************************************************/

//#include <stdlib.h>
//#include <stdio.h>
//#include <unistd.h>
//#include <string.h>

#include "fh_feed_group.h"
#include "fh_notify_constants.h"
#include "fh_arca_headers.h"
#include "fh_data_conversions.h"

uint64_t missing_modulos[64] = 
{
    1LL,2LL,4LL,8LL,16LL,32LL,64LL,128LL,
    256LL,512LL,1024LL,2048LL,4096LL,8192LL,16384LL,32768LL,
    0x10000LL,0x20000LL,0x40000LL,0x80000LL,
    0x100000LL,0x200000LL,0x400000LL,0x800000LL,
    0x1000000LL,0x2000000LL,0x4000000LL,0x8000000LL,
    0x10000000LL,0x20000000LL,0x40000000LL,0x80000000LL,
    0x100000000LL,0x200000000LL,0x400000000LL,0x800000000LL,
    0x1000000000LL,0x2000000000LL,0x4000000000LL,0x8000000000LL,
    0x10000000000LL,0x20000000000LL,0x40000000000LL,0x80000000000LL,
    0x100000000000LL,0x200000000000LL,0x400000000000LL,0x800000000000LL,
    0x1000000000000LL,0x2000000000000LL,
    0x4000000000000LL,0x8000000000000LL,
    0x10000000000000LL,0x20000000000000LL,
    0x40000000000000LL,0x80000000000000LL,
    0x100000000000000LL,0x200000000000000LL,
    0x400000000000000LL,0x800000000000000LL,
    0x1000000000000000LL,0x2000000000000000LL,
    0x4000000000000000LL,0x8000000000000000LL
};
/*--------------------------------------------------------------------------*/
/* set the default parameters for a group                                   */
/*--------------------------------------------------------------------------*/
int default_config(struct feed_group * const group)
{
    init_mutexes(group);
    return 0;
};
/*--------------------------------------------------------------------------*/
/* creates a new feed group on heap and returns ptr; similar to a c++ create*/
/* caller should check for NULL return                                      */
/* one feed group per line supported by a process                           */
/*--------------------------------------------------------------------------*/
struct feed_group* new_feed_group()
{
    struct feed_group *group;
    int    modulo=0;
    int    dwords=0;
    size_t feed_group_size = 0;

    feed_group_size = sizeof(struct feed_group);
    dwords = feed_group_size / 8;
    modulo = feed_group_size % 8;
    if (modulo>0)
    {
        feed_group_size = (dwords+1)*8; //round up to next double word size
    }
    //allocate the feed_group from the heap
    group = (struct feed_group*) malloc((size_t)feed_group_size);
    if (group == NULL) return (struct feed_group*)0;
    memset(group,0,feed_group_size); //set it to null
    group->feed_group_space_size = feed_group_size; //record the size
    // Note: initial missing sequence number object is all zeros
    //   the memset set the state and explicitly using intMissing
    //   would be redundant
    // initialize to inSequence processing
    group->in_sequence = 1;
    //WEM WIP Stage 3 GET requestOrInterval from configs
    return group;
};
/*--------------------------------------------------------------------*/
/* add range of sequence numbers to the list of missing sequence      */
/*  numbers.                                                          */
/*--------------------------------------------------------------------*/
int add_sequences_2_missing(struct feed_group * const group,
    const uint32_t first_sequence, const uint32_t count)
{
    uint32_t nextPktSeq = 0;
    uint32_t modulo_val = 0;
    uint32_t offset     = 0;
    uint32_t index      = 0;
    uint64_t mask;
    uint32_t sequ = first_sequence;

    if (sequ <= group->missing_base) 
    {
        return -1;
    }
    nextPktSeq = first_sequence + count - 1;
    if (group->missing_count ==0) 
    {
        group->missing_base = first_sequence;
        memset(&(group->missing_sequence[0]),0,sizeof(group->missing_sequence));
        group->missing_lowest = first_sequence;
        group->missing_highest = first_sequence + count-1;
    }
    if (nextPktSeq >= group->missing_highest) 
    {
        group->missing_highest = nextPktSeq;
    }
    // WEM WIP this could be optimized for large gaps
    //   set up to first modulo 64 boundary
    //   set up groups of 64 with single instruction
    //   set up residual
    while (sequ <= nextPktSeq) 
    {
        modulo_val = sequ-group->missing_base;
        offset = modulo_val >> 6;
        index = modulo_val % 64;
        mask = missing_modulos[index];
        group->missing_sequence[offset] |= mask;
        sequ++;
    }
    group->missing_count += count;
    return 0;
};
/*------------------------------------------------------------------*/
/* stub function to start refresh processing                        */
/*------------------------------------------------------------------*/
int start_book_refresh(struct feed_group * const group)
{
    if(group) {}
    //WEM WIP this has morphed over time and it may be vestigial organ
    return 0;
};
/*------------------------------------------------------------------*/
/* stub function to stop refresh processing                         */
/*------------------------------------------------------------------*/
int stop_book_refresh(struct feed_group * const group)
{
    if(group){}
    //WEM WIP this has morphed over time and it may be vestigial organ
    return 0;
};

