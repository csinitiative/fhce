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

#ifndef __FH_ITCH_MSG_H__
#define __FH_ITCH_MSG_H__

/*
 * A note about structures in this file - all structures are arranged to minimize padding and
 * maximize natural byte alignment.  For this reason, not all fields are in the same order as
 * in the ITCH specificiation.
 */

/* system headers */
#include <stdint.h>

/* shared FH library headers */
#include "fh_shr_lkp_symbol.h"
#include "fh_shr_lkp_order.h"

/* constants describing the properties of various ITCH messages */
#define FH_ITCH_MSG_SYSTEM_SIZE             (2)
#define FH_ITCH_MSG_STOCK_DIR_SIZE          (16)
#define FH_ITCH_MSG_STOCK_TRADE_ACT_SIZE    (13)
#define FH_ITCH_MSG_MARKET_PART_POS_SIZE    (14)
#define FH_ITCH_MSG_ORDER_ADD_SIZE          (36)
#define FH_ITCH_MSG_ORDER_ADD_ATTR_SIZE     (40)
#define FH_ITCH_MSG_ORDER_EXE_SIZE          (31)
#define FH_ITCH_MSG_ORDER_EXE_PRICE_SIZE    (42)
#define FH_ITCH_MSG_ORDER_CANCEL_SIZE       (19)
#define FH_ITCH_MSG_ORDER_DELETE_SIZE       (13)
#define FH_ITCH_MSG_ORDER_REPLACE_SIZE      (41)
#define FH_ITCH_MSG_TRADE_SIZE              (48)
#define FH_ITCH_MSG_TRADE_CROSS_SIZE        (39)
#define FH_ITCH_MSG_TRADE_BROKEN_SIZE       (13)
#define FH_ITCH_MSG_NOII_SIZE               (58)

/* some convenience typedefs for structs below */
typedef struct fh_itch_msg_header           fh_itch_msg_header_t;
typedef struct fh_itch_msg_raw              fh_itch_msg_raw_t;
typedef struct fh_itch_msg_system           fh_itch_msg_system_t;
typedef struct fh_itch_msg_stock_dir        fh_itch_msg_stock_dir_t;
typedef struct fh_itch_msg_stock_trade_act  fh_itch_msg_stock_trade_act_t;
typedef struct fh_itch_msg_market_part_pos  fh_itch_msg_market_part_pos_t;
typedef struct fh_itch_msg_order_add        fh_itch_msg_order_add_t;
typedef struct fh_itch_msg_order_add_attr   fh_itch_msg_order_add_attr_t;
typedef struct fh_itch_msg_order_exe        fh_itch_msg_order_exe_t;
typedef struct fh_itch_msg_order_exe_price  fh_itch_msg_order_exe_price_t;
typedef struct fh_itch_msg_order_cancel     fh_itch_msg_order_cancel_t;
typedef struct fh_itch_msg_order_delete     fh_itch_msg_order_delete_t;
typedef struct fh_itch_msg_order_replace    fh_itch_msg_order_replace_t;
typedef struct fh_itch_msg_trade            fh_itch_msg_trade_t;
typedef struct fh_itch_msg_trade_cross      fh_itch_msg_trade_cross_t;
typedef struct fh_itch_msg_trade_broken     fh_itch_msg_trade_broken_t;
typedef struct fh_itch_msg_noii             fh_itch_msg_noii_t;

/**
 *  @brief A manufactured ITCH message header (timestamp, sequence number, etc)
 */
struct fh_itch_msg_header {
    uint64_t             seq_no;            /**< sequence number of the message */
    uint32_t             timestamp;         /**< last millisecond timestamp before message */
};

/**
 *  @brief Stores a raw ITCH message (for inclusion in other ITCH message structs)
 */
struct fh_itch_msg_raw {
    uint8_t             *message;           /**< bytes of the message */
    uint32_t             size;              /**< length of the message */
};

/**
 *  @brief An ITCH system event message
 */
struct fh_itch_msg_system {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    char                 event_code;        /**< ITCH event code for this system event */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH stock directory message
 */
struct fh_itch_msg_stock_dir {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    char                 stock[6];          /**< security symbol */
    char                 market_cat;        /**< listing market */
    char                 fin_stat_ind;      /**< financial status indicator */
    char                 round_lots_only;   /**< does NASDAQ limit order entry to round lots? */
    uint32_t             round_lot_size;    /**< round lot size */
    fh_shr_lkp_sym_t    *sym_entry;         /**< entry in the symbol table for this symbol */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH stock trading action message
 */
struct fh_itch_msg_stock_trade_act {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    char                 stock[6];          /**< security symbol */
    char                 trading_state;     /**< current trading state of the stock */
    char                 reason[4];         /**< reason for trading action */
    fh_shr_lkp_sym_t    *sym_entry;         /**< entry in the symbol table for this symbol */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH market participant position message
 */
struct fh_itch_msg_market_part_pos {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    char                 mpid[4];           /**< market participant ID */
    char                 stock[6];          /**< security symbol */
    char                 pri_mkt_mkr;       /**< participant qualifies as primary market maker? */
    char                 mkt_mkr_mode;      /**< market maker mode */
    char                 mkt_part_state;    /**< market participant state */
    fh_shr_lkp_sym_t    *sym_entry;         /**< entry in the symbol table for this symbol */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH add order message
 */
struct fh_itch_msg_order_add {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    uint64_t             order_no;          /**< unique order reference number */
    uint64_t             price;             /**< price (in ISE price format) */
    uint32_t             shares;            /**< number of shares in the order */
    char                 buy_sell_ind;      /**< buy/sell indicator */
    char                 stock[6];          /**< stock symbol */
    fh_shr_lkp_sym_t    *sym_entry;         /**< entry in the symbol table for this symbol */
    fh_shr_lkp_ord_t    *ord_entry;         /**< entry in the order table for this order */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH add order w/ attribution message
 */
struct fh_itch_msg_order_add_attr {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    uint64_t             order_no;          /**< unique order reference number */
    uint64_t             price;             /**< price (in ISE price format) */
    uint32_t             shares;            /**< number of shares in the order */
    char                 buy_sell_ind;      /**< buy/sell indicator */
    char                 stock[6];          /**< stock symbol */
    char                 attribution[4];    /**< MPID attribution  */
    fh_shr_lkp_sym_t    *sym_entry;         /**< entry in the symbol table for this symbol */
    fh_shr_lkp_ord_t    *ord_entry;         /**< entry in the order table for this order */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH order executed message
 */
struct fh_itch_msg_order_exe {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    uint64_t             order_no;          /**< unique order reference number */
    uint64_t             match_no;          /**< unique match number */
    uint32_t             shares;            /**< number of shares in the order */
    fh_shr_lkp_ord_t    *ord_entry;         /**< entry in the order table for this order */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH order executed message
 */
struct fh_itch_msg_order_exe_price {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    uint64_t             order_no;          /**< unique order reference number */
    uint64_t             match_no;          /**< unique match number */
    uint64_t             exe_price;         /**< the execution price */
    uint32_t             shares;            /**< number of shares in the order */
    char                 printable;         /**< reflect on displays and volume calculations? */
    fh_shr_lkp_ord_t    *ord_entry;         /**< entry in the order table for this order */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH order cancel message
 */
struct fh_itch_msg_order_cancel {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    uint64_t             order_no;          /**< unique order reference number */
    uint32_t             shares;            /**< number of shares being cancelled */
    fh_shr_lkp_ord_t    *ord_entry;         /**< entry in the order table for this order */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH order delete message
 */
struct fh_itch_msg_order_delete {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    uint64_t             order_no;          /**< unique order reference number */
    fh_shr_lkp_ord_t    *ord_entry;         /**< entry in the order table for this order */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 * @brief An ITCH order replace message
 */
struct fh_itch_msg_order_replace {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    uint64_t             old_order_no;      /**< old unique order reference number */
    uint64_t             new_order_no;      /**< new unique order reference number */
    uint64_t             price;             /**< the new price */
    uint32_t             shares;            /**< new total number of shares */
    fh_shr_lkp_ord_t    *ord_entry;         /**< entry in the order table for this order */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH trade (non-cross) message
 */
struct fh_itch_msg_trade {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    uint64_t             order_no;          /**< unique order reference number */
    uint64_t             price;             /**< the new price */
    uint64_t             match_no;          /**< unique match number */
    uint32_t             shares;            /**< new total number of shares */
    char                 buy_sell_ind;      /**< buy/sell indicator */
    char                 stock[6];          /**< stock symbol */
    fh_shr_lkp_sym_t    *sym_entry;         /**< entry in the symbol table for this symbol */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH cross trade message
 */
struct fh_itch_msg_trade_cross {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    uint64_t             price;             /**< the new price */
    uint64_t             match_no;          /**< unique match number */
    uint32_t             shares;            /**< new total number of shares */
    char                 stock[6];          /**< stock symbol */
    char                 type;              /**< the cross session for this message */
    fh_shr_lkp_sym_t    *sym_entry;         /**< entry in the symbol table for this symbol */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH broken trade/order execution message
 */
struct fh_itch_msg_trade_broken {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    uint64_t             match_no;          /**< unique match number of the broken execution */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

/**
 *  @brief An ITCH net order imbalance indicator message
 */
struct fh_itch_msg_noii {
    fh_itch_msg_header_t header;            /**< manufactured ITCH header */
    uint64_t             far_price;         /**< hyp. clearing price for cross orders */
    uint64_t             near_price;        /**< hyp. clearing price for cross + cont. orders */
    uint64_t             ref_price;         /**< the current reference price */
    uint32_t             paired_shares;     /**< # of shares elegible to be matched */
    uint32_t             imbalance_shares;  /**< # of shares not mached */
    char                 imbalance_dir;     /**< direction of the imbalance (buy, sell, etc) */
    char                 stock[6];          /**< stock symbol */
    char                 cross_type;        /**< type of cross for which NOII is being generated */
    char                 price_var_ind;     /**< price variation indicator (see ITCH spec) */
    fh_shr_lkp_sym_t    *sym_entry;         /**< entry in the symbol table for this symbol */
    fh_itch_msg_raw_t    raw;               /**< raw ITCH message */
};

#endif /* __FH_ITCH_MSG_H__ */
