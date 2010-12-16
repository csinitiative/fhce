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

#ifndef __FH_OPRA_ML_H__
#define __FH_OPRA_ML_H__

#include "fh_errors.h"
#include "fh_opra_option.h"

/*
 *  The definition for type of feed creating the messages
 */
typedef enum {

     opra_v2 = 1,                   /* Opra v2 feed                           */
     itch30,                        /* Itch30 feed                            */
     arca_XX,                       /* Place holder for the Arca definitions  */
}feedSource;

/*
 * define the OPRA related control Info
 */
typedef struct{
    char msgCat;                    /* Opra message Category                  */
    char msgType;                   /* Opra message Type                      */
    uint16_t proc_id;               /* Opra Process number                    */
}fh_perfOpraCtrlInfo_t;

/*
 * Define the ARCA related control info
 */
typedef struct{
    uint16_t msgCat;                /* The message category                   */
    uint16_t msgType;               /* The messge type                        */
}fh_perfArcaCtrlInfo_t;

/*
 * define the message format for message to the test client
 *
 */
typedef struct perfMsgHdr{
    int        feedType;            /* The type of the feed this message for  */
    union {
       fh_perfOpraCtrlInfo_t opra;
       fh_perfArcaCtrlInfo_t arca;
    }feedCtrl;
    uint64_t    genTime;            /* Generation Time                        */
    uint32_t  msgSeqNum;            /* Message sequence number                */
}fh_perfMsgHdr_t;

/*
 * OPRA Header Wire format
 */
typedef struct {
    char        mh_msg_cat;             /* Message category     */
    char        mh_msg_type;            /* Message type         */
    char        mh_part_id;             /* Participant ID       */
    char        mh_reserved;            /* Padding for alignt   */
    uint32_t    mh_seq_num;             /* Sequence number      */
    uint32_t    mh_time;                /* Participant Time     */
} fh_opra_ml_hdr_t;

/*
 * OPRA Control Wire message
 */
typedef struct {
    fh_perfMsgHdr_t  ctrl_ph;           /* performance header   */
    fh_opra_ml_hdr_t ctrl_mh;           /* Message header       */
    char             ctrl_buffer[450];  /* Control data         */
} fh_opra_ml_ctrl_t;

/*
 * OPRA Open-Interest Wire message
 */
typedef struct {
    fh_perfMsgHdr_t  oi_ph;             /* performance header   */
    fh_opra_ml_hdr_t oi_mh;             /* Message header       */
    uint32_t         oi_volume;         /* Open interest volume */
    uint32_t         oi_exp_sp;         /* Strike price         */
} fh_opra_ml_oi_t;

/*
 * OPRA Underlying Value Last Sale Wire message
 */
typedef struct {
    fh_perfMsgHdr_t  uv_ph;             /* performance header   */
    fh_opra_ml_hdr_t uv_mh;             /* Message header       */
    uint32_t         uv_last_sale;      /* Last Sale            */
} fh_opra_ml_uv_ls_t;

/*
 * OPRA Underlying Value Best Offer Wire message
 */
typedef struct {
    fh_perfMsgHdr_t  uv_ph;             /* performance header   */
    fh_opra_ml_hdr_t uv_mh;             /* Message header       */
    uint32_t         uv_bid_value;      /* Bid Value            */
    uint32_t         uv_offer_value;    /* Offer Value          */
} fh_opra_ml_uv_bo_t;

/*
 * OPRA Last Sale Wire message
 */
typedef struct {
    fh_perfMsgHdr_t  ls_ph;             /* performance header   */
    fh_opra_ml_hdr_t ls_mh;             /* Message header       */
    char             ls_session;        /* Session Indicator    */
    char             ls_reserved[3];    /* Padding for alignmt  */
    opra_exp_date_t  ls_exp_date;       /* Expiration date      */
    uint32_t         ls_exp_sp;         /* Strike price         */
    uint32_t         ls_prem_price;     /* Premium price        */
    uint32_t         ls_volume;         /* Volume               */
    uint32_t         ls_open_price;     /* Opening price        */
    uint32_t         ls_daily_high;     /* Daily high           */
    uint32_t         ls_daily_low;      /* Daily low            */
    uint64_t         ls_cum_volume;     /* Cumulative volume    */
    uint64_t         ls_cum_value;      /* Cumulative value     */
    uint64_t         ls_unhalttime;     /* Unhalt time in usecs */
} fh_opra_ml_ls_t;

#define ls_year  ls_exp_date.exp_year_v1

/*
 * OPRA End-of-Day Summary Wire message
 */
typedef struct {
    fh_perfMsgHdr_t  eod_ph;            /* performance header   */
    fh_opra_ml_hdr_t eod_mh;            /* Message header       */
    opra_exp_date_t  eod_exp_date;      /* Expiration date      */
    uint32_t         eod_volume;        /* Volume               */
    uint32_t         eod_exp_sp;        /* Strike price         */
    uint32_t         eod_last_price;    /* Last price           */
    uint32_t         eod_open_price;    /* Open price           */
    uint32_t         eod_close_price;   /* Close price          */
    uint32_t         eod_high_price;    /* High price           */
    uint32_t         eod_low_price;     /* Low  price           */
    uint32_t         eod_net_change;    /* Net Change           */
    uint32_t         eod_under_price;   /* Net Change           */
    uint32_t         eod_bid_price;     /* Bid price            */
    uint32_t         eod_offer_price;   /* Offer price          */
} fh_opra_ml_eod_t;

#define eod_year  eod_exp_date.exp_year_v1

/*
 * OPRA Quote Wire message
 */
typedef struct {
    fh_perfMsgHdr_t  quote_ph;          /* performance header   */
    fh_opra_ml_hdr_t quote_mh;          /* Message header       */
    opra_exp_date_t  quote_exp_date;    /* Expiration date      */
    char             quote_session;     /* Session              */
    char             quote_bbo_ind;     /* BBO Indicator        */
    char             quote_reserved;    /* Reserved             */
    uint32_t         quote_exp_sp;      /* Strike price         */
    uint32_t         quote_offer_price; /* Offer price          */
    uint32_t         quote_bid_price;   /* Bid price            */
    uint32_t         quote_offer_size;  /* Offer size           */
    uint32_t         quote_bid_size;    /* Bid size             */
    uint64_t         quote_halttime;    /* Halt time            */
    uint32_t         quote_open_bid;    /* Opening bid          */
    uint32_t         quote_open_offer;  /* Opening offer        */
} fh_opra_ml_quote_t;

#define quote_year  quote_exp_date.exp_year_v1

/*
 * OPRA Quote Best-Offer Wire message
 */
typedef struct {
    fh_perfMsgHdr_t  bo_ph;           /* performance header   */
    fh_opra_ml_hdr_t bo_mh;             /* Message header       */
    opra_exp_date_t  bo_exp_date;       /* Expiration date      */
    char             bo_session;        /* Session              */
    char             bo_bbo_ind;        /* BBO Indicator        */
    char             bo_bb_partid;      /* Best-Bid Participant */
    char             bo_bo_partid;      /* Best-Offer Part ID   */
    char             bo_reserved[3];    /* Padding              */
    uint32_t         bo_bb_price;       /* Best-Bid Price       */
    uint32_t         bo_bo_price;       /* Best-Offer Price     */
    uint32_t         bo_bb_size;        /* Best-Bid Size        */
    uint32_t         bo_bo_size;        /* Best-Offer Size      */
    uint32_t         bo_exp_sp;         /* Strike price         */
    uint32_t         bo_offer_price;    /* Offer price          */
    uint32_t         bo_bid_price;      /* Bid price            */
    uint32_t         bo_offer_size;     /* Offer size           */
    uint32_t         bo_bid_size;       /* Bid size             */
    uint64_t         bo_halttime;       /* Halt time            */
    uint32_t         bo_open_bid;       /* Opening bid          */
    uint32_t         bo_open_offer;     /* Opening offer        */
} fh_opra_ml_quote_bo_t;

#define bo_year  bo_exp_date.exp_year_v1

/*
 * OPRA Quote Best-Bid Wire message
 */
typedef struct {
    fh_perfMsgHdr_t  bb_ph;             /* performance header   */
    fh_opra_ml_hdr_t bb_mh;             /* Message header       */
    opra_exp_date_t  bb_exp_date;       /* Expiration date      */
    char             bb_session;        /* Session              */
    char             bb_bbo_ind;        /* BBO Indicator        */
    char             bb_bb_partid;      /* Best-Bid Participant */
    char             bb_bo_partid;      /* Best-Offer Part ID   */
    char             bb_reserved[3];    /* Padding              */
    uint32_t         bb_bb_price;       /* Best-Bid Price       */
    uint32_t         bb_bo_price;       /* Best-Offer Price     */
    uint32_t         bb_bb_size;        /* Best-Bid Size        */
    uint32_t         bb_bo_size;        /* Best-Offer Size      */
    uint32_t         bb_exp_sp;         /* Strike price         */
    uint32_t         bb_offer_price;    /* Offer price          */
    uint32_t         bb_bid_price;      /* Bid price            */
    uint32_t         bb_offer_size;     /* Offer size           */
    uint32_t         bb_bid_size;       /* Bid size             */
    uint64_t         bb_halttime;       /* Halt time            */
    uint32_t         bb_open_bid;       /* Opening bid          */
    uint32_t         bb_open_offer;     /* Opening offer        */
} fh_opra_ml_quote_bb_t;

#define bb_year  bb_exp_date.exp_year_v1

/*
 * OPRA Quote Best-Bid/Offer Wire message
 */
typedef struct {
    fh_perfMsgHdr_t  bbo_ph;            /* performance header   */
    fh_opra_ml_hdr_t bbo_mh;            /* Message header       */
    opra_exp_date_t  bbo_exp_date;      /* Expiration date      */
    char             bbo_session;       /* Session              */
    char             bbo_bbo_ind;       /* BBO Indicator        */
    char             bbo_bb_partid;     /* Best-Bid Participant */
    char             bbo_bo_partid;     /* Best-Offer Part ID   */
    char             bbo_reserved[3];   /* Padding              */
    uint32_t         bbo_bb_price;      /* Best-Bid Price       */
    uint32_t         bbo_bo_price;      /* Best-Offer Price     */
    uint32_t         bbo_bb_size;       /* Best-Bid Size        */
    uint32_t         bbo_bo_size;       /* Best-Offer Size      */
    uint32_t         bbo_exp_sp;        /* Strike price         */
    uint32_t         bbo_offer_price;   /* Offer price          */
    uint32_t         bbo_bid_price;     /* Bid price            */
    uint32_t         bbo_offer_size;    /* Offer size           */
    uint32_t         bbo_bid_size;      /* Bid size             */
    uint64_t         bbo_halttime;      /* Halt time            */
    uint32_t         bbo_open_bid;      /* Opening bid          */
    uint32_t         bbo_open_offer;    /* Opening offer        */
} fh_opra_ml_quote_bbo_t;

#define bbo_year  bbo_exp_date.exp_year_v1

/*
 * Messaging Layer (ML) API
 */
FH_STATUS fh_opra_ml_init();
FH_STATUS fh_opra_ml_flush();
FH_STATUS fh_opra_ml_send(void *msg, int length);
FH_STATUS fh_opra_ml_opt_add(fh_opra_opt_t *opt);

#endif /* __FH_OPRA_ML_H__ */
