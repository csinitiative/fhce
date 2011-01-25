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
/* file: rcv_feed_packets.c                                          */
/* Usage: line handler processing of arca feed handler               */
/* Author: Wally Matthews of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */                        /*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <string.h>
#include <errno.h>

// Common FH headers
#include "fh_udp.h"
#include "fh_errors.h"
#include "fh_log.h"
#include "fh_cpu.h"
#include "fh_prof.h"
#include "fh_hist.h"

// Arca FH headers
#include "fh_arca_constants.h"
#include "fh_arca_headers.h"
#include "fh_feed_group.h"
#include "fh_data_conversions.h"
//#include "profiling.h"

// Exchange-provided FAST codec headers
#include "ArcaL2Msg.h"
#include "AB_Fast.h"

//#define DEBUG_RCV_LOOP 1

#if ARCA_LOOP_PROFILE

#define ARCA_LOOP_TRIGGER 1000
char*  loop_profile_name = "receive_loop_profile";
int loop_profile_trigger = ARCA_LOOP_TRIGGER;
int loop_profile_count   = 0;
FH_PROF_DECL(loop_profile_name,ARCA_LOOP_TRIGGER,50,100);
void init_loop_profile() 
{
    FH_PROF_INIT(loop_profile_name);
};
void print_loop_profile() 
{
    FH_PROF_PRINT(loop_profile_name);
};

#endif

#if ARCA_DRAIN_PROFILE

#define ARCA_DRAIN_TRIGGER 1000
char* drain_profile_name = "socket_drain_profile";
int drain_profile_trigger = ARCA_DRAIN_TRIGGER;
int drain_profile_count = 0;
FH_PROF_DECL(drain_profile_name,ARCA_DRAIN_TRIGGER,50,100);
void init_drain_profile()
{
    FH_PROF_INIT(drain_profile_name);
};
void print_drain_profile() 
{
    FH_PROF_PRINT(drain_profile_name);
};

#endif

// initialization state for fast decoding
// explicitly visible rather than the normal #define
// that is not visible in source code. The author prefers
// to have it visible for code readability and debugging
static const FAST_STATE fastStateInit[AB_MAX_FIELD] =  
{
    { AB_MSG_TYPE,0,OP_NONE,0,{0}},
    { AB_STOCK_IDX,0,OP_COPY,0,{0}},
    { AB_SEQUENCE,0,OP_INCR,0,{0}},
    { AB_TIME,0,OP_COPY,0,{0}},
    { AB_ORDER_ID,0,OP_COPY,0,{0}},
    { AB_VOLUME,0,OP_COPY,0,{0}},
    { AB_PRICE,0,OP_COPY,0,{0}},
    { AB_PRICE_SCALE,0,OP_COPY,0,{0}},
    { AB_BUY_SELL,0,OP_COPY,0,{0}},
    { AB_EXCH_ID,0,OP_COPY,0,{0}},
    { AB_SECURITY_TYPE,0,OP_COPY,0,{0}},
    { AB_FIRM_ID,0,OP_COPY,0,{0}},
    { AB_SESSION_ID,0,OP_COPY,0,{0}},
    { AB_BITMAP,0,OP_NONE,0,{0}}
};
inline const  FAST_STATE* get_pointer2initial_fast()
{
    return &(fastStateInit[0]);
};

/*-------------------------------------------------------------------------*/
/* process a packet: will have msg_type already parsed                     */
/*  return -1 for failure; 0 otherwise                                     */
/*-------------------------------------------------------------------------*/
int process_packet(struct feed_group* const group, struct msg_hdr* const pkthdr,
    char* const pkt_ptr, int pkt_size, const uint64_t rcv_time, 
    const int primary_or_secondary)
{
    int      pkt_action=0;
    int      uncompacted_size=0;
    int      hdr_size=0;
    int      rc=0;
    int      body_size=0;
    int      remaining2process=0;
    int      msg_count=0;
    int      bytes_consumed=0;
    uint32_t *my_expected=0; //need to filter out stale sequence numbers 
             //for cases 3,4,5
    char     *msg_ptr=0;
    char     *decoding_output=0;
    char     *compacted=0;
    char     *uncompacted=0; //pointers to buffers being used
    struct   msg_body body;
    static   FAST_STATE state[AB_MAX_FIELD];

    pkthdr->num_body_entries = *(pkt_ptr+NUMBER_BODIES_OFFSET);
    if (primary_or_secondary==0) {
        compacted = &(group->primary_fast_buffer[0]);
        uncompacted = &(group->primary_buffer[0]);
        my_expected = &(group->primary_expected_sequence);
        group->pckt_rcvd_from_primary++;
        group->bytes_rcvd_from_primary += pkt_size;
        group->msgs_rcvd_from_primary += pkthdr->num_body_entries;
    } 
    else 
    {                  
        compacted = &(group->secondary_fast_buffer[0]);
        uncompacted = &(group->secondary_buffer[0]);
        my_expected = &(group->secondary_expected_sequence);
        group->pckt_rcvd_from_secondary++;
        group->bytes_rcvd_from_secondary += pkt_size;
        group->msgs_rcvd_from_secondary += pkthdr->num_body_entries;
    }
    pkthdr->msg_seq_num = big_endian_32(pkt_ptr+MSG_NUM_OFFSET);
    if (pkthdr->msg_type !=1) {    
        pkt_action = need_2_publish(group,primary_or_secondary,pkthdr->msg_seq_num);
    } 
    else 
    {
        pkt_action = 1; // bypass gap detection and dupe detection 
        //  for sequence number reset messages; THEY HAVE TO BE PROCESSED
    }
    if(pkt_action==0) {
        if(primary_or_secondary==0) 
        {
            group->pkt_duplicate_primary++;
        }
        else
        {
            group->pkt_duplicate_secondary++;
        }
#ifdef DEBUG
        fprintf(stdout," Debug Duplicate pri/sec=%d seq=%u\n",
            primary_or_secondary,pkthdr->msg_seq_num);
#endif
        return 0;  //duplicate packet; avoid any further work
    }

    /* Not a duplicate packet; we can invest more effort into it*/
    pkthdr->msg_rcv_time = rcv_time;
    hdr_size = parse_packet_hdr(pkthdr,pkt_ptr,pkt_size,rcv_time);
#ifdef DEBUG
    fprintf(stdout,
        " Debug rcv packet pri/sec=%d size=%d type=%u seq=%u sent %u msgs=%u act=%d\n",
        primary_or_secondary,pkt_size,pkthdr->msg_type,pkthdr->msg_seq_num,
        pkthdr->send_time,pkthdr->num_body_entries,pkt_action);
#endif
    if(group->fast_mode==1) 
    {  //fast decode the packet; even if we store it      
        memcpy(uncompacted,compacted,hdr_size); //header is not compacted
        remaining2process = pkt_size -= hdr_size;
        uncompacted_size = hdr_size;
        msg_ptr = pkt_ptr+hdr_size;
        decoding_output = uncompacted + hdr_size;
        msg_count = 0;        
        //initialize the fast encoding state 
        memcpy(&state,fastStateInit,sizeof(fastStateInit));
        //we decode all the fields into the uncompacted buffer
        while ((remaining2process>0)&&(msg_count<pkthdr->num_body_entries)) 
        {
            body.msg_type = pkthdr->msg_type;
            bytes_consumed = remaining2process;
            rc = ABFastDecode((union ArcaL2MsgUnion*)decoding_output,
                              (uint8_t*) msg_ptr,
                              (int32_t*) &bytes_consumed,
                              (uint16_t*)&(body.msg_type),
                              state);
            if (rc != AB_OK) {
                // decoding error 
                FH_LOG(LH,ERR,(" %s Fast Decode Fail seq %d side %d",
                    &(group->feed_name[0]),pkthdr->msg_seq_num,
                    primary_or_secondary));
                return -1;
            }
            body_size = get_body_size(body.msg_type); //msg size uncompacted
            uncompacted_size+=body_size;
            msg_ptr+=bytes_consumed; //adjust ptr for compacted
            decoding_output+=body_size; //adumst ptr for uncompacted
            remaining2process-=bytes_consumed; //reduce the residual size
        }
        //Why here? Because when in strict ordering we want to incur performance 
        // as we push to store list to avoid it as we pull from store list
    } 
    else 
    {
        uncompacted_size = pkt_size;        
    }
    //current message needs to be published       
    remaining2process = uncompacted_size - hdr_size;
    msg_ptr = uncompacted + hdr_size;
    msg_count=0;
    //publish message by message
    while ((remaining2process>0)&&(msg_count < pkthdr->num_body_entries)) 
    {
        // parse&publish a message at a time 
        body_size = parse_mesg(group, msg_ptr, pkthdr, &body,
            &remaining2process, primary_or_secondary, msg_count);
        if (body_size==0) 
        {
            // parsing error
            FH_LOG(LH,ERR,(" %s Packet parse Fail seq %d side %d",
                &(group->feed_name[0]),pkthdr->msg_seq_num,
                primary_or_secondary));
            if (primary_or_secondary==0) 
            {
                group->pkt_format_errors_primary++;
            } 
            else 
            {
                group->pkt_format_errors_secondary++;
            }
            parse_error(group,pkthdr->msg_seq_num,pkthdr->num_body_entries,
                (pkthdr->num_body_entries - msg_count), primary_or_secondary);
            break;
        } 
        else if (body_size==1) 
        {
            FH_LOG(LH,ERR,(" %s Runt Packet error seq %d side %d",
                &(group->feed_name[0]),pkthdr->msg_seq_num,                    
                primary_or_secondary));
            if (primary_or_secondary==0) {
                group->pkt_format_errors_primary++;
            } 
            else 
            {
                group->pkt_format_errors_secondary++;
            }
            runt_packet_error(group,pkthdr->msg_seq_num,
                pkthdr->num_body_entries,(pkthdr->num_body_entries -
                msg_count), primary_or_secondary);
            break;
        }
        remaining2process -= body_size; //reduce the residual
        msg_ptr += body_size; //advance the message pointer
        msg_count++; //advance the message count
    }
    msg_flush();
    return 0;
};
/*-------------------------------------------------------------------------*/
/* receive loop for the main feed: all sockets have joined the mcast group */
/* fini is a universal signal to shutdown:                                 */
/* return other than 0 is failure                                          */
/*-------------------------------------------------------------------------*/
int rcv_loop(const struct socket_set * const service_set, int * const fini)
{
    struct timeval  tv;
    fd_set          rdfs;
    fd_set          real_fds;
    int             i;
    int             highest_fd              = 0;
    int             my_socket               = 0;
    int             socket_we_are_servicing = 0;
    int             last_socket_in_cycle    = 0;
    int             first_socket_in_cyle    = 0;
    int             found_active_socket     = 0;
    int             pending                 = 0;
    int             rc                      = 0;

    FD_ZERO(&real_fds);
#ifdef DEBUG_RCV_LOOP
    fprintf(stdout," Debug read_fds contains [");
#endif
    for (i = 0; i < service_set->socket_count; i++) 
    {
        my_socket = service_set->sockets[i];
        FD_SET(my_socket, &real_fds);
#ifdef DEBUG_RCV_LOOP
        fprintf(stdout,",%d",my_socket);
#endif
        if (my_socket > highest_fd) 
        {
            highest_fd = my_socket;
        }
    }
#ifdef DEBUG_RCV_LOOP
    fprintf(stdout,"] highest is %d\n",highest_fd);
#endif
#if ARCA_LOOP_PROFILE
    loop_profile_count = 0;
    init_loop_profile();
#endif
#if ARCA_DRAIN_PROFILE
    drain_profile_count = 0;
    init_drain_profile();
#endif
#if ARCA_MESSAGE_PROFILE
    init_message_profile();
#endif
#if ARCA_ADD_ORDER_PROFILE
    init_add_order_profile();
#endif
#if ARCA_MOD_ORDER_PROFILE
    init_mod_order_profile();
#endif
#if ARCA_DEL_ORDER_PROFILE
    init_del_order_profile();
#endif
#if ARCA_IMBALANCE_PROFILE
    init_imbalance_profile();
#endif
#if ARCA_SYMBOL_MAP_PROFILE
    init_symbol_map_profile();
#endif
#if ARCA_FIRM_MAP_PROFILE
    init_firm_map_profile();
#endif
    /* main select loop*/
    while (*fini==0) 
    {
#if ARCA_LOOP_PROFILE
        FH_PROF_BEG(loop_profile_name);
#endif
        memcpy(&rdfs,&real_fds,sizeof(fd_set)); //refresh rdfs
        errno = 0; //clean out any pre-existing values
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        rc = select(highest_fd + 1, &rdfs, NULL, NULL, &tv);
#if ARCA_DRAIN_PROFILE
        FH_PROF_BEG(drain_profile_name);
#endif
        if (rc == -1 && errno != EINTR) 
        {
            // select error
            FH_LOG(LH, ERR, ("select failure: %s (%d)", strerror(errno), errno));
#ifdef DEBUG_RCV_LOOP
            fprintf(stdout," Debug select error %s\n",strerror(errno));
#endif
            continue;
        }
        else if (rc == 0 || errno == EINTR)
        {
            // time out; try again
#ifdef DEBUG_RCV_LOOP
            fprintf(stdout," Debug select timeout %s\n",strerror(errno));
#endif
            continue;
        }
        
        //good select below here
        //locate the socket that starts the cycle
        for (i = 0; i < service_set->socket_count; i++) 
        {
            if (FD_ISSET(service_set->sockets[i], &rdfs) != 0) {
                last_socket_in_cycle = socket_we_are_servicing = i;
                break;
            }
        }
        if (i >= service_set->socket_count) 
        {
#ifdef DEBUG_RCV_LOOP
            fprintf(stdout," Debug select but no one has data\n");
#endif
            continue;
        }
#ifdef DEBUG_RCV_LOOP
        fprintf(stdout," Debug after select first socket index is %d\n",socket_we_are_servicing);
        fprintf(stdout," Debug is pri/sec %d\n",service_set->primary_or_secondary[socket_we_are_servicing]);
#endif
        // receive and service the packet
        rc = get_packet(service_set->feeds[socket_we_are_servicing],
            service_set->sockets[socket_we_are_servicing],
            service_set->primary_or_secondary[socket_we_are_servicing]);
        // while there are sockets with packets ready to process
        //  round robin draining those sockets as fast as possible
        //  round robin drains them uniformly         
        found_active_socket = 1; //irregardless of an error        
        /* loop to drain packets */
        while ((found_active_socket==1)&&(*fini==0)) 
        { //avoid select until necessary
            found_active_socket = 0;
            //once around the loop each time we find an active socket
            socket_we_are_servicing = last_socket_in_cycle+1;
            if (socket_we_are_servicing>=service_set->socket_count) 
            {
                socket_we_are_servicing = 0;
            }
            first_socket_in_cyle = socket_we_are_servicing; //remember 
            // cycle to the top of the array
            while ((socket_we_are_servicing < service_set->socket_count)&&(*fini==0)) 
            {
                my_socket = service_set->sockets[socket_we_are_servicing];
#ifdef DEBUG_RCV_LOOP
                fprintf(stdout," Debug upper loop my_socket %d index %d\n",my_socket,socket_we_are_servicing);
#endif
                // see if socket has any packets for us
                if (ioctl(my_socket,FIONREAD,&pending)>=0 && pending>0) 
                {
#ifdef DEBUG_RCV_LOOP
                    fprintf(stdout," Debug after ioctl setting active socket\n");
#endif
                    rc=get_packet(service_set->feeds[socket_we_are_servicing],
                        my_socket,
                        service_set->primary_or_secondary[socket_we_are_servicing]);
                    if (service_set->feeds[socket_we_are_servicing]->process_halt) 
                    {
                        *fini=1;
                    }                    
                    found_active_socket = 1;
                }
                socket_we_are_servicing++;
            } //while socket_we_are servicing
            socket_we_are_servicing = 0;
            // conditionally cycle at the beginning of the array
            if (first_socket_in_cyle > 0) 
            {
                while ((socket_we_are_servicing <= last_socket_in_cycle)
                    &&(*fini==0)) 
                {
                    my_socket = service_set->sockets[socket_we_are_servicing];
#ifdef DEBUG_RCV_LOOP
                    fprintf(stdout," Debug lower loop my_socket %d index %d\n",my_socket,socket_we_are_servicing);
#endif
                    // see if socket has any packets for us
                    if (ioctl(my_socket,FIONREAD,&pending)>=0 && pending>0) 
                    {
#ifdef DEBUG_RCV_LOOP
                        fprintf(stdout," Debug after ioctl setting active socket\n");
#endif
                        rc=get_packet(service_set->feeds[socket_we_are_servicing],
                            my_socket,
                            service_set->primary_or_secondary[socket_we_are_servicing]);
                        if (service_set->feeds[socket_we_are_servicing]->process_halt) 
                        {
                            *fini=1;
                        }                     
                        found_active_socket = 1;
                    }
                    socket_we_are_servicing++;
                } //while socket_we_are_servicing
            } //if first_socket_in_cycle
        } //while found_active_socket
#if ARCA_LOOP_PROFILE
        FH_PROF_END(loop_profile_name);
        loop_profile_count += 1;
        if (loop_profile_count > loop_profile_trigger) 
        {
            FH_PROF_PRINT(loop_profile_name);
            loop_profile_count = 0;
        }
#endif
#if ARCA_DRAIN_PROFILE
        FH_PROF_END(drain_profile_name);
        drain_profile_count += 1;
        if (drain_profile_count > drain_profile_trigger) 
        {
            FH_PROF_PRINT(drain_profile_name);
            drain_profile_count = 0;
        }
#endif
    } //while *fini==0
#ifdef DEBUG_RCV_LOOP
    fprintf(stdout," Debug exiting rcv loop fini is %d\n",*fini);
#endif
#if ARCA_LOOP_PROFILE
    print_loop_profile();
#endif
#if ARCA_DRAIN_PROFILE
    print_drain_profile();
#endif
#if ARCA_MESSAGE_PROFILE
    print_message_profile();
#endif
#if ARCA_ADD_ORDER_PROFILE
    print_add_order_profile();
#endif
#if ARCA_MOD_ORDER_PROFILE
    print_mod_order_profile();
#endif
#if ARCA_DEL_ORDER_PROFILE
    print_del_order_profile();
#endif
#if ARCA_IMBALANCE_PROFILE
    print_imbalance_profile();
#endif
#if ARCA_SYMBOL_MAP_PROFILE
    print_symbol_map_profile();
#endif
#if ARCA_FIRM_MAP_PROFILE
    print_firm_map_profile();
#endif
    return 0;
};
/*-------------------------------------------------------------------------*/
/* get a packet and process it                                             */
/*-------------------------------------------------------------------------*/
int get_packet(struct feed_group * const group, const int socket, 
    const int primary_or_secondary)
{
    struct msg_hdr     hdr;
    int                pkt_size = 0;
    int                rc=0;
    char*              pkt_buffer = NULL;
    struct sockaddr_in from_addr;  // packet source
    uint64_t           rcv_time=0; // receive time from socket
    uint32_t           ifindex=0;  // ifindex not used necessary for fh-udp_recv
    uint32_t           ifaddress=0;// ifaddress not used "        "     "

    // get the correct buffer space to use
    if (primary_or_secondary==0) 
    {
        if (group->fast_mode==1) 
        {
            pkt_buffer = &(group->primary_fast_buffer[0]);
        } 
        else 
        {
            pkt_buffer = &(group->primary_buffer[0]);
        }
    } else {
        if (group->fast_mode==1) 
        {
            pkt_buffer = &(group->secondary_fast_buffer[0]);
        } 
        else 
        {
            pkt_buffer = &(group->secondary_buffer[0]);
        }
    }
    // get the packet from the socket
    pkt_size = fh_udp_recv(socket,pkt_buffer,PACKET_MAX,&from_addr,&ifindex,&ifaddress,&rcv_time);
#ifdef DEBUG_RCV_LOOP
    fprintf(stdout," Debug get_packet received %d bytes for %d socket\n",pkt_size,socket);
#endif
    //record possible errors
    if (pkt_size < 0) {
        if (primary_or_secondary==0) 
        {
            group->pkt_format_errors_primary++;
        } 
        else 
        {
            group->pkt_format_errors_secondary++;
        }        
        return 0; //not much else we can do but move on
    }
    if (pkt_size < ARCAM_MSG_HDR_SIZE) 
    {
        runt_packet_error(group,0,0,0,primary_or_secondary);
    }
    //clear out redundant fields in struct
    memset(&hdr,0,sizeof(struct msg_hdr)); 
    hdr.msg_type = big_endian_16(pkt_buffer+MSG_TYPE_OFFSET);
    if (hdr.msg_type == 2) 
    { //heartbeat message
#ifdef DEBUG
        fprintf(stdout," HeartBeat %d: %u\n",
            primary_or_secondary,big_endian_32(pkt_buffer+MSG_NUM_OFFSET));
#endif
        return 0;
    }
    // process the packet
    rc = process_packet(group, &hdr, pkt_buffer, pkt_size, rcv_time,
        primary_or_secondary);
    return rc;
};
