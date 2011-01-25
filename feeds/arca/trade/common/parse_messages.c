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
/* file: parse_messages.c                                            */
/* Usage: message parsing for arca feed handler                      */
/* Author: Wally Matthews of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */                        /*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <string.h>

// Common FH headers
#include "fh_log.h"
#include "fh_plugin.h"
#include "fh_cpu.h"
#include "fh_prof.h"
#include "fh_hist.h"

// Arca FH headers
#include "fh_feed_group.h"
#include "fh_arcatrade_headers.h"
#include "fh_arca_headers.h"
#include "fh_data_conversions.h"
#include "fh_arca_util.h"
#include "profiling.h"

#if ARCA_MESSAGE_PROFILE

#define ARCA_MESSAGE_TRIGGER 10000
char*  message_profile_name = "parse_message_profile";
int message_profile_trigger = ARCA_MESSAGE_TRIGGER;
int message_profile_count = 0;
FH_PROF_DECL(message_profile_name,ARCA_MESSAGE_TRIGGER,50,1);
void init_message_profile() 
{
    FH_PROF_INIT(message_profile_name);
};
void print_message_profile() 
{
    FH_PROF_PRINT(message_profile_name);
};

#endif

// static data
static fh_plugin_hook_t          plug_add_symbol;

/*! \brief Cache hooks that this module will need
 */
void fh_arca_msgparse_cache_hooks()
{
    plug_add_symbol = fh_plugin_get_hook(FH_PLUGIN_ADD_SYMBOL_BY_INDEX);
};

/*------------------------------------------------------------------------------------------*/
/* parse sequence number reset message                                                      */
/*------------------------------------------------------------------------------------------*/
inline int parse_sequence_number_reset(struct msg_body* body, const char* msg_ptr, 
    const int msglngth)
{
    if (msglngth < (int) SEQUENCE_RESET) 
    {
        return 0;
    }
    body->next_seq_number = big_endian_32(msg_ptr);
    return SEQUENCE_RESET;
};
/*------------------------------------------------------------------------------------------*/
/* parse message unavailable message                                                        */
/*------------------------------------------------------------------------------------------*/
inline int parse_message_unavailable(struct msg_body* body, const char* msg_ptr, 
    const int msglngth)
{
    if (msglngth < (int) MESSAGE_UNAVAILABLE_LENGTH) 
    {
        return 0;
    }
    body->begin_seq_number = big_endian_32(msg_ptr);
    body->end_seq_number = big_endian_32(msg_ptr+4);
    return MESSAGE_UNAVAILABLE_LENGTH;
};
inline int parse_trade(struct msg_body* body, const char* msg_ptr, const int msglngth)
{
    if (msglngth < (int) TRADE_LENGTH)
    {
        return 0;
    }
    body->source_time = big_endian_32(msg_ptr);
    body->buy_side_link_id = big_endian_32(msg_ptr+4);
    body->sell_side_link_id = big_endian_32(msg_ptr+8);
    body->price_numerator = big_endian_32(msg_ptr+12);
    body->volume = big_endian_32(msg_ptr+16);
    body->source_seq_num = big_endian_32(msg_ptr+20);
    body->session_id = *(msg_ptr+24);
    body->price_scale_code = (unsigned char) *(msg_ptr+25);
    body->exchange_id = *(msg_ptr+26);
    body->security_type = *(msg_ptr+27);
    body->trade_cond_1 = *(msg_ptr+28);
    body->trade_cond_2 = *(msg_ptr+29);
    body->trade_cond_3 = *(msg_ptr+30);
    body->trade_cond_4 = *(msg_ptr+31);
    body->quote_link_id = big_endian_32(msg_ptr+48);
    memcpy(&body->symbol,msg_ptr+32,ARCABOOK_SYMBOL_LENGTH);
    body->price = make_price(body->price_scale_code,body->price_numerator);
    return TRADE_LENGTH;
};
inline int parse_trade_cancel(struct msg_body* body, const char* msg_ptr, const int msglngth)
{
    if (msglngth < (int) TRADE_CANCEL_LENGTH)
    {
        return 0;
    }
    body->source_time = big_endian_32(msg_ptr);
    body->source_seq_num = big_endian_32(msg_ptr+4);
    body->original_src_seq_num = big_endian_32(msg_ptr+8);
    body->session_id = *(msg_ptr+12);
    body->exchange_id = *(msg_ptr+13);
    body->security_type = *(msg_ptr+14);
    memcpy(&body->symbol,msg_ptr+16,ARCABOOK_SYMBOL_LENGTH);
    return TRADE_CANCEL_LENGTH;
};
inline int parse_trade_correction(struct msg_body* body, const char* msg_ptr, 
    const int msglngth)
{
    if (msglngth < (int) TRADE_CORRECTION_LENGTH)
    {
        return 0;
    }
    body->source_time = big_endian_32(msg_ptr);
    body->buy_side_link_id = big_endian_32(msg_ptr+4);
    body->sell_side_link_id = big_endian_32(msg_ptr+8);
    body->price_numerator = big_endian_32(msg_ptr+12);
    body->volume = big_endian_32(msg_ptr+16);
    body->source_seq_num = big_endian_32(msg_ptr+20);
    body->original_src_seq_num = big_endian_32(msg_ptr+24);
    body->session_id = *(msg_ptr+28);
    body->price_scale_code = (unsigned char) *(msg_ptr+29);
    body->exchange_id = *(msg_ptr+30);
    body->security_type = *(msg_ptr+31);
    body->trade_cond_1 = *(msg_ptr+32);
    body->trade_cond_2 = *(msg_ptr+33);
    body->trade_cond_3 = *(msg_ptr+34);
    body->trade_cond_4 = *(msg_ptr+35);
    body->quote_link_id = big_endian_32(msg_ptr+52);
    memcpy(&body->symbol,msg_ptr+36,ARCABOOK_SYMBOL_LENGTH);
    body->price = make_price(body->price_scale_code,body->price_numerator);
    return TRADE_CORRECTION_LENGTH;
};
/*------------------------------------------------------------------------------------------*/
/* parse a message pointed at by msg_ptr into body for a maximum size of body_sze           */
/*  return the number of bytes consumed or 0 if an error                                    */
/*------------------------------------------------------------------------------------------*/
int parse_mesg(struct feed_group* const group, char* const msg_ptr, struct msg_hdr* const hdr, 
    struct msg_body* const body, int* body_sze, const int primary_or_secondary, 
    const int body_count)
{
    //mea culpa: sorry that this violates the 25 line rule; switch has too many cases

    int bytes_consumed = 0;

    if(body_count){} //body_count is used in ArcaBook; must maintain same profile 
#if ARCA_MESSAGE_PROFILE
    FH_PROF_BEG(message_profile_name);
#endif

    memset(body,0,sizeof(struct msg_body)); //initialize the redundant fields in the struct
    body->msg_seq_num = hdr->msg_seq_num; //initialize seq num for body from hdr
    switch (hdr->msg_type) {
        case SEQUENCE_NUMBER_RESET:{
            bytes_consumed = parse_sequence_number_reset(body,msg_ptr,*body_sze);
            if (bytes_consumed==0) { 
                break; //parsing error; runt packet
            }
            //force both expected sequence number to next
            // just in case the message is lost in one feed or the other
            if (group->primary_expected_sequence != body->next_seq_number) {
                group->primary_expected_sequence = body->next_seq_number;
            }
            if (group->secondary_expected_sequence != body->next_seq_number) {
                group->secondary_expected_sequence = body->next_seq_number;
            }
            // NOTE sequence number reset will be published for each feed
            // This is being done on purpose so that a subscriber
            //   can confirm that it was seen on both feeds
            publish_start_of_day(group,hdr,body);
            //group->process_halt = 1;
            break;
        }
        case MESSAGE_UNAVAILABLE:{
            bytes_consumed = parse_message_unavailable(body,msg_ptr,*body_sze);
            if (bytes_consumed==0) { 
                break; //parsing error; runt packet
            }
            body->alert_type =  EXCHANGE_LOST_PACKETS; 
            notify_packet_loss(group,EXCHANGE_LOST_PACKETS,
                body->begin_seq_number,(body->end_seq_number-body->begin_seq_number),    
                primary_or_secondary);
            break;
        }
        case TRADE:{
            bytes_consumed = parse_trade(body,msg_ptr,*body_sze);
            if(bytes_consumed==0) {
                break; //parsing error; runt packet
            }
            publish_trade(group,hdr,body);
            break;
        }
        case TRADE_CANCEL:{
            bytes_consumed = parse_trade_cancel(body,msg_ptr,*body_sze);
            if(bytes_consumed==0) {
                break; //parsing error; runt packet
            }
            publish_trade_cancel(group,hdr,body);
            break;
        }
        case TRADE_CORRECTION:{
            bytes_consumed = parse_trade_correction(body,msg_ptr,*body_sze);
            if(bytes_consumed==0) {
                break; //parsing error; runt packet
            }
            publish_trade_correction(group,hdr,body);
            break;
        }
        default:{
            // invalid message type
            FH_LOG(LH,ERR,(" Rcvd invalid msg type %d at %d",
                hdr->msg_type,body->msg_seq_num));
            bytes_consumed = 0; //do not know how big; flush all
            notify_packet_loss(group,LOST_PACKETS,body->msg_seq_num,1,
                primary_or_secondary);
        }
    }
#if ARCA_MESSAGE_PROFILE
    FH_PROF_END(message_profile_name);
    message_profile_count += 1;
    if (message_profile_count > ARCA_MESSAGE_TRIGGER) {
       FH_PROF_PRINT(message_profile_name);
       message_profile_count = 0; 
    }
#endif
    return bytes_consumed;
};
