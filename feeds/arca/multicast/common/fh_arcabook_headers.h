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

#ifndef __ARCABOOK_HEADERS_H__
#define __ARCABOOK_HEADERS_H__

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
/* headers for functions that call plugins*/

void build_tables(struct process_maps *p_map);

int initialize_plug_ins(struct process_maps *p_map);
// plug in that initializes all the plug in infra-structure

int shutdown_plug_ins();
// plug in that gracefully shuts down plug in infra-structure

inline int msg_init(struct feed_group *group, struct msg_hdr *hdr,
    struct msg_body *body, char** msg_space, int* space_size);
// initialize space to send a message

inline int msg_send(char* msg_space, int space_size, struct msg_body *body);
// send a message

int publish_symbol_clear(struct feed_group *group, const struct msg_hdr *hdr,
    struct msg_body *body);
//  plug in for publication of symbol clear message body

int publish_book_refresh(struct feed_group *group, struct msg_hdr *hdr,
    struct msg_body *body);
// plug in for publication of book refresh message body

int publish_imbalance_refresh(struct feed_group *group, struct msg_hdr *hdr,
    struct msg_body *body);
// plug in for publication of imbalance refresh message body

int publish_symbol_mapping(struct feed_group *group, struct msg_hdr *hdr,
    struct msg_body *body);
// plug in for publication of symbol mapping message body

int publish_firm_mapping(struct feed_group *group, struct msg_hdr *hdr,
    struct msg_body *body);
// plug in for publication of firm mapping message body

int publish_add_order(struct feed_group *group, struct msg_hdr *hdr, 
  struct msg_body *body);
//  plug in for publication of add order message body

int publish_mod_order(struct feed_group *group, struct msg_hdr *hdr,
  struct msg_body *body);
//  plug in for publication of modify order message body

int publish_del_order(struct feed_group *group, struct msg_hdr *hdr,
  struct msg_body *body);
//  plug in for publication of delete order message body

int publish_imbalance(struct feed_group *group, struct msg_hdr *hdr,
  struct msg_body *body);
//  plug in for publication of imbalance message body

int publish_packet_loss(struct msg_body *body);
//  plug in for publication of packet loss events

int publish_feed_alert(struct msg_body *body);
//  plug in for publication of state change events

int publish_start_of_day(struct feed_group *group, struct msg_hdr *hdr,
  struct msg_body *body);
//  plug in for publication of start of day message body
/*----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
inline int parse_sequence_number_reset(struct msg_body* body, char* msg_ptr, int msglngth);
//parse a sequence number reset message

inline int parse_message_unavailable(struct msg_body* body, char* msg_ptr, int msglngth);
//parse a message unavailable message

inline int parse_book_refresh(struct msg_body* body, char* msg_ptr, int msglngth);
//parse a book refresh message

inline int parse_imbalance_refresh(struct msg_body* body, char* msg_ptr, int msglngth);
//parse a imbalance refresh message

inline int parse_symbol_mapping(struct msg_body* body, char* msg_ptr, int msglngth);
//parse a symbol mapping message

inline int parse_symbol_clear(struct msg_body* body, char* msg_ptr, int msglngth);
//parse a symbol clear message

inline int parse_firm_mapping(struct msg_body* body, char* msg_ptr, int msglngth);
//parse a firm mapping message

inline int parse_orders(struct msg_body* body, char* msg_ptr, int msglngth);
//parse order messages

/*----------------------------------------------------------------------------*/
//profiling functions
void init_loop_profile();
void print_loop_profile();
void init_drain_profile();
void print_drain_profile();
void init_message_profile();
void print_message_profile();
void init_add_order_profile();
void print_add_order_profile();
void init_mod_order_profile();
void print_mod_order_profile();
void init_del_order_profile();
void print_del_order_profile();
void init_imbalance_profile();
void print_imbalance_profile();
void init_symbol_map_profile();
void print_symbol_map_profile();
void init_firm_map_profile();
void print_firm_map_profile();

#endif /* __ARCABOOK_HEADERS_H__ */

