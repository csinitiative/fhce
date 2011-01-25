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
#ifndef __ARCAMCONSTANTS_H
#define __ARCAMCONSTANTS_H
/*********************************************************************/
/* file: fh_arca_constants.h                                         */
/* Usage: symbolic constants for arca multicast line handler         */
/* Author: Wally Matthews of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/* Inherited from Tervela Arca Multicast feed handler                */
/*   with mods to remove Tervela specific plug in constants          */
/*********************************************************************/
#include <stdint.h>  //uints and others

//#define DEBUG         1
//`#define __UNIT_TEST__ 1
//#define RATE_TESTING 1

#define PACKET_MAX 1500
// ARCAM Message specific constants
//.. Short Header
#define ARCAM_MSG_HDR_SIZE 16
//.. LONG HEADER
#define ARCAM_REFRESH_MSG_HDR_SIZE 48
//.. Header field offsets
#define MSG_SIZE_OFFSET 0
#define MSG_TYPE_OFFSET 2
#define MSG_NUM_OFFSET 4
#define SEND_TIME_IN_OFFSET 8
#define PRODUCT_ID_IN_OFFSET 12
#define RETRANS_FLAG_OFFSET 13
#define NUMBER_BODIES_OFFSET 14
#define SESSIONID_OFFSET 17
#define SYMBOLINDEX_OFFSET 18
#define CURRENT_REFRESH_MSG_SEQ_OFFSET 20
#define TOTAL_REFRESH_MSG_SEQ_OFFSET 22
#define LAST_SOURCE_SEQ_NUM_OFFSET 24
#define LST_MSG_SEQ_OFFSET 28
#define SYMBOL_OFFSET 32
// feedGroup specific constants
//.. intfc string max length usually ended with null char
#define INTFC_LENGTH 10
//.. ip address max length
#define IPADRS_LENGTH 16
//.. maximum requests that the exchange will reply to in a session
#define MAX_REQUESTS_PER_SESSION 5000
//.. when using retrans option; shift to refresh after X requests
#define WHEN_TO_STOP_REQUESTS MAX_REQUESTS_PER_SESSION/2
//.. maximum processors allowed in configuration
#define MAX_PROCESSORS 64
//.. max symbol length in arcabook spec.
#define ARCABOOK_SYMBOL_LENGTH 16
//.. max firm length in arcabook spec.
#define ARCABOOK_ATTRIBUTION_LENGTH 5
#define FILENAME_ALLOC_SIZE 64
//.. maximum space allowed for missing bit list
#define MISSING_SIZE 256*128 //in double words
//.. number of sequence numbers that can be in missing bit list
#define MISSING_RANGE MISSING_SIZE*64
//.. when using refresh; gap size that triggers auto refresh switch
#define GAP_SIZE_TOO_BIG 20
//.. number of feedgroups for Listed,OTC,ETF
#define FEEDGROUP_MAXIMUM 10
//.. size of stored message ring buffer when using strict ordering
#define STORED_MESSAGE_RING_BUFFER_SIZE 256*256*4
//.. max sessions supported
#define MAX_SESSIONS 16
//.. max free list size for stored messages
#define FREE_LIST_SIZE MISSING_RANGE+2
// number of feeds each having mirrored feed groups
// arcabook has 10 but new feeds will use protocol
#define SOCKET_SET_SIZE 30*2              
// stupidly big but trying to be safe
#define PROCESS_NAME_MAX 128
#define SOURCE_ID_LENGTH 21
// french people like to use long names too many vowels
#define MAXPATHLEN 256
#define LOG_LEVEL 0

//.. sockets
enum SOCKET_FUNCTION {
    PRIMARYFEED = 0,         //feed primary mcast group
    SECONDARYFEED = 1,       //feed secondary mcast group
    PRIMARY_REFRESH = 2,     //refresh socket
    SECONDARY_REFRESH =3,
    PRIMARY_RETRANS = 4,     //retrans socket
    SECONDARY_RETRANS = 5
};
//.. values found in msg_type field of parsed message
//.. most are exchange defined, alerts are CSI defined
enum MESSAGE_TYPE{
    SEQUENCE_NUMBER_RESET = 1,
    MESSAGE_UNAVAILABLE = 5,
    RETRANSMISSION_RESPONSE = 10,
    RETRANSMISSION_REQUEST = 20,
    HEARTBEAT_RESPONSE =  24,
    BOOK_REFRESH_REQUEST = 30,
    IMBALANCE_REFRESH_REQUEST = 31,
    BOOK_REFRESH = 32,
    IMBALANCE_REFRESH = 33,
    SYMBOL_MAPPING_REQUEST = 34,
    SYMBOL_MAPPING = 35,
    SYMBOL_CLEAR = 36,
    FIRM_MAPPING_REQUEST = 38,
    FIRM_MAPPING = 37,
    ORDERS = 99,
    ADD_ORDER = 100,
    MODIFY_ORDER = 101,
    DELETE_ORDER = 102,
    IMBALANCE = 103,              
    TRADE = 220,
    TRADE_CANCEL = 221,
    TRADE_CORRECTION = 222,
    ALERT = 2,                    //CSI defined rather than exchange defined
    PACKET_LOSS = 3               //CSI defined rather than exchange defined
};
enum MESSAGE_LENGTH{
    MESSAGE_UNAVAILABLE_LENGTH = 8,
    SEQUENCE_RESET = 4,
    TRADE_LENGTH = 52,
    TRADE_CANCEL_LENGTH = 32,
    TRADE_CORRECTION_LENGTH = 56
};
//.. exhanges that currently use the arcabook for equities protocol
enum EXCHANGE {
    LISTED = 0,                   // listed equities
    OTC = 1,                      // Over The Counter equities
    ETF = 2,                      // Exchange Traded Funds
    BB  = 3,                      // Bulletin Board Traded
    ARCATRADE = 4                 // TRADES for Listed,OTC,ETF
};
//.. buy or sell
enum ORDER_SIDE {
    BUY = 1,                      
    SELL = 2
};
//.. lost packet recovery strategy 
enum REQUEST_OR_INTERVAL {
    SECOND_FEED = 0,              // mirrored groups recovery only
    INTERVAL = 1,                 // periodic refresh only
    REQUEST = 2,                  // request refresh only
    MIXED = 3                     // request refresh for small gaps until requests are maxed out for the session
};
//.. what encoding to use
enum FAST_MODE {
    UNCOMPACTED = 0,
    FAST = 1
};
//.. alerts
enum ALERT_TYPE {
    ORDERING_STATE_CHANGE = 1,   // state change from in order -> out of order or vice versa
    STRICT_ORDERING_STATE = 2,   // line handler strict ordering state is
    LOST_FEED = 3,               // state change for primary & secondary feeds
    LOST_PACKETS = 4,            // packets have been declared as permanently lost
    EXCHANGE_LOST_PACKETS = 5,   // exchange reported it had lost packets
    PARSE_ERROR = 10,            // parse error in exchange message
    RUNT_PACKET = 11             // Runt packet received
};
//.. parsed form of a messaage header See exchange protocol specification for details
struct msg_hdr {
    uint64_t msg_rcv_time;
    uint32_t msg_seq_num;
    uint32_t send_time;
    uint16_t msg_size;
    uint16_t msg_type;
    unsigned char product_id;
    unsigned char retrans_flag;
    unsigned char num_body_entries;
    // below are book refresh specific fields
    unsigned char session_id;
    uint32_t last_source_seq_num;
    uint32_t last_msg_seq;
    uint16_t symbol_index;
    uint16_t current_refresh_msg_seq;
    uint16_t total_refresh_msg_seq;
    char symbol[16];
};
//.. parsed form of a message body  See exchange protocol specification for details
struct msg_body {
    uint64_t price;
    uint32_t source_seq_num;
    uint32_t source_time;
    uint32_t order_id;
    uint32_t volume;
    uint32_t price_numerator;
    uint32_t total_imbalance;          //imbalance msg
    uint32_t market_imbalance;         //imbalance msg
    uint32_t next_seq_number;          //seq reset msg
    uint32_t begin_seq_number;         //msg unavailable msg
    uint32_t end_seq_number;           //msg unavailable msg
    uint32_t msg_seq_num;              // replicated from hdr
    uint32_t buy_side_link_id;         // specific to trades
    uint32_t sell_side_link_id;        // specific to trades
    uint32_t quote_link_id;            // specific to trades
    uint32_t original_src_seq_num;     // specific to trade cancel/correction
    uint32_t status;                   //used for alerts
    int      primary_or_secondary;     //so that pub can identify source
    int      alert_type;
    uint16_t symbol_index;
    uint16_t firm_index;
    uint16_t msg_type;
    uint16_t auction_time;             //imbalance msg
    unsigned char price_scale_code;
    char side;
    char exchange_id;
    char security_type;
    uint8_t session_id;
    char auction_type;                 //imbalance msg
    char trade_cond_1;                 // specific to trades
    char trade_cond_2;                 // specific to trades
    char trade_cond_3;                 // specific to trades
    char trade_cond_4;                 // specific to trades
    char symbol[ARCABOOK_SYMBOL_LENGTH+1];                   //book refresh msg
    char firm[ARCABOOK_ATTRIBUTION_LENGTH+1];                     
};
#endif //__ARCAMCONSTANTS_H
