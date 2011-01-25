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

#ifndef __ARCATRADE_PLUGIN_H__
#define __ARCATRADE_PLUGIN_H__
/*-----------------------------------------------------------------------*/
/* tva_arca_plugin.h                                                     */
/* author - wally matthews  5 Dec. 2008                                  */
/*  headers for tervela plug in for arca mcast                           */
/*  see hooks defined in /csi-fh/common/fh_plugin.h                      */
/*-----------------------------------------------------------------------*/

#include "fh_arca_constants.h"
#include "fh_arca_headers.h"
#include "fh_feed_group.h"

// FH_PLUGIN_MSG_SEND
void arca_send_msg (FH_STATUS *rc, //pointer to store return code
                   char    *msg_space,         //input ptr for packed msg
                   int     space_size,         //input ptr for packed size
                   struct msg_body *body       //input ptr
                            //useful for symbol (for topic) and msg_type
                   );
// Notes: The customer sends the packed message in msg_space and then frees
//     the resources used

// FH_PLUGIN_MSG_FLUSH
void arca_msg_flush(FH_STATUS *rc); // pointer to store return code
// Notes: Line Handler after processing a packet will give the messaging
//     infra-structure the opportunity to push any residual messages 
//     so that they are delivered expeditiously rather than waiting in
//     a queue until the next message is processed

// FH_PLUGIN_ARCA_INIT
void init_plugin  (FH_STATUS *rc, //pointer to store return code
                   int       feed_count,     //numbers
                   struct feed_group **feeds //array of ptrs to feeds
                   ); 
// Notes: The customer is given the opportunity to initialize any
//    messaging infra-structure they need, build any tables they
//    might need (including choosing not to use those in the reference
//    plug ins) and to over ride any of the configuration of the feeds
//    (see fh_feed_group.h)

// FH_PLUGIN_ARCA_SHUTDOWN
void shutdown_plugin(FH_STATUS *rc); //pointer to store return code
// Notes: The customer is given the opportunity to shutdown their supporting
//    messaging infra-structure, free up any table space, close connections,
//    files, ....
                       
// FH_PLUGIN_ARCA_MSG_INIT
void arca_msg_init(FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,  //input ptr to feed struct
                   struct msg_hdr *hdr,       //input ptr to pkt hdr
                   struct msg_body *body,     //input ptr to msg body
                   char** msg_space,          //output ptr of msg space
                            // that packing will use to pack the msg in
                   int*   space_size          //output ptr to size returned
                   );
// Notes: The customer is given the opportunity to allocate the space in which
//    the published message will be assembled in. They can also either create
//    a topic or select one that has been build at plugin initialization time
//    The expected function call sequence will be lookup symbol using csi 
//    using csi reference plugin, arca_msg_init, specific message function, 
//    arca_msg_send. arca_msg_init will assign a 
//    char* pointer to *msg_space and assign the byte length to *space_size
//    These values will be provided to the following specific message function
//    Lastly, the msg_send will pass these pointers so that the customer can
//    return their specific messaging infra-structure and then
//    free up any resources allocated in the arca_msg_init.
//    The intention is to seperate the allocation, packing, and messaging
//    operations so that there is maximum reuse and sharing
//    of code functions
//    WARNING group and hdr may be NULL pointers
//       this will happen when the messages are feed alerts and
//       packet loss alerts where the group and hdr are not defined

// FH_PLUGIN_SEQUENCE_RESET
void sequence_reset(FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size);           //packing space size
// Notes: See symbol clear above

// FH_PLUGIN_FEED_ALERT
void arca_feed_alert(FH_STATUS *rc, //pointer to store return code
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size);           //packing space size
// Notes: See symbol clear above

// FH_PLUGIN_PACKET_LOSS
void arca_pkt_loss (FH_STATUS *rc, //pointer to store return code
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size);           //packing space size
// Notes: See symbol clear above

// FH_PLUGIN_ARCA_TRADE
void trade(FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size);           //packing space size
// Notes: the customer is given the opportunity to pack a trade message

// FH_PLUGIN_ARCA_TRADE_CANCEL
void cancel_trade(FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size);           //packing space size

// Notes: the customer is given the opportunity to pack a trade cancel message

// FH_PLUGIN_ARCA_TRADE_CORRECTION
void correct_trade(FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size);           //packing space size
// Notes: the customer is given the opportunity to pack a trade correction

#endif /* __ARCATRADE_PLUGIN_H__ */
