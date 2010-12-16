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

#ifndef __FH_OPRA_OPTION_EXT_H__
#define __FH_OPRA_OPTION_EXT_H__

#include "queue.h"
#include "fh_opra_lo_ext.h"

/**
 * @brief Option hash key structure
 * THIS STRUCTURE IS BYTE-ALIGNED AND KEY LOOKUPS WILL SOMETIMES FAIL IF IT IS NOT KEPT SO
 */
typedef struct {
    char     k_symbol[5];           /**< underlying/root symbol (from the feed) */
    uint8_t  k_year;                /**< expiration year (YY) */
    uint8_t  k_month;               /**< expiration month (MM) */
    uint8_t  k_day;                 /**< expiration day (DD) */
    char     k_putcall;             /**< put/call indication (P or C) */
    char     k_exchid;              /**< exchange ID code */
    uint16_t k_fraction;            /**< fractional portion of the strike price */
    uint32_t k_decimal;             /**< decimal portion of the strike price */
} fh_opra_opt_key_t;

#define FH_OPRA_OPT_KEY_SIZE   (sizeof(fh_opra_opt_key_t)/sizeof(uint32_t))

/*
 * All options are linked together on a given FT line, because a given option
 * only belongs to a single FT line.
 */
struct fh_opra_opt;

typedef TAILQ_ENTRY(fh_opra_opt) fh_opra_opt_le_t;
typedef TAILQ_HEAD(,fh_opra_opt) fh_opra_opt_lh_t;

/*
 * OPRA expiration date for Fast V1 and V2
 */
typedef union {
    char exp_year_v1;
    struct {
        char exp_year[2];
        char exp_date[2];
    } exp_date_v2;
} opra_exp_date_t;

/*
 * Option structure
 */
typedef struct fh_opra_opt {
    fh_opra_opt_le_t   opt_line_le;     /* Line list elment             */
    char               opt_topic[32];   /* Option topic                 */
    fh_opra_opt_key_t  opt_key;         /* Option hash key              */
    fh_opra_lo_t      *opt_lo;          /* Listed option reference      */
    void              *opt_priv;        /* Private context              */
    uint16_t           opt_ftline_idx;  /* FT Line index                */
    uint16_t           opt_init;        /* Initialization of a new opt  */
    uint32_t           opt_uflags;      /* Update flags                 */

    /*
     * RAW data saved for value-added and partial publish
     */
    opra_exp_date_t    opt_exp_date;    /* OPRA expiration date         */
    char               opt_session;     /* Session                      */
    char               opt_bo_partid;   /* Best offer participant ID    */
    char               opt_bb_partid;   /* Best bid participant ID      */
    uint32_t           opt_seq_num;     /* Sequence number              */
    uint32_t           opt_time;        /* Participant time             */
    uint32_t           opt_open_bid;    /* Opening bid price            */
    uint32_t           opt_open_offer;  /* Opening offer price          */
    uint32_t           opt_open_price;  /* Opening price                */
    uint32_t           opt_close_price; /* Closing price                */
    uint32_t           opt_last_price;  /* Last price                   */
    uint32_t           opt_high_price;  /* EOD high                     */
    uint32_t           opt_low_price;   /* EOD low                      */
    uint32_t           opt_daily_high;  /* Daily high                   */
    uint32_t           opt_daily_low;   /* Daily low                    */
    uint32_t           opt_bid_price;   /* Bid value                    */
    uint32_t           opt_offer_price; /* Offer value                  */
    uint64_t           opt_cum_volume;  /* Cumulative volume            */
    uint64_t           opt_cum_value;   /* Cumulative value             */
    uint64_t           opt_halttime;    /* Halt timestamp in usecs      */
    uint64_t           opt_unhalttime;  /* Unhalt timestamp in usecs    */
} fh_opra_opt_t;

/*
 * Fast OPRA V1/V2 access macros
 */
#define opt_year    opt_exp_date.exp_year_v1
#define opt_year_v2 opt_exp_date.exp_date_v2.exp_year
#define opt_date    opt_exp_date.exp_date_v2.exp_date

/*
 * Fast access macros for OPRA key fields
 */
#define opt_root     opt_key.k_root
#define opt_exp      opt_key.k_exp
#define opt_strike   opt_key.k_srike
#define opt_exch     opt_key.k_part_id


/*
 * Partial publish flags. When the option update flags are set, it means that
 * the corresponding option field has been updated, and it is up to the
 * downstream publishing logic to take advantage of these flags if it wants
 * to implement a partial publish or full-image publish.
 */
#define FH_OPRA_MSG_PART_ID         (0x00000001)
#define FH_OPRA_MSG_YEAR            (0x00000002)
#define FH_OPRA_MSG_OPEN            (0x00000004)
#define FH_OPRA_MSG_HIGH            (0x00000008)
#define FH_OPRA_MSG_LOW             (0x00000010)
#define FH_OPRA_MSG_SESSION         (0x00000020)
#define FH_OPRA_MSG_BID             (0x00000040)
#define FH_OPRA_MSG_OFFER           (0x00000080)
#define FH_OPRA_MSG_LAST            (0x00000100)
#define FH_OPRA_MSG_BO_PART_ID      (0x00000200)
#define FH_OPRA_MSG_BB_PART_ID      (0x00000400)

/*
 * New fields that are partially published
 */
#define FH_OPRA_MSG_UNHALTTIME      (0x00001000)
#define FH_OPRA_MSG_HALTTIME        (0x00002000)
#define FH_OPRA_MSG_OPEN_BID        (0x00004000)
#define FH_OPRA_MSG_OPEN_OFFER      (0x00008000)
#define FH_OPRA_MSG_DAILY_HIGH      (0x00010000)
#define FH_OPRA_MSG_DAILY_LOW       (0x00020000)
#define FH_OPRA_MSG_OPENING         (0x00040000)
#define FH_OPRA_MSG_CLOSING         (0x00080000)
#define FH_OPRA_MSG_LINE_NUM        (0x00100000)

/*
 * When the partial-publish mode is set to VALUE_ADDED, then we will publish all
 * the fields below by default, regardless of the fact that they changed from
 * one update to another.
 */
#define FH_OPRA_MSG_PP_VALUE_ADDED  (FH_OPRA_MSG_PART_ID        | \
                                     FH_OPRA_MSG_YEAR           | \
                                     FH_OPRA_MSG_OPEN           | \
                                     FH_OPRA_MSG_LAST           | \
                                     FH_OPRA_MSG_HIGH           | \
                                     FH_OPRA_MSG_LOW            | \
                                     FH_OPRA_MSG_BID            | \
                                     FH_OPRA_MSG_OFFER          | \
                                     FH_OPRA_MSG_SESSION        | \
                                     FH_OPRA_MSG_BO_PART_ID     | \
                                     FH_OPRA_MSG_BB_PART_ID)

#endif /* __FH_OPRA_OPTION_EXT_H__ */
