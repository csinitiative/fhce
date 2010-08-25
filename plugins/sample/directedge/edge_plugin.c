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


/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* FH common includes */
#include "fh_plugin.h"
#include "fh_errors.h"
#include "fh_alerts.h"

/* FH shared module includes */
#include "fh_shr_cfg_lh.h"
#include "fh_shr_tcp_lh.h"
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_symbol.h"
#include "fh_shr_lkp_order.h"

/* FH direct edge headers */
#include "fh_edge_msg.h"

/* macro to do a verbose plugin include */
#define DE_PLUGIN_REGISTER(x, h)                                                      \
do {                                                                                    \
    printf("Loading DirectEdge hook: %s (%d) hook=%s...", #x, x, #h);                         \
    if (fh_plugin_register((x), (fh_plugin_hook_t)(h)) != FH_OK) {                      \
        fprintf(stderr, "DIRECT_EDGE_PLUGIN: Unable to register %s (%d) hook\n", #x, x);       \
        exit(1);                                                                        \
    }                                                                                   \
    printf("done\n");                                                                   \
} while (0)

/*
 * Function to turn ISE prices into double precision floats (for display)
 */
static inline double dbl_price(uint64_t price)
{
    double result = 0.0;

    /* extract the whole dollar part of the price */
    result += price / 10000;

    /* extract the decimal part of the price */
    result += (double)(price % 10000) / 10000;

    return result;
}

/*
 * Log fields and structure of a message header
 */
void de_log_msg_header(fh_edge_msg_hdr_t *header)
{
    FH_LOG(CSI, VSTATE, ("fh_edge_msg_hdr_t => { timestamp => %u  Message Type => %c}",
                         header->timestamp, header->message_type));
}

/*
 * Log fields belonging to a symbol table entry
 */
void de_log_sym_table(fh_shr_lkp_sym_t *entry)
{
    if (entry != NULL) {
        FH_LOG(CSI, VSTATE, ("fh_shr_lkp_sym_t => { symbol => %s }", entry->symbol));
    }
    else {
        FH_LOG(CSI, VSTATE, ("fh_shr_lkp_sym_t => NULL"));
    }
}

/*
 * Log fields belonging to an order table entry
 */
void de_log_ord_table(fh_shr_lkp_ord_t *entry)
{
    if (entry != NULL) {
        FH_LOG(CSI, VSTATE, ("fh_shr_lkp_ord_t => { order_no => %lu, order_no_str => \"%.12s\", "
                             "price => $%.4f, "
                             "shares => %u, buy_sell_ind => '%c', stock => \"%.6s\" }",
                             entry->order_no, entry->order_no_str,
                             dbl_price(entry->price), entry->shares,
                             entry->buy_sell_ind, entry->stock));
        if (entry->sym_entry != NULL) {
            FH_LOG(CSI, VSTATE, ("    sym_entry => { symbol => %s }", entry->sym_entry->symbol));
        }
        else {
            FH_LOG(CSI, VSTATE, ("    sym_entry => NULL"));
        }
    }
    else {
        FH_LOG(CSI, VSTATE, ("fh_shr_lkp_ord_t => NULL"));
    }
}

/**
 *  @brief Function that is called just after the feed handler configuration has been loaded
 *
 *  @param rc return code to be "returned"
 *  @param config pointer to the configuration that has been loaded by the feed handler
 */
void de_cfg_load(FH_STATUS *rc, fh_shr_cfg_lh_proc_t *config)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_CFG_LOAD called: configuration at 0x%x", config));
    *rc = FH_OK;
}

/**
 *  @brief Function that is called just after the line handler has been initialized
 *
 *  @param rc return code to be "returned"
 *  @param config pointer to the feed handler's process data structure
 */
void de_lh_init(FH_STATUS *rc, fh_shr_lh_proc_t *process)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_LH_INIT called: process structure at 0x%x", process));
    *rc = FH_OK;
}

/**
 *  @brief Function that is called for any alert condition that occurs in the feed handler
 *
 *  @param rc return code to be "returned"
 *  @param alert the alert that has been generated
 */
void de_alert(FH_STATUS *rc, FH_ALERT alert)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ALERT called (%d)", alert));
    *rc = FH_OK;
}

/**
 *  @brief Send a message (mostly for plugins that plan to publish)
 *
 *  @param rc return code to be "returned"
 *  @param data a pointer to the data being sent
 *  @param length the length of the data being sent
 */
void de_msg_send(FH_STATUS *rc, void *data, uint32_t length)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_MSG_SEND called: data of length %u at 0x%x", length, data));
    *rc = FH_OK;
}

/**
 *  @brief Called before (potentially) long periods of inactivity to give the plugin a chance to
 *         to flush any message buffers
 *
 *  @param rc return code to be "returned"
 */
void de_msg_flush(FH_STATUS *rc)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_MSG_FLUSH called"));
    *rc = FH_OK;
}


/**
 *  @brief Function that is called for system event messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void de_system_event_msg(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_edge_sysEvent_msg_t *message,
                         void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_DIR_EDGE_MSG_SYSTEM:%s(%s)", conn->line->config->name, conn->tag));
    de_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_edge_sysEvent_msg_t => { event_code => '%c' }", message->event_code));

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for order add  messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void de_add_order_msg(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_edge_add_order_msg_t *message,
                            void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_DIR_EDGE_MSG_ORDER_ADD:%s(%s)",
                         conn->line->config->name, conn->tag));
    de_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_edge_add_order_msg_t => { order_ref => \"%.12s\", Bid_order_ind => %c"
                         ", shares => %u , stock => \"%.6s\", price => $%.4f"
                         ", display  => '%c', mmid => \"%.4s\" }",
                         message->order_ref, message->side_indicator,
                         message->order_quantity, message->security,
                         dbl_price(message->price), message->display,
                         (message->display == 'A'? message->mmid :"    ")));
    de_log_sym_table(message->sym_entry);
    de_log_ord_table(message->ord_entry);

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}


/**
 *  @brief Function that is called for order executed messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void de_order_exe_msg(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_edge_orderExecuted_msg_t *message,
                        void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_DIR_EDGE_MSG_ORDER_EXE:%s(%s)",
                         conn->line->config->name, conn->tag));
    de_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_edge_orderExecuted_msg_t => { order_ref => \"%.12s\" "
                         ", executed_shares => %u , match_number => \"%.21s\" }",
                         message->order_ref, message->executed_shares, message->match_number));
    de_log_ord_table(message->ord_entry);

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for order canceled messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void de_order_cancel_msg(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                           fh_edge_orderCanceled_msg_t *message, void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_DIR_EDGE_MSG_ORDER_CANCEL:%s(%s)",
                         conn->line->config->name, conn->tag));
    de_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_edge_orderCanceled_msg_t => { order_ref => \"%.12s\" "
                         ", canceled_shares => %u }",
                         message->order_ref, message->canceled_shares));
    de_log_ord_table(message->ord_entry);

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}
/**
 *  @brief Function that is called for trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void de_trade_msg(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_edge_trade_msg_t *message,
                    void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_DIR_EDGE_MSG_TRADE:%s(%s)", conn->line->config->name, conn->tag));
    de_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_edge_trade_msg_t => { order_ref => \"%.12s\", Bid_order_ind => %c"
                         ", shares => %u , stock => \"%.6s\", price => $%.4f"
                         ", match_number => \"%.21s\" }",
                         message->order_ref, message->side_indicator,
                         message->shares, message->security,
                         dbl_price(message->price), message->match_number));
//    de_log_sym_table(message->sym_entry);

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for broken trade messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void de_broken_trade_msg(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                           fh_edge_brokenTrade_msg_t *message, void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_DIR_EDGE_MSG_TRADE_BROKEN:%s(%s)",
                         conn->line->config->name, conn->tag));
    de_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_edge_brokenTrade_msg_t => { match_number => \"%.21s\" }", message->match_number));

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for security status messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void de_security_status_msg(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                           fh_edge_securityStatus_msg_t *message, void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_DIR_EDGE_MSG_SECURITY_STATUS:%s(%s)",
                         conn->line->config->name, conn->tag));
    de_log_msg_header(&message->hdr);
    FH_LOG(CSI, VSTATE, ("fh_edge_brokenTrade_msg_t => { security => \"%.6s\" "
                         ", halted_state => %c }", message->security, message->halted_state));

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Startup function
 *  The "__attribute__ ((constructor))" syntax will cause this function to be run whenever the
 *  shared object, compiled from this .c file, is loaded.  It is in this function that all
 *  registration of hook functions must be done.
 */
void __attribute__ ((constructor)) de_plugin_init(void)
{
    DE_PLUGIN_REGISTER(FH_PLUGIN_CFG_LOAD,                     de_cfg_load);
    DE_PLUGIN_REGISTER(FH_PLUGIN_LH_INIT,                      de_lh_init);
    DE_PLUGIN_REGISTER(FH_PLUGIN_ALERT,                        de_alert);
    DE_PLUGIN_REGISTER(FH_PLUGIN_MSG_SEND,                     de_msg_send);
    DE_PLUGIN_REGISTER(FH_PLUGIN_MSG_FLUSH,                    de_msg_flush);
    DE_PLUGIN_REGISTER(FH_PLUGIN_DIR_EDGE_MSG_SYSTEM_EVENT,    de_system_event_msg);
    DE_PLUGIN_REGISTER(FH_PLUGIN_DIR_EDGE_MSG_ADD_ORDER,       de_add_order_msg);
    DE_PLUGIN_REGISTER(FH_PLUGIN_DIR_EDGE_MSG_ORDER_EXECUTED,  de_order_exe_msg);
    DE_PLUGIN_REGISTER(FH_PLUGIN_DIR_EDGE_MSG_ORDER_CANCELED,  de_order_cancel_msg);
    DE_PLUGIN_REGISTER(FH_PLUGIN_DIR_EDGE_MSG_TRADE,           de_trade_msg);
    DE_PLUGIN_REGISTER(FH_PLUGIN_DIR_EDGE_MSG_BROKEN_TRADE,    de_broken_trade_msg);
    DE_PLUGIN_REGISTER(FH_PLUGIN_DIR_EDGE_MSG_SECURITY_STATUS, de_security_status_msg);
}
