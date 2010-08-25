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

#ifndef __FH_DIR_EDGE_MSG_H__
#define __FH_DIR_EDGE_MSG_H__


/*
 *
 *  In this header files are the SCRATCH Book Message definitions.
 *
 */
typedef struct {
    uint32_t       timestamp;               /** converted timestamp          **/
    char           message_type;            /** Direct Edge Message type     **/
}fh_edge_msg_hdr_t;


typedef struct {
    char *          message;                /** Raw message pointer          **/
    int             len;                    /** Raw message length           **/
}fh_edge_raw_msg_t;

typedef struct {
    fh_edge_msg_hdr_t hdr;
    char                  event_code;        /** Event code                  **/
    fh_edge_raw_msg_t raw;               /** Raw message reproduced      **/
}fh_edge_sysEvent_msg_t ;


/* Add Order Message definiton  */
typedef struct {
    fh_edge_msg_hdr_t hdr;
    char                  order_ref[12];     /** Order Reference AlphaNumeric**/
    char                  side_indicator;    /** Bid Or Offer                **/
    uint32_t              order_quantity;    /** Converted Order quantity    **/
    char                  security[6];       /** Security Symbol             **/
    uint64_t              price;             /** Converted price field       **/
    char                  display;           /** Attributed/Non Attributed   **/
    char                  mmid[4];           /** MMID valid for Attributed Only**/
    fh_shr_lkp_sym_t     *sym_entry;         /**< entry in the symbol table for this symbol **/
    fh_shr_lkp_ord_t     *ord_entry;         /** entry in the Order Table    **/
    fh_edge_raw_msg_t raw;
}fh_edge_add_order_msg_t;

/* Order Executed Message definitiuon */

typedef struct {
    fh_edge_msg_hdr_t hdr;
    char                  order_ref[12];     /** Order Reference Alpha/Num   **/
    uint32_t              executed_shares;   /** Converted No of shares      **/
    char                  match_number[21];  /** Match Number Alpha/Numeric  **/
    fh_shr_lkp_ord_t     *ord_entry;         /** Entry in Order Table        **/
    fh_edge_raw_msg_t raw;               /** Raw Message                 **/
}fh_edge_orderExecuted_msg_t;

/* Order Cancelled Message definition */

typedef struct {
    fh_edge_msg_hdr_t hdr;
    char                  order_ref[12];     /** Order Reference Alpha/Num   **/
    uint32_t              canceled_shares;   /** Converted Canceled shares count **/
    fh_shr_lkp_ord_t     *ord_entry;         /** entry in the Order Table    **/
    fh_edge_raw_msg_t raw;               /** Raw Message                 **/
}fh_edge_orderCanceled_msg_t;

/* Trade Message Definition  */
typedef struct {
    fh_edge_msg_hdr_t hdr;
    char                  order_ref[12];     /** Order Reference Alpha/Num   **/
    char                  side_indicator;    /** Bid Or Offer                **/
    uint32_t              shares;            /** Converted No of shares      **/
    char                  security[6];       /** Symbol                      **/
    uint64_t              price;             /** Converted price field       **/
    char                  match_number[21];  /** Match Number Alpha/Numeric  **/
    fh_edge_raw_msg_t raw;
}fh_edge_trade_msg_t;

/* Broken Trade Message definition */
typedef struct {
    fh_edge_msg_hdr_t hdr;
    char                  match_number[21];  /** Match Number Alpha/Numeric  **/
    fh_edge_raw_msg_t raw;
}fh_edge_brokenTrade_msg_t;

/* Security Status Message definition */
typedef struct {
    fh_edge_msg_hdr_t hdr;
    char                  security[6];       /** Symbol                      **/
    char                  halted_state;      /** Indicates Security Halted/Trading **/
    fh_edge_raw_msg_t raw;
}fh_edge_securityStatus_msg_t;


#endif
