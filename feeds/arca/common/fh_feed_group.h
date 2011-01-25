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
#ifndef __FH_FEED_GROUP_H
#define __FH_FEED_GROUP_H
/*********************************************************************/
/* file: fh_feed_group.h                                             */
/* Usage: properties for arca multicast line handler                 */
/* Author: Wally Matthews of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/* Inherited from Tervela Arca Multicast feed handler                */
/*   with mods to remove Tervela specific plug in properties         */
/*********************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include "queue.h"
#include "AB_Fast.h"
#include "fh_config.h"
#include "fh_arca_constants.h"
#include "fh_notify_constants.h"

struct feed_group 
{
    // all the possible child threads that may be configured
    //  if associated affinity is -1 then the thread isnt configured to run
    // Only required feed is the primary of the main feed; all secondaries
    //  are optional 
    // one or two symbol feeds is optional
    // retransmission requesting can be optional
    // measurement service is optional
    pthread_t arca_primary_feed;              //main feed REQUIRED
    pthread_t arca_publishing;                //publication thread OBSOLETE
    pthread_t arca_retrans_feed;              //Retrans feed    optional
    pthread_t arca_request_service;           //Request srvr    optional
    pthread_t arca_measurement_service;       //measurement srvr optional
    pid_t arca_main;                          //thread id of the main process

    //-----------------64 bit pointer values---------------------
    //-----------------arrays of 64 bit values ------------------
    uint64_t missing_sequence[MISSING_SIZE];  //window of missing sequence nums
    //------------------64 bit scalors---------------------------
    uint64_t feed_time;                 //time last packet on the feed
    uint64_t publish_time;              //time last pub from feed
    uint64_t origination_time;          //time feed handler was instantiated
    uint64_t missing_packet_incidence;  //incidences of missing packets
    uint64_t missing_message_range;     //aggregate range of missing packets
    uint64_t unrecoverable_messages;    //unrecovered messages
    uint64_t restored_message_incidence;//incidence of recovered messages
    uint64_t loss_of_in_sequence;       //incidence of sequence loss
    uint64_t restoral_of_in_sequence;   //incidence of sequence restoral
    uint64_t packets_lost_incidence;    //incidence of declaring packet loss
    uint64_t loss_of_feed_source;       //incidence of loss of feed
    uint64_t symbol_table_error;        //incidence of symbol table errors
    uint64_t firm_table_error;          //incidence of firm table errors
    uint64_t publication_failed;
    uint64_t publication_succeeded;
    //------------------statistics-------------------------------
    uint64_t pckt_rcvd_from_primary;
    uint64_t pckt_rcvd_from_secondary;
    uint64_t bytes_rcvd_from_primary;
    uint64_t bytes_rcvd_from_secondary;
    uint64_t msgs_rcvd_from_primary;
    uint64_t msgs_rcvd_from_secondary;
    uint64_t pkt_format_errors_primary;
    uint64_t pkt_format_errors_secondary;
    uint64_t pkt_duplicate_primary;
    uint64_t pkt_duplicate_secondary;
    uint64_t previous_primary;
    uint64_t previous_secondary;
    //------------------arrays of 32 bit values------------------
    //------------------32 bit scalors---------------------------
    uint32_t feed_group_space_size;       //allocated size for feed group
    uint32_t primary_expected_sequence;   //expected sequence of primary feed
    uint32_t secondary_expected_sequence; //expected sequence of secondary feed
    // missing parameters
    uint32_t missing_base;             //seq num base of the missing window
    uint32_t missing_count;            //number of missing sequence nums 
    uint32_t missing_lowest;           //lowest seq num in window
    uint32_t missing_highest;          //highest seq num in window
    // reference plugin parameters
    uint32_t maximum_sessions;         // needed by reference plugins 
    uint32_t maximum_symbols;          //  ditto
    uint32_t maximum_firms;            //  ditto
    uint32_t maximum_orders;           //  ditto
    // line identifier for notification
    uint32_t notification_line_id;     // needed by customer plugins
    // todays time
    time_t latest_mapping_update;
    // configured port numbers; 0 means not configured
    int  primary_mcast_port;
    int  secondary_mcast_port;
    int  primary_retran_tcp_port;
    int  primary_retran_mcast_port;
    int  secondary_retran_mcast_port;
    // open sockets; 0 means not supported
    int  primary_mcast_socket;
    int  secondary_mcast_socket;
    int  primary_retran_tcp_socket;
    int  primary_retran_mcast_socket;
    int  secondary_retran_mcast_socket;
    // affinity is the core that a thread will execute on; -1 no thread exec
    int  primary_mcast_affinity;
    int  publish_affinity;
    int  retrans_mcast_affinity;
    int  measurement_affinity;
    int  restart_time;                        //secs after midnight restart
    // --------------int arrays--------------------------------------------
    //---------------short int---------------------------------------------
    //---------------chars-------------------------------------------------
    // configured ip addresses of sockets
    char primary_mcast_ip_addrs[IPADRS_LENGTH];
    char secondary_mcast_ip_addrs[IPADRS_LENGTH];
    char primary_retran_tcp_ip_addrs[IPADRS_LENGTH];
    char primary_retran_mcast_ip_addrs[IPADRS_LENGTH];
    char secondary_retran_mcast_ip_addrs[IPADRS_LENGTH];
    // configured interfaces of sockets
    char primary_mcast_intfc[INTFC_LENGTH];
    char secondary_mcast_intfc[INTFC_LENGTH];
    char primary_request_intfc[INTFC_LENGTH]; 
    char primary_retran_intfc[INTFC_LENGTH];
    char secondary_retran_intfc[INTFC_LENGTH];
    //  feed_name (ie. ARCA_LISTED_AC , ARCA_ETF_NZ, )
    char feed_name[64];
    //  process_name
    char process_name[64];
    // sourceID is required to authenticate each retransmission request
    char source_id[21];                        //includes null term 
    // ---------unsigned chars------------------------
    unsigned char session_id;                  // ~== to line identifier
    unsigned char fast_mode;                   // 0 non compacted 1 compacted
    unsigned char request_or_interval;         // retrans request or interval
    unsigned char enable_restart;              // enable restart on time
    unsigned char purge_mode;                  // WIP out-of-order wind down
    unsigned char in_sequence;                 // out-of-order state
    unsigned char primary_on_line;             // primary feed state
    unsigned char secondary_on_line;           // mirrored feed state
    unsigned char process_halt;                // stop the process at earliest opportunity
    //  we want to avoid parsing costs for retrans when we are not target
    unsigned char expecting_retrans;            //  controls for retrans
    unsigned char expecting_book_refresh;
    unsigned char expecting_imbalance_refresh;
    // space to store unprocessed messages when strictOrdering
    char store_ring[STORED_MESSAGE_RING_BUFFER_SIZE];
    pthread_mutex_t access_missing;           // access control for missing
    pthread_mutex_t access_primary_sequence;   // access control for seq nums
    pthread_mutex_t access_secondary_sequence;
    // message buffers for processing
    char primary_fast_buffer[PACKET_MAX];
    char secondary_fast_buffer[PACKET_MAX];
    char primary_buffer[PACKET_MAX*4]; //fast encoding may need to expand
    char secondary_buffer[PACKET_MAX*4];
};
struct socket_set 
{ //set of mcast sockets for a thread to rotate through
    struct feed_group *feeds[SOCKET_SET_SIZE];  //pointer to group
    int               sockets[SOCKET_SET_SIZE]; //socket value  
    unsigned char     primary_or_secondary[SOCKET_SET_SIZE];
    unsigned char     socket_count;             //how many in struct
};
struct arca_process_args 
{ //arguments passed into a thread
    struct socket_set  *main_sockets;    //main feed thread
    struct socket_set  *retrans_sockets; //retrans feed thread
    int                *fini;            //global stop all processes 
    int                arca_version;     //version of arca protocol 
    short int          feed_cpu;         //main feed cpu
    short int          refresh_cpu;      //refresh cpu
    short int          retrans_cpu;      //retrans cpu
    short int          proc_idx;         //short process id
    char               process_name[PROCESS_NAME_MAX];
};
struct process_maps 
{ //used by config to map processes to lines
    struct feed_group* groups[10];       //feeds that process services
    int                feed_cpu;         //node dedicated to feeds
    int                refresh_cpu;      //node dedicated to refresh
    int                retrans_cpu;      //node dedicated to retrans  
    int                line_count;       //number of feeds for process
    int                max_symbols;      //max symbols in symbol mapping
    int                max_sessions;     //max sessions supported by process
    int                max_firms;        //max firms in firm mapping
    int                max_orders;       //max orders retained
    char               line_names[10][20]; //feed names
};

// external definitions needed for inline functions
extern int send_alert(struct feed_group *group, const int notificationtype);
extern uint64_t missing_modulos[64];

/*--------------------------------------------------------------------------*/
/* initialize the mutexen for a group                                       */
/*--------------------------------------------------------------------------*/
static inline void init_mutexes(struct feed_group * const group)
{
    pthread_mutex_init(&group->access_missing,NULL);
    pthread_mutex_init(&group->access_primary_sequence,NULL);
    pthread_mutex_init(&group->access_secondary_sequence,NULL);
};
/*--------------------------------------------------------------------------*/
/* destroy the mutexen for a group                                          */
/*--------------------------------------------------------------------------*/
static inline void destroy_mutexes(struct feed_group * const group)
{
    memset(&(group->missing_sequence[0]),0,(MISSING_SIZE*8));
    group->missing_base    = 0;
    group->missing_count   = 0;
    group->missing_lowest  = 0;
    group->missing_highest = 0;
    return;
};

struct feed_group* new_feed_group();
/*--------------------------------------------------------------------------*/
/* cleanup a group and return the memory to the heap                        */
/*--------------------------------------------------------------------------*/
static inline void destroy_group(struct feed_group * const group)
{
    destroy_mutexes(group);
    if (group->primary_mcast_socket!=0)
    {
        close(group->primary_mcast_socket);
        group->primary_mcast_socket = 0;
    }
    if (group->secondary_mcast_socket!=0)
    {
        close(group->secondary_mcast_socket);
        group->secondary_mcast_socket = 0;
    }
    if (group->primary_retran_tcp_socket!=0)
    {
        close(group->primary_retran_tcp_socket);
        group->primary_retran_tcp_socket = 0;
    }
    if (group->primary_retran_mcast_socket!=0)
    {
        close(group->primary_retran_mcast_socket);
        group->primary_retran_mcast_socket = 0;
    }
    if (group->secondary_retran_mcast_socket!=0)
    {
        close(group->secondary_retran_mcast_socket);
        group->secondary_retran_mcast_socket = 0;
    }
    free(group);
};
int default_config(struct feed_group * const group);
inline int get_prev_seq_number(struct feed_group * const group, 
    const int fast, 
    const int primary, const int session);

/*----------------------------------------------------------------------*/
/* intialize the missing sequence number members embedded in feed group */
/* missing members keep history of missing sequence numbers             */
/*----------------------------------------------------------------------*/
static inline void init_missing(struct feed_group * const group)
{
    memset(&(group->missing_sequence[0]),0,(MISSING_SIZE*8));
    group->missing_base    = 0;
    group->missing_count   = 0;
    group->missing_lowest  = 0;
    group->missing_highest = 0;
    return;
};
/*---------------------------------------------------------------------*/
/* set in sequence state which is the lowest latency path in the line  */
/*  handler                                                            */
/*---------------------------------------------------------------------*/
static inline void set_in_sequence(struct feed_group * const group)
{
    struct msg_hdr  hdr;
    struct msg_body body;

    group->in_sequence = 1;
    // Publish alert to tell subscribers that in sequence processing
    //   will resume; allow them to make decision about whether to
    //   resume trading or not
    hdr.msg_type = ALERT;
    body.alert_type = 1;
    send_alert(group,1);
    group->restoral_of_in_sequence++;
    return;
};
/*--------------------------------------------------------------------*/
/* set out of sequence state which is a slower latency path in the    */
/*  line handler.                                                     */
/*--------------------------------------------------------------------*/
static inline void set_out_of_sequence(struct feed_group * const group)
{
    struct msg_hdr  hdr;
    struct msg_body body;

    group->in_sequence = 0;
    // Publish alert to tell subscribers that packets are being received
    //   out of order; allow them to make decision about whether to
    //   stop trading or not
    hdr.msg_type = ALERT;
    body.alert_type = 1;
    send_alert(group,1);
    group->loss_of_in_sequence++;
    return;
};
/*----------------------------------------------------------------------*/
/* test if gaps are filled                                              */
/*----------------------------------------------------------------------*/
static inline int  is_missing_empty(const struct feed_group * const group)
{
    if (group->missing_count==0) return 1;
    return 0;
};
/*--------------------------------------------------------------------*/
/* return 1 if this sequence number is for a previously missed packet */
/*--------------------------------------------------------------------*/
static inline int is_sequence_in_missing(
    const struct feed_group * const group,
    const uint32_t sequence_number)
{
    uint32_t modulo_val=0;
    uint32_t offset=0;
    uint32_t index=0;
    uint64_t mask = 0UL;

    if ((group->in_sequence) ||
      (sequence_number >= (group->missing_lowest + MISSING_RANGE)) ||
      (sequence_number < group->missing_lowest))
    {
        return 0;
    }
    if (sequence_number<group->missing_base)
    {
        return 0;
    }
    modulo_val = sequence_number - group->missing_base;
    offset = modulo_val >> 6;
    index = modulo_val % 64;
    mask = missing_modulos[index];
    if ((group->missing_sequence[offset] & mask) !=0) return 1;
    return 0;
};
/*-------------------------------------------------------------------*/
/* is this sequence number the lowest missing sequence number        */
/* return 1 if it is and 0 otherwise                                 */
/*-------------------------------------------------------------------*/
static inline int is_low_sequence_in_missing(
    const struct feed_group * const group,
    const uint32_t sequence_number)
{
    if (sequence_number==group->missing_lowest) return 1;
    return 0;
};
/*-------------------------------------------------------------------*/
/* remove this sequence number from the list of missing sequence     */
/* numbers                                                           */
/*-------------------------------------------------------------------*/
static inline int remove_sequence_from_missing(
    struct feed_group * const group,
    const uint32_t sequence_number)
{
    int ix =0;
    uint32_t modulo_val = 0;
    uint32_t offset     = 0;
    uint32_t index      = 0;
    uint64_t mask;

    if (group->missing_count==0)
    {
        return -1;
    }
    if (sequence_number < group->missing_base)
    {
        return -1;
    }
    if (sequence_number >= (group->missing_base + MISSING_RANGE)||
        (sequence_number < group->missing_base)) return -1;
    modulo_val = sequence_number - group->missing_base;
    offset = modulo_val >> 6;
    index = modulo_val % 64;
    mask = missing_modulos[index];
    group->missing_sequence[offset] &= ~mask;
    group->missing_count--;
    if (sequence_number<=group->missing_lowest)
    {
        if (group->missing_count>0)
        {
            ix = sequence_number+1;
            while (ix <= (int)group->missing_highest)
            {
                if (is_sequence_in_missing(group,ix))
                {
                    group->missing_lowest = ix;
                    break;
                }
                ix++;
            }
        }
        else
        {
           group->missing_lowest = 0;
        }
    }
    return 0;
};
int add_sequences_2_missing(struct feed_group * const group, 
    const uint32_t first_sequence, const uint32_t count);
int start_book_refresh(struct feed_group * const group);
int stop_book_refresh(struct feed_group * const group);
/*------------------------------------------------------------------*/
/* build status word for alerts or publication                      */
/*------------------------------------------------------------------*/
static inline unsigned long int build_summary_status(
    const struct feed_group * const group)
{
    unsigned long int summary_status = group->notification_line_id;
    if (group->in_sequence)
        summary_status |= INSEQUENCE_SUMMARY;
    if (group->unrecoverable_messages ==0)
        summary_status |= NO_PACKET_LOSS_SUMMARY;
    if (group->primary_on_line)
        summary_status |= PRIMARY_FEED_UP;
    if (group->secondary_on_line)
        summary_status |= SECONDARY_FEED_UP;
    if (group->primary_retran_tcp_socket>0)
        summary_status |= REREQUEST_FEED_UP;
    if (group->unrecoverable_messages > (PACKET_LOSS_MASK))
        summary_status |= EXTREME_PACKET_LOSS;
    summary_status |= group->unrecoverable_messages & PACKET_LOSS_MASK;
    return summary_status;
};

static inline int copy_cfg_parms(const struct feed_group *src, 
    struct feed_group * const dest)
{
    dest->maximum_sessions = src->maximum_sessions;
    dest->maximum_symbols = src->maximum_symbols;
    dest->maximum_firms = src->maximum_firms;
    dest->maximum_orders = src->maximum_orders;
    dest->primary_mcast_port = src->primary_mcast_port;
    dest->secondary_mcast_port = src->secondary_mcast_port;
    dest->primary_retran_tcp_port = src->primary_retran_tcp_port;
    dest->primary_retran_mcast_port = src->primary_retran_mcast_port;
    dest->secondary_retran_mcast_port = src->secondary_retran_mcast_port;
    dest->primary_mcast_affinity = src->primary_mcast_affinity;
    dest->publish_affinity = src->publish_affinity;
    dest->measurement_affinity = src->measurement_affinity;
//------- primary & secondary feeds
    memcpy(&(dest->primary_mcast_ip_addrs[0]),
        &(src->primary_mcast_ip_addrs[0]),IPADRS_LENGTH);
    memcpy(&(dest->primary_mcast_intfc[0]),
        &(src->primary_mcast_intfc[0]),INTFC_LENGTH);
    memcpy(&(dest->secondary_mcast_ip_addrs[0]),
        &(src->secondary_mcast_ip_addrs[0]),IPADRS_LENGTH);
    memcpy(&(dest->secondary_mcast_intfc[0]),
        &(src->secondary_mcast_intfc[0]),INTFC_LENGTH);
//------- primary retrans requestor
    memcpy(&(dest->primary_retran_tcp_ip_addrs[0]),
        &(src->primary_retran_tcp_ip_addrs[0]),IPADRS_LENGTH);
    memcpy(dest->primary_request_intfc,src->primary_request_intfc,INTFC_LENGTH);
//------- primary & secondary retransmission feeds
    memcpy(&(dest->primary_retran_mcast_ip_addrs[0]),
        &(src->primary_retran_mcast_ip_addrs[0]),IPADRS_LENGTH);
    memcpy(dest->primary_retran_intfc,src->primary_retran_intfc,INTFC_LENGTH);
    memcpy(&(dest->secondary_retran_mcast_ip_addrs[0]),
        &(src->secondary_retran_mcast_ip_addrs[0]),IPADRS_LENGTH);
    memcpy(dest->secondary_retran_intfc,src->secondary_retran_intfc,
        INTFC_LENGTH);
//------- feed name & process name
    memcpy(dest->feed_name,src->feed_name,64);
    memcpy(dest->process_name,src->process_name,64);
//--------source id to authorize requests from exchange
    memcpy(dest->source_id,src->source_id,21);
//------------- copy state variables
    dest->fast_mode = src->fast_mode;
    dest->request_or_interval = src->request_or_interval;
    dest->enable_restart = src->enable_restart;
    dest->purge_mode = src->purge_mode;
    dest->in_sequence = src->in_sequence;
    dest->primary_on_line = src->primary_on_line;
    dest->secondary_on_line = src->secondary_on_line;
    return 0;
};
char* get_uncompacted_mesg(struct feed_group *group, char* uncompacted,
    char* input, int* compactsize, int* uncompactedsize, int origsize,
    FAST_STATE *state, unsigned short mesgtype );
int rcv_retrans_packets(struct feed_group *group, int primary_or_secondary, 
    int socket);
int feed_handler_thread(void* data);
int refresh_handler_thread(void* data);
int symbol_handler_thread(void* data);
int retrans_handler_thread(void* data);
int request_handler_thread(void* data);
int measurement_handler_thread(void* data);
int management_handler_thread(void* data);
#endif //FH_FEED_GROUP_H
