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
/*********************************************************************/
/* file: parse_packet_hdr.c                                          */
/* Usage:parse message headers                                       */
/* Author: Wally Matthews of  Collaborative Software Initiative      */
/* Conception: Nov. 9, 2008                                          */
/* Inherited from Tervela Arca Multicast feed handler                */
/*   with mods to remove Tervela specifics                           */
/*********************************************************************/

#include <string.h>
//#include "fh_feed_group.h"
#include "fh_arca_headers.h"
#include "fh_data_conversions.h"
#include "ArcaL2Msg.h"

const int body_size[] = 
{
     0,                                     /* 0 */
    sizeof(ArcaL2SequenceReset_t),          /* 1 */
     0,                                     /* 2 */
     0, 0,                                  /* 3, 4 */
    sizeof(ArcaL2Unavailable_t),            /* 5 */
     0, 0, 0, 0,                            /* 6, 7, 8, 9 */
    sizeof(ArcaL2RetransResponse_t),        /* 10 */
     0, 0, 0, 0, 0, 0, 0, 0, 0,             /* 11 - 19 */
    sizeof(ArcaL2RetransRequest_t),         /* 20 */
     0, 0, 0,                               /* 21, 22, 23 */
    sizeof(ArcaL2HeartbeatResponse_t),      /* 24 */
     0, 0, 0, 0, 0,                         /* 25, 26, 27, 28, 29 */
    sizeof(ArcaL2RefreshRequest_t),         /* 30 */
    sizeof(ArcaL2ImbalanceRequest_t),       /* 31 */
    sizeof(ArcaL2BookOrder_t),              /* 32 */
    36, /* sizeof() */                      /* 33 */
    sizeof(ArcaL2SymbolUpdateRequest_t),    /* 34 */
    sizeof(ArcaL2SymbolUpdate_t),           /* 35 */
    sizeof(ArcaL2SymbolClear_t),            /* 36 */
    sizeof(ArcaL2FirmUpdate_t),             /* 37 */
    sizeof(ArcaL2FirmUpdateRequest_t),      /* 38 */
     0,                                     /* 39 */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,          /* 40 - 49 */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,          /* 50 - 59 */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,          /* 60 - 69 */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,          /* 70 - 79 */
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0,          /* 80 - 89 */
     0, 0, 0, 0, 0, 0, 0, 0, 0,             /* 90 - 98 */
    sizeof(ArcaL2Delete_t),                 /* 99 smallest of below*/
    sizeof(ArcaL2Add_t),                    /* 100 */
    sizeof(ArcaL2Modify_t),                 /* 101 */
    sizeof(ArcaL2Delete_t),                 /* 102 */
    sizeof(ArcaL2Imbalance_t),              /* 103 */
};

/*-------------------------------------------------------------------------*/
/* return the size of a message from its msg_type                          */
/*-------------------------------------------------------------------------*/
int get_body_size(const unsigned char msg_type)
{
    return body_size[msg_type];
};
/*-------------------------------------------------------------------------*/
/* parse the exchange message header into a msg_hdr struct; return length  */
/*-------------------------------------------------------------------------*/
int parse_packet_hdr(struct msg_hdr* const pkthdr, const char* const pkt_ptr, 
    const int pktlngth, const uint64_t rcv_time) 
{
    if (pktlngth < ARCAM_MSG_HDR_SIZE) return -1;
    pkthdr->msg_type = big_endian_16(pkt_ptr+MSG_TYPE_OFFSET);
    pkthdr->msg_seq_num = big_endian_32(pkt_ptr+MSG_NUM_OFFSET);
    pkthdr->msg_rcv_time = rcv_time;
    pkthdr->send_time = big_endian_32(pkt_ptr+SEND_TIME_IN_OFFSET);
    pkthdr->msg_size = big_endian_16(pkt_ptr);
    pkthdr->product_id = (unsigned char) *(pkt_ptr+PRODUCT_ID_IN_OFFSET);
    pkthdr->retrans_flag = (unsigned char) *(pkt_ptr+RETRANS_FLAG_OFFSET);
    pkthdr->num_body_entries = (unsigned char) *(pkt_ptr+NUMBER_BODIES_OFFSET);
    if (pkthdr->msg_type != BOOK_REFRESH) return ARCAM_MSG_HDR_SIZE;
    if (pktlngth<ARCAM_REFRESH_MSG_HDR_SIZE) return -1;
    pkthdr->session_id = (unsigned char) *(pkt_ptr+SESSIONID_OFFSET);
    pkthdr->symbol_index = big_endian_16(pkt_ptr+SYMBOLINDEX_OFFSET);
    pkthdr->current_refresh_msg_seq = big_endian_16(pkt_ptr+SYMBOLINDEX_OFFSET);
    pkthdr->total_refresh_msg_seq = big_endian_16(pkt_ptr+SYMBOLINDEX_OFFSET);
    pkthdr->last_source_seq_num = big_endian_32(
        pkt_ptr+LAST_SOURCE_SEQ_NUM_OFFSET);
    pkthdr->last_msg_seq = big_endian_32(pkt_ptr+LST_MSG_SEQ_OFFSET);
    memcpy(&(pkthdr->symbol),pkt_ptr+SYMBOL_OFFSET,ARCABOOK_SYMBOL_LENGTH);
    return ARCAM_REFRESH_MSG_HDR_SIZE;
};

