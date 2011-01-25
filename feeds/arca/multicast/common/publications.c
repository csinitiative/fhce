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
/* file: fh_arca_cfg.c                                               */
/* Usage: configuration support for arca feed handler                */
/* Author: Wally Matthews Collaborative Software Initiative          */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */                        /*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <stdio.h>
#include <string.h>

// Common FH headers
#include "fh_plugin.h"
#include "fh_log.h"

// Arca FH headers
#include "fh_arca_constants.h"
#include "fh_arcabook_headers.h"
#include "fh_feed_group.h"

// reference plug in hooks
static fh_plugin_hook_t plug_add_symbol;
static fh_plugin_hook_t plug_add_firm;
static fh_plugin_hook_t plug_add_order;
static fh_plugin_hook_t plug_mod_order;
static fh_plugin_hook_t plug_del_order;
static fh_plugin_hook_t plug_del_symbol;
static fh_plugin_hook_t plug_lookup_symbol;
static fh_plugin_hook_t plug_lookup_firm;
static fh_plugin_hook_t plug_lookup_order;
static fh_plugin_hook_t plug_get_symbol;
static fh_plugin_hook_t plug_get_volume;
static fh_plugin_hook_t plug_get_price;
static fh_plugin_hook_t plug_get_side;

// feed handler specific plug in hooks
static fh_plugin_hook_t arca_send_msg;
static fh_plugin_hook_t arca_msg_flush;
static fh_plugin_hook_t arca_msg_init;
static fh_plugin_hook_t symbol_clear;
static fh_plugin_hook_t messages_lost;
static fh_plugin_hook_t sequence_reset;
static fh_plugin_hook_t book_refresh;
static fh_plugin_hook_t symbol_mapping;
static fh_plugin_hook_t firm_mapping;
static fh_plugin_hook_t imbalance_refresh;
static fh_plugin_hook_t add_order;
static fh_plugin_hook_t modify_order;
static fh_plugin_hook_t delete_order;
static fh_plugin_hook_t imbalance;
static fh_plugin_hook_t arca_feed_alert;
static fh_plugin_hook_t arca_pkt_loss;


void fh_arca_pub_cache_hooks() 
{
    // cache reference hooks used in this module
    plug_add_symbol    = fh_plugin_get_hook(FH_PLUGIN_ADD_SYMBOL_BY_INDEX);
    plug_add_firm      = fh_plugin_get_hook(FH_PLUGIN_ADD_FIRM);
    plug_add_order     = fh_plugin_get_hook(FH_PLUGIN_ADD_ORDER_REF);
    plug_mod_order     = fh_plugin_get_hook(FH_PLUGIN_MOD_ORDER_REF);
    plug_del_order     = fh_plugin_get_hook(FH_PLUGIN_DEL_ORDER_REF);
    plug_del_symbol    = fh_plugin_get_hook(FH_PLUGIN_DEL_SYMBOL);
    plug_lookup_symbol = fh_plugin_get_hook(FH_PLUGIN_LOOKUP_SYMBOL_BY_INDEX);
    plug_lookup_firm   = fh_plugin_get_hook(FH_PLUGIN_FIRM_LOOKUP);
    plug_lookup_order  = fh_plugin_get_hook(FH_PLUGIN_ORDER_LOOKUP);
    plug_get_symbol    = fh_plugin_get_hook(FH_PLUGIN_GET_SYMBOL);
    plug_get_volume    = fh_plugin_get_hook(FH_PLUGIN_GET_VOLUME);
    plug_get_price     = fh_plugin_get_hook(FH_PLUGIN_GET_PRICE);
    plug_get_side      = fh_plugin_get_hook(FH_PLUGIN_GET_SIDE);

    // cache customer specific hooks used in this module
    arca_send_msg      = fh_plugin_get_hook(FH_PLUGIN_MSG_SEND);
    arca_msg_flush     = fh_plugin_get_hook(FH_PLUGIN_MSG_FLUSH);
    arca_msg_init      = fh_plugin_get_hook(FH_PLUGIN_ARCA_MSG_INIT);
    symbol_clear       = fh_plugin_get_hook(FH_PLUGIN_SYMBOL_CLEAR);
    messages_lost      = fh_plugin_get_hook(FH_PLUGIN_MESSAGE_UNAVAILABLE);
    sequence_reset     = fh_plugin_get_hook(FH_PLUGIN_SEQUENCE_RESET);
    book_refresh       = fh_plugin_get_hook(FH_PLUGIN_BOOK_REFRESH);
    symbol_mapping     = fh_plugin_get_hook(FH_PLUGIN_SYMBOL_MAPPING);
    firm_mapping       = fh_plugin_get_hook(FH_PLUGIN_FIRM_MAPPING);
    imbalance_refresh  = fh_plugin_get_hook(FH_PLUGIN_IMBALANCE_REFRESH);
    add_order          = fh_plugin_get_hook(FH_PLUGIN_ADD_ORDER);
    modify_order       = fh_plugin_get_hook(FH_PLUGIN_MOD_ORDER);
    delete_order       = fh_plugin_get_hook(FH_PLUGIN_DEL_ORDER);
    imbalance          = fh_plugin_get_hook(FH_PLUGIN_IMBALANCE);
    arca_feed_alert    = fh_plugin_get_hook(FH_PLUGIN_FEED_ALERT);
    arca_pkt_loss      = fh_plugin_get_hook(FH_PLUGIN_PACKET_LOSS);
};
                 
#ifdef __UNIT_TEST__
/*----------------------------------------------------------------------------*/
/* display the packet header                                                  */
/*----------------------------------------------------------------------------*/
void print_pkt_header(struct msg_hdr *hdr) 
{
    fprintf(stdout,    
        " Header : msg type=%d size=%d, seq num=%d send_time=%d rcv_time=%lld",
        hdr->msg_type,hdr->msg_size,hdr->msg_seq_num,hdr->send_time,
        (long long int)hdr->msg_rcv_time);
    fprintf(stdout,
        " product id=%d retrans flag=%d num body entries=%d\n",
        hdr->product_id, hdr->retrans_flag, hdr->num_body_entries);
};
#endif

/*----------------------------------------------------------------------------*/
/* send an alert to subscribers                                               */
/*----------------------------------------------------------------------------*/
int send_alert(struct feed_group * const group, const int notificationtype)
{
    struct msg_body body;
    int rc =0;

    //  send a short alert; includes line handler state changes
    body.msg_type = ALERT;
    body.alert_type = notificationtype;
    body.status = build_summary_status(group);
    rc = publish_feed_alert(&body);
    if (rc==(int)FH_OK) 
    {
        group->publication_succeeded++;
    }
    else 
    {
        group->publication_failed++;
    }
    return 0;
};
/*----------------------------------------------------------------------------*/
/* send a packet loss alert to subscribers                                    */
/*----------------------------------------------------------------------------*/
int notify_packet_loss(struct feed_group * const group,
    const int notificationtype, const int missing_sequence, const int gapsize, 
    const int primary_or_secondary)
{
    int rc=0;
    struct msg_body body;

    body.status = build_summary_status(group);
    body.msg_type = PACKET_LOSS;
    body.alert_type = notificationtype;
    body.primary_or_secondary = primary_or_secondary;
    body.begin_seq_number = missing_sequence;
    body.end_seq_number = missing_sequence+gapsize-1;
    rc = publish_packet_loss(&body);
    // send a packet loss alert; includes line handler state changes
    if (rc==(int)FH_OK) 
    {
        group->publication_succeeded++;
    } 
    else 
    {
        group->publication_failed++;
    }
    return 0;
};
/*----------------------------------------------------------------------------*/
/* send a parse error alert to subscribers                                    */
/*----------------------------------------------------------------------------*/
int parse_error(struct feed_group * const group, const int sequence, 
    const int num_bodies, const int missing, const int primary_or_secondary) 
{
    int rc=0;
    struct msg_body body;

    body.status = build_summary_status(group);
    body.msg_type = ALERT;
    body.primary_or_secondary = primary_or_secondary;
    body.msg_seq_num = sequence;
    body.begin_seq_number = num_bodies;
    body.end_seq_number = missing;
    body.alert_type = PARSE_ERROR;
    rc = publish_feed_alert(&body);
    if (rc==(int)FH_OK) 
    {
        group->publication_succeeded++;
    } 
    else 
    {
        group->publication_failed++;
    }
    return 0;
};
/*----------------------------------------------------------------------------*/
/* send a runt packet alert to subscribers                                    */
/*----------------------------------------------------------------------------*/
int runt_packet_error(struct feed_group * const group, const int sequence, 
    const int num_bodies, const int missing, const int primary_or_secondary)
{
    int rc=0;
    struct msg_body body;

    body.status = build_summary_status(group);
    body.msg_type = ALERT;
    body.primary_or_secondary = primary_or_secondary;
    body.msg_seq_num = sequence;
    body.begin_seq_number = num_bodies;
    body.end_seq_number = missing;
    body.alert_type = RUNT_PACKET;
    rc = publish_feed_alert(&body);
    if (rc==(int)FH_OK) 
    {
        group->publication_succeeded++;
    } 
    else 
    {
        group->publication_failed++;
    }    
    return 0;
};
/*----------------------------------------------------------------------------*/
/* send a msg flush to messaging infra-structure                              */
/*----------------------------------------------------------------------------*/    
inline int msg_flush() 
{
    // synchronize flushing a channel with packing of multiple 
    // messages
    FH_STATUS rc=0;

    if (arca_msg_flush) 
    {
        arca_msg_flush(&rc);
    }
    return rc;
};
/*----------------------------------------------------------------------------*/
/* publish a symbol clear to subscribers                                      */
/*----------------------------------------------------------------------------*/
int publish_symbol_clear(struct feed_group *group, const struct msg_hdr *hdr,
    struct msg_body *body)
{
    FH_STATUS rc=0;
    char *symbol_ptr=NULL; 
    char *msg_space=NULL;
    int msg_size=0;
    int symbol_lth=0;
    char local_msg_space[512];

    body->status = build_summary_status(group);
    
#ifdef __UNIT_TEST__
    print_pkt_header(hdr);
    fprintf(stdout,
        " symbol clear body : next sequence=%d index=%d session=%d\n",
        body->next_seq_number,body->symbol_index,body->session_id);
#endif

    if (symbol_clear) 
    {
        //  plug in for publication of symbol clear message body
        if (plug_lookup_symbol) 
        { //if packing needs explicit symbol
            plug_lookup_symbol(&rc,body,&symbol_ptr);
            if (rc==FH_OK) 
            {        
                symbol_lth = strlen(symbol_ptr)+1;
                memcpy(&(body->symbol[0]),symbol_ptr,symbol_lth);
            } 
            else 
            {
                group->symbol_table_error++;
                //   Do not over use logging
                //publication needs to handle missing symbol
                body->symbol[0] = '\0'; //insure null symbol
            }
        } // if no plugin we assume symbol lookup is not reqd
        // if no packing; why bother with space for packing
        if (arca_msg_init) 
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if (rc!=FH_OK) 
            {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        } 
        else 
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        } // possible customer avoids allocation
        symbol_clear(&rc,group,hdr,body,msg_space,msg_size);
        if (rc == FH_OK && arca_send_msg) 
        { //if not pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if (rc==FH_OK) 
            {
                group->publication_succeeded++;
            } 
            else 
            {
                group->publication_failed++;
            }
        } 
        else if (rc != FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin symbol clear failed %d",rc));
        }
    } //if no plugin we assume no pubs necessary
    return rc;
};

/*! \brief Publish a book refresh to subscribers
 *
 *  \param group NEED DESCRIPTION
 *  \param hdr NEED DESCRIPTION
 *  \param body NEED DESCRIPTION
 *  \return status code indicating success or failure
 */
int publish_book_refresh(struct feed_group *group, struct msg_hdr *hdr, struct msg_body *body)
{
    FH_STATUS    rc = 0;
    char         *msg_space=NULL;
    int          msg_size=0;
    char         local_msg_space[512];

    if(book_refresh)
    {
        body->status = build_summary_status(group);
        body->session_id = hdr->session_id;
        if (hdr->current_refresh_msg_seq == 1 && plug_add_symbol)
        {
            //refresh the symbol table
            //just in case symbol index mapping changed
            //or was not initialized
            body->symbol_index = hdr->symbol_index;
            memcpy(&body->symbol,&hdr->symbol,ARCABOOK_SYMBOL_LENGTH);
            plug_add_symbol(&rc,body);
            if (rc!=FH_OK)
            {
                group->symbol_table_error++;
                //   Do not over use logging
            }
        }
// if no packing; why bother with space for packing
        if (arca_msg_init)
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if (rc!=FH_OK)
            {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        }
        else
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        }
        book_refresh(&rc,group,hdr,body,&msg_space,&msg_size);
        if (rc == FH_OK && arca_send_msg)
        { //if not pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if (rc==FH_OK)
            {
                group->publication_succeeded++;
            }
            else
            {
                group->publication_failed++;
            }
        }
        else if (rc!=FH_OK)
        {
            FH_LOG(LH,ERR,("Customer Plugin book refresh Failed %d",rc));
        }
#ifdef __UNIT_TEST__
    print_pkt_header(hdr);
#endif
    }
 
    // plug in for publication of book refresh message body
    return 0;
}

/*! \brief Publish an imbalance refresh to subscribers
 *
 *  \param group NEED DESCRIPTION
 *  \param hdr NEED DESCRIPTION
 *  \param body NEED DESCRIPTION
 *  \return status code indicating success or failure
 */
int publish_imbalance_refresh(struct feed_group *group, struct msg_hdr *hdr, 
    struct msg_body *body)
{
    FH_STATUS    rc = 0;
    char         *msg_space=NULL;
    int          msg_size=0;
    char         local_msg_space[512];

    if(imbalance_refresh) 
    {
        body->status = build_summary_status(group);

// if no packing; why bother with space for packing
        if (arca_msg_init)
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if (rc!=FH_OK)
            {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        }
        else
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        }
        imbalance_refresh(&rc,group,hdr,body,&msg_space,&msg_size);
        if (rc == FH_OK && arca_send_msg)
        { //if not pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if (rc==FH_OK)
            {
                group->publication_succeeded++;
            }
            else
            {
                group->publication_failed++;
            }
        }
        else if (rc!=FH_OK)
        {
            FH_LOG(LH,ERR,("Customer Plugin imbalance refresh Failed %d",rc));
        }
    }
#ifdef __UNIT_TEST__
    print_pkt_header(hdr);
    fprintf(stdout,
        " imbalance refresh body : msg_type=%d index=%d session=%d",
        body->msg_type,body->symbol_index,body->session_id);
    fprintf(stdout,
        " source seq=%d source time=%d vol=%d auction time=%d" ,
        body->source_seq_num,body->source_time,body->volume,
        body->auction_time);
    fprintf(stdout,
        " total=%d, body=%d price=%d:%d",
        body->total_imbalance,body->market_imbalance,body->price_scale_code,
        body->price_numerator);
    fprintf(stdout,
        " auction type=%c exchange=%c security type=%c\n",
        body->auction_type,body->exchange_id,body->security_type);
#endif

    // plug in for publication of imbalance refresh message body
    return 0;
}

/*----------------------------------------------------------------------------*/
/* process symbol mapping and publish to subscribers                          */
/*----------------------------------------------------------------------------*/
int publish_symbol_mapping(struct feed_group *group, struct msg_hdr *hdr, 
    struct msg_body *body)
{
    FH_STATUS    rc = 0;
    char        *msg_space=NULL;
    int          msg_size=0;
    char         local_msg_space[512];
    
    body->status = build_summary_status(group);
    
#ifdef __UNIT_TEST__
    print_pkt_header(hdr);
    fprintf(stdout,
        " symbol mapping body : index=%d session=%d symbol=%s\n",
        body->symbol_index,body->session_id,body->symbol);
#endif
    if (plug_add_symbol) 
    {   //reference plugin for adding a symbol
        // to the symbol mapping table
        plug_add_symbol(&rc,body);
        if (rc!=FH_OK) 
        {
            group->symbol_table_error++;
            //   Do not over use logging
        }
    } //possible customer avoids table use
    // plug in for publication of symbol mapping message body
    if (symbol_mapping) 
    {
        // if no packing; why bother with space for packing
        if (arca_msg_init) 
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if (rc!=FH_OK)  
            {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        } 
        else 
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        } //possibe customer doesnt need allocation
        symbol_mapping(&rc,group,hdr,body,msg_space,msg_size);
        if (rc == FH_OK && arca_send_msg)  
        { //if not pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if (rc==FH_OK) 
            {
                group->publication_succeeded++;
            } 
            else 
            {
                group->publication_failed++;
            }
        } 
        else if (rc!=FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin symbol mapping failed %d",rc));
        }
    } //customer may choose no publication
    //   may feel that reference plugin is totally sufficient
    return rc;
};
/*----------------------------------------------------------------------------*/
/* process a firm mapping and publish to subscribers                          */
/*----------------------------------------------------------------------------*/
int publish_firm_mapping(struct feed_group *group, struct msg_hdr *hdr, 
    struct msg_body *body)
{
    FH_STATUS    rc = 0;
    char         *msg_space=NULL;
    int          msg_size=0;
    char         local_msg_space[512];
    
    body->status = build_summary_status(group);
    
#ifdef __UNIT_TEST__
    print_pkt_header(hdr);
    fprintf(stdout,
        " firm mapping body : index=%d firm=%s\n",
        body->firm_index,body->firm);
#endif
    if (plug_add_firm) 
    {   //reference plugin for adding a firm
        // to the firm mapping table
        plug_add_firm(&rc,body);
        if (rc!=FH_OK) 
        {
            group->firm_table_error++;
            // Do not overuse the log
        }
    }
    // plug in for publication of firm mapping message body
    if (firm_mapping) 
    {
        // if no packing do not allocate or use space
        if (arca_msg_init) 
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if (rc!=FH_OK) 
            {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        } 
        else 
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        } //possible customer does not need allocation
        firm_mapping(&rc,group,hdr,body,msg_space,msg_size);
        if (rc == FH_OK && arca_send_msg) 
        { //if not pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if (rc==FH_OK) 
            {
                group->publication_succeeded++;
            } 
            else 
            {
                group->publication_failed++;
            }
        } 
        else if (rc != FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin firm mapping failed %d",rc));
        }
    } //customer may choose to not publish
    //    may feel that reference plugin is totally sufficient
    return rc;
};
/*----------------------------------------------------------------------------*/
/* publish an add order to subscribers                                        */
/*----------------------------------------------------------------------------*/
int publish_add_order(struct feed_group *group, struct msg_hdr *hdr, 
    struct msg_body *body)
{
    FH_STATUS rc=0;
    char *symbol_ptr=NULL;
    char *firm_ptr=NULL;
    char *msg_space=NULL;
    int  msg_size=0;
    int  symbol_lth=0;
    char local_msg_space[512];

    body->status = build_summary_status(group);
    
#ifdef __UNIT_TEST__
    print_pkt_header(hdr);
    fprintf(stdout,
        " add order body : msg_type=%d index=%d session=%d",
        body->msg_type,body->symbol_index,body->session_id);
    fprintf(stdout,
        " source seq=%d source time=%d vol=%d order id=%d" ,
        body->source_seq_num,body->source_time,body->volume,
        body->order_id);
    fprintf(stdout,
        " price=%d:%d",
        body->price_scale_code,
        body->price_numerator);
    fprintf(stdout,
        " side=%c firm index=%d exchange=%c security type=%c\n",
        body->side,body->firm_index,body->exchange_id,body->security_type);
#endif

    //  plug in for publication of add order message body
    if (add_order) 
    {
        // if we are not going to publish we dont need to do lookups
        if (plug_lookup_symbol) 
        { //if packing needs explicit symbol
            plug_lookup_symbol(&rc,body,&symbol_ptr);
            if (rc==FH_OK) 
            {        
                symbol_lth = strlen(symbol_ptr)+1;
                memcpy(&(body->symbol[0]),symbol_ptr,symbol_lth);
            } 
            else 
            {
                group->symbol_table_error++;
            }
        }
        if (plug_lookup_firm) { //if packing needs explicit firm
            plug_lookup_firm(&rc,body,&firm_ptr);
            if (rc==FH_OK) {        
                symbol_lth = strlen(firm_ptr)+1;
                memcpy(&(body->firm[0]),firm_ptr,symbol_lth);
            } 
            else 
            {
                group->firm_table_error++;
            }
        }
        if (plug_add_order) 
         {
            plug_add_order(&rc,body);
            if (rc!=FH_OK) 
            {
                FH_LOG(LH,ERR,("Customer Plugin add order Failed %d",rc));
            }
        }
        // if no packing; why bother with space for packing
        if (arca_msg_init)  
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if (rc!=FH_OK) 
            {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        } 
        else 
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        }
        add_order(&rc,group,hdr,body,msg_space,msg_size);
        if (rc == FH_OK && arca_send_msg) 
        { //if not pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if (rc==FH_OK) 
            {
                group->publication_succeeded++;
            } 
            else 
            {
                group->publication_failed++;
            }
        } 
        else if (rc!=FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin add order Failed %d",rc));
        }
    }
    return rc;
};
/*----------------------------------------------------------------------------*/
/* publish a modify order to subscribers                                      */
/*----------------------------------------------------------------------------*/
int publish_mod_order(struct feed_group *group, struct msg_hdr *hdr,
    struct msg_body *body)
{
    FH_STATUS rc=0;
    char *symbol_ptr=NULL;
    char *msg_space=NULL;
    char *firm_ptr=NULL;
    int msg_size=0;
    int symbol_lth=0;
    char local_msg_space[512];

    body->status = build_summary_status(group);
    
#ifdef __UNIT_TEST__
    print_pkt_header(hdr);
    fprintf(stdout,
        " mod order body : msg_type=%d index=%d session=%d",
        body->msg_type,body->symbol_index,body->session_id);
    fprintf(stdout,
        " source seq=%d source time=%d vol=%d order id=%d" ,
        body->source_seq_num,body->source_time,body->volume,
        body->order_id);
    fprintf(stdout,
        " price=%d:%d",
        body->price_scale_code,
        body->price_numerator);
    fprintf(stdout,
        " side=%c firm index=%d exchange=%c security type=%c\n",
        body->side,body->firm_index,body->exchange_id,body->security_type);
#endif

    //  plug in for publication of modify order message body
    if (modify_order) 
    {
        if (plug_lookup_symbol) 
        { //if packing needs explicit symbol
            plug_lookup_symbol(&rc,body,&symbol_ptr);
            if (rc==FH_OK) 
            {        
                symbol_lth = strlen(symbol_ptr)+1;
                memcpy(&(body->symbol[0]),symbol_ptr,symbol_lth);
            } else {
                group->symbol_table_error++;
            }
        }
        if (plug_lookup_firm) 
        { //if packing needs explicit firm
            plug_lookup_firm(&rc,body,&firm_ptr);
            if (rc==FH_OK) 
            {        
                symbol_lth = strlen(firm_ptr)+1;
                memcpy(&(body->firm[0]),firm_ptr,symbol_lth);
            } 
            else 
            {
                group->firm_table_error++;
            }
        }
        if (plug_mod_order) 
        {
            plug_mod_order(&rc,body);
            if (rc!=FH_OK) 
            {
                FH_LOG(LH,ERR,("Customer Plugin modify order failed %d",rc));
            }
        }
        // if no packing; why bother with space for packing
        if (arca_msg_init) 
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if (rc!=FH_OK) 
            {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        } 
        else 
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        }
        modify_order(&rc,group,hdr,body,msg_space,msg_size);
        if (rc == FH_OK && arca_send_msg) 
        { //if pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if (rc==FH_OK)  
            {
                group->publication_succeeded++;
            } 
            else 
            {
                group->publication_failed++;
            }
        } 
        else if (rc!=FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin modify order failed %d",rc));
        }
    }
    return rc;
};
/*----------------------------------------------------------------------------*/
/* publish a delete order to subscribers                                      */
/*----------------------------------------------------------------------------*/
int publish_del_order(struct feed_group *group, struct msg_hdr *hdr,
    struct msg_body *body)
{
    void *order=NULL;
    FH_STATUS rc=0;
    char *symbol_ptr=NULL;
    char *msg_space=NULL;
    char *firm_ptr=NULL;
    int msg_size=0; 
    int symbol_lth=0;
    char local_msg_space[512];

    body->status = build_summary_status(group);
    
#ifdef __UNIT_TEST__
    print_pkt_header(hdr);
    fprintf(stdout,
        " del order body : msg_type=%d index=%d",
        body->msg_type,body->symbol_index);
    fprintf(stdout,
        " source seq=%d source time=%d order id=%d session id=%d" ,
        body->source_seq_num,body->source_time,
        body->order_id,body->session_id);
    fprintf(stdout,
        " side=%c firm index=%d exchange=%c security type=%c\n",
        body->side,body->firm_index,body->exchange_id,body->security_type);
#endif

    //  plug in for publication of delete order message body
    if (delete_order) 
    {
        if (plug_lookup_symbol) 
        { //if packing needs explicit symbol
            plug_lookup_symbol(&rc,body,&symbol_ptr);
            if (rc==FH_OK) 
            {        
                symbol_lth = strlen(symbol_ptr)+1;
                memcpy(&(body->symbol[0]),symbol_ptr,symbol_lth);
            } 
            else 
            {
                group->symbol_table_error++;
            }
        }
        if (plug_lookup_firm) 
        { //if packing needs explicit firm
            plug_lookup_firm(&rc,body,&firm_ptr);
            if (rc==FH_OK) 
            {        
                symbol_lth = strlen(firm_ptr)+1;
                memcpy(&(body->firm[0]),firm_ptr,symbol_lth);
            } 
            else 
            {
                group->firm_table_error++;
            }
        }
        // delete order does not have all the data needed for differential
        // publishing; lookup that info from the order table
        if (plug_del_order) 
        {
            if (plug_lookup_order) 
            {
                plug_lookup_order(&rc,body,&order);
                if (rc==FH_OK && order!=NULL) 
                {
                    if (plug_get_volume) 
                    {
                        plug_get_volume(&rc,order,&(body->volume));
                    }
                    if (plug_get_price) 
                    {
                        plug_get_price(&rc,order,&(body->price));
                        if (rc==FH_OK) 
                        {
                            body->price_numerator = body->price/100;
                            body->price_scale_code = 4;
                        }
                    }
                    if (plug_get_side) 
                    {
                        plug_get_side(&rc,order,&(body->side));
                    }
                } 
                else 
                {
                    FH_LOG(LH,ERR,("Plugin Lookup order failed %d:%u",
                        rc,body->order_id)); //possible order not known
                }
            }
            plug_del_order(&rc,body);
            if (rc!=FH_OK) 
            {
                FH_LOG(LH,ERR,("Customer Plugin delete order failed %d",rc));
            }
        } //we assume that if plugin does not exist; no differential pub
        // if no packing; why bother with space for packing
        if (arca_msg_init) {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if (rc!=FH_OK) {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        } 
        else 
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        }
        delete_order(&rc,group,hdr,body,msg_space,msg_size);
        if (rc == FH_OK && arca_send_msg) 
        { //if pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if (rc==FH_OK) 
            {
                group->publication_succeeded++;
            } 
            else 
            {
                group->publication_failed++;
            }
        } 
        else if (rc!=FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin delete order failed %d",rc));
        }
    }
    return rc;
};
/*----------------------------------------------------------------------------*/
/* publish an imbalance to subscribers                                        */
/*----------------------------------------------------------------------------*/
int publish_imbalance(struct feed_group *group, struct msg_hdr *hdr,
    struct msg_body *body)
{
    FH_STATUS rc=0;
    char *symbol_ptr=NULL; 
    char *msg_space=NULL;
    int msg_size=0;
    int symbol_lth=0;
    char local_msg_space[512];

    body->status = build_summary_status(group);
    
#ifdef __UNIT_TEST__
    print_pkt_header(hdr);
    fprintf(stdout,
        " imbalance body : msg_type=%d index=%d session=%d",
        body->msg_type,body->symbol_index,body->session_id);
    fprintf(stdout,
        " source seq=%d source time=%d vol=%d auction time=%d" ,
        body->source_seq_num,body->source_time,body->volume,
        body->auction_time);
    fprintf(stdout,
        " total=%d, body=%d price=%d:%d",
        body->total_imbalance,body->market_imbalance,body->price_scale_code,
        body->price_numerator);
    fprintf(stdout,
        " auction type=%c exchange=%c security type=%c\n",
        body->auction_type,body->exchange_id,body->security_type);
#endif

    //  plug in for publication of imbalance message body
    if (imbalance) 
    {
        if (plug_lookup_symbol) 
        { //if packing needs explicit symbol
            plug_lookup_symbol(&rc,body,&symbol_ptr);
            if (rc==FH_OK) 
            {        
                symbol_lth = strlen(symbol_ptr)+1;
                memcpy(&(body->symbol[0]),symbol_ptr,symbol_lth);
            } 
            else 
            {
                group->symbol_table_error++;
            }
        }
        if (arca_msg_init) 
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if (rc!=FH_OK) 
            {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        } 
        else 
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        }
        imbalance(&rc,group,hdr,body,msg_space,msg_size);
        if (rc == FH_OK && arca_send_msg)  
        { //if pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if (rc==FH_OK) 
            {
                group->publication_succeeded++;
            } 
            else 
            {
                group->publication_failed++;
            }
        } 
        else if (rc!=FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin imbalance failed %d",rc));
        }
    }
    return rc;
};
/*----------------------------------------------------------------------------*/
/* publish a packet loss alert to subscribers                                 */
/*----------------------------------------------------------------------------*/
int publish_packet_loss(struct msg_body *body)
{
    FH_STATUS    rc = 0;
    char        *msg_space=NULL;
    int          msg_size=0;
    char         local_msg_space[512];
    
#ifdef __UNIT_TEST__
    fprintf(stdout,
        " Packet loss %d alert type %d feed %d first %d last %d status %d\n",
        body->msg_type,body->alert_type,body->primary_or_secondary,
        body->begin_seq_number,body->end_seq_number,body->status);
#endif

    //  plug in for publication of packet loss events
    if (arca_pkt_loss) 
    {
        if (arca_msg_init) 
        {
            arca_msg_init(&rc,NULL,NULL,body,&msg_space,&msg_size);
            if (rc!=FH_OK) 
            {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        } 
        else 
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        }
        arca_pkt_loss(&rc,body,msg_space,msg_size);
        if (rc == FH_OK && arca_send_msg) 
        { //if pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
        } 
        else if (rc!=FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin packet loss failed %d",rc));
        }
    }
    return (int) rc;
};
/*----------------------------------------------------------------------------*/
/* publish a feed alert to subscribers                                        */
/*----------------------------------------------------------------------------*/

int publish_feed_alert(struct msg_body *body) 
{
    FH_STATUS    rc = 0;
    char        *msg_space=NULL;
    int          msg_size=0;
    char         local_msg_space[512];
    
#ifdef __UNIT_TEST__
      fprintf(stdout,
          " ALERT msg type=%d alert type=%d status=%x\n",
          body->msg_type,body->alert_type,body->status);
#endif

    //  plug in for publication of state change events
    if (arca_feed_alert) 
    {
        if (arca_msg_init) 
        {
            arca_msg_init(&rc,NULL,NULL,body,&msg_space,&msg_size);
            if (rc!=FH_OK) 
            {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        } 
        else 
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        }
        arca_feed_alert(&rc,body,msg_space,msg_size);
        if (rc == FH_OK && arca_send_msg) 
        { //if pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
        } 
        else if (rc!=FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin feed alert failed %d",rc));
        }
    }
    return (int) rc;
};
/*----------------------------------------------------------------------------*/
/* publish a sequence number reset to subscribers                             */
/*----------------------------------------------------------------------------*/
int publish_start_of_day(struct feed_group *group, struct msg_hdr *hdr, 
    struct msg_body *body)
{
    FH_STATUS    rc = 0;
    char        *msg_space=NULL;
    int          msg_size=0;
    char         local_msg_space[512];
    
    body->status = build_summary_status(group);

#ifdef __UNIT_TEST__
    print_pkt_header(hdr);
    fprintf(stdout," sequence number reset body : next sequence number=%d\n",
        body->next_seq_number);
#endif

    //  plug in for publication of start of day message body
    if (sequence_reset) 
    {
        if (arca_msg_init) 
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if (rc!=FH_OK) 
            {
                FH_LOG(LH,ERR,("Plugin msg init failed %d",rc));
                //msg init reqd and failed; serious enough to abort
                return rc;
            }
        } 
        else 
        { //use local space, send will use it also
            msg_space = &(local_msg_space[0]);
            msg_size = 512;
            memset(msg_space,0,512); //insure its clean
        }
        sequence_reset(&rc,group,hdr,body,msg_space,msg_size);
        if (rc == FH_OK && arca_send_msg) 
        { //if pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if (rc==FH_OK) 
            {
                group->publication_succeeded++;
            } else {
                group->publication_failed++;
            }
        } 
        else if (rc!=FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin sequence reset failed %d",rc));
        }
    }
    return rc;
};

