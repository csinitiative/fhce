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

/* System headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


/* FH common headers */
#include "fh_errors.h"
#include "fh_config.h"
#include "fh_log.h"
#include "fh_plugin_internal.h"
#include "fh_alerts.h"

/* Order and symbol table */
#include "fh_mpool.h"
#include "fh_htable.h"

/* shared library headers */
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_symbol.h"
#include "fh_shr_cfg_table.h"


#include "fh_shr_cfg_table.h"
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_order.h"
#include "fh_shr_lkp_symbol.h"

/* FH Dir Edge headers */
#include "fh_shr_tcp_lh.h"
#include "fh_edge_parse.h"
#include "fh_edge_msg.h"



/* cached plugin hooks */
static fh_plugin_hook_t     hook_alert                  = NULL;
static fh_plugin_hook_t     hook_msg_send               = NULL;
static fh_plugin_hook_t     hook_msg_system_event       = NULL;
static fh_plugin_hook_t     hook_msg_add_order          = NULL;
static fh_plugin_hook_t     hook_msg_order_executed     = NULL;
static fh_plugin_hook_t     hook_msg_order_canceled     = NULL;
static fh_plugin_hook_t     hook_msg_trade              = NULL;
static fh_plugin_hook_t     hook_msg_broken_trade       = NULL;
static fh_plugin_hook_t     hook_msg_security_status    = NULL;

/* macro to cache a hook function */
#define FH_DIREDGE_PARSE_CACHE_HOOK(lc, uc)                                                     \
if (fh_plugin_is_hook_registered(FH_PLUGIN_ ## uc)) {                                           \
    hook_ ## lc = fh_plugin_get_hook(FH_PLUGIN_ ## uc);                                         \
}

#define FH_DE_FETCH_SYMBOL                                                                    \
    message.sym_entry = NULL;                                                                 \
                                                                                              \
    if (line->process->symbol_table.enabled) {                                                \
        fh_shr_lkp_sym_key_t     key;                                                         \
        fh_shr_lkp_sym_t        *entry;                                                       \
                                                                                              \
        memcpy(key.symbol, message.security, 6);                                              \
        memset(key.symbol + 6, 0, sizeof(fh_shr_lkp_sym_key_t) - 6);                          \
        if ((fh_shr_lkp_sym_get(&conn->line->process->symbol_table, &key, &entry)) == FH_OK) { \
            message.sym_entry = entry;                                                        \
        }                                                                                     \
    }

/* Macro to add a new order table entry  */
#if 0
#define FH_DE_ADD_ORDER_TABLE_ENTRY                                                          \
    message.ord_entry = NULL;                                                              \
                                                                                             \
    if(line->process->order_table.enabled){                                                  \
        fh_shr_lkp_ord_t     entry;                                                          \
                                                                                             \
        memset(&entry, 0, sizeof(fh_shr_lkp_ord_t));                                          \
        memcpy(&entry.key.order_no_str[0],&message.order_ref[0], 12);                         \
        entry.price = message.price;                                                         \
        entry.shares = message.order_quantity;                                               \
        entry.buy_sell_ind = message.side_indicator;                                         \
        memcpy(&entry.stock[0], &message.security[0], 6);                                        \
        entry.sym_entry = message.sym_entry;                                                 \
        if((fh_shr_lkp_ord_add(&conn->line->process->order_table,&entry,&message.ord_entry)) !=  FH_OK) {\
        message.ord_entry = NULL;   \
        } \
    }
#endif

/* convert a sized ascii coded integer field to unsigned int */
static inline uint32_t  fh_dirEdge_convert_int(char *src, int field_sz)
{
    uint32_t temp = 0;
    int i;
    int y ;
    for ( i = field_sz -1, y = 1; i >= 0; i--) {
        if(src[i] == ' ') break;
        temp += ((src[i]) - 0x30) * y;
        y = y *10;
    }
    return temp;
}


/*
 * Convert a 10 character ascii coded integer to a 64 bit int value
 */
uint64_t fh_dirEdge_get_price(char * price)
{
    uint64_t temp = 0;
    int i;
    int y ;
    for ( i = 9, y = 1; i >= 0; i--) {
        if(price[i] == ' ') break;
        temp += ((price[i]) - 0x30) * y;
        y = y *10;
    }
    return temp;
}


/*
 * System Event Message Parsing function
 */
static inline FH_STATUS fh_de_parse_sys_event_msg(char * buffer, int len,fh_shr_lh_conn_t* conn,fh_shr_cfg_lh_line_t *line,
                                                  void **data, int * data_length)
{
    FH_STATUS rc;
    fh_edge_sysEvent_msg_t message;

    if(line) {}

    if (len != SYSTEM_EVENT_MSG_SIZE){
        FH_LOG(LH, ERR, (" Invalid System Event Message received message type rx = %c, length = %d",
                         *(buffer + 9), len));
        return FH_ERROR;
    }

    message.hdr.timestamp    = fh_dirEdge_convert_int(buffer + 1, 8);
    message.hdr.message_type = *(buffer + 9);
    message.event_code       = *(buffer + 10);
    message.raw.message      = buffer;
    message.raw.len          = SYSTEM_EVENT_MSG_SIZE; /* replace with define */
    if ( hook_msg_system_event ) {
        hook_msg_system_event (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = len;
    }

    return FH_OK;

}

/*
 * Add Order Message parsing function
 */
static inline FH_STATUS fh_de_parse_add_order_msg(char * buffer, int len,fh_shr_lh_conn_t* conn,
                                                  fh_shr_cfg_lh_line_t *line,
                                                  void **data, int * data_length)
{
    FH_STATUS  rc;
    fh_edge_add_order_msg_t message;


    if ((len != ADD_ORDER_MSG_SIZE) && ( len != ADD_ORDER_ATTIRBUTED_QUOTE_MSG_SIZE)){
        FH_LOG(LH, ERR, (" Invalid Add Order Message size received message type rx = %c, length = %d",
                         *(buffer + 9), len));
        return FH_ERROR;
    }

    message.hdr.timestamp    = fh_dirEdge_convert_int(buffer + 1, 8);
    message.hdr.message_type = *(buffer + 9);
    memcpy(&message.order_ref[0], (buffer +10), 12);
    message.side_indicator   = *(buffer + 22);
    message.order_quantity   = fh_dirEdge_convert_int(buffer + 23, 6);
    memcpy(&message.security, (buffer + 29),6);
    message.price            = fh_dirEdge_get_price(buffer+35);

    /* get the symbol table entry for this symbol */
    FH_DE_FETCH_SYMBOL

    /* adda new order table entry and set it up   */
    //FH_DE_ADD_ORDER_TABLE_ENTRY

    message.ord_entry = NULL;

    if(line->process->order_table.enabled){
        fh_shr_lkp_ord_t     entry;

        memset(&entry, 0, sizeof(fh_shr_lkp_ord_t));
        memcpy(&entry.key.order_no_str[0],&message.order_ref[0], 12);
        memcpy(&entry.order_no_str[0],&message.order_ref[0], 12);
        entry.price = message.price;
        entry.shares = message.order_quantity;
        entry.buy_sell_ind = message.side_indicator;
        memcpy(&entry.stock[0], &message.security[0], 6);
        entry.sym_entry = message.sym_entry;
        if((fh_shr_lkp_ord_add(&conn->line->process->order_table,&entry,&message.ord_entry)) !=  FH_OK) {
            message.ord_entry = NULL;
        }else{
            FH_LOG(LH, DIAG, ("Successfully added Order Entry Key = %12s",&entry.order_no_str[0]));
        }
    }

    message.display          = *(buffer+45);
    if(message.display == 'A'){
        strncpy(&message.mmid[0],(buffer+46),4);
        message.raw.len          = ADD_ORDER_ATTIRBUTED_QUOTE_MSG_SIZE;
    } else {
        message.raw.len          = SYSTEM_EVENT_MSG_SIZE;
    }
    message.raw.message      = buffer;


    if (hook_msg_add_order) {
        hook_msg_add_order (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = len;
    }
    return FH_OK;
}


/*
 *  Order Executed Message parsing function
 */
static inline FH_STATUS fh_de_parse_order_executed_msg(char * buffer, int len,
                                                       fh_shr_lh_conn_t* conn,
                                                       fh_shr_cfg_lh_line_t * line,
                                                       void **data, int * data_length)
{
    FH_STATUS  rc;
    fh_edge_orderExecuted_msg_t message;
    fh_shr_lkp_ord_key_t              key;
    fh_shr_lkp_ord_t            *entry;

    if (len != ORDER_EXECUTED_MSG_SIZE){
        FH_LOG(LH, ERR, (" Invalid Order Executed Message size received message type rx = %c, length = %d",
                         *(buffer + 9), len));
        return FH_ERROR;
    }

    message.hdr.timestamp    = fh_dirEdge_convert_int(buffer + 1,8);
    message.hdr.message_type = *(buffer + 9);
    memcpy(&message.order_ref[0], (buffer +10), 12);
    message.executed_shares  = fh_dirEdge_convert_int(buffer + 22, 6);
    memcpy(&message.match_number, (buffer + 28),21);
    message.raw.message      = buffer;
    message.raw.len          = ORDER_EXECUTED_MSG_SIZE;

    /* Get the order from the order table and see if we can  */
    /* And then manipulate the size until the number of shares become */
    /* Zero. At which point the order table entry can be removed      */
    if(line->process->order_table.enabled){
        memset(&key, 0, sizeof(fh_shr_lkp_ord_key_t));
        memcpy(&key.order_no_str[0], &message.order_ref[0], 12);
        if (fh_shr_lkp_ord_get(&conn->line->process->order_table,&key,&message.ord_entry) == FH_OK){
            if (message.executed_shares <= message.ord_entry->shares){
                message.ord_entry->shares -= message.executed_shares;
                if ( message.ord_entry->shares == 0) {
                    FH_LOG(LH, DIAG,("Order Table entry after execute shares adjustment is 0 for key %20s",&key.order_no_str[0]));
                    /* we have to remove the order table entry completely */
                    if( fh_shr_lkp_ord_del(&conn->line->process->order_table,&key,&entry) != FH_OK){
                        FH_LOG(LH, ERR, ("Error in removing Order Table entry when shares count == 0"));
                    }
                }else{
                    FH_LOG(LH, DIAG,("Order table Key = %20s executed %d left %d shares",
                                    &key.order_no_str[0],message.executed_shares,
                                    message.ord_entry->shares));
                }
            }else{
                /* executed shares is more than no of shares in the Order Table */
            FH_LOG(LH, ERR, (" Executed shares %d is GREATER than shares left %d in order Table",
                             message.executed_shares,message.ord_entry->shares));
            }
        }else{
            FH_LOG(LH,ERR, ("Could Not Find Order Table entry for Key = %20s ",&key.order_no_str[0]));
        }
    }

    if (hook_msg_order_executed) {
        hook_msg_order_executed (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = len;
    }
    return FH_OK;
}


/*
 *  Order Cancelled Message parsing function
 */
static inline FH_STATUS fh_de_parse_order_canceled_msg(char * buffer, int len,
                                                       fh_shr_lh_conn_t* conn,
                                                       fh_shr_cfg_lh_line_t *line,
                                                       void **data, int * data_length)
{
    FH_STATUS  rc;
    fh_edge_orderCanceled_msg_t message;
    fh_shr_lkp_ord_key_t              key;
    fh_shr_lkp_ord_t            *entry;


    if (len != ORDER_CANCELED_MSG_SIZE){
        FH_LOG(LH, ERR, (" Invalid Order Canceled Message size received message type rx = %c, length = %d",
                         *(buffer + 9), len));
        return FH_ERROR;
    }

    message.hdr.timestamp    = fh_dirEdge_convert_int(buffer + 1,8);
    message.hdr.message_type = *(buffer + 9);
    memcpy(&message.order_ref[0], (buffer +10), 12);
    message.canceled_shares  = fh_dirEdge_convert_int(buffer + 22, 6);
    message.raw.message      = buffer;
    message.raw.len          = ORDER_CANCELED_MSG_SIZE;
    /* Get the order from the order table and see if we can  */
    /* And then manipulate the size until the number of shares become */
    /* Zero. At which point the order table entry can be removed      */
    if(line->process->order_table.enabled){
        memset(&key, 0, sizeof(fh_shr_lkp_ord_key_t));
        memcpy(&key.order_no_str[0], &message.order_ref[0], 12);
        if (fh_shr_lkp_ord_get(&conn->line->process->order_table,&key,&message.ord_entry) == FH_OK){
            if (message.canceled_shares <= message.ord_entry->shares){
                message.ord_entry->shares -= message.canceled_shares;
                if ( message.ord_entry->shares == 0) {
                    FH_LOG(LH, DIAG,("Order Table entry after canceled shares adjustment is 0 for key %20s",&key.order_no_str[0]));
                    /* we have to remove the order table entry completely */
                    if( fh_shr_lkp_ord_del(&conn->line->process->order_table,&key,&entry) != FH_OK){
                        FH_LOG(LH, ERR, ("Error in removing Order Table entry when shares count == 0"));
                    }
                }else{
                    FH_LOG(LH,DIAG,("Order table Key = %20s canceled %d left %d shares",
                                    &key.order_no_str[0],message.canceled_shares,
                                    message.ord_entry->shares));
                }
            }else{
                /* executed shares is more than no of shares in the Order Table */
                FH_LOG(LH, ERR, (" Executed shares %d is GREATER than shares left %d in order Table",
                             message.canceled_shares,message.ord_entry->shares));
            }
        }else{
            FH_LOG(LH,ERR, ("Could Not Find Order Table entry for Key = %20s ",&key.order_no_str[0]));
        }
    }


    if (hook_msg_order_canceled) {
        hook_msg_order_canceled (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = len;
    }
    return FH_OK;
}


/*
 * Trade Message parsing function
 */
static inline FH_STATUS fh_de_parse_trade_msg(char * buffer, int len, fh_shr_lh_conn_t* conn,
                                              fh_shr_cfg_lh_line_t *line,
                                              void **data, int * data_length)
{
    FH_STATUS  rc;
    fh_edge_trade_msg_t message;

    if(line) {}

    if (len != TRADE_MSG_SIZE){
        FH_LOG(LH, ERR, (" Invalid Trade Message size received message type rx = %c, length = %d",
                         *(buffer + 9), len));
        return FH_ERROR;
    }

    message.hdr.timestamp    = fh_dirEdge_convert_int(buffer + 1, 8);
    message.hdr.message_type = *(buffer + 9);
    memcpy(&message.order_ref[0], (buffer +10), 12);
    message.side_indicator   = *(buffer + 22);
    message.shares           = fh_dirEdge_convert_int(buffer + 23, 6);
    memcpy(&message.security, (buffer + 29),6);
    message.price            = fh_dirEdge_get_price(buffer+35);
    memcpy(&message.match_number[0], (buffer + 45),21);
    message.raw.message      = buffer;
    message.raw.len          = TRADE_MSG_SIZE;
    if (hook_msg_trade) {
        hook_msg_trade (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = len;
    }
    return FH_OK;;
}


/*
 *  Broken Trade Message parsing function
 */
static inline FH_STATUS fh_de_parse_broken_trade_msg(char * buffer, int len,
                                                     fh_shr_lh_conn_t* conn,
                                                     fh_shr_cfg_lh_line_t *line,
                                                     void **data, int * data_length)
{
    FH_STATUS  rc;
    fh_edge_brokenTrade_msg_t message;

    if(line) {}

    if (len != BROKEN_TRADE_MSG_SIZE){
        FH_LOG(LH, ERR, ("Invalid Broken Trade Message size received message type rx = %c, length = %d",
                         *(buffer + 9), len));
        return FH_ERROR;
    }

    message.hdr.timestamp    = fh_dirEdge_convert_int(buffer + 1, 8);
    message.hdr.message_type = *(buffer + 9);
    memcpy(&message.match_number[0],buffer +10, 21);
    message.raw.message      = buffer;
    message.raw.len          = BROKEN_TRADE_MSG_SIZE;
    if (hook_msg_broken_trade) {
        hook_msg_broken_trade (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = len;
    }
    return FH_OK;
}

/*
 *  Security Status Message parsing function
 */
static inline FH_STATUS fh_de_parse_security_status_msg(char * buffer, int len,
                                                        fh_shr_lh_conn_t* conn,
                                                        fh_shr_cfg_lh_line_t *line,
                                                        void **data, int * data_length)
{
    FH_STATUS  rc;
    fh_edge_securityStatus_msg_t message;

    if(line) {}

    if (len != SECURITY_STATUS_MSG_SIZE){
        FH_LOG(LH, ERR, ("Invalid Security Status Message size received message type rx = %c, length = %d",
                         *(buffer + 9), len));
        return FH_ERROR;
    }

    message.hdr.timestamp    = fh_dirEdge_convert_int(buffer + 1,8);
    message.hdr.message_type = *(buffer + 9);
    memcpy(&message.security[0],buffer +10, 6);
    message.halted_state     = *(buffer + 16);
    message.raw.message      = buffer;
    message.raw.len          = SECURITY_STATUS_MSG_SIZE;
    if (hook_msg_security_status) {
        hook_msg_security_status (&rc, conn, &message, data, data_length);
        if ( rc != FH_OK) {
            return rc;
        }
    } else {
        /* there is no hook function , just point to the message struct */
        *data        = (void*)buffer;
        *data_length = len;
    }
    return FH_OK;
}

/*
 * Parse a single direct edge message
 */
FH_STATUS fh_edge_parse_msg(char *rx_buf, uint32_t len, char rx_char, fh_shr_lh_conn_t * conn,
                                fh_shr_cfg_lh_line_t *line ,uint64_t *seq_no)
{
    FH_STATUS rc;
    void * data;
    int data_length;

    switch (rx_char) {
    case 'S':
        /* a Sequenced System Event Message */
        rc = fh_de_parse_sys_event_msg(rx_buf,len,conn,line, &data, &data_length);
        if ( rc != FH_OK) {
            return -1;
        }
        break;
    case 'A':
        /* a Sequenced Add Order Message    */
        rc = fh_de_parse_add_order_msg(rx_buf,len,conn,line, &data, &data_length);
        if ( rc != FH_OK) {
            return -1;
        }
        break;
    case 'E':
        /* a Sequenced Order Executed Message */
        rc = fh_de_parse_order_executed_msg(rx_buf,len,conn,line, &data, &data_length);
        break;
    case 'X':
        /* a Sequenced Order Canceled Message */
        rc = fh_de_parse_order_canceled_msg(rx_buf,len, conn, line,&data, &data_length);
        if ( rc != FH_OK) {
            return -1;
        }
        break;
    case 'P':
        /* a Sequenced Trade Message */
        rc = fh_de_parse_trade_msg(rx_buf,len, conn, line,&data, &data_length);
        if ( rc != FH_OK) {
            return -1;
        }
        break;
    case 'B':
        /* a Sequenced Broken Trade Message */
        rc = fh_de_parse_broken_trade_msg(rx_buf,len,conn,line, &data, &data_length);
        if ( rc != FH_OK) {
            return -1;
        }
        break;
    case 'H':
        /* a Sequenced Security Status Message */
        rc = fh_de_parse_security_status_msg(rx_buf,len,conn,line, &data, &data_length);
        if ( rc != FH_OK) {
            return -1;
        }
        break;
    default:
        FH_LOG(LH,ERR,("received Invalid Sequenced Message Type %c",rx_char));
        return -1;
    }

    /* increase the next sequence number to receive  */
    (*seq_no)++;
    if(hook_msg_send) {
        hook_msg_send(&rc, data, data_length);
        if ( rc != FH_OK) {
            return -1;
        }
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Session alarm generation
 */

inline FH_STATUS fh_edge_alarm(char * session, int len, int alarm_condition)
{
    FH_STATUS rc = FH_OK;
    if(len) {}
    if (hook_alert) {
        hook_alert(&rc, alarm_condition);
    }
    FH_LOG_PGEN(LH,("session  %10s , alarm condition %d",session,alarm_condition));
    return rc;
}
/*
 * Initialize direct edge message parsing
 */
FH_STATUS fh_edge_parse_init(fh_shr_lh_proc_t *process)
{
    /* supress the warning as we do not need the process data */
    if (process) {}

    FH_DIREDGE_PARSE_CACHE_HOOK(alert,               ALERT);
    FH_DIREDGE_PARSE_CACHE_HOOK(msg_send,            MSG_SEND);
    FH_DIREDGE_PARSE_CACHE_HOOK(msg_system_event,    DIR_EDGE_MSG_SYSTEM_EVENT);
    FH_DIREDGE_PARSE_CACHE_HOOK(msg_add_order,       DIR_EDGE_MSG_ADD_ORDER);
    FH_DIREDGE_PARSE_CACHE_HOOK(msg_order_executed,  DIR_EDGE_MSG_ORDER_EXECUTED);
    FH_DIREDGE_PARSE_CACHE_HOOK(msg_order_canceled,  DIR_EDGE_MSG_ORDER_CANCELED);
    FH_DIREDGE_PARSE_CACHE_HOOK(msg_trade,           DIR_EDGE_MSG_TRADE);
    FH_DIREDGE_PARSE_CACHE_HOOK(msg_broken_trade,    DIR_EDGE_MSG_BROKEN_TRADE);
    FH_DIREDGE_PARSE_CACHE_HOOK(msg_security_status, DIR_EDGE_MSG_SECURITY_STATUS);

    /* if we get here, success */
    return FH_OK;
}
