/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 * 
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
// #include <stdarg.h>

/* FH common includes */
#include "fh_plugin.h"
#include "fh_errors.h"
#include "fh_alerts.h"

/* FH shared module includes */
#include "fh_shr_cfg_lh.h"
#include "fh_shr_lh.h"
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_symbol.h"
#include "fh_shr_lkp_order.h"

/* FH ITCH headers */
#include "fh_itch_msg.h"

/* macro to do a verbose plugin include */
#define ITCH_PLUGIN_REGISTER(x, h)                                                      \
do {                                                                                    \
    printf("Loading ITCH hook: %s (%d) hook=%s...", #x, x, #h);                         \
    if (fh_plugin_register((x), (fh_plugin_hook_t)(h)) != FH_OK) {                      \
        fprintf(stderr, "ITCH_PLUGIN: Unable to register %s (%d) hook\n", #x, x);       \
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
void itch_log_msg_header(fh_itch_msg_header_t *header, fh_shr_lh_conn_t *conn)
{
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_header_t => { seq_no => %lu, timestamp => %u "
                         "pkt_timestamp => %lu }",
                         header->seq_no, header->timestamp, conn->last_recv));
}

/*
 * Log fields belonging to a symbol table entry
 */
void itch_log_sym_table(fh_shr_lkp_sym_t *entry)
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
void itch_log_ord_table(fh_shr_lkp_ord_t *entry)
{
    if (entry != NULL) {
        FH_LOG(CSI, VSTATE, ("fh_shr_lkp_ord_t => { order_no => %lu, price => $%.4f, "
                             "shares => %u, buy_sell_ind => '%c', stock => \"%.6s\" }",
                             entry->order_no, dbl_price(entry->price), entry->shares,
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
void itch_cfg_load(FH_STATUS *rc, fh_shr_cfg_lh_proc_t *config)
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
void itch_lh_init(FH_STATUS *rc, fh_shr_lh_proc_t *process)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_LH_INIT called: process structure at 0x%x", process));
    *rc = FH_OK;
}

/**
 *  @brief Function that is called for any alert condition that occurs in the feed handler
 *
 *  @param rc return code to be "returned"
 *  @param alert the alert that has been generated
 *  @param conn connection object for which this alert was generated
 */
void itch_alert(FH_STATUS *rc, FH_ALERT alert, fh_shr_lh_conn_t *conn)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ALERT:%s(%s)", conn->line->config->name, conn->tag));
    FH_LOG(CSI, VSTATE, ("alert => %d", alert));
    
    *rc = FH_OK;
}

/**
 *  @brief Send a message (mostly for plugins that plan to publish)
 *
 *  @param rc return code to be "returned"
 *  @param data a pointer to the data being sent
 *  @param length the length of the data being sent
 */
void itch_msg_send(FH_STATUS *rc, void *data, uint32_t length)
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
void itch_msg_flush(FH_STATUS *rc)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_MSG_FLUSH called"));
    *rc = FH_OK;
}


/**
 *  @brief Function that is called for system messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_system(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_itch_msg_system_t *message,
                         void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_SYSTEM:%s(%s)", conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_system_t => { event_code => '%c' }", message->event_code));
    
    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for stock directory messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_stock_dir(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_itch_msg_stock_dir_t *message,
                            void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_STOCK_DIR:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_stock_dir_t => { stock => \"%.6s\", market_cat => '%c', "
                         "fin_stat_ind => '%c', round_lots_only => %c, round_lot_size => %u }",
                         message->stock, message->market_cat, message->fin_stat_ind,
                         message->round_lots_only, message->round_lot_size));
    itch_log_sym_table(message->sym_entry);

    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for stock trading action messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_stock_trade_act(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                                  fh_itch_msg_stock_trade_act_t *message, void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_STOCK_TRADE_ACT:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_stock_trade_act_t => { stock => \"%.6s\", trading_state => "
                         "'%c', reason => \"%.4s\" }",
                         message->stock, message->trading_state, message->reason));
    itch_log_sym_table(message->sym_entry);
                             
    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for market participant position messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_market_part_pos(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                                  fh_itch_msg_market_part_pos_t *message, void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_MARKET_PART_POS:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_market_part_pos_t => { mpid => \"%.4s\", stock => \"%.6s\", "
                         "pri_mkt_mkr => %c, mkt_mkr_mode => '%c', mkt_part_state => '%c' }",
                         message->mpid, message->stock, message->pri_mkt_mkr,
                         message->mkt_mkr_mode, message->mkt_part_state));
    itch_log_sym_table(message->sym_entry);
    
    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for order add (w/o MPID attribution) messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_order_add(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_itch_msg_order_add_t *message,
                            void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_ORDER_ADD:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_order_add_t => { order_no => %lu, price => $%.4f, shares => %u"
                         ", buy_sell_ind => '%c', stock => \"%.6s\"}",
                         message->order_no, dbl_price(message->price), message->shares,
                         message->buy_sell_ind, message->stock));
    itch_log_sym_table(message->sym_entry);
    itch_log_ord_table(message->ord_entry);
    
    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for order add (w/ MPID attribution) messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_order_add_attr(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                                 fh_itch_msg_order_add_attr_t *message, void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_ORDER_ADD_ATTR:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_order_add_attr_t => { order_no => %lu, price => $%.4f, "
                         "shares => %u, buy_sell_ind => '%c', stock => \"%.6s\", "
                         "attribution => \"%.4s\"}",
                         message->order_no, dbl_price(message->price), message->shares,
                         message->buy_sell_ind, message->stock, message->attribution));
    itch_log_sym_table(message->sym_entry);
    itch_log_ord_table(message->ord_entry);
    
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
void itch_msg_order_exe(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_itch_msg_order_exe_t *message,
                        void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_ORDER_EXE:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_order_exe_t => { order_no => %lu, shares => %u, "
                         "match_no => %lu }",
                         message->order_no, message->shares, message->match_no));
    itch_log_ord_table(message->ord_entry);
    
    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for order executed (w/ price) messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_order_exe_price(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                              fh_itch_msg_order_exe_price_t *message, void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_ORDER_EXE_PRICE:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_order_exe_price_t => { order_no => %lu, shares => %u, "
                         "match_no => %lu, printable => %c, exe_price => $%.4f }",
                         message->order_no, message->shares, message->match_no,
                         message->printable, dbl_price(message->exe_price)));
    itch_log_ord_table(message->ord_entry);
    
    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for order cancel messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_order_cancel(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                           fh_itch_msg_order_cancel_t *message, void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_ORDER_CANCEL:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_order_cancel_t => { order_no => %lu, shares => %u }",
                         message->order_no, message->shares));
    itch_log_ord_table(message->ord_entry);
    
    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for order delete messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_order_delete(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                           fh_itch_msg_order_delete_t *message, void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_ORDER_DELETE:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_order_delete_t => { order_no => %lu }", message->order_no));
    itch_log_ord_table(message->ord_entry);
    
    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for order replace messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_order_replace(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                            fh_itch_msg_order_replace_t *message, void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_ORDER_REPLACE:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_order_replace_t => { old_order_no => %lu, new_order_no => "
                         "%lu, shares => %u, price => $%.4f }",
                         message->old_order_no, message->new_order_no, message->shares,
                         dbl_price(message->price)));
    itch_log_ord_table(message->ord_entry);
                         
    
    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for trade (non-cross) messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_trade(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_itch_msg_trade_t *message,
                    void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_TRADE:%s(%s)", conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_trade_t => { order_no => %lu, buy_sell_ind => '%c', "
                         "shares => %u, stock => \"%.6s\", price => $%.4f, match_no => %lu }",
                         message->order_no, message->buy_sell_ind, message->shares,
                         message->stock, dbl_price(message->price), message->match_no));
    itch_log_sym_table(message->sym_entry);
    
    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for trade (cross) messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_trade_cross(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_itch_msg_trade_cross_t *message,
                          void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_TRADE_CROSS:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_trade_cross_t => { shares => %u, stock => \"%.6s\", "
                         "price => $%.4f, match_no => %lu, type => '%c' }",
                         message->shares, message->stock, dbl_price(message->price),
                         message->match_no, message->type));
    itch_log_sym_table(message->sym_entry);
    
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
void itch_msg_trade_broken(FH_STATUS *rc, fh_shr_lh_conn_t *conn,
                           fh_itch_msg_trade_broken_t *message, void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_TRADE_BROKEN:%s(%s)",
                         conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_trade_broken_t => { match_no => %lu }", message->match_no));
    
    /* set data to message and data_length to the size of message */
    *data   = (void *)message;
    *length = sizeof(*message);

    *rc = FH_OK;
}

/**
 *  @brief Function that is called for net order imbalance indicator messages
 *
 *  @param rc return code to be "returned"
 *  @param conn connection object on which this message was received
 *  @param message pointer to the message structure for this message
 *  @param data pointer to the pointer that will be set to the wire-ready buffer
 *  @param length pointer to the and integer to set to the length of the wire-ready message
 */
void itch_msg_noii(FH_STATUS *rc, fh_shr_lh_conn_t *conn, fh_itch_msg_noii_t *message,
                   void **data, int *length)
{
    /* log all data contained in the message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_ITCH_MSG_NOII:%s(%s)", conn->line->config->name, conn->tag));
    itch_log_msg_header(&message->header, conn);
    FH_LOG(CSI, VSTATE, ("fh_itch_msg_noii_t => { paired_shares => %u, imbalance_shares => %u, "
                         "imbalance_dir => '%c', stock => \"%.6s\", far_price => $%.4f, "
                         "near_price => $%.4f, ref_price => $%.4f, cross_type => '%c', "
                         "price_var_ind => '%c'",
                         message->paired_shares, message->imbalance_shares, message->imbalance_dir,
                         message->stock, dbl_price(message->far_price),
                         dbl_price(message->near_price), dbl_price(message->ref_price),
                         message->cross_type, message->price_var_ind));
    itch_log_sym_table(message->sym_entry);

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
void __attribute__ ((constructor)) itch_plugin_init(void)
{
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_CFG_LOAD,                     itch_cfg_load);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_LH_INIT,                      itch_lh_init);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ALERT,                        itch_alert);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_MSG_SEND,                     itch_msg_send);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_MSG_FLUSH,                    itch_msg_flush);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_SYSTEM,              itch_msg_system);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_STOCK_DIR,           itch_msg_stock_dir);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_STOCK_TRADE_ACT,     itch_msg_stock_trade_act);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_MARKET_PART_POS,     itch_msg_market_part_pos);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_ORDER_ADD,           itch_msg_order_add);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_ORDER_ADD_ATTR,      itch_msg_order_add_attr);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_ORDER_EXE,           itch_msg_order_exe);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_ORDER_EXE_PRICE,     itch_msg_order_exe_price);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_ORDER_CANCEL,        itch_msg_order_cancel);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_ORDER_DELETE,        itch_msg_order_delete);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_ORDER_REPLACE,       itch_msg_order_replace);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_TRADE,               itch_msg_trade);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_TRADE_CROSS,         itch_msg_trade_cross);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_TRADE_BROKEN,        itch_msg_trade_broken);
    ITCH_PLUGIN_REGISTER(FH_PLUGIN_ITCH_MSG_NOII,                itch_msg_noii);
}
