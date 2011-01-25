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
/* file: seq_handling.c                                              */
/* Usage: duplicate, gap detection, & gap fill arca multicast        */
/* Author: Wally Matthews of  Collaborative Software Initiative      */
/* Conception: Nov. 9, 2008                                          */
/*********************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>                       //usage of mutexen
#include "fh_log.h"                        //usage of logging
#include "fh_feed_group.h"                 //access to data model
#include "fh_arca_headers.h"               //headers and constants
#include "fh_notify_constants.h"           //usage of notify constants

/*----------------------------------------------------------------------*/
/* inline code for processing first gap while in sequence               */
/*----------------------------------------------------------------------*/
inline int first_gap(struct feed_group* const group, const uint32_t gap_size,
    const uint32_t seq_number, const uint32_t most_advanced, 
    const int primary_or_secondary)
{
    struct msg_hdr  hdr;
    struct msg_body body;
    int             new_gap_size  = 0;
    int             new_gap_start = 0;
    int             loss_gap_size = 0;

    new_gap_start = most_advanced;
    new_gap_size = gap_size;
    // update missing stats
    group->missing_packet_incidence++;
    group->missing_message_range += gap_size;
    // initialize the missing members of feed group
    pthread_mutex_lock(&group->access_missing);
    init_missing(group);
    pthread_mutex_unlock(&group->access_missing);
    /* OBSOLETE
    if (gap_size>GAP_SIZE_TOO_BIG)
    {
        start_book_refresh(group); //start refresh of books
    }
    */
    if (gap_size > MISSING_RANGE)
    {
        // what if gap is realllllly BIGGGG
        //  we adjust the gap start to half maximum gap size just b4 seq_number
        new_gap_start = seq_number - (MISSING_RANGE/2);
        if (new_gap_start<(int)most_advanced)
        {
            new_gap_start = most_advanced;
        }
        new_gap_size = seq_number - new_gap_start;
        if (new_gap_size < (int) gap_size)
        {
            new_gap_size = gap_size;
        }
        if ((int)gap_size > new_gap_size)
        {
            // declare a permanent packet loss
            loss_gap_size = new_gap_start - most_advanced;
            group->packets_lost_incidence++; //record stats
            group->unrecoverable_messages += loss_gap_size;
            hdr.msg_type = PACKET_LOSS;
            body.alert_type = LOST_PACKETS;
            body.begin_seq_number = most_advanced;
            body.end_seq_number = most_advanced +loss_gap_size-1;
            body.primary_or_secondary = primary_or_secondary;
            FH_LOG(LH,ERR,(" %s Packet Loss Detected at %d size %d %d",
                &(group->feed_name[0]),most_advanced,loss_gap_size,
                primary_or_secondary));
            notify_packet_loss(group,LOST_PACKETS,most_advanced,
                loss_gap_size,primary_or_secondary);
        }
        return 0;
    }
    // add possibly adjusted gap to missing sequence number list
    pthread_mutex_lock(&group->access_missing);
    add_sequences_2_missing(group,new_gap_start,new_gap_size);
    pthread_mutex_unlock(&group->access_missing);
    // set out of sequence state; side effect is to alert subscribers
    set_out_of_sequence(group);
    return 0;
};
/*----------------------------------------------------------------------*/
/* inline code for processing gap while out of sequence                 */
/*----------------------------------------------------------------------*/
inline int second_gap(struct feed_group* const group, const uint32_t gap_size, 
    const uint32_t seq_number,const uint32_t most_advanced,
    const int primary_or_secondary)
{
    struct msg_hdr  hdr;
    struct msg_body body;
    int             new_gap_size=0;
    int             new_gap_start=0;

    FH_LOG(LH, ERR,("%s : gap detected in gap at %d size %d : %d", group->feed_name,
                    most_advanced, gap_size, primary_or_secondary));
    group->missing_packet_incidence++;
    group->missing_message_range += gap_size;
    /* OBSOLETE FUNCTION
    if (gap_size>GAP_SIZE_TOO_BIG) 
    {
        start_book_refresh(group);  //start refreshing books
    }
    */
    if (seq_number < (group->missing_base + MISSING_RANGE)) 
    {
        // subsequent gap in range of missing list
        pthread_mutex_lock(&group->access_missing);
        add_sequences_2_missing(group,most_advanced,gap_size);
        pthread_mutex_unlock(&group->access_missing);
        return 1;
    }
    // subsequent gap out of range of missing window
    // clean out missing
    new_gap_start = seq_number - (MISSING_RANGE/2);
    //notify clients of permanent loss
    hdr.msg_type = PACKET_LOSS;
    body.alert_type = LOST_PACKETS;
    body.begin_seq_number = group->missing_lowest;
    body.end_seq_number = seq_number -1;
    body.primary_or_secondary = primary_or_secondary;
    notify_packet_loss(group,LOST_PACKETS,body.begin_seq_number,
        group->missing_count,primary_or_secondary);
    new_gap_size = seq_number - new_gap_start;
    pthread_mutex_lock(&group->access_missing);
    init_missing(group);
    // build new missing 
    add_sequences_2_missing(group,new_gap_start,new_gap_size);
    pthread_mutex_unlock(&group->access_missing);
    return 1;
};

/*----------------------------------------------------------------------*/
/* duplicate and gap detection; returns one of codes below              */
// return 0 - packet dropped
//            duplicate of itself or duplicated by other feed
// return 1 - immediate publish
//
/*----------------------------------------------------------------------*/
int need_2_publish(struct feed_group* const group, const int primary_or_secondary,
    const uint32_t seq_number)
{
    uint32_t        missing_range=0;
    register uint32_t        most_advanced=0;
    uint32_t        rc=0;
    register uint32_t        *other_expected;
    register uint32_t        *my_expected;
    int             loss_gap_size=0;
    struct msg_hdr  hdr;
    struct msg_body body;
/*  mea culpa */
/*    I apologize for this code exceeding the 25 lines per function rule */
/*    It is critical for low latency to avoid superflous function calls  */
/*    to be in compliance with the rules */
    my_expected = &(group->primary_expected_sequence);
    other_expected = &(group->secondary_expected_sequence);
    if (primary_or_secondary==1) 
    {
        other_expected = &(group->primary_expected_sequence);
        my_expected = &(group->secondary_expected_sequence);
    }
    if (*my_expected==0) 
    {
        *my_expected = seq_number;
    }
    if (group->in_sequence) 
    {
        /* lowest latency; most frequent code path*/
        if (seq_number==*my_expected) 
        {
            *my_expected +=1; //advance expected
            if(seq_number< *other_expected) return 0; //duplicate
            return 1; //faster feed; needs to be published
        } 
        else if (seq_number < *my_expected) 
        {
            return 0; //duplicate with itself;no publish or advance
        } 
        else 
        { //seq_number is > *my_expected
            if (seq_number == *other_expected) 
            { //this feed is faster
                *my_expected = seq_number+1; //advance expected
                return 1; //publish
            } 
            else if(seq_number < *other_expected) 
            {
                *my_expected = seq_number+1; //advance expected
                return 0; // duplicate; do not publish
            } 
            else 
            { //ooops! gap detected
#ifdef RATE_TESTING
                fprintf(stderr,"*** LIMIT FOUND GAPPING %d : %u\n"
                    ,primary_or_secondary,seq_number);
#endif
                most_advanced = *my_expected;
                if (*other_expected >most_advanced) 
                    most_advanced = *other_expected;
                missing_range = seq_number - most_advanced;
                first_gap(group,missing_range,seq_number,
                    most_advanced,primary_or_secondary);                
                *my_expected = seq_number + 1; //must follow first_gap
                return 1;
            }
        }
    } 
    else 
    {
        /* higher latency; less frequent code path*/
        if (seq_number > (group->missing_base+MISSING_RANGE))
        {   //always need to check if we have advanced out of the window
            //out of window; send notification; re-initialize to in sequence
            //very infrequenct operation depending on window size
            loss_gap_size = group->missing_highest - group->missing_lowest + 1;
            group->packets_lost_incidence++;                     //record stats
            group->unrecoverable_messages += group->missing_count;
            hdr.msg_type = PACKET_LOSS;
            body.alert_type = LOST_PACKETS;
            body.begin_seq_number = group->missing_lowest;
            body.end_seq_number = group->missing_highest;
            body.primary_or_secondary = primary_or_secondary;
            FH_LOG(LH,ERR,(" %s Packet Loss Detected at %d size %d %d",
                &(group->feed_name[0]),group->missing_lowest,loss_gap_size,
                primary_or_secondary));
            notify_packet_loss(group,LOST_PACKETS,body.begin_seq_number,
                loss_gap_size,primary_or_secondary);
            // clear the window
            init_missing(group);
            *my_expected = seq_number;
            set_in_sequence(group);
            // continue processing
        }
        if ((seq_number == *my_expected)&&(!(seq_number < *other_expected)))
        {
            //faster feed but not gapping; no need to check missing
            *my_expected +=1;
            return 1;
        }
        if ((seq_number >= *my_expected)&&(seq_number==*other_expected))
        {
            // suddenly faster feed but not gapping
            *my_expected = seq_number+1;
            return 1;
        }
        if ((seq_number > *my_expected)&&(seq_number > *other_expected))
        {
            //gapping again
            most_advanced = *my_expected;
            if (*other_expected >most_advanced)
                most_advanced = *other_expected;
            missing_range = seq_number - most_advanced;
            rc = second_gap(group,missing_range,seq_number,
               most_advanced,primary_or_secondary);
            *my_expected = seq_number+1;
            return rc;
        }
        if (is_sequence_in_missing(group,seq_number))
        {
            // low sequence already handled
            pthread_mutex_lock(&group->access_missing);
            remove_sequence_from_missing(group,seq_number);
            if (is_missing_empty(group)) 
            {
                init_missing(group);
                set_in_sequence(group);
            }
            pthread_mutex_unlock(&group->access_missing);
            if (seq_number >= *my_expected) *my_expected = seq_number+1;
            return 1;
        }
        // duplicated by other feed and not inSequence
    }
    return 0;
};
