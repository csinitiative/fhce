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
/* file: publication.c                                               */
/* Usage: configuration support for arca feed handler                */
/* Author: Wally Matthews Collaborative Software Initiative          */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <stdio.h>
#include <string.h>

// Common FH headers
#include "fh_plugin.h"
#include "fh_log.h"

// Arca FH headers
#include "fh_arca_constants.h"
#include "fh_arcatrade_headers.h"
#include "fh_feed_group.h"

// reference plug in hooks
static fh_plugin_hook_t plug_set_contxt_ptr;
static fh_plugin_hook_t plug_get_contxt_ptr;

// feed handler specific plug in hooks
static fh_plugin_hook_t arca_send_msg;
static fh_plugin_hook_t arca_msg_flush;
static fh_plugin_hook_t arca_msg_init;
static fh_plugin_hook_t messages_lost;
static fh_plugin_hook_t sequence_reset;
static fh_plugin_hook_t arca_feed_alert;
static fh_plugin_hook_t arca_pkt_loss;
static fh_plugin_hook_t trade;
static fh_plugin_hook_t cancel_trade;
static fh_plugin_hook_t correct_trade;

void fh_arca_pub_cache_hooks() 
{
    // cache reference hooks used in this module
    plug_set_contxt_ptr = fh_plugin_get_hook(FH_PLUGIN_SET_CONTEXT_POINTER);
    plug_get_contxt_ptr = fh_plugin_get_hook(FH_PLUGIN_GET_CONTEXT_POINTER);

    // cache customer specific hooks used in this module
    arca_send_msg      = fh_plugin_get_hook(FH_PLUGIN_MSG_SEND);
    arca_msg_flush     = fh_plugin_get_hook(FH_PLUGIN_MSG_FLUSH);
    arca_msg_init      = fh_plugin_get_hook(FH_PLUGIN_ARCA_MSG_INIT);
    messages_lost      = fh_plugin_get_hook(FH_PLUGIN_MESSAGE_UNAVAILABLE);
    sequence_reset     = fh_plugin_get_hook(FH_PLUGIN_SEQUENCE_RESET);
    trade              = fh_plugin_get_hook(FH_PLUGIN_ARCA_TRADE);
    cancel_trade       = fh_plugin_get_hook(FH_PLUGIN_ARCA_TRADE_CANCEL);       
    correct_trade      = fh_plugin_get_hook(FH_PLUGIN_ARCA_TRADE_CORRECTION);
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
    if(rc==(int)FH_OK) 
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
    if(rc==(int)FH_OK) 
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
    if(rc==(int)FH_OK) 
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
    if(rc==(int)FH_OK) 
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

    if(arca_msg_flush) 
    {
        arca_msg_flush(&rc);
    }
    return rc;
};
/*----------------------------------------------------------------------------*/
/* publish a trade message                                                    */
/*----------------------------------------------------------------------------*/
int publish_trade(struct feed_group *group, const struct msg_hdr *hdr,
  struct msg_body *body)
{
    FH_STATUS rc=0;
    char *msg_space=NULL;
    int msg_size=0;
    char local_msg_space[512];

    if(trade)
    {
        if(arca_msg_init)
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if(rc!=FH_OK)
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
        } //possible customer avoids allocation
        body->status = build_summary_status(group);
        trade(&rc,group,hdr,body,msg_space,msg_size);
        if(rc == FH_OK && arca_send_msg)
        { //if not pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if(rc==FH_OK)
            {
                group->publication_succeeded++;
            }
            else
            {
                group->publication_failed++;
            }
        }
        else if(rc != FH_OK)
        {
            FH_LOG(LH,ERR,("Customer Plugin trade failed %d",rc));
        }
    }
    return rc;
};
/*----------------------------------------------------------------------------*/
/* publish a trade cancel message                                             */
/*----------------------------------------------------------------------------*/
int publish_trade_cancel(struct feed_group *group, const struct msg_hdr *hdr,
  struct msg_body *body)
{
    FH_STATUS rc=0;
    char *msg_space=NULL;
    int msg_size=0;
    char local_msg_space[512];

    if(cancel_trade)
    {
        if(arca_msg_init)
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if(rc!=FH_OK)
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
        } //possible customer avoids allocation
        body->status = build_summary_status(group);
        cancel_trade(&rc,group,hdr,body,msg_space,msg_size);
        if(rc == FH_OK && arca_send_msg)
        { //if not pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if(rc==FH_OK)
            {
                group->publication_succeeded++;
            }
            else
            {
                group->publication_failed++;
            }
        }
        else if(rc != FH_OK)
        {
            FH_LOG(LH,ERR,("Customer Plugin trade cancel failed %d",rc));
        }
    }
    return rc;
};
/*----------------------------------------------------------------------------*/
/* publish a trade correction message                                         */
/*----------------------------------------------------------------------------*/
int publish_trade_correction(struct feed_group *group, const struct msg_hdr *hdr,
  struct msg_body *body)
{
    FH_STATUS rc=0;
    char *msg_space=NULL;
    int msg_size=0;
    char local_msg_space[512];

    if(correct_trade)
    {
        if(arca_msg_init)
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if(rc!=FH_OK)
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
        } //possible customer avoids allocation
        body->status = build_summary_status(group);
        correct_trade(&rc,group,hdr,body,msg_space,msg_size);
        if(rc == FH_OK && arca_send_msg)
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
        else if(rc != FH_OK)
        {
            FH_LOG(LH,ERR,("Customer Plugin trade correction failed %d",rc));
        }
    }
    return rc;
};
/*----------------------------------------------------------------------------*/
/* publish a packet loss alert to subscribers                                 */
/*----------------------------------------------------------------------------*/
int publish_packet_loss(const struct msg_body *body)
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
        if(arca_msg_init) 
        {
            arca_msg_init(&rc,NULL,NULL,body,&msg_space,&msg_size);
            if(rc!=FH_OK) 
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
        if(rc == FH_OK && arca_send_msg) 
        { //if pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
        } 
        else if(rc!=FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin packet loss failed %d",rc));
        }
    }
    return (int) rc;
};
/*----------------------------------------------------------------------------*/
/* publish a feed alert to subscribers                                        */
/*----------------------------------------------------------------------------*/

int publish_feed_alert(const struct msg_body *body) 
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
    if(arca_feed_alert) 
    {
        if(arca_msg_init) 
        {
            arca_msg_init(&rc,NULL,NULL,body,&msg_space,&msg_size);
            if(rc!=FH_OK) 
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
        if(rc == FH_OK && arca_send_msg) 
        { //if pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
        } 
        else if(rc!=FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin feed alert failed %d",rc));
        }
    }
    return (int) rc;
};
/*----------------------------------------------------------------------------*/
/* publish a sequence number reset to subscribers                             */
/*----------------------------------------------------------------------------*/
int publish_start_of_day(struct feed_group *group, const struct msg_hdr *hdr, 
    struct msg_body *body)
{
    FH_STATUS    rc = 0;
    char        *msg_space=NULL;
    int          msg_size=0;
    char         local_msg_space[512];
    
#ifdef __UNIT_TEST__
    print_pkt_header(hdr);
    fprintf(stdout," sequence number reset body : next sequence number=%d\n",
        body->next_seq_number);
#endif

    //  plug in for publication of start of day message body
    if(sequence_reset) 
    {
        if(arca_msg_init) 
        {
            arca_msg_init(&rc,group,hdr,body,&msg_space,&msg_size);
            if(rc!=FH_OK) 
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
        body->status = build_summary_status(group);
        sequence_reset(&rc,group,hdr,body,msg_space,msg_size);
        if(rc == FH_OK && arca_send_msg) 
        { //if pack failed; why send it
            arca_send_msg(&rc,msg_space,msg_size,body);
            if(rc==FH_OK) 
            {
                group->publication_succeeded++;
            } else {
                group->publication_failed++;
            }
        } 
        else if(rc!=FH_OK) 
        {
            FH_LOG(LH,ERR,("Customer Plugin sequence reset failed %d",rc));
        }
    }
    return rc;
};

