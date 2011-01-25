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
#include "fh_arcabook_headers.h"
#include "fh_arca_headers.h"
#include "fh_data_conversions.h"
#include "profiling.h"

// Exchange-provided FAST codec headers
#include "ArcaL2Msg.h"

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

#if ARCA_ADD_ORDER_PROFILE

#define ARCA_ADD_ORDER_TRIGGER 10000
char* add_order_profile_name = "add_order_profile";
int add_order_trigger = ARCA_ADD_ORDER_TRIGGER;
int add_order_count = 0;
FH_PROF_DECL(add_order_profile_name,ARCA_ADD_ORDER_TRIGGER,50,1);
void init_add_order_profile() 
{
    FH_PROF_INIT(add_order_profile_name);
};
void print_add_order_profile() i
{
    FH_PROF_PRINT(add_order_profile_name);
};

#endif

#if ARCA_MOD_ORDER_PROFILE

#define ARCA_MOD_ORDER_TRIGGER 10000

char* mod_order_profile_name = "mod_order_profile";
int mod_order_trigger = ARCA_MOD_ORDER_TRIGGER;
int mod_order_count = 0;
FH_PROF_DECL(mod_order_profile_name,ARCA_MOD_ORDER_TRIGGER,50,1);
void init_mod_order_profile() 
{
    FH_PROF_INIT(mod_order_profile_name);
};
void print_mod_order_profile() 
{
    FH_PROF_PRINT(mod_order_profile_name);
};

#endif

#if ARCA_DEL_ORDER_PROFILE

#define ARCA_DEL_ORDER_TRIGGER 10000
char* del_order_profile_name = "del_order_profile";
int del_order_trigger = ARCA_DEL_ORDER_TRIGGER;
int del_order_count = 0;
FH_PROF_DECL(del_order_profile_name,ARCA_DEL_ORDER_TRIGGER,50,1);
void init_del_order_profile() 
{
    FH_PROF_INIT(del_order_profile_name);
};
void print_del_order_profile() 
{
    FH_PROF_PRINT(del_order_profile_name);
};

#endif

#if ARCA_IMBALANCE_PROFILE

#define ARCA_IMBALANCE_TRIGGER 1000
char* imbalance_profile_name = "imbalance_profile";
int imbalance_trigger = ARCA_IMBALANCE_TRIGGER;
int imbalance_count = 0;
FH_PROF_DECL(imbalance_profile_name,ARCA_IMBALANCE_TRIGGER,50,1);
void init_imbalance_profile() 
{
    FH_PROF_INIT(imbalance_profile_name);
};
void print_imbalance_profile() 
{
    FH_PROF_PRINT(imbalance_profile_name);
};

#endif

#if ARCA_SYMBOL_MAP_PROFILE

#define ARCA_SYMBOL_MAP_TRIGGER 100

char* symbol_map_profile_name = "symbol_map_profile";
int symbol_map_trigger = ARCA_SYMBOL_MAP_TRIGGER;
int symbol_map_count = 0;
FH_PROF_DECL(symbol_map_profile_name,ARCA_SYMBOL_MAP_TRIGGER,50,1);
void init_symbol_map_profile() {
    FH_PROF_INIT(symbol_map_profile_name);
};
void print_symbol_map_profile() {
    FH_PROF_PRINT(symbol_map_profile_name);
};

#endif

#if ARCA_FIRM_MAP_PROFILE

#define ARCA_FIRM_MAP_TRIGGER 100
char* firm_map_profile_name = "firm_map_profile";
int firm_map_trigger = ARCA_FIRM_MAP_TRIGGER;
int firm_map_count = 0;
FH_PROF_DECL(firm_map_profile_name,ARCA_FIRM_MAP_TRIGGER,50,1);
void init_firm_map_profile() 
{
    FH_PROF_INIT(firm_map_profile_name);
};
void print_firm_map_profile()
{
    FH_PROF_PRINT(firm_map_profile_name);
};

#endif

#if ARCA_IMBALANCE_REFRESH_PROFILE

#define ARCA_IMBALANCE_RERESH_TRIGGER 100

char* imbalance_refresh_profile_name = "imbalance_refresh_profile";
int imbalance_refresh_trigger = ARCA_IMBALANCE_RERESH_TRIGGER;
int imbalance_refresh_count = 0;
FH_PROF_DECL(imbalance_refresh_profile_name,ARCA_IMBALANCE_RERESH_TRIGGER,50,1);
void init_imbalance_refresh_profile()
{
    FH_PROF_INIT(imbalance_refresh_profile_name);
};
void print_imbalance_refresh_profile()
{
    FH_PROF_PRINT(imbalance_refresh_profile_name);
};

#endif

#if ARCA_BOOK_REFRESH_PROFILE

#define ARCA_BOOK_REFRESH_TRIGGER 100

char* book_refresh_profile_name = "book_refresh_profile";
int book_refresh_trigger = ARCA_BOOK_REFRESH_TRIGGER;
int book_refresh_count = 0;
FH_PROF_DECL(book_refresh_profile_name,ARCA_BOOK_REFRESH_TRIGGER,50,1);
void init_book_refresh_profile()
{
    FH_PROF_INIT(book_refresh_profile_name);
};
void print_book_refresh_profile()
{
    FH_PROF_PRINT(book_refresh_profile_name);
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

static fh_plugin_hook_t plug_add_symbol;  //hook for book refresh use

/*------------------------------------------------------------------------------------------*/
/* parse sequence number reset message                                                      */
/*------------------------------------------------------------------------------------------*/
inline int parse_sequence_number_reset(struct msg_body* body, char* msg_ptr, int msglngth)
{
    if (msglngth < (int) sizeof(ArcaL2SequenceReset_t)) 
    {
        return 0;
    }
    body->next_seq_number = big_endian_32(msg_ptr);
    return sizeof(ArcaL2SequenceReset_t);
};
/*------------------------------------------------------------------------------------------*/
/* parse message unavailable message                                                        */
/*------------------------------------------------------------------------------------------*/
inline int parse_message_unavailable(struct msg_body* body, char* msg_ptr, int msglngth)
{
    //HACK WARNING ArcaL2Msg.h which comes from the exchange is inconsistent
    //  about inclusion of headers in some messages so I have to massage the length
    //  to get to the correct solution
    static int body_length=(int) (sizeof(ArcaL2Unavailable_t)-sizeof(ArcaL2Header_t));
    if (msglngth < body_length) 
    {
        return 0;
    }
    body->begin_seq_number = big_endian_32(msg_ptr);
    body->end_seq_number = big_endian_32(msg_ptr+4);
    return body_length;
};
/*------------------------------------------------------------------------------------------*/
/* parse book refresh message                                                               */
/*------------------------------------------------------------------------------------------*/
inline int parse_book_refresh(struct msg_body* body, char* msg_ptr, int msglngth)
{
    if(msglngth < (int) sizeof(ArcaL2BookOrder_t)) 
    {
        return 0;
    }
    body->source_seq_num = big_endian_32(msg_ptr);
    body->source_time = big_endian_32(msg_ptr+4);
    body->order_id = big_endian_32(msg_ptr+8);
    body->volume = big_endian_32(msg_ptr+12);
    body->price_numerator = big_endian_32(msg_ptr+16);
    body->price_scale_code = (unsigned char) *(msg_ptr+20);
    body->side = (unsigned char) *(msg_ptr+21);
    body->exchange_id = *(msg_ptr+22);
    body->security_type = *(msg_ptr+23);
    body->firm_index = big_endian_16(msg_ptr+24);
    return sizeof(ArcaL2BookOrder_t);
};

/*------------------------------------------------------------------------------------------*/
/* parse imbalance refresh message                                                          */
/*------------------------------------------------------------------------------------------*/
inline int parse_imbalance_refresh(struct msg_body* body, char* msg_ptr, int msglngth)
{
    if(msglngth < (int) sizeof(ArcaL2Imbalance_t))
    {
        return 0;
    }
    body->symbol_index = big_endian_16(msg_ptr);
    body->msg_type = big_endian_16(msg_ptr+2);
    body->source_seq_num = big_endian_32(msg_ptr+4);
    body->source_time = big_endian_32(msg_ptr+8);
    body->volume = big_endian_32(msg_ptr+12);
    body->total_imbalance = big_endian_32(msg_ptr+16);
    body->market_imbalance = big_endian_32(msg_ptr+20);
    body->price_numerator = big_endian_32(msg_ptr+24);
    body->price_scale_code = (unsigned char) *(msg_ptr+28);
    body->auction_type = *(msg_ptr+29);
    body->exchange_id = *(msg_ptr+30);
    body->security_type = *(msg_ptr+31);
    body->session_id = (unsigned char) *(msg_ptr+32);
    body->auction_time = big_endian_16(msg_ptr+34);
    return sizeof(ArcaL2Imbalance_t);
};
/*------------------------------------------------------------------------------------------*/
/* parse symbol mapping message                                                             */
/*------------------------------------------------------------------------------------------*/
inline int parse_symbol_mapping(struct msg_body* body, char* msg_ptr, int msglngth)
{
    if(msglngth < (int) sizeof(ArcaL2SymbolUpdate_t)) 
    {
        return 0;
    }
    body->symbol_index = big_endian_16(msg_ptr);
    body->session_id = (unsigned char) *(msg_ptr+2);
    memcpy(&(body->symbol),msg_ptr+4,ARCABOOK_SYMBOL_LENGTH);
    return sizeof(ArcaL2SymbolUpdate_t);
};
/*------------------------------------------------------------------------------------------*/
/* parse symbol clear message                                                               */
/*------------------------------------------------------------------------------------------*/
inline int parse_symbol_clear(struct msg_body* body, char* msg_ptr, int msglngth)
{
    if(msglngth < (int) sizeof(ArcaL2SymbolClear_t)) 
    {
        return 0;
    }
    body->next_seq_number = big_endian_32(msg_ptr);
    body->symbol_index = big_endian_16(msg_ptr+4);
    body->session_id = (unsigned char) *(msg_ptr+6);
    return sizeof(ArcaL2SymbolClear_t);
};
/*------------------------------------------------------------------------------------------*/
/* parse firm mapping message                                                               */
/*------------------------------------------------------------------------------------------*/
inline int parse_firm_mapping(struct msg_body* body, char* msg_ptr, int msglngth)
{
    if(msglngth < (int) sizeof(ArcaL2FirmUpdate_t)) 
    {
        return 0;
    }
    body->firm_index = big_endian_16(msg_ptr);
    memcpy(&(body->firm),msg_ptr+7,ARCABOOK_ATTRIBUTION_LENGTH);
    return sizeof(ArcaL2FirmUpdate_t);
};
/*------------------------------------------------------------------------------------------*/
/* parse  add order, modify order, delete order, imbalance messages                         */
/*------------------------------------------------------------------------------------------*/
inline int parse_orders(struct msg_body* body, char* msg_ptr, int msglngth)
{
    int minlngth = (int) sizeof(ArcaL2Delete_t);
    if(msglngth < minlngth) 
    {  //prevent segfault for a runt message
        return 0;
    }
    body->symbol_index = big_endian_16(msg_ptr);
    body->msg_type = big_endian_16(msg_ptr+2);
    body->source_seq_num = big_endian_32(msg_ptr+4);
    body->source_time = big_endian_32(msg_ptr+8);
    switch(body->msg_type) 
    {
    case MODIFY_ORDER:  //use same format
    case ADD_ORDER:{
            minlngth = (int) sizeof(ArcaL2Add_t);
            if(msglngth<minlngth)
            { //prevent segfault for a runt message
                return 0;
            }
            body->order_id = big_endian_32(msg_ptr+12);
            body->volume = big_endian_32(msg_ptr+16);
            body->price_numerator = big_endian_32(msg_ptr+20);
            body->price_scale_code = *(msg_ptr+24);
            body->side = *(msg_ptr+25);
            body->exchange_id = *(msg_ptr+26);
            body->security_type = *(msg_ptr+27);
            body->session_id = *(msg_ptr+30);
            body->firm_index = big_endian_16(msg_ptr+28);
            break;
        }
    case DELETE_ORDER:{  //checked length before switch statement
            body->order_id = big_endian_32(msg_ptr+12);
            body->side = *(msg_ptr+16);
            body->exchange_id = *(msg_ptr+17);
            body->security_type = *(msg_ptr+18);
            body->session_id = *(msg_ptr+19);
            body->firm_index = big_endian_16(msg_ptr+20);
            break;
        }
    case IMBALANCE:
    {
            minlngth = (int) sizeof(ArcaL2Imbalance_t);
            if(msglngth <  minlngth)
            { //prevent segfault for a runt message
                return 0;
            }
            body->volume = big_endian_32(msg_ptr+12);
            body->total_imbalance = big_endian_32(msg_ptr+16);
            body->market_imbalance = big_endian_32(msg_ptr+20);
            body->price_numerator = big_endian_32(msg_ptr+24);
            body->price_scale_code = *(msg_ptr+28);
            body->auction_type = *(msg_ptr+29);
            body->exchange_id = *(msg_ptr+30);
            body->security_type = *(msg_ptr+31);
            body->session_id = *(msg_ptr+32);
            body->auction_time = big_endian_16(msg_ptr+34);
            break;
    }
    default:
    {
            return 1;  //unexpected message type; only happens if exchange adds new message type OR mistake on msg gen
    }
    }
    return minlngth;
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

    FH_STATUS rc = 0;
    int bytes_consumed = 0;

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
        case BOOK_REFRESH:{
#if ARCA_BOOK_REFRESH_PROFILE
            FH_PROF_BEG(book_refresh_profile_name);
#endif
            bytes_consumed = parse_book_refresh(body,msg_ptr,*body_sze);
            if (bytes_consumed==0) { 
                break; //parsing error; runt packet            
            }
            body->session_id = hdr->session_id;
            body->symbol_index = hdr->symbol_index;
            if ((hdr->current_refresh_msg_seq==0) &&(body_count==0)) {
                if (plug_add_symbol) {
                    memcpy(body->symbol,hdr->symbol,ARCABOOK_SYMBOL_LENGTH+1);
                    plug_add_symbol(&rc,body);
                    if (rc!=FH_OK) {
                        group->symbol_table_error++;
                    }                    
                }             
            }
            publish_book_refresh(group,hdr,body);
#if ARCA_BOOK_REFRESH_PROFILE
            FH_PROF_END(book_refresh_profile_name); 
            book_refresh_count += 1;
            if(book_refresh_count > book_refresh_trigger)
            {
                FH_PROF_PRINT(book_refresh_profile_name);
                book_refresh_count = 0;
            }
#endif
            break;
        }
        case IMBALANCE_REFRESH:{                                
#if ARCA_IMBALANCE_REFRESH_PROFILE
            FH_PROF_BEG(imbalance_refresh_profile_name);
#endif
            bytes_consumed = parse_imbalance_refresh(body,msg_ptr,*body_sze);
            if (bytes_consumed==0) { 
                break; //parsing error; runt packet            
            }
            publish_imbalance_refresh(group,hdr,body);
#if ARCA_IMBALANCE_REFRESH_PROFILE
            FH_PROF_END(imbalance_refresh_profile_name);
            imbalance_refresh_count += 1;            
            if(imbalance_refresh_count > imbalance_refresh_trigger)
            {
                FH_PROF_PRINT(imbalance_refresh_profile_name);
                imbalance_refresh_count = 0;
            }
#endif
            break;
        }
        case SYMBOL_MAPPING:{
#if ARCA_SYMBOL_MAP_PROFILE
            FH_PROF_BEG(symbol_map_profile_name);
#endif
            bytes_consumed = parse_symbol_mapping(body,msg_ptr,*body_sze);
            if (bytes_consumed==0) { 
                break; //parsing error; runt packet
            }
            publish_symbol_mapping(group,hdr,body);
#if ARCA_SYMBOL_MAP_PROFILE
            FH_PROF_END(symbol_map_profile_name);
            symbol_map_count += 1;
            if (symbol_map_count > symbol_map_trigger) {
                FH_PROF_PRINT(symbol_map_profile_name);
                symbol_map_count = 0;
            }
#endif
            break;
        }
        case SYMBOL_CLEAR:{
            bytes_consumed = parse_symbol_clear(body,msg_ptr,*body_sze);
            if (bytes_consumed==0) { 
                break; //parsing error; runt packet
            }
            publish_symbol_clear(group,hdr,body);
            break;
        }
        case FIRM_MAPPING:{
#if ARCA_FIRM_MAP_PROFILE
            FH_PROF_BEG(firm_map_profile_name);
#endif
            bytes_consumed = parse_firm_mapping(body,msg_ptr,*body_sze);
            if (bytes_consumed==0) { 
                break; //parsing error; runt packet           
            }
            publish_firm_mapping(group,hdr,body);
#if ARCA_FIRM_MAP_PROFILE
            FH_PROF_END(firm_map_profile_name);
            firm_map_count += 1;
            if (firm_map_count > firm_map_trigger) {
                FH_PROF_PRINT(firm_map_profile_name);
                firm_map_count = 0;
            }
#endif
            break;
        }
        case ORDERS:{
            bytes_consumed = parse_orders(body,msg_ptr,*body_sze);
            if (bytes_consumed==0) {
                break;//parsing error; runt message
            }
            switch(body->msg_type){
                case ADD_ORDER:{
#if ARCA_ADD_ORDER_PROFILE
                    FH_PROF_BEG(add_order_profile_name);
#endif
                    publish_add_order(group,hdr,body);
#if ARCA_ADD_ORDER_PROFILE
                    FH_PROF_END(add_order_profile_name);
                    add_order_count += 1;
                    if (add_order_count > add_order_trigger) {
                        FH_PROF_PRINT(add_order_profile_name);
                        add_order_count = 0;
                    }
#endif
                    break;
                }
                case MODIFY_ORDER:{
#if ARCA_MOD_ORDER_PROFILE
                    FH_PROF_BEG(mod_order_profile_name);
#endif
                    publish_mod_order(group,hdr,body);
#if ARCA_MOD_ORDER_PROFILE
                    FH_PROF_END(mod_order_profile_name);
                    mod_order_count += 1;
                    if (mod_order_count > mod_order_trigger) {
                         FH_PROF_PRINT(mod_order_profile_name);
                         mod_order_count = 1;
                    }
#endif
                    break;
                }
                case DELETE_ORDER:{
#if ARCA_DEL_ORDER_PROFILE
                    FH_PROF_BEG(del_order_profile_name);
#endif

                    publish_del_order(group,hdr,body);
#if ARCA_DEL_ORDER_PROFILE
                    FH_PROF_END(del_order_profile_name);
                    del_order_count += 1;
                    if (del_order_count > del_order_trigger) {
                        FH_PROF_PRINT(del_order_profile_name);
                        del_order_count = 0;
                    }
#endif
                    break;
                }
                case IMBALANCE:{
#if ARCA_IMBALANCE_PROFILE
                    FH_PROF_BEG(imbalance_profile_name);
#endif
                    publish_imbalance(group,hdr,body);
#if ARCA_IMBALANCE_PROFILE
                    FH_PROF_END(imbalance_profile_name);
                    imbalance_count += 1;
                    if (imbalance_count > imbalance_trigger) {
                        FH_PROF_PRINT(imbalance_profile_name);
                        imbalance_count = 0;
                    }
#endif
                    break;
                }
                default: {
                    //invalid order message type
                    FH_LOG(LH,ERR,("Rcvd invalid order msg type %d at %d",
                        body->msg_type,hdr->msg_seq_num));
                    notify_packet_loss(group,LOST_PACKETS,body->msg_seq_num,1,
                        primary_or_secondary);
                }
            }
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
