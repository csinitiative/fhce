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

#ifndef __FH_OPRA_LH_TAP_H__
#define __FH_OPRA_LH_TAP_H__

#include "fh_opra_lh.h"
#include "fh_htable.h"

typedef struct {
    uint32_t        hdr_magic;
    uint32_t        hdr_version;
    uint32_t        hdr_tap_size;
    uint32_t        hdr_num_msgs;
    uint32_t        hdr_num_pkts;
    uint32_t        hdr_max_pkts;
    uint32_t        hdr_sn_min;
    uint32_t        hdr_sn_max;
} lh_tap_hdr_t;

typedef struct {
    char            tap_filename[MAXPATHLEN];
    int             tap_fd;
    char           *tap_address;
    lh_tap_hdr_t   *tap_hdr;
    uint32_t        tap_num_pkts;
    uint32_t        tap_max_pkts;
    uint32_t        tap_index;
    uint8_t         tap_side;
} lh_tap_t;

typedef struct {
    uint64_t        tap_msg_rxtime;
    uint32_t        tap_msg_sn;
    uint16_t        tap_msg_count;
    uint8_t         tap_msg_type;
    uint8_t         tap_msg_cat;
} lh_tap_msg_t;

typedef struct {
    fh_ht_t        *rpt_htable;
    uint32_t        rpt_dup_sn;
    uint32_t        rpt_missing_sn;
    lh_tap_hdr_t   *rpt_hdr;
    int             rpt_fd;
} lh_tap_rpt_t;

#define TAP_SN(_a, _i)      ((lh_tap_msg_t *)((char *)(_a) + sizeof(lh_tap_hdr_t) + (_i)*sizeof(lh_tap_msg_t)))
#define TAP_FILE_MAGIC      (0xab12cd34)
#define TAP_FILE_VERSION    (1)

void fh_opra_lh_tap_init(uint32_t file_size);

void fh_opra_lh_tap_open(char side, uint32_t index, uint32_t line_cnt);

void fh_opra_lh_tap(lh_line_t *l, uint8_t msg_cat, uint8_t msg_type,
                    uint32_t msg_sn, uint32_t num_msgs, uint64_t rxtime);

void fh_opra_lh_tap_deltas(char *a_filename, char *b_filename);
void fh_opra_lh_tap_rates(char *filename, uint64_t period);

#endif /* __FH_OPRA_LH_TAP_H__ */
