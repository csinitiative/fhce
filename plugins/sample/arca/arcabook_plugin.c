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

/*
 * System includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

/*
 * FH includes
 */
#include "fh_plugin.h"
#include "fh_errors.h"

/*
 * Plugin includes
 */
#include "arcabook_plugin.h"

#define TVA_PLUGIN_REGISTER(x, h)                                                       \
do {                                                                                    \
    printf("Loading Tervela Arca hook: %s (%d) hook=%s...", #x, x, #h);                 \
    if (fh_plugin_register((x), (fh_plugin_hook_t)(h)) != FH_OK) {                      \
        fprintf(stderr, "TVA_ARCA_PLUGIN: Unable to register %s (%d) hook\n", #x, x);   \
        exit(1);                                                                        \
    }                                                                                   \
    printf("done\n");                                                                   \
} while (0)

/*
 * Startup function
 *
 * The "__attribute__ ((constructor))" syntax will cause this function to be run
 * whenever the shared object, compiled from this .c file, is loaded.  It is in this
 * function that all registration of hook functions must be done.
 */
void __attribute__ ((constructor)) tva_arca_plugin_init(void)
{
    TVA_PLUGIN_REGISTER(FH_PLUGIN_MSG_FLUSH,             arca_msg_flush);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_MSG_SEND,              arca_send_msg);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_ARCA_INIT,             init_plugin);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_ARCA_SHUTDOWN,         shutdown_plugin);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_ARCA_MSG_INIT,         arca_msg_init);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_SYMBOL_CLEAR,          symbol_clear);
    //TVA_PLUGIN_REGISTER(FH_PLUGIN_MESSAGE_UNAVAILABLE,   messages_lost);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_SYMBOL_MAPPING,        symbol_mapping);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_FIRM_MAPPING,          firm_mapping);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_SEQUENCE_RESET,        sequence_reset);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_BOOK_REFRESH,          book_refresh);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_IMBALANCE_REFRESH,     imbalance_refresh);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_ADD_ORDER,             add_order);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_MOD_ORDER,             modify_order);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_DEL_ORDER,             delete_order);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_IMBALANCE,             imbalance);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_FEED_ALERT,            arca_feed_alert);
    TVA_PLUGIN_REGISTER(FH_PLUGIN_PACKET_LOSS,           arca_pkt_loss);
}
/*------------------------------------------------------------------------*/
/* deliver the message to the messaging layer; if the message resources   */
/* need to be explicitly returned it must be done before returning from   */
/* this function. If the messaging layer does this implicitly, then       */
/* processing for this message is complete.                               */
/*------------------------------------------------------------------------*/
void arca_send_msg (FH_STATUS *rc, //pointer to store return code
                   char    *msg_space,         //input ptr for packed msg
                   int     space_size,         //input ptr for packed size
                   struct msg_body *body){       //input ptr
                            //useful for symbol (for topic) and msg_type
    
    // this code just eliminates compiler warnings resulting from unused parameters
    if (msg_space || space_size || body) {}
                   
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* If the messaging layer is packing multiple messages per transmission   */
/* unit, we tell it when we have delivered the last message of a packet   */
/* so that it can flush any partially filled transmission unit.           */
/* This reduces average latency by eliminating a partial transmission unit*/
/* waiting on a queue until there is additional traffic                   */
/*------------------------------------------------------------------------*/
void arca_msg_flush(FH_STATUS *rc){
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do any customer specific configuration processing to over-ride the     */
/* default settings in arca.conf. "feeds" is an array of feed groups for  */
/* this process. Each feed_group contains the configuration parameters    */
/* the feed. Customer specific configuration can over ride any of these   */
/* values.                                                                */
/* Initialize the messaging layer and any other customer specific infra-  */
/* structure that is required                                             */
/*------------------------------------------------------------------------*/
void init_plugin  (FH_STATUS *rc, //pointer to store return code
                   int       feed_count,     //numbers
                   struct feed_group **feeds){ //array of ptrs to feeds
                       
    // this code just eliminates compiler warnings resulting from unused parameters
    if (feed_count || feeds) {}
                   
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Shutdown the messaging layer. Return any resources used by customer    */
/* infra-structure or the messaging layer.                                */
/*------------------------------------------------------------------------*/
void shutdown_plugin(FH_STATUS *rc){
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* allocate (if necessary) the space that the message will be packed into */
/* initialize the space. return a pointer for the space in the location   */
/* *msg_space and the size in bytes in the location *space_size.          */
/* *group provides access to the line handler state and config info       */
/* to build status information.                                           */
/* *hdr has info from the message header. *body has info from msg body    */
/*------------------------------------------------------------------------*/
void arca_msg_init(FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,  //input ptr to feed struct
                   struct msg_hdr *hdr,       //input ptr to pkt hdr
                   struct msg_body *body,     //input ptr to msg body
                   char** msg_space,          //output ptr of msg space
                            // that packing will use to pack the msg in
                   int*   space_size){        //output ptr to size returned
                       
    // this code just eliminates compiler warnings resulting from unused parameters
    if (group || hdr || body || msg_space || space_size) {}
    
    *rc = FH_OK;

};
/*------------------------------------------------------------------------*/
/* Do message packing for symbol clear message.                           */
/*------------------------------------------------------------------------*/
void symbol_clear  (FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){
                       
    // this code just eliminates compiler warnings resulting from unused parameters
    if (group || hdr || body || msg_space || msg_size) {}
    
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do message packing for message unavailable.                            */
/*------------------------------------------------------------------------*/
/* not needed for Tervela; Packet loss alert to be used
//  this would be used if the customer were implementing a raw feed
//   and wanted it to deliver the raw message + packet loss alerts
void messages_lost (FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){
    *rc = FH_OK;
};
*/
/*------------------------------------------------------------------------*/
/* Do message packing for sequence number reset message                   */
/*------------------------------------------------------------------------*/
void sequence_reset(FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){
                       
    // this code just eliminates compiler warnings resulting from unused parameters
    if (group || hdr || body || msg_space || msg_size) {}                   
    
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do message packing for symbol mapping message                          */
/*------------------------------------------------------------------------*/
void symbol_mapping(FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){           //packing space size

    if (group || hdr || body || msg_space || msg_size) {}

    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do message packing for firm mapping message                          */
/*------------------------------------------------------------------------*/
void firm_mapping(FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){           //packing space size

    if (group || hdr || body || msg_space || msg_size) {}

    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do message packing for book refresh. Not supported in Stage 1          */
/*------------------------------------------------------------------------*/
void book_refresh  (FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){
                       
    // this code just eliminates compiler warnings resulting from unused parameters
    if (group || hdr || body || msg_space || msg_size) {}
                    
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do message packing for imbalance refresh. Not supported in Stage 1     */
/*------------------------------------------------------------------------*/
void imbalance_refresh(FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){
                       
    // this code just eliminates compiler warnings resulting from unused parameters
    if (group || hdr || body || msg_space || msg_size) {}
    
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do message packing for add order message                               */
/*------------------------------------------------------------------------*/
void add_order     (FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){
                       
    // this code just eliminates compiler warnings resulting from unused parameters
    if (group || hdr || body || msg_space || msg_size) {}
        
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do message packing for modify order message                            */
/*------------------------------------------------------------------------*/
void modify_order  (FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){
                       
    // this code just eliminates compiler warnings resulting from unused parameters
    if (group || hdr || body || msg_space || msg_size) {}
        
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do message packing for delete order message                            */
/*------------------------------------------------------------------------*/
void delete_order  (FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){
                       
    // this code just eliminates compiler warnings resulting from unused parameters
    if (group || hdr || body || msg_space || msg_size) {}
          
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do message packing for imbalance message                               */
/*------------------------------------------------------------------------*/
void imbalance     (FH_STATUS *rc, //pointer to store return code
                   struct feed_group *group,   //input ptr to feed struct
                   struct msg_hdr *hdr,        //input ptr to msg hdr
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){
                       
    // this code just eliminates compiler warnings resulting from unused parameters
    if (group || hdr || body || msg_space || msg_size) {}
    
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do message packing for feed alert                                      */
/*------------------------------------------------------------------------*/
void arca_feed_alert(FH_STATUS *rc, //pointer to store return code
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){
                       
    // this code just eliminates compiler warnings resulting from unused parameters
    if (body || msg_space || msg_size) {}
                           
    *rc = FH_OK;
};
/*------------------------------------------------------------------------*/
/* Do message packing for packet loss                                     */
/*------------------------------------------------------------------------*/
void arca_pkt_loss (FH_STATUS *rc, //pointer to store return code
                   struct msg_body *body,      //input ptr to message body
                   char   *msg_space,          //input ptr to packing space
                   int    msg_size){

    // this code just eliminates compiler warnings resulting from unused parameters
    if (body || msg_space || msg_size) {}

    *rc = FH_OK;
};
