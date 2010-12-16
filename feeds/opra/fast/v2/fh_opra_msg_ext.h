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

#ifndef __FH_OPRA_MSG_EXT_H__
#define __FH_OPRA_MSG_EXT_H__

/*
 * This file redefines the different OPRA messages that we handle in the
 * feed handler. It points at the RAW message coming from the line, and
 * adds a couple of value-added fields that the FH computes.
 */

#include "fast_opra.h"

/*
 * OPRA Control/Administrative message
 */
typedef struct {
    CatHMsg_v2 *om_msg;           /* Pointer to the RAW OPRA message  */
} fh_opra_msg_ctrl_t;

/*
 * OPRA Open Interest message
 */
typedef struct {
    CatdMsg_v2 *om_msg;           /* Pointer to the RAW OPRA message  */
    uint32_t    om_exp_sp;        /* Explicit strike price (ISE fmt)  */
} fh_opra_msg_oi_t;

/*
 * OPRA Underlying Value message
 */
typedef struct {
    CatYMsg_v2 *om_msg;           /* Pointer to the RAW OPRA message  */
    uint32_t    om_index_value;   /* Index value (ISE fmt)            */
    uint32_t    om_bo_bid_value;  /* Bid value (ISE fmt)              */
    uint32_t    om_bo_offer_value;/* Offer value (ISE fmt)            */
} fh_opra_msg_uv_t;

/*
 * OPRA Last Sale message
 */
typedef struct {
    CataMsg_v2 *om_msg;           /* Pointer to the RAW OPRA message  */
    uint32_t    om_exp_sp;        /* Explicit strike price (ISE fmt)  */
    uint32_t    om_prem_price;    /* Premium price (ISE fmt)          */
} fh_opra_msg_ls_t;

/*
 * OPRA End-Of-Day message
 */
typedef struct {
    CatfMsg_v2 *om_msg;           /* Pointer to the RAW OPRA message  */
    uint32_t    om_exp_sp;        /* Explicit strike price (ISE fmt)  */
    uint32_t    om_open_price;    /* Open price (ISE fmt)             */
    uint32_t    om_high_price;    /* High price (ISE fmt)             */
    uint32_t    om_low_price;     /* Low  price (ISE fmt)             */
    uint32_t    om_last_price;    /* Last price (ISE fmt)             */
    uint32_t    om_net_change;    /* Net Change (ISE fmt)             */
    uint32_t    om_under_price;   /* Net Change (ISE fmt)             */
    uint32_t    om_bid_price;     /* Bid price (ISE fmt)              */
    uint32_t    om_offer_price;   /* Offer price (ISE fmt)            */
} fh_opra_msg_eod_t;

/*
 * OPRA option quote message
 */
typedef struct {
    CatkMsg_v2 *om_msg;           /* Pointer to the RAW OPRA message  */
    uint32_t    om_exp_sp;        /* Explicit strike price (ISE fmt)  */
    uint32_t    om_offer_price;   /* Offer price (ISE fmt)            */
    uint32_t    om_bid_price;     /* Bid price (ISE fmt)              */
    uint32_t    om_bo_price;      /* Best Offer price (ISE fmt)       */
    uint32_t    om_bb_price;      /* Best Bid price (ISE fmt)         */
} fh_opra_msg_quote_t;

#endif /* __FH_OPRA_MSG_EXT_H__ */
