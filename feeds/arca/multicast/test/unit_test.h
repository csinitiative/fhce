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

#ifndef __ARCAM_TEST_HEADERS_H
#define __ARCAM_TEST_HEADERS_H

#include <sys/types.h>
#include "fh_arca_constants.h"

#define SYMBOL_MAPPING_BODIES 3
#define FIRM_MAPPING_BODIES 3

#pragma pack(1)
struct packet_hdr 
{
    uint16_t  msg_size;
    uint16_t  msg_type;
    uint32_t  msg_seq_num;
    uint32_t  send_time;
    uint8_t   product_id;
    uint8_t   retrans_flag;
    uint8_t   num_body_entries;
    char      filler;
};
#pragma pack()
#pragma pack(1)
struct seq_number_reset 
{
    uint16_t  msg_size;
    uint16_t  msg_type;
    uint32_t  msg_seq_num;
    uint32_t  send_time;
    uint8_t   product_id;
    uint8_t   retrans_flag;
    uint8_t   num_body_entries;
    char      filler;
    uint32_t  next_seq_number;
};
#pragma pack()
#pragma pack(1)
struct message_unavailable 
{
    uint16_t  msg_size;
    uint16_t  msg_type;
    uint32_t  msg_seq_num;
    uint32_t  send_time;
    uint8_t   product_id;
    uint8_t   retrans_flag;
    uint8_t   num_body_entries;
    char      filler;
    uint32_t  begin_seq_num;
    uint32_t  end_seq_num;
};
#pragma pack()
#pragma pack(1)
struct symbol_clear 
{
    uint16_t  msg_size;
    uint16_t  msg_type;
    uint32_t  msg_seq_num;
    uint32_t  send_time;
    uint8_t   product_id;
    uint8_t   retrans_flag;
    uint8_t   num_body_entries;
    char      filler;
    uint32_t  next_seq_num;
    uint16_t  symbol_index;
    uint8_t   session_id;
    char      body_filler;
};
#pragma pack()
#pragma pack(1)
struct symbol_mapping_body 
{
    uint16_t  index;
    uint8_t   session_id;
    char      body_filler;
    char      symbol[16];
};
#pragma pack()
#pragma pack(1)
struct symbol_mapping 
{
    uint16_t  msg_size;
    uint16_t  msg_type;
    uint32_t  msg_seq_num;
    uint32_t  send_time;
    uint8_t   product_id;
    uint8_t   retrans_flag;
    uint8_t   num_body_entries;
    char      filler;
    struct symbol_mapping_body bodies[SYMBOL_MAPPING_BODIES];
};
#pragma pack()
#pragma pack(1)
struct firm_mapping_body 
{
    uint16_t  index;
    char      body_filler[5];
    char      firm[5];
};
#pragma pack()
#pragma pack(1)
struct firm_mapping 
{
    uint16_t  msg_size;
    uint16_t  msg_type;
    uint32_t  msg_seq_num;
    uint32_t  send_time;
    uint8_t   product_id;
    uint8_t   retrans_flag;
    uint8_t   num_body_entries;
    char      filler;
    struct firm_mapping_body bodies[FIRM_MAPPING_BODIES];
};
#pragma pack()
#pragma pack(1)
struct imbalance_body 
{
    uint16_t  symbol_index;
    uint16_t  msg_type;                   //103
    uint32_t  source_seq_num; 
    uint32_t  source_time;
    uint32_t  volume;
    uint32_t  total_imbalance;
    uint32_t  mkt_imbalance;
    uint32_t  price_numerator;
    uint8_t   price_scale;
    char      auction_type;      //'O','H','M','C'
    char      exchange;
    char      security_type;
    uint8_t   session;
    char      body_filler;
    uint16_t  auction_time;
};
#pragma pack()
#pragma pack(1)
struct order_body 
{
    uint16_t  symbol_index;
    uint16_t msg_type;                    //100 or 101
    uint32_t  source_seq_num; 
    uint32_t  source_time;
    uint32_t  order_id;
    uint32_t  volume;
    uint32_t  price_numerator;
    uint8_t   price_scale;
    char      side;
    char      exchange;
    uint8_t   security_type;
    uint16_t  firm_index;
    uint8_t   session_id;
    char      body_filler;
};
#pragma pack()
#pragma pack(1)
struct delete_body 
{
    uint16_t  symbol_index;
    uint16_t  msg_type;                     //102
    uint32_t  source_seq_num; 
    uint32_t  source_time;
    uint32_t  order_id;
    char      side;
    char      exchange;
    uint8_t   security_type;
    uint8_t   session_id;
    uint16_t  firm_index;
    char      body_filler[2];
};
#pragma pack()
#pragma pack(1)
struct imbalance_refresh 
{
    uint16_t  msg_size;
    uint16_t  msg_type;
    uint32_t  msg_seq_num;
    uint32_t  send_time;
    uint8_t   product_id;
    uint8_t   retrans_flag;
    uint8_t   num_body_entries;
    char      filler;
    struct imbalance_body body;
};
#pragma pack()
#pragma pack(1)
struct mixed_orders 
{
    uint16_t msg_size;
    uint16_t msg_type;
    uint32_t  msg_seq_num;
    uint32_t  send_time;
    uint8_t   product_id;
    uint8_t   retrans_flag;
    uint8_t   num_body_entries;
    char      filler;
    struct imbalance_body first_imbalance;
    struct order_body first_add;
    struct order_body first_mod;
    struct order_body second_add;
    struct delete_body first_del;
};
#pragma pack()
int unit_test_main();
int in_order();
int order_recovery_primary();
int order_recovery_secondary();
int gap_fill();
int big_first_gap();
int big_second_gap();
int unit_test_sequence_reset();
int unit_test_missing_msgs();
int unit_test_symbol_clear();
int unit_test_symbol_mapping();
int unit_test_firm_mapping();
int unit_test_imbalance_refresh();
int unit_test_orders();
int unit_test_invalid_msg();
int unit_test_invalid_order_type();
int unit_test_runt_header();
int unit_test_runt_sequence_reset();
int unit_test_runt_missing_msgs();
int unit_test_runt_symbol_clear();
int unit_test_runt_symbol_mapping();
int unit_test_runt_firm_mapping();
int unit_test_runt_imbalance_refresh();
int unit_test_runt_add_order();
int unit_test_runt_mod_order();
int unit_test_runt_imbalance();
int unit_test_runt_del_order();
#endif
