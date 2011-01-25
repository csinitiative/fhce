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

#ifndef __ARCATRADE_HEADERS_H__
#define __ARCATRADE_HEADERS_H__

/*********************************************************************/
/* file: fh_arcabook_headers.h                                       */
/* Usage: internal headers for components of arca mulitcast handler  */
/* Author: Wally Matthews & Ross Cooperman of                        */
/*   Collaborative Software Initiative                               */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// Arca FH headers
#include "fh_arca_constants.h"
#include "fh_feed_group.h"

/*----------------------------------------------------------------------------*/

int initialize_plug_ins(struct process_maps *p_map);
// plug in that initializes all the plug in infra-structure

int shutdown_plug_ins();
// plug in that gracefully shuts down plug in infra-structure

inline int msg_init(struct feed_group *group, struct msg_hdr *hdr,
    struct msg_body *body, char** msg_space, int* space_size);
// initialize space to send a message

int publish_packet_loss(const struct msg_body *body);
//  plug in for publication of packet loss events

int publish_feed_alert(const struct msg_body *body);
//  plug in for publication of state change events

int publish_start_of_day(struct feed_group *group, const struct msg_hdr *hdr,
  struct msg_body *body);
//  plug in for publication of start of day message body

int publish_trade(struct feed_group *group, const struct msg_hdr *hdr,
  struct msg_body *body);
//  plug in for publication trade message body

int publish_trade_cancel(struct feed_group *group, const struct msg_hdr *hdr,
  struct msg_body *body);
//  plug in for publication trade message cancel body

int publish_trade_correction(struct feed_group *group, const struct msg_hdr *hdr,
  struct msg_body *body);
//  plug in for publication trade correction message body
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
inline int parse_sequence_number_reset(struct msg_body* body, const char* msg_ptr, 
    const int msglngth);
//parse a sequence number reset message

inline int parse_message_unavailable(struct msg_body* body, const char* msg_ptr, 
    const int msglngth);
//parse a message unavailable message

inline int parse_trade(struct msg_body* body, const char* msg_ptr, const int msglngth);
//parse a trade message

inline int parse_trade_cancel(struct msg_body* body, const char* msg_ptr, 
    const int msglngth);
//parse a trade cancel message

inline int parse_trade_correction(struct msg_body* body, const char* msg_ptr, 
    const int msglngth);
//parse a trade correction message
/*----------------------------------------------------------------------------*/
//profiling functions
void init_loop_profile();
void print_loop_profile();
void init_drain_profile();
void print_drain_profile();
void init_message_profile();
void print_message_profile();

#endif /* __ARCATRADE_HEADERS_H__ */

