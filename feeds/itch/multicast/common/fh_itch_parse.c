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

/* system headers */
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* FH common headers */
#include "fh_errors.h"
#include "fh_log.h"
#include "fh_alerts.h"
#include "fh_plugin_internal.h"

/* FH shared component headers */
#include "fh_shr_lh.h"
#include "fh_shr_cfg_lh.h"
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_symbol.h"
#include "fh_shr_lkp_order.h"
#include "fh_shr_gap_fill.h"

/* ITCH headers */
#include "fh_itch_parse.h"
#include "fh_itch_moldudp64.h"
#include "fh_itch_msg.h"

/* cached plugin hooks */
static fh_plugin_hook_t          hook_alert                 = NULL;
static fh_plugin_hook_t          hook_msg_send              = NULL;
static fh_plugin_hook_t          hook_msg_system            = NULL;
static fh_plugin_hook_t          hook_msg_stock_dir         = NULL;
static fh_plugin_hook_t          hook_msg_stock_trade_act   = NULL;
static fh_plugin_hook_t          hook_msg_market_part_pos   = NULL;
static fh_plugin_hook_t          hook_msg_order_add         = NULL;
static fh_plugin_hook_t          hook_msg_order_add_attr    = NULL;
static fh_plugin_hook_t          hook_msg_order_exe         = NULL;
static fh_plugin_hook_t          hook_msg_order_exe_price   = NULL;
static fh_plugin_hook_t          hook_msg_order_cancel      = NULL;
static fh_plugin_hook_t          hook_msg_order_delete      = NULL;
static fh_plugin_hook_t          hook_msg_order_replace     = NULL;
static fh_plugin_hook_t          hook_msg_trade             = NULL;
static fh_plugin_hook_t          hook_msg_trade_cross       = NULL;
static fh_plugin_hook_t          hook_msg_trade_broken      = NULL;
static fh_plugin_hook_t          hook_msg_noii              = NULL;

/* global variables to track gap filling status */
static fh_shr_gap_fill_list_t   *gaplist                    = NULL;
static fh_shr_gap_fill_node_t   *gapnode                    = NULL;

/* macro to cache a hook function */
#define FH_ITCH_PARSE_CACHE_HOOK(lc, uc)                                                        \
if (fh_plugin_is_hook_registered(FH_PLUGIN_ ## uc)) {                                           \
    hook_ ## lc = fh_plugin_get_hook(FH_PLUGIN_ ## uc);                                         \
}

/* macro to fetch symbol table information and store it in the proper field */
#define FH_ITCH_FETCH_SYMBOL                                                                    \
    message.sym_entry = NULL;                                                                   \
                                                                                                \
    if (conn->line->process->config->symbol_table.enabled) {                                    \
        fh_shr_lkp_sym_key_t     key;                                                           \
        fh_shr_lkp_sym_t        *entry;                                                         \
                                                                                                \
        memcpy(key.symbol, message.stock, 6);                                                   \
        memset(key.symbol + 6, 0, sizeof(fh_shr_lkp_sym_key_t) - 6);                            \
        if (fh_shr_lkp_sym_get(&conn->line->process->symbol_table, &key, &entry) == FH_OK) {    \
            message.sym_entry = entry;                                                          \
        }                                                                                       \
    }

/*
 * Convert the specified number of ASCII digits into a number
 */
static inline uint64_t fh_itch_parse_atoi(uint8_t *chars, int count)
{
    int       i;
    uint64_t  exp;
    uint64_t  result = 0;

    /* loop through "count" digits, adding each one to the total */
    for (i = count - 1, exp = 1; i >= 0; i--, exp *= 10) {
        /* if the character at this position is a space, break (fields are space padded) */
        if (chars[i] == ' ') {
            break;
        }
        /* otherwise, add this digit, converted to a number, then multiplied by 10^place */
        result += ((chars[i] - '0') * exp);
    }

    /* return the result */
    return result;
}

/*
 * Convert the specified number of ASCII digits into a number
 */
static inline bool fh_itch_parse_yesno(uint8_t *ch)
{
    if (*ch == 'Y' || *ch == 'y') {
        return true;
    }
    return false;
}

/*
 * Parse an ISE formatted price out of the first 10 bytes of buffer (6 digits to the left of the
 * decimal and 4 to the right)
 */
static inline uint64_t fh_itch_parse_price(uint8_t *buffer)
{
    int      i;
    uint32_t exp;
    uint64_t result = 0;

    /* first, loop through the decimal parts of the price, converting to numeric values */
    for (i = 6, exp = 1000; i < 10; i++, exp /= 10) {
        /* if the character at this position is a space, break (fields are space padded) */
        if (buffer[i] == ' ') {
            break;
        }
        /* otherwise, add this digit, converted to a number, then multiplied by 10^place */
        result += ((buffer[i] - '0') * exp);
    }

    /* now, loop through the whole number parts of the price... */
    for (i = 5, exp = 10000; i >= 0; i--, exp *= 10) {
        /* if the character at this position is a space, break (fields are space padded) */
        if (buffer[i] == ' ') {
            break;
        }
        /* otherwise, add this digit, converted to a number, then multiplied by 10^place */
        result += ((buffer[i] - '0') * exp);
    }

    /* return the final result */
    return result;
}

/*
 * Process an ITCH system message
 */
static inline FH_STATUS fh_itch_parse_sys_msg(uint8_t *buffer, int length, fh_shr_lh_conn_t *conn,
                                              void **data, int *data_length)
{
    fh_itch_msg_system_t     message;
    int                      rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_SYSTEM_SIZE) {
        FH_LOG(LH, ERR, ("message length is incorrect (%d) for system message on line %s (%s)",
                         length, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.event_code       = *(char *)(buffer + 1);

    /* construct header and raw message fields */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_system) {
        hook_msg_system(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH stock directory message
 */
static inline FH_STATUS fh_itch_parse_stock_dir_msg(uint8_t *buffer, int length,
                                                    fh_shr_lh_conn_t *conn, void **data,
                                                    int *data_length)
{
    fh_itch_msg_stock_dir_t      message;
    int                          rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_STOCK_DIR_SIZE) {
        FH_LOG(LH, ERR, ("message length (%d) != %d for stock directory on line %s (%s)",
                         length, FH_ITCH_MSG_STOCK_DIR_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    memcpy(message.stock,       (char *)(buffer + 1), 6);
    message.market_cat       = *(char *)(buffer + 7);
    message.fin_stat_ind     = *(char *)(buffer + 8);
    message.round_lot_size   = fh_itch_parse_atoi(buffer + 9, 6);
    message.round_lots_only  = *(char *)(buffer + 15);

    /* get the symbol table entry for this symbol */
    FH_ITCH_FETCH_SYMBOL

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_stock_dir) {
        hook_msg_stock_dir(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH stock trading action message
 */
static inline FH_STATUS fh_itch_parse_stock_trade_act_msg(uint8_t *buffer, int length,
                                                          fh_shr_lh_conn_t *conn, void **data,
                                                          int *data_length)
{
    fh_itch_msg_stock_trade_act_t    message;
    int                              rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_STOCK_TRADE_ACT_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for stock trade action on line %s (%s)", length,
                         FH_ITCH_MSG_STOCK_TRADE_ACT_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    memcpy(message.stock,       (char *)(buffer + 1), 6);
    message.trading_state    = *(char *)(buffer + 7);
    memcpy(message.reason,      (char *)(buffer + 9), 4);

    /* get the symbol table entry for this symbol */
    FH_ITCH_FETCH_SYMBOL

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_stock_trade_act) {
        hook_msg_stock_trade_act(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH market participant position message
 */
static inline FH_STATUS fh_itch_parse_market_part_pos_msg(uint8_t *buffer, int length,
                                                          fh_shr_lh_conn_t *conn, void **data,
                                                          int *data_length)
{
    fh_itch_msg_market_part_pos_t   message;
    int                             rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_MARKET_PART_POS_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for market part. pos. on line %s (%s)", length,
                         FH_ITCH_MSG_MARKET_PART_POS_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    memcpy(message.mpid,        (char *)(buffer + 1), 4);
    memcpy(message.stock,       (char *)(buffer + 5), 6);
    message.pri_mkt_mkr      = *(char *)(buffer + 11);
    message.mkt_mkr_mode     = *(char *)(buffer + 12);
    message.mkt_part_state   = *(char *)(buffer + 13);

    /* get the symbol table entry for this symbol */
    FH_ITCH_FETCH_SYMBOL

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_market_part_pos) {
        hook_msg_market_part_pos(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH order add (w/o MPID attribution) message
 */
static inline FH_STATUS fh_itch_parse_order_add_msg(uint8_t *buffer, int length,
                                                    fh_shr_lh_conn_t *conn, void **data,
                                                    int *data_length)
{
    fh_itch_msg_order_add_t   message;
    int                       rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_ORDER_ADD_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for order add on line %s (%s)", length,
                         FH_ITCH_MSG_ORDER_ADD_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.order_no         = fh_itch_parse_atoi(buffer + 1, 12);
    message.buy_sell_ind     = *(char *)(buffer + 13);
    message.shares           = fh_itch_parse_atoi(buffer + 14, 6);
    memcpy(message.stock,       (char *)(buffer + 20), 6);
    message.price            = fh_itch_parse_price(buffer + 26);

    /* get the symbol table entry for this symbol */
    FH_ITCH_FETCH_SYMBOL

    /* set up a new order table entry for this order */
    message.ord_entry = NULL;
    if (conn->line->process->config->order_table.enabled) {
        fh_shr_lkp_ord_t     entry;
        fh_shr_lkp_tbl_t    *order_table = &conn->line->process->order_table;

        /* set up order table entry attributes */
        memset(&entry, 0, sizeof(fh_shr_lkp_ord_t));
        memcpy(entry.stock, message.stock, 6);
        entry.order_no      = message.order_no;
        entry.price         = message.price;
        entry.shares        = message.shares;
        entry.buy_sell_ind  = message.buy_sell_ind;
        entry.sym_entry     = message.sym_entry;

        /* add the new order table entry to the table */
        if (fh_shr_lkp_ord_add(order_table, &entry, &message.ord_entry) != FH_OK) {
            FH_LOG(LH, ERR, ("unable to add order %lu to the order table", message.order_no));
        }
    }

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_order_add) {
        hook_msg_order_add(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH order add (w/ MPID attribution) message
 */
static inline FH_STATUS fh_itch_parse_order_add_attr_msg(uint8_t *buffer, int length,
                                                         fh_shr_lh_conn_t *conn, void **data,
                                                         int *data_length)
{
    fh_itch_msg_order_add_attr_t    message;
    int                             rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_ORDER_ADD_ATTR_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for order add (w/ attr) on line %s (%s)", length,
                         FH_ITCH_MSG_ORDER_ADD_ATTR_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.order_no            = fh_itch_parse_atoi(buffer + 1, 12);
    message.buy_sell_ind        = *(char *)(buffer + 13);
    message.shares              = fh_itch_parse_atoi(buffer + 14, 6);
    memcpy(message.stock,          (char *)(buffer + 20), 6);
    message.price               = fh_itch_parse_price(buffer + 26);
    memcpy(message.attribution,    (char *)(buffer + 36), 4);

    /* get the symbol table entry for this symbol */
    FH_ITCH_FETCH_SYMBOL

    /* set up a new order table entry for this order */
    message.ord_entry = NULL;
    if (conn->line->process->config->order_table.enabled) {
        fh_shr_lkp_ord_t     entry;
        fh_shr_lkp_tbl_t    *order_table = &conn->line->process->order_table;

        /* set up order table entry attributes */
        memset(&entry, 0, sizeof(fh_shr_lkp_ord_t));
        memcpy(entry.stock, message.stock, 6);
        entry.order_no      = message.order_no;
        entry.price         = message.price;
        entry.shares        = message.shares;
        entry.buy_sell_ind  = message.buy_sell_ind;
        entry.sym_entry     = message.sym_entry;

        /* add the new order table entry to the table */
        if (fh_shr_lkp_ord_add(order_table, &entry, &message.ord_entry) != FH_OK) {
            FH_LOG(LH, ERR, ("unable to add order %lu to the order table", message.order_no));
        }
    }

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_order_add_attr) {
        hook_msg_order_add_attr(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH order executed message
 */
static inline FH_STATUS fh_itch_parse_order_exe_msg(uint8_t *buffer, int length,
                                                    fh_shr_lh_conn_t *conn, void **data,
                                                    int *data_length)
{
    fh_itch_msg_order_exe_t    message;
    int                             rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_ORDER_EXE_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for order executed on line %s (%s)", length,
                         FH_ITCH_MSG_ORDER_EXE_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.order_no            = fh_itch_parse_atoi(buffer + 1, 12);
    message.shares              = fh_itch_parse_atoi(buffer + 13, 6);
    message.match_no            = fh_itch_parse_atoi(buffer + 19, 12);

    /* modify the order table entry for this order appropriately */
    message.ord_entry = NULL;
    if (conn->line->process->config->order_table.enabled) {
        fh_shr_lkp_tbl_t        *order_table = &conn->line->process->order_table;
        fh_shr_lkp_ord_key_t     key = {
            .order_no = message.order_no
        };

        /* fetch the order table entry */
        if (fh_shr_lkp_ord_get(order_table, &key, &message.ord_entry) != FH_OK) {
            FH_LOG(LH, ERR, ("unable to fetch order %lu from the order table", message.order_no));
        }
        else {
            message.ord_entry->shares -= message.shares;
            if (message.ord_entry->shares <= 0) {
                if (fh_shr_lkp_ord_del(order_table, &key, &message.ord_entry) != FH_OK) {
                    FH_LOG(LH, ERR, ("unable to remove order %lu from the order table",
                                     message.order_no));
                }

            }
        }
    }

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_order_exe) {
        hook_msg_order_exe(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH order executed (w/ price) message
 */
static inline FH_STATUS fh_itch_parse_order_exe_price_msg(uint8_t *buffer, int length,
                                                          fh_shr_lh_conn_t *conn, void **data,
                                                          int *data_length)
{
    fh_itch_msg_order_exe_price_t    message;
    int                              rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_ORDER_EXE_PRICE_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for order executed (w/ price) on line %s (%s)",
                         length, FH_ITCH_MSG_ORDER_EXE_PRICE_SIZE, conn->line->config->name,
                         conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.order_no            = fh_itch_parse_atoi(buffer + 1, 12);
    message.shares              = fh_itch_parse_atoi(buffer + 13, 6);
    message.match_no            = fh_itch_parse_atoi(buffer + 19, 12);
    message.printable           = *(char *)(buffer + 31);
    message.exe_price           = fh_itch_parse_price(buffer + 32);

    /* modify the order table entry for this order appropriately */
    message.ord_entry = NULL;
    if (conn->line->process->config->order_table.enabled) {
        fh_shr_lkp_tbl_t        *order_table = &conn->line->process->order_table;
        fh_shr_lkp_ord_key_t     key = {
            .order_no = message.order_no
        };

        /* fetch the order table entry */
        if (fh_shr_lkp_ord_get(order_table, &key, &message.ord_entry) != FH_OK) {
            FH_LOG(LH, ERR, ("unable to fetch order %lu from the order table", message.order_no));
        }
        else {
            message.ord_entry->shares -= message.shares;
            if (message.ord_entry->shares <= 0) {
                if (fh_shr_lkp_ord_del(order_table, &key, &message.ord_entry) != FH_OK) {
                    FH_LOG(LH, ERR, ("unable to remove order %lu from the order table",
                                     message.order_no));
                }

            }
        }
    }

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_order_exe_price) {
        hook_msg_order_exe_price(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH order cancel message
 */
static inline FH_STATUS fh_itch_parse_order_cancel_msg(uint8_t *buffer, int length,
                                                       fh_shr_lh_conn_t *conn, void **data,
                                                       int *data_length)
{
    fh_itch_msg_order_cancel_t    message;
    int                           rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_ORDER_CANCEL_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for order cancel on line %s (%s)", length,
                         FH_ITCH_MSG_ORDER_CANCEL_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.order_no            = fh_itch_parse_atoi(buffer + 1, 12);
    message.shares              = fh_itch_parse_atoi(buffer + 13, 6);

    /* modify the order table entry for this order appropriately */
    message.ord_entry = NULL;
    if (conn->line->process->config->order_table.enabled) {
        fh_shr_lkp_tbl_t        *order_table = &conn->line->process->order_table;
        fh_shr_lkp_ord_key_t     key = {
            .order_no = message.order_no
        };

        /* fetch the order table entry */
        if (fh_shr_lkp_ord_get(order_table, &key, &message.ord_entry) != FH_OK) {
            FH_LOG(LH, ERR, ("unable to fetch order %lu from the order table", message.order_no));
        }
        else {
            message.ord_entry->shares -= message.shares;
            if (message.ord_entry->shares <= 0) {
                if (fh_shr_lkp_ord_del(order_table, &key, &message.ord_entry) != FH_OK) {
                    FH_LOG(LH, ERR, ("unable to remove order %lu from the order table",
                                     message.order_no));
                }

            }
        }
    }

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_order_cancel) {
        hook_msg_order_cancel(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH order cancel message
 */
static inline FH_STATUS fh_itch_parse_order_delete_msg(uint8_t *buffer, int length,
                                                       fh_shr_lh_conn_t *conn, void **data,
                                                       int *data_length)
{
    fh_itch_msg_order_delete_t    message;
    int                           rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_ORDER_DELETE_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for order delete on line %s (%s)", length,
                         FH_ITCH_MSG_ORDER_DELETE_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.order_no            = fh_itch_parse_atoi(buffer + 1, 12);

    /* set up a new order table entry for this order */
    message.ord_entry = NULL;
    if (conn->line->process->config->order_table.enabled) {
        fh_shr_lkp_tbl_t        *order_table = &conn->line->process->order_table;
        fh_shr_lkp_ord_key_t     key = {
            .order_no = message.order_no
        };

        /* remove the order table entry */
        if (fh_shr_lkp_ord_del(order_table, &key, &message.ord_entry) != FH_OK) {
            FH_LOG(LH, ERR, ("unable to remove order %lu from the order table", message.order_no));
        }
    }

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_order_delete) {
        hook_msg_order_delete(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH order cancel message
 */
static inline FH_STATUS fh_itch_parse_order_replace_msg(uint8_t *buffer, int length,
                                                        fh_shr_lh_conn_t *conn, void **data,
                                                        int *data_length)
{
    fh_itch_msg_order_replace_t    message;
    int                            rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_ORDER_REPLACE_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for order replace on line %s (%s)", length,
                         FH_ITCH_MSG_ORDER_REPLACE_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.old_order_no    = fh_itch_parse_atoi(buffer + 1, 12);
    message.new_order_no    = fh_itch_parse_atoi(buffer + 13, 12);
    message.shares          = fh_itch_parse_atoi(buffer + 25, 6);
    message.price           = fh_itch_parse_price(buffer + 31);

    /* set up a new order table entry for this order */
    message.ord_entry = NULL;
    if (conn->line->process->config->order_table.enabled) {
        fh_shr_lkp_tbl_t        *order_table = &conn->line->process->order_table;
        fh_shr_lkp_ord_t         new_entry;
        fh_shr_lkp_ord_t        *old_entry;
        fh_shr_lkp_ord_key_t     key = {
            .order_no = message.old_order_no
        };

        /* remove the old order table entry */
        if (fh_shr_lkp_ord_del(order_table, &key, &old_entry) != FH_OK) {
            FH_LOG(LH, ERR, ("unable to remove order %lu from order table", message.old_order_no));
        }

        /* copy the contents of the old order over the new one and fix the replaced fields */
        if (old_entry != NULL) {
            memcpy(&new_entry, old_entry, sizeof(fh_shr_lkp_ord_t));
        }
        else {
            memset(&new_entry, 0, sizeof(fh_shr_lkp_ord_t));
        }
        new_entry.order_no  = message.new_order_no;
        new_entry.shares    = message.shares;
        new_entry.price     = message.price;

        /* add the new order table entry to the table */
        if (fh_shr_lkp_ord_add(order_table, &new_entry, &message.ord_entry) != FH_OK) {
            FH_LOG(LH, ERR, ("unable to add order %lu to order table", message.new_order_no));
        }
    }

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_order_replace) {
        hook_msg_order_replace(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH trade (non-cross) message
 */
static inline FH_STATUS fh_itch_parse_trade_msg(uint8_t *buffer, int length, fh_shr_lh_conn_t *conn,
                                                void **data, int *data_length)
{
    fh_itch_msg_trade_t    message;
    int                    rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_TRADE_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for trade (non-cross) on line %s (%s)", length,
                         FH_ITCH_MSG_TRADE_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.order_no        = fh_itch_parse_atoi(buffer + 1, 12);
    message.buy_sell_ind    = *(char *)(buffer + 13);
    message.shares          = fh_itch_parse_atoi(buffer + 14, 6);
    memcpy(message.stock,      (char *)(buffer + 20), 6);
    message.price           = fh_itch_parse_price(buffer + 26);
    message.match_no        = fh_itch_parse_atoi(buffer + 36, 12);

    /* get the symbol table entry for this symbol */
    FH_ITCH_FETCH_SYMBOL

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_trade) {
        hook_msg_trade(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH trade (cross) message
 */
static inline FH_STATUS fh_itch_parse_trade_cross_msg(uint8_t *buffer, int length,
                                                      fh_shr_lh_conn_t *conn,
                                                      void **data, int *data_length)
{
    fh_itch_msg_trade_cross_t   message;
    int                         rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_TRADE_CROSS_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for trade (cross) on line %s (%s)", length,
                         FH_ITCH_MSG_TRADE_CROSS_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.shares        = fh_itch_parse_atoi(buffer + 1, 9);
    memcpy(message.stock,    (char *)(buffer + 10), 6);
    message.price         = fh_itch_parse_price(buffer + 16);
    message.match_no      = fh_itch_parse_atoi(buffer + 26, 12);
    message.type          = *(char *)(buffer + 38);

    /* get the symbol table entry for this symbol */
    FH_ITCH_FETCH_SYMBOL

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_trade_cross) {
        hook_msg_trade_cross(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH broken trade message
 */
static inline FH_STATUS fh_itch_parse_trade_broken_msg(uint8_t *buffer, int length,
                                                       fh_shr_lh_conn_t *conn,
                                                       void **data, int *data_length)
{
    fh_itch_msg_trade_broken_t  message;
    int                         rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_TRADE_BROKEN_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for broken trade on line %s (%s)", length,
                         FH_ITCH_MSG_TRADE_BROKEN_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.match_no         = fh_itch_parse_atoi(buffer + 1, 12);

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_trade_broken) {
        hook_msg_trade_broken(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Process an ITCH net order imbalance indicator message
 */
static inline FH_STATUS fh_itch_parse_noii_msg(uint8_t *buffer, int length, fh_shr_lh_conn_t *conn,
                                               void **data, int *data_length)
{
    fh_itch_msg_noii_t  message;
    int                 rc;

    /* ensure that the message length is correct for this type of message */
    if (length != FH_ITCH_MSG_NOII_SIZE) {
        FH_LOG(LH, ERR, ("message length %d != %d for NOII on line %s (%s)", length,
                         FH_ITCH_MSG_NOII_SIZE, conn->line->config->name, conn->tag));
        return FH_ERROR;
    }

    /* parse fields out of the buffer into the message */
    message.paired_shares       = fh_itch_parse_atoi(buffer + 1, 9);
    message.imbalance_shares    = fh_itch_parse_atoi(buffer + 10, 9);
    message.imbalance_dir       = *(char *)(buffer + 19);
    memcpy(message.stock,          (char *)(buffer + 20), 6);
    message.far_price           = fh_itch_parse_price(buffer + 26);
    message.near_price          = fh_itch_parse_price(buffer + 36);
    message.ref_price           = fh_itch_parse_price(buffer + 46);
    message.cross_type          = *(char *)(buffer + 56);
    message.price_var_ind       = *(char *)(buffer + 57);

    /* get the symbol table entry for this symbol */
    FH_ITCH_FETCH_SYMBOL

    /* copy some header and raw message values into the message */
    message.header.seq_no    = conn->line->next_seq_no;
    message.header.timestamp = conn->timestamp;
    message.raw.message      = buffer;
    message.raw.size         = length;

    /* if there is a hook function registered for system messages, call it now */
    if (hook_msg_noii) {
        hook_msg_noii(&rc, conn, &message, data, data_length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    /* if there is no hook function, just point data at the message struct */
    else {
        *data        = (void *)&message;
        *data_length = sizeof(message);
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Return < 0 if message is in a gap, > 0 if message should be processed normally, or 0 if message
 * is a duplicate.
 */
static inline int fh_itch_is_duplicate(uint64_t seq_no, fh_shr_lh_conn_t *conn)
{
    int gap_size;
    int rc;

    FH_LOG(LH, DIAG, ("duplicate detection (%s) -- line %lu, header %lu",
                      conn->tag, conn->line->next_seq_no, seq_no));

    /* OPTIMIZATION: this is the most common case so return immediately */
    if (seq_no == conn->line->next_seq_no) return 1;

    /* if the sequence number is smaller than the current line sequence number...*/
    if (seq_no < conn->line->next_seq_no) {
        /* see if there is a gap list entry for this sequence number */
        gapnode = fh_shr_gap_fill_find(gaplist, seq_no);
        if (gapnode != NULL) {
            return -1;
        }
        return 0;
    }

    /* if the sequence number is larger than the current line sequence number, there is a gap */
    else if (seq_no > conn->line->next_seq_no) {
        FH_LOG(LH, WARN, ("gap on line %s (%s) -- expected %lu, got %lu",
                          conn->line->config->name, conn->tag, conn->line->next_seq_no, seq_no));

        /* increment gap statistics and calculate the size of the gap */
        conn->stats.gaps++;
        gap_size = seq_no - conn->line->next_seq_no;

        /* if there is a gap list configured, insert into the gap list */
        if (gaplist) {
            rc = fh_shr_gap_fill_push(gaplist, conn->line->next_seq_no, gap_size);

            /* pushing the new gap to the list resulted in an error */
            if (rc < 0) {
                FH_LOG(LH, ERR, ("error adding gap to the gap list -- messages lost"));
                conn->stats.lost_messages += gap_size;
                if (hook_alert) {
                    hook_alert(&rc, FH_ALERT_LOSS, conn);
                }
            }

            /* pushing the new gap was a success... */
            else {
                if (hook_alert) {
                    hook_alert(&rc, FH_ALERT_GAP, conn);
                }

                /* ...but there was loss as well */
                if (rc > 0) {
                    FH_LOG(LH, WARN, ("gap list overflow -- %d messages lost", rc));
                    conn->stats.lost_messages += rc;
                    if (hook_alert) {
                        hook_alert(&rc, FH_ALERT_LOSS, conn);
                    }
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
        conn->line->next_seq_no = seq_no;
    }

    /* should really never get here but resolves "no return statement" warning */
    return 1;
}

/*
 * Process an ITCH message from the passed in buffer
 */
static inline int fh_itch_parse_msg(uint64_t seq_no, uint8_t *buffer, int length,
                                    fh_shr_lh_conn_t *conn)
{
    fh_shr_cfg_lh_line_t    *linecfg    = conn->line->config;
    uint16_t                 msg_length;
    char                     msg_type;
    void                    *data;
    int                      data_length;
    int                      dup;
    FH_STATUS                rc;

    /* make sure there are enough bytes remaining and then parse off the message length */
    if (length < 2) {
        FH_LOG(LH, ERR, ("message block < 2 bytes on line %s (%s)", linecfg->name, conn->tag));
        conn->stats.message_errors++;
        return -1;
    }
    msg_length   = ntoh16(*(uint16_t *)buffer);
    length      -= 2;
    buffer      += 2;

    /* make sure there are enough bytes remaining for the message itself */
    if (length < msg_length) {
        FH_LOG(LH, ERR, ("buffer length (%d bytes) < message length (%d bytes) on line %s (%s)",
                         length, msg_length, linecfg->name, conn->tag));
        conn->stats.message_errors++;
        return -1;

    }
    else if (msg_length < 1) {
        FH_LOG(LH, ERR, ("invalid message length (%d) on line %s (%s)",
                         msg_length, linecfg->name, conn->tag));
        conn->stats.message_errors++;
        return -1;
    }

    /* check to see whether message is a duplicate */
    dup = fh_itch_is_duplicate(seq_no, conn);
    if (dup == 0) {
        conn->stats.duplicate_packets++;
        return msg_length + 2;
    }

    /* read the message type and process the message */
    msg_type = *(char *)buffer;
    switch (msg_type) {

    /* set the millisecond timestamp to seconds * value and remember this value for later */
    case 'T':
        conn->timestamp = fh_itch_parse_atoi(buffer + 1, 5) * 1000;
        break;

    /* subtract the current milliseconds from the timestamp and add the new milliseconds */
    case 'M':
        conn->timestamp -= conn->timestamp % 1000;
        conn->timestamp += fh_itch_parse_atoi(buffer + 1, 3);
        break;

    /* process a system event message */
    case 'S':
        if (fh_itch_parse_sys_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a stock directory message */
    case 'R':
        if (fh_itch_parse_stock_dir_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a stock trading action message */
    case 'H':
        if (fh_itch_parse_stock_trade_act_msg(buffer, msg_length, conn, &data,
                                              &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a market participant position message */
    case 'L':
        if (fh_itch_parse_market_part_pos_msg(buffer, msg_length, conn, &data,
                                              &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process an order add (w/o MPID attribution) message */
    case 'A':
        if (fh_itch_parse_order_add_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process an order add (w/ MPID attribution) message */
    case 'F':
        if (fh_itch_parse_order_add_attr_msg(buffer, msg_length, conn, &data,
                                             &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process an order executed message */
    case 'E':
        if (fh_itch_parse_order_exe_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process an order executed (w/ price) message */
    case 'C':
        if (fh_itch_parse_order_exe_price_msg(buffer, msg_length, conn, &data,
                                              &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process an order cancel message */
    case 'X':
        if (fh_itch_parse_order_cancel_msg(buffer, msg_length, conn, &data,
                                           &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process and order delete message */
    case 'D':
        if (fh_itch_parse_order_delete_msg(buffer, msg_length, conn, &data,
                                           &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process an order replace message */
    case 'U':
        if (fh_itch_parse_order_replace_msg(buffer, msg_length, conn, &data,
                                            &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a trade (non-cross) message */
    case 'P':
        if (fh_itch_parse_trade_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a trade (cross) message */
    case 'Q':
        if (fh_itch_parse_trade_cross_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a broken trade message */
    case 'B':
        if (fh_itch_parse_trade_broken_msg(buffer, msg_length, conn, &data,
                                           &data_length) != FH_OK) {
            return -1;
        }
        break;

    /* process a net order imbalance indicator message */
    case 'I':
        if (fh_itch_parse_noii_msg(buffer, msg_length, conn, &data, &data_length) != FH_OK) {
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

    /* message is not in a gap, not a dup, and processed successfully */
    if (dup > 0) {
        conn->line->next_seq_no++;
    }

    /* message is in a gap but not a dup and processed successfully */
    else if (dup < 0 && gapnode != NULL) {
        rc = fh_shr_gap_fill_del(&gapnode, seq_no);

        /* everything was normal removing the sequence number */
        if (rc >= 0) {
            conn->stats.recovered_messages++;
        }

        /* if some sequence numbers were deleted in the process... */
        if (rc > 0) {
            FH_LOG(LH, WARN, ("gap list size exceeded -- messages have been lost"));
            conn->stats.lost_messages += rc;
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
            /* check whether all gaps have now been filled and send an alert if they have */
            if (gaplist->count <= 0 && hook_alert) {
                hook_alert(&rc, FH_ALERT_NOGAP, conn);
            }
        }
    }

    /* there is data to send so... */
    if (msg_type != 'T' && msg_type != 'M' && hook_msg_send) {
        hook_msg_send(&rc, data, data_length);
        if (rc != FH_OK) {
            return -1;
        }
    }

    /* if we get here, success so return the number of bytes we used */
    return msg_length + 2;
}

/*
 *  Entry point for parsing of an ITCH packet
 */
FH_STATUS fh_itch_parse_pkt(uint8_t *packet, int length, fh_shr_lh_conn_t *conn)
{
    fh_itch_moldudp64_t      pkt_header;
    int                      i;
    int                      bytes_used;
    int                      rc;
    fh_shr_cfg_lh_line_t    *linecfg = conn->line->config;

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

    /* increment the number of bytes received on this line by the packet length */
    conn->stats.bytes += length;

    /* make sure the packet is at least the size of a MoldUDP64 header */
    if (length < FH_ITCH_MOLDUDP64_SIZE) {
        FH_LOG(LH, ERR, ("invalid packet size on line %s (%s)", linecfg->name, conn->tag));
        /* increment the number of bytes received on this line by the packet length */
        conn->stats.packet_errors++;
        return FH_ERROR;
    }

    /* populate the packet header with data from the buffer */
    fh_itch_moldudp64_extract(packet, &pkt_header);

    /* heartbeat packets -- update the next expected SN iff packet SN is bigger */
    if (pkt_header.msg_count == 0x0000) {
        if (pkt_header.seq_no > conn->line->next_seq_no) {
            conn->line->next_seq_no = pkt_header.seq_no;
        }
        return FH_OK;
    }

    /* end of session -- reset the next expected sequence number to prepare for next session */
    if (pkt_header.msg_count == 0xffff) {
        conn->line->next_seq_no = 1;
        return FH_OK;
    }

    /* adjust the buffer and count of bytes remaining */
    packet += FH_ITCH_MOLDUDP64_SIZE;
    length -= FH_ITCH_MOLDUDP64_SIZE;

    /* process each message in the packet */
    for (i = 0; i < pkt_header.msg_count; i++) {
        /* parse the i'th message */
        if ((bytes_used = fh_itch_parse_msg(pkt_header.seq_no + i, packet, length, conn)) < 0) {
            return FH_ERROR;
        }

        /* adjust counters, sequence numbers, etc */
        packet += bytes_used;
        length -= bytes_used;
        conn->stats.messages++;
    }

    /* once execution gets here, success! */
    return FH_OK;
}

/*
 * Function to initialize the message parser
 */
FH_STATUS fh_itch_parse_init(fh_shr_lh_proc_t *process)
{
    /* cache any hooks that have been registered and will later be called */
    FH_ITCH_PARSE_CACHE_HOOK(alert,               ALERT);
    FH_ITCH_PARSE_CACHE_HOOK(msg_send,            MSG_SEND);
    FH_ITCH_PARSE_CACHE_HOOK(msg_system,          ITCH_MSG_SYSTEM);
    FH_ITCH_PARSE_CACHE_HOOK(msg_stock_dir,       ITCH_MSG_STOCK_DIR);
    FH_ITCH_PARSE_CACHE_HOOK(msg_stock_trade_act, ITCH_MSG_STOCK_TRADE_ACT);
    FH_ITCH_PARSE_CACHE_HOOK(msg_market_part_pos, ITCH_MSG_MARKET_PART_POS);
    FH_ITCH_PARSE_CACHE_HOOK(msg_order_add,       ITCH_MSG_ORDER_ADD);
    FH_ITCH_PARSE_CACHE_HOOK(msg_order_add_attr,  ITCH_MSG_ORDER_ADD_ATTR);
    FH_ITCH_PARSE_CACHE_HOOK(msg_order_exe,       ITCH_MSG_ORDER_EXE);
    FH_ITCH_PARSE_CACHE_HOOK(msg_order_exe_price, ITCH_MSG_ORDER_EXE_PRICE);
    FH_ITCH_PARSE_CACHE_HOOK(msg_order_cancel,    ITCH_MSG_ORDER_CANCEL);
    FH_ITCH_PARSE_CACHE_HOOK(msg_order_delete,    ITCH_MSG_ORDER_DELETE);
    FH_ITCH_PARSE_CACHE_HOOK(msg_order_replace,   ITCH_MSG_ORDER_REPLACE);
    FH_ITCH_PARSE_CACHE_HOOK(msg_trade,           ITCH_MSG_TRADE);
    FH_ITCH_PARSE_CACHE_HOOK(msg_trade_cross,     ITCH_MSG_TRADE_CROSS);
    FH_ITCH_PARSE_CACHE_HOOK(msg_trade_broken,    ITCH_MSG_TRADE_BROKEN);
    FH_ITCH_PARSE_CACHE_HOOK(msg_noii,            ITCH_MSG_NOII);

    /* initialize the static gap tracking list to the max number of entries configured */
    if (process->config->gap_list_max > 0) {
        gaplist = fh_shr_gap_fill_new((uint32_t)process->config->gap_list_max,
                                      (uint32_t)process->config->gap_timeout);
    }

    /* if we get here, success */
    return FH_OK;
}
