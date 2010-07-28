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

#ifndef __FH_BATS_PITCH20_MSG_H__
#define __FH_BATS_PITCH20_MSG_H__


#define TIME_MESSAGE_TYPE                0x20
#define ADD_ORDER_LONG_MESSAGE_TYPE      0x21
#define ADD_ORDER_SHORT_MESSAGE_TYPE     0x22
#define ORDER_EXECUTED_MESSAGE_TYPE      0x23
#define ORDER_EXECUTE_PRICE_MESSAGE_TYPE 0x24
#define REDUCE_SIZE_LONG_MESSAGE_TYPE    0x25
#define REDUCE_SIZE_SHORT_MESSAGE_TYPE   0x26
#define MODIFY_LONG_MESSAGE_TYPE         0x27
#define MODIFY_SHORT_MESSAGE_TYPE        0x28
#define DELETE_ORDER_MESSAGE_TYPE        0x29
#define TRADE_LONG_MESSAGE_TYPE          0x2A
#define TRADE_SHORT_MESSAGE_TYPE         0x2B
#define TRADE_BREAK_MESSAGE_TYPE         0x2C
#define END_OF_SESSION_MESSAGE_TYPE      0x2D

/*
 *  Message sizes as per the specification of Pitch 2.0 messages
 */

#define FH_BATS_SEQUENCE_UNIT_HEADER_SIZE       8
#define FH_BATS_ADD_ORDER_LONG_MSG_SIZE         34
#define FH_BATS_ADD_ORDER_SHORT_MSG_SIZE        26
#define FH_BATS_ORDER_EXECUTED_MSG_SIZE         26
#define FH_BATS_ORDER_EXECUTE_PRICE_MSG_SIZE    38
#define FH_BATS_REDUCE_SIZE_LONG_MSG_SIZE       18
#define FH_BATS_REDUCE_SIZE_SHORT_MSG_SIZE      16
#define FH_BATS_MODIFY_LONG_MSG_SIZE            27
#define FH_BATS_MODIFY_SHORT_MSG_SIZE           19
#define FH_BATS_DELETE_ORDER_MSG_SIZE           14
#define FH_BATS_TRADE_LONG_MSG_SIZE             41
#define FH_BATS_TRADE_SHORT_MSG_SIZE            33
#define FH_BATS_TRADE_BREAK_MSG_SIZE            14
#define FH_BATS_END_OF_SESSION_MSG_SIZE         6



/**
 *  @brief Stores a raw BATS message (for inclusion in other BATS message structs)
 */
typedef struct {
    uint8_t             *message;           /**< bytes of the message */
    uint32_t             len;               /**< length of the message */
}fh_bats_msg_raw;

/* Sequenced Unit Header   */
typedef struct {
    uint16_t       hdr_length; /* contains the length of the entire packet              */
    uint8_t        msg_count;  /* No of messages that follow this header                */
    uint8_t        unit     ;  /* Unit that applies to messages included in header      */
    uint32_t       seq_no   ;  /* sequence number of the first message following header */
}sequenced_unit_header_t;

/*  Message header for FH created message for down stream consumption                   */
typedef struct {
    uint8_t         msg_type;   /* Message type                                          */
    uint64_t          seq_no;   /* message sequence number                               */
    uint64_t       timestamp;   /* Time stamp in nanoseconds since midnight              */
}fh_bats_msg_header;

 /* Add Order Long message definition    */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t       order_id;   /* Order Id                                              */
    char           side    ;   /* "B" = Buy Order, "S" = sell order                     */
    uint32_t       shares  ;   /* Shares in this transaction                            */
    char           stock[6];   /* stock symbol right padded with space                  */
    uint64_t       price   ;   /* Limit Order price                                     */
    int8_t         add_flags;  /* Bit field, Bit 0 ; 0 = Not Displayed in SIP,          */
                               /*   1 = Displayed in SIP                                */
    fh_shr_lkp_sym_t *sym_entry; /* entry in the symbol table for this symbol           */
    fh_shr_lkp_ord_t *ord_entry; /* entry in the Order Table                            */

    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */
}fh_bats_add_order_long_t;


/* Add Order Short message definition   */

typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t       order_id;   /* Order Id                                              */
    char           side    ;   /* "B" = Buy Order, "S" = sell order                     */
    uint16_t       shares  ;   /* Shares in this transaction                            */
    char           stock[6];   /* stock symbol right padded with space                  */
    uint64_t       price   ;   /* Limit Order price                                     */
    int8_t         add_flags;  /* Bit field, Bit 0 ; 0 = Not Displayed in SIP,          */
                               /*   1 = Displayed in SIP                                */
    fh_shr_lkp_sym_t *sym_entry; /* entry in the symbol table for this symbol           */
    fh_shr_lkp_ord_t *ord_entry; /* entry in the Order Table                            */

    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */
}fh_bats_add_order_short_t;


/* Execute Order Message           */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t      order_id ;   /* Order id of the previously sent add order             */
    uint32_t      shares   ;   /* number of shares executed                             */
    uint64_t      exec_id  ;   /* BATS generated day-unique execution id                */
    fh_shr_lkp_ord_t *ord_entry; /* entry in the Order Table                            */

    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */
}fh_bats_order_execute_t;


/* Order executed at Price/size message      */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t      order_id ;   /* Order id of the previously sent add order             */
    uint32_t      shares   ;   /* number of shares executed                             */
    uint32_t      rem_shares;  /* num of shares remaining after execution               */
    uint64_t      exec_id  ;   /* BATS generated day-unique execution id                */
    uint64_t      price    ;   /* The execution price of the order                      */
    fh_shr_lkp_ord_t *ord_entry; /* entry in the Order Table                            */

    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */
}fh_bats_order_execute_price_t;


/* Reduce Size Long message                  */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t      order_id ;   /* Order id of the previously sent add order             */
    uint32_t      shares   ;   /* number of shares canceled                             */
    fh_shr_lkp_ord_t *ord_entry; /* entry in the Order Table                            */
    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */
}fh_bats_reduce_size_long_t;

/* Reduce Size Short message                 */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t      order_id ;   /* Order id of the previously sent add order             */
    uint16_t      shares   ;   /* number of shares canceled                             */
    fh_shr_lkp_ord_t *ord_entry; /* entry in the Order Table                            */
    fh_bats_msg_raw     raw;
}fh_bats_reduce_size_short_t;


/* Modify Order Long                         */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t       order_id;   /* Order Id                                              */
    uint32_t       shares  ;   /* Shares in this transaction                            */
    uint64_t       price   ;   /* Limit Order price after this modify                   */
    uint8_t    modify_flags;   /* Bit field, Bit 0 ; 0 = Not Displayed in SIP,          */
                               /*   1 = Dispalyed in SIP                                */
                               /* Bit 1: 1 = reset Priority, 0 = Maintain Priority      */
    fh_shr_lkp_ord_t *ord_entry; /* entry in the Order Table                            */
    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */
}fh_bats_modify_long_t;


/* Modify Order Short                        */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t       order_id;   /* Order Id                                              */
    uint16_t       shares  ;   /* Number of shares associated with this order           */
    uint64_t       price   ;   /* Limit Order price after this modify                   */
    uint8_t    modify_flags;   /* Bit field, Bit 0 ; 0 = Not Displayed in SIP,          */
                               /*   1 = Dispalyed in SIP                                */
                               /* Bit 1: 1 = reset Priority, 0 = Maintain Priority      */
    fh_shr_lkp_ord_t *ord_entry; /* entry in the Order Table                            */
    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */
}fh_bats_modify_short_t;


/* Delete Order Message                      */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t       order_id;   /* Order Id                                              */
    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */
}fh_bats_delete_order_t;

/*  Trade Long Message                       */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t       order_id;   /* Order Id of non displayed executed order              */
    char           side    ;   /* "B" = Buy Order, "S" = sell order                     */
    uint32_t       shares  ;   /* Incremental number of shares executed                 */
    char           stock[6];  /* stock symbol right padded with space                  */
    uint64_t       price   ;   /* The execution price of the order                      */
    uint64_t    execution_id;  /* BATS generated day-unique execution identifier of this trade */
    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */
}fh_bats_trade_long_t;

/*  Trade Short Message                       */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t       order_id;   /* Order Id of non displayed executed order              */
    char           side    ;   /* "B" = Buy Order, "S" = sell order                     */
    uint32_t       shares  ;   /* Incremental number of shares executed                 */
    char           stock[6];  /* stock symbol right padded with space                  */
    uint64_t       price   ;   /* The execution price of the order                      */
    uint64_t    execution_id;  /* BATS generated day-unique execution identifier of this trade */
    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */
}fh_bats_trade_short_t;

/*  Trade Break Message                       */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    uint64_t    execution_id;  /* BATS execution identifier of the execution that was   */
                               /* broken. Execution Id refers to previously sent Order  */
                               /* Order Execution or Trade message                      */
    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */

}fh_bats_trade_break_t;

/* End of Session Message                     */
typedef struct{
    fh_bats_msg_header  hdr;   /* Header containing common and important info           */
    fh_bats_msg_raw     raw;   /* Raw BATS message                                      */
}fh_bats_end_of_session_t;


/**
 *  @brief Inline function to extract a pitch20 sequenced unit header from a byte buffer
 */
inline void fh_bats_pitch20_extract(uint8_t *buffer, sequenced_unit_header_t *header)
{
    header->hdr_length = (*((uint16_t *)(buffer)));
    /* convert the sequence number and message count to little-endian */
    header->msg_count = (*((uint16_t *)(buffer + 2)));
    header->seq_no    = (*((uint32_t *)(buffer + 4)));
    header->unit      = *(buffer + 3);

}


#endif  /* __FH_BATS_PITCH20_MSG_H__ */
