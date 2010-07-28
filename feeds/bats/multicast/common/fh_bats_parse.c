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

// System headers
#include <stdlib.h>
#include <string.h>

// FH common headers
#include "fh_info.h"

/* FH common headers */
#include "fh_errors.h"
#include "fh_log.h"
#include "fh_alerts.h"
#include "fh_plugin_internal.h"

/* FH shared component headers */
#include "fh_shr_lh.h"
#include "fh_shr_cfg_lh.h"
#include "fh_shr_cfg_table.h"
#include "fh_shr_config.h"
#include "fh_shr_mmcast.h"
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_symbol.h"
#include "fh_shr_lkp_order.h"
#include "fh_shr_gap_fill.h"



// FH BATS headers
#include "fh_bats_revision.h"
#include "fh_bats_parse.h"
#include "fh_bats_pitch20_msg.h"


/* cached plugin hooks */
static fh_plugin_hook_t          hook_alert                 = NULL;
static fh_plugin_hook_t          hook_msg_send              = NULL;
static fh_plugin_hook_t          hook_msg_add_order_long    = NULL;
static fh_plugin_hook_t          hook_msg_add_order_short   = NULL;
static fh_plugin_hook_t          hook_msg_order_exe         = NULL;
static fh_plugin_hook_t          hook_msg_order_exe_price   = NULL;
static fh_plugin_hook_t          hook_msg_reduce_size_long  = NULL;
static fh_plugin_hook_t          hook_msg_reduce_size_short = NULL;
static fh_plugin_hook_t          hook_msg_modify_order_long = NULL;
static fh_plugin_hook_t          hook_msg_modify_order_short = NULL;
static fh_plugin_hook_t          hook_msg_order_delete      = NULL;
static fh_plugin_hook_t          hook_msg_trade_long        = NULL;
static fh_plugin_hook_t          hook_msg_trade_short       = NULL;
static fh_plugin_hook_t          hook_msg_trade_break       = NULL;
static fh_plugin_hook_t          hook_msg_end_of_session    = NULL;

/* global variables to track gap filling status */
static fh_shr_gap_fill_list_t   *gaplist                    = NULL;
static fh_shr_gap_fill_node_t   *gapnode                    = NULL;


/* macro to cache a hook function */
#define FH_BATS_PARSE_CACHE_HOOK(lc, uc)                                                        \
if (fh_plugin_is_hook_registered(FH_PLUGIN_ ## uc)) {                                           \
    hook_ ## lc = fh_plugin_get_hook(FH_PLUGIN_ ## uc);                                         \
}

/* Macro to add entry into the symbol table  */
#define FH_BATS_FETCH_SYMBOL                                                                    \
    message.sym_entry = NULL;                                                                 \
                                                                                              \
    if (conn->line->process->config->symbol_table.enabled) {                                                \
        fh_shr_lkp_sym_key_t     key;                                                         \
        fh_shr_lkp_sym_t        *entry;                                                       \
                                                                                              \
        memcpy(key.symbol, message.stock, 6);                                              \
        memset(key.symbol + 6, 0, sizeof(fh_shr_lkp_sym_key_t) - 6);                          \
        if ((fh_shr_lkp_sym_get(&conn->line->process->symbol_table, &key, &entry)) == FH_OK) { \
            message.sym_entry = entry;                                                        \
        }                                                                                     \
    }


/*  Add Order Long message processing       */
static inline FH_STATUS fh_bats_parse_add_order_long_msg(uint8_t *buffer,
                                                         uint8_t msg_length,
                                                         fh_shr_lh_conn_t *conn,
                                                         void **data, int *data_length)
{
    FH_STATUS                rc;
    fh_bats_add_order_long_t message;

    if ( msg_length != FH_BATS_ADD_ORDER_LONG_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("%s :Invalid message length received for ADD ORDER LONG, len rx = %d, should be %d",
                         conn->line->process->config->name,
                         msg_length,FH_BATS_ADD_ORDER_LONG_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.order_id      = (*((uint64_t*)(buffer + 6))) ;
    message.side          = *(buffer + 14);
    message.shares        = (*((uint32_t*)(buffer + 15)));
    memcpy(&message.stock[0], (buffer + 19), 6);
    message.price         = (*((uint64_t*)(buffer + 25)));
    message.add_flags     = *(buffer + 33);
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_ADD_ORDER_LONG_MSG_SIZE ;

    /* get the entry in the symbol table  */
    FH_BATS_FETCH_SYMBOL

    /* add a new order table entry and set it up   */

    message.ord_entry = NULL;

    if(conn->line->process->config->order_table.enabled){
        fh_shr_lkp_ord_t     entry;

        memset(&entry, 0, sizeof(fh_shr_lkp_ord_t));
        entry.key.order_no   = message.order_id;
        entry.order_no       = message.order_id;

        entry.price          = message.price;    /* Price saved as ISE format divisor 10000 */
        entry.shares         = (uint32_t)message.shares;
        entry.buy_sell_ind   = message.side;
        memcpy(&entry.stock[0], &message.stock[0], 6);
        entry.sym_entry      = message.sym_entry;
        if((fh_shr_lkp_ord_add(&conn->line->process->order_table,&entry,&message.ord_entry)) !=  FH_OK) {
            message.ord_entry = NULL;
        }else{
            FH_LOG(LH, DIAG, ("%s :Successfully added Order Entry Key = %lld",
                              conn->line->process->config->name,entry.order_no));
        }
    }
    if (hook_msg_add_order_long) {
        hook_msg_add_order_long (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/*  Add Order Short message processing       */
static inline FH_STATUS fh_bats_parse_add_order_short_msg(uint8_t *buffer,
                                                         uint8_t msg_length,
                                                         fh_shr_lh_conn_t *conn,
                                                         void **data, int *data_length)
{
    FH_STATUS                rc;
    fh_bats_add_order_short_t message;

    if ( msg_length != FH_BATS_ADD_ORDER_SHORT_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for ADD ORDER SHORT, len rx = %d, should be %d",msg_length,FH_BATS_ADD_ORDER_SHORT_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.order_id      = (*((uint64_t*)(buffer + 6))) ;
    message.side          = *(buffer + 14);
    message.shares        = (*((uint16_t*)(buffer + 15)));
    memcpy(&message.stock[0], (buffer + 17), 6);
    message.price         = ((uint64_t)(*((uint16_t*)(buffer + 23)))) * 100; /* Price saved as ISE format divisor 10000 */
    message.add_flags     = *(buffer + 25);
    /* add a new order table entry and set it up   */

    message.ord_entry = NULL;
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_ADD_ORDER_SHORT_MSG_SIZE ;
    /* get the entry in the symbol table  */
    FH_BATS_FETCH_SYMBOL

    /* add a new order table entry and set it up   */

    message.ord_entry = NULL;

    if(conn->line->process->config->order_table.enabled){
        fh_shr_lkp_ord_t     entry;

        memset(&entry, 0, sizeof(fh_shr_lkp_ord_t));
        entry.key.order_no   = message.order_id;
        entry.order_no       = message.order_id;

        entry.price          = message.price ;    /* Price saved as ISE format divisor 10000 */
        entry.shares         = (uint32_t)message.shares;
        entry.buy_sell_ind   = message.side;
        memcpy(&entry.stock[0], &message.stock[0], 6);
        entry.sym_entry      = message.sym_entry;
        if((fh_shr_lkp_ord_add(&conn->line->process->order_table,&entry,&message.ord_entry)) !=  FH_OK) {
            message.ord_entry = NULL;
        }else{
            FH_LOG(LH, DIAG, ("%s :Successfully added Order Entry Key = %lld",
                              conn->line->process->config->name,entry.order_no));
        }
    }

    if (hook_msg_add_order_short) {
        hook_msg_add_order_short (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/* Order Execute message processing         */
inline FH_STATUS fh_bats_parse_order_execute_msg(uint8_t *buffer,
                                                 uint8_t msg_length,
                                                 fh_shr_lh_conn_t *conn,
                                                 void **data, int *data_length)
{
    FH_STATUS                    rc;
    fh_bats_order_execute_t message;
    fh_shr_lkp_ord_key_t        key;

    if ( msg_length != FH_BATS_ORDER_EXECUTED_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for ORDER EXECUTED, len rx = %d, should be %d",msg_length,FH_BATS_ORDER_EXECUTED_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.order_id      = (*((uint64_t*)(buffer + 6))) ;
    message.shares        = (*((uint32_t*)(buffer + 14)));
    message.exec_id       = (*((uint64_t*)(buffer + 18))) ;
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_ORDER_EXECUTED_MSG_SIZE ;

    /* add a new order table entry and set it up   */

    message.ord_entry = NULL;

    /* Get the order from the order table and see if we can  */
    /* And then manipulate the size until the number of shares become */
    /* Zero. At which point the order table entry can be removed      */
    if(conn->line->process->config->order_table.enabled){
        memset(&key, 0, sizeof(fh_shr_lkp_ord_key_t));
        key.order_no      = message.order_id;
        if (fh_shr_lkp_ord_get(&conn->line->process->order_table,&key,&message.ord_entry) == FH_OK){
            if (message.shares <= message.ord_entry->shares){
                message.ord_entry->shares -= message.shares;
                if ( message.ord_entry->shares == 0) {
                    FH_LOG(LH, DIAG,("%s :Order Table entry after execute shares adjustment is 0 for key %lld",
                                     conn->line->process->config->name,key.order_no));
                }else{
                    FH_LOG(LH, DIAG,("%s :Order table Key = %lld executed %d left %d shares",
                                    conn->line->process->config->name,key.order_no,message.shares,
                                    message.ord_entry->shares));
                }
            }else{
                /* executed shares is more than no of shares in the Order Table */
            FH_LOG(LH, ERR, ("%s :Order Execute: Executed shares %d is GREATER than shares left %d in order Table",
                             conn->line->process->config->name,
                             message.shares,message.ord_entry->shares));
            }
        }else{
            FH_LOG(LH,ERR, ("%s :Order Execute: Could Not Find Order Table entry for Key = %lld ",
                            conn->line->process->config->name,key.order_no));
        }
    }

    if (hook_msg_order_exe) {
        hook_msg_order_exe (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/* Order Execute at Price/Size message         */

inline FH_STATUS fh_bats_parse_order_execute_price_msg(uint8_t *buffer,
                                                       uint8_t msg_length,
                                                       fh_shr_lh_conn_t *conn,
                                                       void **data, int *data_length)
{
   FH_STATUS                      rc;
    fh_bats_order_execute_price_t message;
    fh_shr_lkp_ord_key_t          key;
    fh_shr_lkp_ord_t             *entry;



    if ( msg_length != FH_BATS_ORDER_EXECUTE_PRICE_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for ORDER EXECUTE PRICE, len rx = %d, should be %d",msg_length,FH_BATS_ORDER_EXECUTE_PRICE_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.order_id      = (*((uint64_t*)(buffer + 6))) ;
    message.shares        = (*((uint32_t*)(buffer + 14)));
    message.rem_shares    = (*((uint32_t*)(buffer + 18)));
    message.exec_id       = (*((uint64_t*)(buffer + 22))) ;
    message.price         = (*((uint64_t*)(buffer + 30))) ;
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_ORDER_EXECUTE_PRICE_MSG_SIZE ;

    /* add a new order table entry and set it up   */

    message.ord_entry = NULL;

    if (conn->line->process->config->order_table.enabled) {
        memset(&key, 0, sizeof(fh_shr_lkp_ord_key_t));
        key.order_no      = message.order_id;
        if (fh_shr_lkp_ord_get(&conn->line->process->order_table,&key,&message.ord_entry) == FH_OK){
            if (message.rem_shares  == 0) {
                if( fh_shr_lkp_ord_del(&conn->line->process->order_table,&key,&entry) != FH_OK){
                    FH_LOG(LH, ERR, ("%s :Order Execute Price: Error in removing Order Table entry when shares count == 0",conn->line->process->config->name));
                }else {
                    FH_LOG(LH, DIAG, ("%s :Order Execute Price: Successfully deleted Order Id %lld, as remaining shares is 0",
                                      conn->line->process->config->name,message.order_id));

                }

            } else if ( (message.rem_shares + message.shares) == message.ord_entry->shares) {
                message.ord_entry->shares = message.rem_shares;
                message.ord_entry->price  = message.price;
            } else {/* condition where it becomes a new order */
                /* exec shares + remaining shares != existing shares */
                message.ord_entry->shares = message.rem_shares;
                message.ord_entry->price  = message.price;
            }
        }else{
            FH_LOG(LH,ERR, ("%s: Order Execute Price: Could Not Find Order Table entry for Key = %lld ",
                            conn->line->process->config->name,key.order_no));
        }
    }

    if (hook_msg_order_exe_price) {
        hook_msg_order_exe_price (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/* Reduce Size Long message processing         */
inline FH_STATUS fh_bats_parse_reduce_size_long_msg(uint8_t *buffer,
                                                    uint8_t msg_length,
                                                    fh_shr_lh_conn_t *conn,
                                                    void **data, int *data_length)
{
    FH_STATUS                rc;
    fh_bats_reduce_size_long_t message;
    fh_shr_lkp_ord_key_t          key;

    if ( msg_length != FH_BATS_REDUCE_SIZE_LONG_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for REDUCE SIZE LONG, len rx = %d, should be %d",msg_length,FH_BATS_ORDER_EXECUTED_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.order_id      = (*((uint64_t*)(buffer + 6))) ;
    message.shares        = (*((uint32_t*)(buffer + 14)));
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_REDUCE_SIZE_LONG_MSG_SIZE ;

    /* add a new order table entry and set it up   */

    message.ord_entry = NULL;

    if (conn->line->process->config->order_table.enabled) {
        memset(&key, 0, sizeof(fh_shr_lkp_ord_key_t));
        key.order_no      = message.order_id;
        if (fh_shr_lkp_ord_get(&conn->line->process->order_table,&key,&message.ord_entry) == FH_OK){
            message.ord_entry->shares -= message.shares;
        }else{
            FH_LOG(LH,ERR, ("%s: Reduce Size Long: Could Not Find Order Table entry for Key = %lld ",
                            conn->line->process->config->name, key.order_no));
        }
    }

    if (hook_msg_reduce_size_long) {
        hook_msg_reduce_size_long (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/* Reduce Size Short message processing         */
inline FH_STATUS fh_bats_parse_reduce_size_short_msg(uint8_t *buffer,
                                                    uint8_t msg_length,
                                                    fh_shr_lh_conn_t *conn,
                                                    void **data, int *data_length)
{
    FH_STATUS                rc;
    fh_bats_reduce_size_short_t message;
    fh_shr_lkp_ord_key_t          key;

    if ( msg_length != FH_BATS_REDUCE_SIZE_SHORT_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for REDUCE SIZE SHORT, len rx = %d, should be %d",msg_length,FH_BATS_REDUCE_SIZE_SHORT_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.order_id      = (*((uint64_t*)(buffer + 6))) ;
    message.shares        = (*((uint16_t*)(buffer + 14)));
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_REDUCE_SIZE_SHORT_MSG_SIZE ;

    /* add a new order table entry and set it up   */

    message.ord_entry = NULL;

    if (conn->line->process->config->order_table.enabled) {
        memset(&key, 0, sizeof(fh_shr_lkp_ord_key_t));
        key.order_no      = message.order_id;
        if (fh_shr_lkp_ord_get(&conn->line->process->order_table,&key,&message.ord_entry) == FH_OK){
            message.ord_entry->shares -= (uint32_t)message.shares;
        }else{
            FH_LOG(LH,ERR, ("%s :Reduce Size Short: Could Not Find Order Table entry for Key = %lld ",
                            conn->line->process->config->name,key.order_no));
        }
    }

    if (hook_msg_reduce_size_short) {
        hook_msg_reduce_size_short (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/*  Modify Order Long message processing       */
static inline FH_STATUS fh_bats_parse_modify_order_long_msg(uint8_t *buffer,
                                                            uint8_t msg_length,
                                                            fh_shr_lh_conn_t *conn,
                                                            void **data, int *data_length)
{
    FH_STATUS                rc;
    fh_bats_modify_long_t message;
    fh_shr_lkp_ord_key_t          key;

    if ( msg_length != FH_BATS_MODIFY_LONG_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for MODIFY ORDER LONG, len rx = %d, should be %d",msg_length,FH_BATS_MODIFY_LONG_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.order_id      = (*((uint64_t*)(buffer + 6))) ;
    message.shares        = (*((uint32_t*)(buffer + 14)));
    message.price         = (*((uint64_t*)(buffer + 18)));
    message.modify_flags  = *(buffer + 26);
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_MODIFY_LONG_MSG_SIZE ;

    /* add a new order table entry and set it up   */

    message.ord_entry = NULL;

    if (conn->line->process->config->order_table.enabled) {
        memset(&key, 0, sizeof(fh_shr_lkp_ord_key_t));
        key.order_no      = message.order_id;
        if (fh_shr_lkp_ord_get(&conn->line->process->order_table,&key,&message.ord_entry) == FH_OK){
            message.ord_entry->shares = message.shares;
            message.ord_entry->price  = message.price;
        }else{
            FH_LOG(LH,ERR, ("%s :Modify Order Long: Could Not Find Order Table entry for Key = %lld ",
                            conn->line->process->config->name,key.order_no));
        }
    }

    if (hook_msg_modify_order_long) {
        hook_msg_modify_order_long (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/*  Modify Order Short message processing       */
static inline FH_STATUS fh_bats_parse_modify_order_short_msg(uint8_t *buffer,
                                                           uint8_t msg_length,
                                                           fh_shr_lh_conn_t *conn,
                                                           void **data, int *data_length)
{
    FH_STATUS                rc;
    fh_bats_modify_short_t message;
    fh_shr_lkp_ord_key_t          key;

    if ( msg_length != FH_BATS_MODIFY_SHORT_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for MODIFY ORDER SHORT, len rx = %d, should be %d",msg_length,FH_BATS_MODIFY_SHORT_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.order_id      = (*((uint64_t*)(buffer + 6))) ;
    message.shares        = (*((uint16_t*)(buffer + 14)));
    message.price         = ((uint64_t)(*((uint16_t*)(buffer + 16)))) * 100;
    message.modify_flags  = *(buffer + 18);
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_MODIFY_SHORT_MSG_SIZE ;

    /* add a new order table entry and set it up   */

    message.ord_entry = NULL;

    if (conn->line->process->config->order_table.enabled) {
        memset(&key, 0, sizeof(fh_shr_lkp_ord_key_t));
        key.order_no      = message.order_id;
        if (fh_shr_lkp_ord_get(&conn->line->process->order_table,&key,&message.ord_entry) == FH_OK){
            message.ord_entry->shares = (uint32_t)message.shares;
            message.ord_entry->price  = message.price;
        }else{
            FH_LOG(LH,ERR, ("%s :Modify Order Short: Could Not Find Order Table entry for Key = %lld ",
                            conn->line->process->config->name,key.order_no));
        }
    }

    if (hook_msg_modify_order_short) {
        hook_msg_modify_order_short (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}


/*  Delete Order message processing       */
static inline FH_STATUS fh_bats_parse_delete_order_msg(uint8_t *buffer,
                                                           uint8_t msg_length,
                                                           fh_shr_lh_conn_t *conn,
                                                           void **data, int *data_length)
{
    FH_STATUS                rc;
    fh_bats_delete_order_t message;
    fh_shr_lkp_ord_key_t          key;
    fh_shr_lkp_ord_t             *entry;

    if ( msg_length != FH_BATS_DELETE_ORDER_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for DELETE ORDER SHORT, len rx = %d, should be %d",msg_length,FH_BATS_DELETE_ORDER_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.order_id      = (*((uint64_t*)(buffer + 6))) ;
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_DELETE_ORDER_MSG_SIZE ;

    if (conn->line->process->config->order_table.enabled) {
        memset(&key, 0, sizeof(fh_shr_lkp_ord_key_t));
        key.order_no      = message.order_id;

        if (fh_shr_lkp_ord_del(&conn->line->process->order_table,&key,&entry) != FH_OK){
            FH_LOG(LH,ERR, ("%s :Delete Order: Could Not Delete Order Table entry for Key = %lld ",
                            conn->line->process->config->name, key.order_no));
        }else{
            FH_LOG(LH,DIAG, ("%s :Delete Order: Successfully deleted order table entry for Key = %lld ",
                             conn->line->process->config->name, key.order_no));
        }
    }

    if (hook_msg_order_delete) {
        hook_msg_order_delete (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/*  Trade Long message processing       */
static inline FH_STATUS fh_bats_parse_trade_long_msg(uint8_t *buffer,
                                                     uint8_t msg_length,
                                                     fh_shr_lh_conn_t *conn,
                                                     void **data, int *data_length)
{
    FH_STATUS                rc;
    fh_bats_trade_long_t message;

    if ( msg_length != FH_BATS_TRADE_LONG_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for TRADE LONG, len rx = %d, should be %d",msg_length,FH_BATS_TRADE_LONG_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.order_id      = (*((uint64_t*)(buffer + 6))) ;
    message.side          = *(buffer + 14);
    message.shares        = (*((uint32_t*)(buffer + 15)));
    memcpy(&message.stock[0], (buffer + 19), 6);
    message.price         = (*((uint64_t*)(buffer + 25)));
    message.execution_id  = (*((uint64_t*)(buffer + 33)));
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_TRADE_LONG_MSG_SIZE ;

    if (hook_msg_trade_long) {
        hook_msg_trade_long (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/*  Trade Short message processing       */
static inline FH_STATUS fh_bats_parse_trade_short_msg(uint8_t *buffer,
                                                      uint8_t msg_length,
                                                      fh_shr_lh_conn_t *conn,
                                                      void **data, int *data_length)
{
    FH_STATUS                rc;
    fh_bats_trade_short_t message;

    if ( msg_length != FH_BATS_TRADE_SHORT_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for TRADE SHORT, len rx = %d, should be %d",msg_length,FH_BATS_TRADE_SHORT_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.order_id      = (*((uint64_t*)(buffer + 6))) ;
    message.side          = *(buffer + 14);
    message.shares        = (uint32_t)(*((uint16_t*)(buffer + 15)));
    memcpy(&message.stock[0], (buffer + 17), 6);
    message.price         = ((uint64_t)(*((uint16_t*)(buffer + 23)))) * 100;
    message.execution_id  = (*((uint64_t*)(buffer + 25))) ;
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_TRADE_SHORT_MSG_SIZE ;

    if (hook_msg_trade_short) {
        hook_msg_trade_short (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/*  Trade break message processing       */
static inline FH_STATUS fh_bats_parse_trade_break_msg(uint8_t *buffer,
                                                      uint8_t msg_length,
                                                      fh_shr_lh_conn_t *conn,
                                                      void **data, int *data_length)
{
    FH_STATUS                rc;
    fh_bats_trade_break_t message;

    if ( msg_length != FH_BATS_TRADE_BREAK_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for TRADE SHORT, len rx = %d, should be %d",msg_length,FH_BATS_TRADE_BREAK_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.seq_no    = conn->line->next_seq_no;
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));

    message.execution_id  = (*((uint64_t*)(buffer + 6))) ;
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_TRADE_BREAK_MSG_SIZE ;

    if (hook_msg_trade_break) {
        hook_msg_trade_break (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/*  End of Session message processing       */
static inline FH_STATUS fh_bats_parse_end_of_session_msg(uint8_t *buffer,
                                                         uint8_t msg_length,
                                                         fh_shr_lh_conn_t *conn,
                                                         void **data, int *data_length)
{
    FH_STATUS                rc;
    fh_bats_end_of_session_t message;

    if ( msg_length != FH_BATS_END_OF_SESSION_MSG_SIZE ) {
        FH_LOG(LH, ERR, ("Invalid message length received for TRADE SHORT, len rx = %d, should be %d",msg_length,FH_BATS_END_OF_SESSION_MSG_SIZE));
               return FH_ERROR;
    }

    /* parse fields out of the buffer into the message  */
    message.hdr.msg_type  = *(buffer + 1);
    message.hdr.timestamp = conn->timestamp + ( (uint64_t)(*((uint32_t*)(buffer + 2))));
    message.raw.message      = buffer;
    message.raw.len          = FH_BATS_END_OF_SESSION_MSG_SIZE ;

    FH_LOG( LH, DIAG, ("%s : Received End Of Session Message for Line %s",
                       conn->line->process->config->name,
                       conn->line->config->name ));

    if (hook_msg_end_of_session) {
        hook_msg_end_of_session (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = msg_length;
    }
    return FH_OK;

}

/*
 * Process an BATS message from the passed in buffer
 */
static inline int fh_bats_parse_msg(uint8_t *buffer, int length, fh_shr_lh_conn_t *conn)
{
    fh_shr_cfg_lh_line_t    *linecfg    = conn->line->config;
    uint8_t                  msg_length;
    uint8_t                  msg_type;
    void                    *data;
    int                      data_length;
    FH_STATUS                rc;

    /* make sure there are enough bytes remaining and then parse off the message length */
    if (length < 1) {
        FH_LOG(LH, ERR, ("message block < 1 byte on line %s (%s)", linecfg->name, conn->tag));
        conn->stats.message_errors++;
        return -1;
    }
    msg_length   = *buffer;
    //length      -= 1;
    //buffer      += 1;

    /* make sure there are enough bytes remaining for the message itself */
    if (length < msg_length) {
        FH_LOG(LH, ERR, ("buffer length (%d bytes) < message length (%d bytes) on line %s (%s)",
                         length, msg_length, linecfg->name, conn->tag));
        conn->stats.message_errors++;
        return -1;

    }
    else if (msg_length < 6) {
        FH_LOG(LH, ERR, ("invalid message length (%d) on line %s (%s)",
                         msg_length, linecfg->name, conn->tag));
        conn->stats.message_errors++;
        return -1;
    }

    /* read the message type and process the message */
    msg_type = *(buffer + 1);
    switch (msg_type) {

    /* convert the seconds after midnight to a nanoseconds timestamp field   */
    case TIME_MESSAGE_TYPE :
        conn->timestamp = ((uint64_t)(*((uint32_t *)(buffer + 2)))) * 1000000000;
        return msg_length ;

    /* process the add order long message */
    case ADD_ORDER_LONG_MESSAGE_TYPE :
        if(fh_bats_parse_add_order_long_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a add prder short message */
    case ADD_ORDER_SHORT_MESSAGE_TYPE :
        if (fh_bats_parse_add_order_short_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a Order Execute message */
    case ORDER_EXECUTED_MESSAGE_TYPE :
        if (fh_bats_parse_order_execute_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a Order Execute Price message */
    case ORDER_EXECUTE_PRICE_MESSAGE_TYPE :
        if (fh_bats_parse_order_execute_price_msg(buffer, msg_length, conn, &data,
                                              &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a Reduce Size Long message */
    case REDUCE_SIZE_LONG_MESSAGE_TYPE :
        if (fh_bats_parse_reduce_size_long_msg(buffer, msg_length, conn, &data,
                                              &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a Reduce Size Short message */
    case REDUCE_SIZE_SHORT_MESSAGE_TYPE :
        if (fh_bats_parse_reduce_size_short_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process Modify Long message */
    case MODIFY_LONG_MESSAGE_TYPE :
        if (fh_bats_parse_modify_order_long_msg(buffer, msg_length, conn, &data,
                                             &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process Modify Short message */
    case MODIFY_SHORT_MESSAGE_TYPE :
        if (fh_bats_parse_modify_order_short_msg(buffer, msg_length, conn, &data,
                                             &data_length) != FH_OK) {
            return -1;
        }
        break;


    /* process a Delete Order message */
    case DELETE_ORDER_MESSAGE_TYPE:
        if (fh_bats_parse_delete_order_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process Trade Long  message */
    case TRADE_LONG_MESSAGE_TYPE :
        if (fh_bats_parse_trade_long_msg(buffer, msg_length, conn, &data,
                                              &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process Trade Short  message */
    case TRADE_SHORT_MESSAGE_TYPE :
        if (fh_bats_parse_trade_short_msg(buffer, msg_length, conn, &data,
                                              &data_length) != FH_OK) {
            return -1;
        }
        break;


    /* process Trade Break  message */
    case TRADE_BREAK_MESSAGE_TYPE :
        if (fh_bats_parse_trade_break_msg(buffer, msg_length, conn, &data,
                                              &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process an end of session message */
    case END_OF_SESSION_MESSAGE_TYPE :
        if (fh_bats_parse_end_of_session_msg(buffer, msg_length, conn, &data,
                                           &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* message type is unknown */
    default:
        FH_LOG(LH, ERR, ("unknown/invalid message type %c on line %s (%s)",
                         msg_type, linecfg->name, conn->tag));
        conn->stats.message_errors++;
        return -1;
    }

    /* there is data to send so... */
    if (hook_msg_send) {
        hook_msg_send(&rc, data, data_length);
        if (rc != FH_OK) {
            return -1;
        }
    }

    /* if we get here, success so return the number of bytes we used */
    return msg_length ;
}

/*
 * Return true if the packet is a duplicate, false otherwise
 */
static inline bool fh_bats_parse_duplicate(sequenced_unit_header_t *header, fh_shr_lh_conn_t *conn)
{
    fh_shr_lh_line_t *line = conn->line;
    int               rc;
    int               gap_size;

    FH_LOG(LH, DIAG, ("%s :duplicate detection (%s) -- line %lu, header %lu",
                      conn->line->process->config->name,
                      conn->tag, line->next_seq_no, header->seq_no));

    /* first, flush out any expired gaps and declare loss if appropriate */
    if (gaplist && gaplist->count > 0) {
        rc = fh_shr_gap_fill_flush(gaplist);
        if (rc > 0) {
            conn->stats.lost_messages += rc;
            if (hook_alert) {
                hook_alert(&rc, FH_ALERT_LOSS, conn);
            }
        }
    }

    /* if the sequence number is smaller than the current line sequence number, drop the packet */
    if (header->seq_no < line->next_seq_no) {
        /* see if there is a gap list entry for this sequence number */
        gapnode = fh_shr_gap_fill_find(gaplist, header->seq_no);
        if (gapnode != NULL) {
            return false;
        }
        return true;
    }
    /* if the sequence number is larger than the current line sequence number, there is a gap */
    else if (header->seq_no > line->next_seq_no) {
        FH_LOG(LH, WARN, ("%s: gap on line %s (%s) -- expected %lu, got %lu",
                          conn->line->process->config->name,
                          line->config->name, conn->tag, line->next_seq_no, header->seq_no));

        /* increment gap statistics and calculate the size of the gap */
        conn->stats.gaps++;
        gap_size = header->seq_no - line->next_seq_no;

        /* if there is a gap list configured, insert into the gap list */
        if (gaplist) {
            rc = fh_shr_gap_fill_push(gaplist, line->next_seq_no, gap_size);
            if (rc < 0) {
                FH_LOG(LH, ERR, ("%s: error adding gap to the gap list -- messages lost",
                                 conn->line->process->config->name));
                conn->stats.lost_messages += gap_size;
                if (hook_alert) {
                    hook_alert(&rc, FH_ALERT_LOSS, conn);
                }
            }
            else if (rc > 0) {
                FH_LOG(LH, WARN, ("%s: gap list overflow -- %d messages lost",
                                  conn->line->process->config->name,rc));
                conn->stats.lost_messages += rc;
                if (hook_alert) {
                    hook_alert(&rc, FH_ALERT_LOSS, conn);
                }
            }
        }
        /* if there is no gap list, just add the size of the gap to the message loss counter */
        else {
            conn->stats.lost_messages += gap_size;
            if (hook_alert) {
                hook_alert(&rc, FH_ALERT_LOSS, conn);
            }
        }

        /* update the next-expected-sequence-number so we can move on */
        line->next_seq_no = header->seq_no;

        /* if an alert hook has been registered, tell it there is a gap */
        if (hook_alert) {
            hook_alert(&rc, FH_ALERT_GAP, conn);
        }
    }

    /* if we have gotten here, packet is not a duplicate so return false */
    return false;
}


FH_STATUS fh_bats_parse_pkt(uint8_t *packet, int length, fh_shr_lh_conn_t * conn)
{

    sequenced_unit_header_t  pkt_header;   /*  header on the packet       */
    int          i;
    int          bytes_used;
    int          rc;
    uint64_t     next_packet_seqno;
    fh_shr_cfg_lh_line_t    *linecfg = conn->line->config;

    /* increment the number of bytes received on this line by the packet length */
    conn->stats.bytes += length;

    /* make sure the packet is at least the size of a "sequenced unit header" */
    if (length < FH_BATS_SEQUENCE_UNIT_HEADER_SIZE) {
        FH_LOG(LH, ERR, ("invalid packet size on line %s (%s)", linecfg->name, conn->tag));
        /* increment the number of bytes received on this line by the packet length */
        conn->stats.packet_errors++;
        return FH_ERROR;
    }

    /* populate the packet header with data from the buffer */
    fh_bats_pitch20_extract(packet, &pkt_header);

    /* perform duplicate detection on this packet */
    if (fh_bats_parse_duplicate(&pkt_header, conn)) {
        conn->stats.duplicate_packets++;
        return FH_OK;
    }

    /* if the packet is a heartbeat  */
    if (pkt_header.msg_count == 0 ) {
        /* if the new sequence number is bigger than the old */
        if (pkt_header.seq_no > conn->line->next_seq_no ) {
            conn->line->next_seq_no = pkt_header.seq_no;
        }
        return FH_OK;
    }

    /* adjust the buffer and count of bytes remaining */
    packet += FH_BATS_SEQUENCE_UNIT_HEADER_SIZE;
    length -= FH_BATS_SEQUENCE_UNIT_HEADER_SIZE;

    /* when we get here it means the packet is going to be processed so we must prepare */
    next_packet_seqno = (gapnode) ? conn->line->next_seq_no : pkt_header.seq_no;
    conn->line->next_seq_no = pkt_header.seq_no;

    /* process each message in the packet */
    for (i = 0; i < pkt_header.msg_count; i++) {
        if ((bytes_used = fh_bats_parse_msg(packet, length, conn)) < 0) {
            conn->line->next_seq_no = next_packet_seqno;
            return FH_ERROR;
        }

        /* if we are currently filling a gap, remove this sequence number */
        if (gapnode) {
            rc = fh_shr_gap_fill_del(&gapnode, conn->line->next_seq_no);
            /* everything was normal removing the sequence number */
            if (rc == 0) {
                conn->stats.recovered_messages++;
            }
            /* if some sequence numbers were deleted in the process... */
            else if(rc > 0) {
                FH_LOG(LH, WARN, ("gap list size exceeded -- messages have been lost"));
                conn->stats.lost_messages += rc;
                conn->stats.recovered_messages++;
                if (hook_alert) {
                    hook_alert(&rc, FH_ALERT_LOSS, conn);
                }
            }
            /* there was an error removing the sequence number */
            else if (rc < 0) {
                FH_LOG(LH, WARN, ("unable to remove gap list entry -- will likely result in loss"));
            }

            /* gap_fill_del sets pointer to NULL if the gap node has been filled */
            if (gapnode == NULL) {
                /* make sure the loop will not execute another time */
                i = 10000;

                /* check whether all gaps have now been filled and send an alert if they have */
                if (gaplist->count <= 0 && hook_alert) {
                    hook_alert(&rc, FH_ALERT_NOGAP, conn);
                }
            }
        }
        /* if we are not filling a gap we have advanced the global seqno cursor */
        else {
            next_packet_seqno++;
        }

        /* adjust counters, sequence numbers, etc */
        packet += bytes_used;
        length -= bytes_used;
        conn->line->next_seq_no++;
        conn->stats.messages++;
    }

    /* if we get here, success */
    conn->line->next_seq_no = next_packet_seqno;
    return FH_OK;

}



/*
 * Function to initialize the message parser
 */
FH_STATUS fh_bats_parse_init(fh_shr_lh_proc_t *process)
{
    /* cache any hooks that have been registered and will later be called */
    FH_BATS_PARSE_CACHE_HOOK(alert,                  ALERT);
    FH_BATS_PARSE_CACHE_HOOK(msg_send,               MSG_SEND);
    FH_BATS_PARSE_CACHE_HOOK(msg_add_order_long,     BATS_MSG_ADD_ORDER_LONG);
    FH_BATS_PARSE_CACHE_HOOK(msg_add_order_short,    BATS_MSG_ADD_ORDER_SHORT);
    FH_BATS_PARSE_CACHE_HOOK(msg_order_exe,          BATS_MSG_ORDER_EXE);
    FH_BATS_PARSE_CACHE_HOOK(msg_order_exe_price,    BATS_MSG_ORDER_EXE_PRICE);
    FH_BATS_PARSE_CACHE_HOOK(msg_reduce_size_long,   BATS_MSG_REDUCE_SIZE_LONG);
    FH_BATS_PARSE_CACHE_HOOK(msg_reduce_size_short,  BATS_MSG_REDUCE_SIZE_SHORT);
    FH_BATS_PARSE_CACHE_HOOK(msg_modify_order_long,  BATS_MSG_MODIFY_ORDER_LONG);
    FH_BATS_PARSE_CACHE_HOOK(msg_modify_order_short, BATS_MSG_MODIFY_ORDER_SHORT);
    FH_BATS_PARSE_CACHE_HOOK(msg_order_delete,       BATS_MSG_ORDER_DELETE);
    FH_BATS_PARSE_CACHE_HOOK(msg_trade_long,         BATS_MSG_TRADE_LONG);
    FH_BATS_PARSE_CACHE_HOOK(msg_trade_short,        BATS_MSG_TRADE_SHORT);
    FH_BATS_PARSE_CACHE_HOOK(msg_trade_break,        BATS_MSG_TRADE_BREAK);
    FH_BATS_PARSE_CACHE_HOOK(msg_end_of_session,     BATS_MSG_END_OF_SESSION);

    /* initialize the static gap tracking list to the max number of entries configured */
    if (process->config->gap_list_max > 0) {
        gaplist = fh_shr_gap_fill_new((uint32_t)process->config->gap_list_max,
                                      (uint32_t)process->config->gap_timeout);
    }

    /* if we get here, success */
    return FH_OK;

}
