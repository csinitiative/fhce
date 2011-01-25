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

#ifndef __ARCA_HEADERS_H__
#define __ARCA_HEADERS_H__

/*********************************************************************/
/* file: fh_arca_headers.h                                           */
/* Usage: internal headers for components of arca mulitcast handler  */
/* Author: Wally Matthews & Ross Cooperman of                        */
/*   Collaborative Software Initiative                               */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// Arca FH headers
#include "fh_arca_constants.h"
#include "AB_Fast.h"
#include "fh_config.h"
//#include "fh_feed_group.h"

#define ARCA_LOOP_PROFILE              (0)  //profile receive loop with select
#define ARCA_DRAIN_PROFILE             (0)  //profile receive loop wo select

struct feed_group;
struct socket_set;
struct arca_process_args;
struct process_maps;

uint32_t hex2int(char* ascii);    //convert ascii string to 32 bit hex value

int rcv_loop(const struct socket_set * const service_set, int * const fini);
// handles select loop and draining of sockets
/*----------------------------------------------------------------------------*/

int get_packet(struct feed_group * const group, const int socket,
    const int primary_or_secondary);
// receives the packet and processes the packet

int get_body_size(const unsigned char msg_type);
//  return the size of the msg_type

int default_cfg(struct feed_group* const group);
//  hard wired defaults to be used when other config plug ins fail or are
//  not fully implemented

int process_packet(struct feed_group* const group, struct msg_hdr* const pkthdr,
    char* const pkt_ptr, int pkt_size, const uint64_t rcv_time, 
    const int primary_or_secondary);

/*----------------------------------------------------------------------------*/
/* headers needed to do gap and duplicate detection                           */
/*   side effects are state changes; subscriber notifications, strict ordering*/
int need_2_publish(struct feed_group* const group, const int primary_or_secondary,
  const uint32_t seq_number);
//  see discussion of strict ordering; line handler may defer publishing
//  when strict ordering is enabled

inline int first_gap(struct feed_group* const group, const uint32_t gap_size,
    const uint32_t seq_number, const uint32_t most_advanced, 
    const int primary_or_secondary);

inline int second_gap(struct feed_group* const group, const uint32_t gap_size,
    const uint32_t seq_number,const uint32_t most_advanced,
    const int primary_or_secondary);
//  helper function for need_2_publish

/*----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*/
int parse_private_configs(struct feed_group *group);   //OBSOLETE
//  plug in to parse private configs for a feed group
//  private configs are tuning parameters that are not shared with
//  casual users but may be shared with power users
//  an example would be affinity core values that have to be adjusted
//  depending upon the hardware environment and customer usage for other
//  threads/processes 
/*----------------------------------------------------------------------------*/

int parse_xml_line(char* lineBuffer,char *first_tag, char* value, 
    char* second_tag);
//  utility function for private config parsing

/*----------------------------------------------------------------------------*/
//.. headers for parsing packet header or message bodies
int parse_packet_hdr(struct msg_hdr* const pkthdr, const char* const pkt_ptr, 
    const int pktlngth, const uint64_t rcv_time);
//  parsing packet header from pkt_ptr to pkthdr; returns the hdr length

int parse_error(struct feed_group * const group, const int sequence, 
    const int num_bodies, const int missing, const int primary_or_secondary);
// parsing error occurred; invalid msg_type is about all we detect in line handler

/*----------------------------------------------------------------------------*/

inline const FAST_STATE* get_pointer2initial_fast();
//accessor to initial fast state 

/*----------------------------------------------------------------------------*/
//.. headers for configuration processing
int get_config(const fh_cfg_node_t *config, struct process_maps *p_map);
//.. get configuration for a process from a unified configuration file

int init_mcast_sockets(const struct arca_process_args* args);
//.. initialize all mcast sockets, join mcast groups, enable interfaces

int init_request_sockets(const struct arca_process_args* args);
//.. initialize all tcp request sockets

int build_request_socket(const char* tcp_address, const int tcp_port, 
    const char* intfc_name);
//.. build a ready to transmit from tcp request socket

inline void chk_cfg(struct process_maps *p_map);
// have derived feed sanity check the configuration

void fh_arca_version_info(int version);
// version info is feed specific

void fh_arca_msgparse_cache_hooks();  // hooks for parse_messages module

void fh_arca_pub_cache_hooks();  // hooks for publication

inline char* get_build_date();  //build info is feed specific

inline char* get_build_rev();

inline char* get_config_path();

inline char* get_plugin_dir_name();

int parse_mesg(struct feed_group* const group, char* const msg_ptr, 
    struct msg_hdr* const hdr, struct msg_body* const body, int* body_sze, 
    const int primary_or_secondary, const int body_count);
// parse a message pointed at by msg_ptr into body for a max size of body_sze
// return the number of bytes of the message or 0 if error

int runt_packet_error(struct feed_group * const group, const int sequence, 
    const int num_bodies, const int missing, const int primary_or_secondary);
// runt packet error occurred;if num_bodies 0 header insufficient for sequence
// to be useful

inline int msg_flush();
// synchronize flushing of output stream that packs multiple messages

int notify_packet_loss(struct feed_group * const group,
    const int notificationtype, const int missing_sequence, const int gapsize,
    const int primary_or_secondary);
//  send a packet loss alert; includes line handler state changes

int send_alert(struct feed_group * const group, const int notificationtype);
//  send a short alert; includes line handler state changes

//int unit_test_main();

#endif /* __ARCA_HEADERS_H__ */

