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

/*
 * System includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <math.h>

/*
 * FH Common includes
 */
#include "fh_log.h"
#include "fh_util.h"
#include "fh_time.h"
#include "fh_plugin.h"
#include "fh_mpool.h"
#include "fh_htable.h"
#include "fh_msg.h"

/*
 * FH OPRA includes
 */
#include "fh_opra_ml.h"
#include "fh_opra_lh.h"
#include "fh_opra_msg.h"
#include "fh_opra_option.h"
#include "fh_opra_msg_inline.h"

/*
 *  Latency measurement headers
 */
#include "fh_prof.h"
#include "fh_hist.h"

extern uint64_t fh_opra_lh_recv_time;


#define FH_OPRA_DUP_DETECT  (1)
#define FH_OPRA_MSG_LATENCY (0)



/*
 * Profiling declarations for message Latency measurements
 */
#if FH_OPRA_MSG_LATENCY
FH_PROF_DECL(opra_ctrl_latency, 1000000, 20, 2);
FH_PROF_DECL(opra_oi_latency,   1000000, 20, 2);
FH_PROF_DECL(opra_uv_latency,   1000000, 20, 2);
FH_PROF_DECL(opra_uv_ls_latency,1000000, 20, 2);
FH_PROF_DECL(opra_uv_bo_latency,1000000, 20, 2);
FH_PROF_DECL(opra_ls_latency,   1000000, 20, 2);
FH_PROF_DECL(opra_eod_latency,  1000000, 20, 2);
FH_PROF_DECL(opra_quote_latency,1000000, 20, 2);
#endif
/*
 * Out of order detection
 */
#if FH_OPRA_DUP_DETECT

#define FH_OPRA_DROP_DUPS(_msg, _opt) do {                                      \
    if (_msg->hdr.seqNumber > 0 && _opt->opt_seq_num > _msg->hdr.seqNumber) {   \
        fh_opra_lh_late_opt(fh_opra_lh_line_num, _opt);                         \
        return FH_OK;                                                           \
    }                                                                           \
} while (0)

#else

#define FH_OPRA_DROP_DUPS(_msg, _opt)

#endif

/*
 * @brief Macro to efficiently convert ascii numeric strings to binary
 *
 * @param _str string being converted
 * @param _len maximum number of characters to parse
 * @param _val where to assign the result
 */
#define FH_OPRA_ATOI(_str, _len, _val) do {                                                     \
    int _i, _m;                                                                                 \
    _val = 0;                                                                                   \
    for (_i = _len - 1, _m = 1; _i >= 0; _i--, _m *= 10) {                                      \
        _val += ((_str[_i] - '0') * _m);                                                        \
    }                                                                                           \
} while (0)

/*
 * @brief Macro to efficiently convert an OPRA month character to a binary value
 *
 * @param _ch month character to convert
 * @param where to assign the result
 */
#define FH_OPRA_ATOMONTH(_ch, _val) do {                                                        \
    _val = (_ch > 'L') ? _ch - 'L' : _ch - '@';                                                 \
} while (0)

/*
 * @brief Retrieve an option table entry for a non-option (underlying value messages, etc)
 * with the given key parameters
 *
 * @param _msg pointer to the FAST decoded message structure for this message
 * @param _symbol symbol string associated with this message
 * @param _size maximum size of the symbol string
 * @param _opt pointer where newly created/looked up option will be stored
 */
#define FH_OPRA_GET_NOOPT(_msg, _symbol, _size, _opt) do {                                      \
    _opt = opra_opt_lookup(0, 0, 0, 'C', 0, 0, msg->hdr.participantId, _symbol, _size);         \
} while (0)

/*
 * @brief Retrieve an option table entry with the given key parameters
 *
 * @param _msg pointer to the FAST decoded message structure for this message
 * @param _symbol symbol string associated with this message
 * @param _size maximum size of the symbol string
 * @param _opt pointer where newly created/looked up option will be stored
 */
#define FH_OPRA_GET_OPT(_msg, _symbol, _size, _opt) do {                                        \
    uint8_t  _year, _month, _day;                                                               \
    uint32_t _denom = pow(10, msg->strikePriceDenomCode - '@');                                 \
    /* prevent "divide by 0 errors" when there is bad data sent to the feed handler */          \
    if (_denom == 0) _denom = 1;                                                                \
    uint32_t _dec   = (uint32_t)msg->explicitStrike / _denom;                                   \
    uint32_t _frac  = (uint32_t)msg->explicitStrike % _denom;                                   \
    FH_OPRA_ATOI(msg->year, 2, _year);                                                          \
    FH_OPRA_ATOMONTH(msg->expirationMonth, _month);                                             \
    FH_OPRA_ATOI(msg->expirationDate, 2, _day);                                                 \
    _opt = opra_opt_lookup(_year, _month, _day, ((_msg->expirationMonth >= 'M') ? 'P' : 'C'),   \
                           _dec, _frac, msg->hdr.participantId, _symbol, _size);                \
} while (0)


/* messaging plugins */
static fh_plugin_hook_t msg_ctrl_pack       = NULL;
static fh_plugin_hook_t msg_oi_pack         = NULL;
static fh_plugin_hook_t msg_uv_ls_pack      = NULL;
static fh_plugin_hook_t msg_uv_bo_pack      = NULL;
static fh_plugin_hook_t msg_ls_pack         = NULL;
static fh_plugin_hook_t msg_eod_pack        = NULL;
static fh_plugin_hook_t msg_quote_pack      = NULL;
static fh_plugin_hook_t msg_quote_bo_pack   = NULL;
static fh_plugin_hook_t msg_quote_bb_pack   = NULL;
static fh_plugin_hook_t msg_quote_bbo_pack  = NULL;

static uint32_t pp_flags = 0;



/*
 * fh_opra_msg_init
 *
 * Initialize the OPRA message processing logic.
 */
FH_STATUS fh_opra_msg_init()
{
    FH_STATUS rc;

    /*
     * Initialize the common messaging layer
     */
    rc = fh_opra_ml_init();
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to initialize the OPRA messaging layer"));
        return rc;
    }

    if (opra_cfg.ocfg_partial_publish == OPRA_CFG_PP_VALUE_ADDED) {
        pp_flags = FH_OPRA_MSG_PP_VALUE_ADDED;
    } else {
        pp_flags = 0;
    }

    FH_LOG(MGMT, STATE, ("Partial Publish flags are initialized to: 0x%08x (mode:%d)",
                         pp_flags, opra_cfg.ocfg_partial_publish));

    /*
     * Load the message packing plugins
     */
    msg_ctrl_pack = fh_plugin_get_hook(FH_PLUGIN_OPRA_CTRL_PACK);
    if (msg_ctrl_pack) {
        FH_LOG(MGMT, VSTATE, ("FH Plugin loaded: OPRA control message packing"));
    }

    msg_oi_pack = fh_plugin_get_hook(FH_PLUGIN_OPRA_OI_PACK);
    if (msg_oi_pack) {
        FH_LOG(MGMT, VSTATE, ("FH Plugin loaded: OPRA open-interest message packing"));
    }

    msg_uv_ls_pack = fh_plugin_get_hook(FH_PLUGIN_OPRA_UV_LS_PACK);
    if (msg_uv_ls_pack) {
        FH_LOG(MGMT, VSTATE, ("FH Plugin loaded: OPRA Underlying-Value Last-Sale message packing"));
    }

    msg_uv_bo_pack = fh_plugin_get_hook(FH_PLUGIN_OPRA_UV_BO_PACK);
    if (msg_uv_bo_pack) {
        FH_LOG(MGMT, VSTATE, ("FH Plugin loaded: OPRA Underlying-Value BO message packing"));
    }

    msg_ls_pack = fh_plugin_get_hook(FH_PLUGIN_OPRA_LS_PACK);
    if (msg_ls_pack) {
        FH_LOG(MGMT, STATE, ("FH Plugin loaded: OPRA Last-Sale message packing"));
    }

    msg_eod_pack = fh_plugin_get_hook(FH_PLUGIN_OPRA_EOD_PACK);
    if (msg_eod_pack) {
        FH_LOG(MGMT, VSTATE, ("FH Plugin loaded: OPRA EOD message packing"));
    }

    msg_quote_pack = fh_plugin_get_hook(FH_PLUGIN_OPRA_QUOTE_PACK);
    if (msg_quote_pack) {
        FH_LOG(MGMT, VSTATE, ("FH Plugin loaded: OPRA quote message packing"));
    }

    msg_quote_bo_pack = fh_plugin_get_hook(FH_PLUGIN_OPRA_QUOTE_BO_PACK);
    if (msg_quote_bo_pack) {
        FH_LOG(MGMT, VSTATE, ("FH Plugin loaded: OPRA quote BO message packing"));
    }

    msg_quote_bb_pack = fh_plugin_get_hook(FH_PLUGIN_OPRA_QUOTE_BB_PACK);
    if (msg_quote_bb_pack) {
        FH_LOG(MGMT, VSTATE, ("FH Plugin loaded: OPRA quote BB message packing"));
    }

    msg_quote_bbo_pack = fh_plugin_get_hook(FH_PLUGIN_OPRA_QUOTE_BBO_PACK);
    if (msg_quote_bbo_pack) {
        FH_LOG(MGMT, VSTATE, ("FH Plugin loaded: OPRA quote BBO message packing"));
    }
#if FH_OPRA_MSG_LATENCY
    /* To setup for latency measurements  */
    if (FH_LL_OK(LH, STATS)) {
        FH_PROF_INIT(opra_ctrl_latency);
        FH_PROF_INIT(opra_oi_latency);
        FH_PROF_INIT(opra_uv_ls_latency);
        FH_PROF_INIT(opra_uv_bo_latency);
        FH_PROF_INIT(opra_uv_latency);
        FH_PROF_INIT(opra_ls_latency);
        FH_PROF_INIT(opra_eod_latency);
        FH_PROF_INIT(opra_quote_latency);
    }
#endif
    return FH_OK;
}

/*
 * fh_opra_msg_ctrl_send
 *
 * Send an open-interest message to the messaging layer.
 */
static FH_STATUS fh_opra_msg_ctrl_send(fh_opra_msg_ctrl_t *om)
{
    FH_STATUS         rc     = FH_OK;
    void             *msg    = NULL;
    int               length = 0;
    //fh_perfMsg_t      ctrl;
    fh_opra_ml_ctrl_t   ctrl;

    if (msg_ctrl_pack) {
        msg_ctrl_pack(&rc, om, &msg, &length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    else {
        /*
         * Fill in the control message
         */
        ctrl.ctrl_ph.feedType = opra_v2;
        ctrl.ctrl_ph.feedCtrl.opra.msgCat  = om->om_msg->hdr.category;
        ctrl.ctrl_ph.feedCtrl.opra.msgType = om->om_msg->hdr.type;
        ctrl.ctrl_ph.feedCtrl.opra.proc_id = opra_cfg.ocfg_proc_id;
        ctrl.ctrl_ph.genTime = fh_opra_lh_recv_time;
        ctrl.ctrl_ph.msgSeqNum   = om->om_msg->hdr.seqNumber;
        ctrl.ctrl_mh.mh_msg_cat  = om->om_msg->hdr.category;
        ctrl.ctrl_mh.mh_msg_type = om->om_msg->hdr.type;
        ctrl.ctrl_mh.mh_part_id  = om->om_msg->hdr.participantId;
        ctrl.ctrl_mh.mh_seq_num  = om->om_msg->hdr.seqNumber;
        ctrl.ctrl_mh.mh_time     = om->om_msg->hdr.time;
        strcpy(ctrl.ctrl_buffer, (char *)om->om_msg->text);

        msg    = &ctrl;
        length = sizeof(ctrl);
    }
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_END(opra_ctrl_latency);
    }
#endif
    return fh_opra_ml_send(msg, length);
}

/*
 * fh_opra_msg_ctrl_process
 *
 * Process a open-interest message from the OPRA feed.
 */
FH_STATUS fh_opra_msg_ctrl_process(CatHMsg_v2 *msg)
{
    fh_opra_msg_ctrl_t om;

    /*
     * Store a reference to the RAW OPRA message
     */
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_BEG(opra_ctrl_latency);
    }
#endif
    om.om_msg = msg;

    return fh_opra_msg_ctrl_send(&om);
}

/*
 * fh_opra_msg_oi_send
 *
 * Sends an open-interest message to the messaging layer.
 */
static FH_STATUS fh_opra_msg_oi_send(fh_opra_msg_oi_t *om, fh_opra_opt_t *opt)
{
    FH_STATUS         rc     = FH_OK;
    void             *msg    = NULL;
    int               length = 0;
    fh_opra_ml_oi_t   oi;

    if (msg_oi_pack) {
        msg_oi_pack(&rc, om, opt, &msg, &length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    else {

        // OPRA Perf header
        oi.oi_ph.feedType = opra_v2;
        oi.oi_ph.feedCtrl.opra.msgCat  = om->om_msg->hdr.category;
        oi.oi_ph.feedCtrl.opra.msgType = om->om_msg->hdr.type;
        oi.oi_ph.feedCtrl.opra.proc_id = opra_cfg.ocfg_proc_id;
        oi.oi_ph.genTime               = fh_opra_lh_recv_time;
        oi.oi_ph.msgSeqNum   = om->om_msg->hdr.seqNumber;

        // OPRA Header fields
        oi.oi_mh.mh_msg_cat  = om->om_msg->hdr.category;
        oi.oi_mh.mh_msg_type = om->om_msg->hdr.type;
        oi.oi_mh.mh_part_id  = om->om_msg->hdr.participantId;
        oi.oi_mh.mh_seq_num  = om->om_msg->hdr.seqNumber;
        oi.oi_mh.mh_time     = om->om_msg->hdr.time;

        // RAW fields
        oi.oi_exp_sp         = om->om_msg->explicitStrike;
        oi.oi_volume         = om->om_msg->openIntVolume;

        msg    = &oi;
        length = sizeof(oi);
    }
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_END(opra_oi_latency);
    }
#endif
    return fh_opra_ml_send(msg, length);
}

/*
 * fh_opra_msg_oi_process
 *
 * Process an open-interest message from the OPRA feed.
 */
FH_STATUS fh_opra_msg_oi_process(CatdMsg_v2 *msg)
{
    fh_opra_msg_oi_t om;
    fh_opra_opt_t   *opt = NULL;
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_BEG(opra_oi_latency);
    }
#endif
    /* compute the OPRA key */
    FH_OPRA_GET_OPT(msg, msg->symbol, sizeof(msg->symbol), opt);
    if (opt == NULL) {
        return FH_ERROR;
    }

    FH_OPRA_DROP_DUPS(msg, opt);

    opt->opt_seq_num  = msg->hdr.seqNumber;
    opt->opt_time     = msg->hdr.time;

    opt->opt_uflags = pp_flags;
    if (opt->opt_init) {
        opt->opt_uflags |= FH_OPRA_MSG_LINE_NUM;
        opt->opt_init    = 0;
    }

    if (opt->opt_year_v2[0] == 0) {
        /*
         * New option entry
         */
        opt->opt_uflags  |= FH_OPRA_MSG_YEAR|FH_OPRA_MSG_PART_ID;

        memcpy(opt->opt_year_v2, msg->year, sizeof(opt->opt_year_v2));
        memcpy(opt->opt_date, msg->expirationDate, sizeof(opt->opt_date));
    }

    /*
     * Store a reference to the RAW OPRA message, and compute other value-added
     * fields (prices in ISE format)
     */
    om.om_msg    = msg;
    om.om_exp_sp = iseprice(msg->explicitStrike, msg->strikePriceDenomCode);

    return fh_opra_msg_oi_send(&om, opt);
}

/*
 * fh_opra_msg_uv_ls_send
 *
 * Sends an underlying value last sale message to the messaging layer.
 */
static FH_STATUS fh_opra_msg_uv_ls_send(fh_opra_msg_uv_t *om, fh_opra_opt_t *opt)
{
    FH_STATUS          rc     = FH_OK;
    void              *msg    = NULL;
    int                length = 0;
    fh_opra_ml_uv_ls_t uv;

    if (msg_uv_ls_pack) {
        msg_uv_ls_pack(&rc, om, opt, &msg, &length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    else {
        // OPRA perf header
        uv.uv_ph.feedType = opra_v2;
        uv.uv_ph.feedCtrl.opra.msgCat  = om->om_msg->hdr.category;
        uv.uv_ph.feedCtrl.opra.msgType = om->om_msg->hdr.type;
        uv.uv_ph.feedCtrl.opra.proc_id = opra_cfg.ocfg_proc_id;
        uv.uv_ph.genTime               = fh_opra_lh_recv_time;
        uv.uv_ph.msgSeqNum             = om->om_msg->hdr.seqNumber;

        // OPRA Header fields
        uv.uv_mh.mh_msg_cat  = om->om_msg->hdr.category;
        uv.uv_mh.mh_msg_type = om->om_msg->hdr.type;
        uv.uv_mh.mh_part_id  = om->om_msg->hdr.participantId;
        uv.uv_mh.mh_seq_num  = om->om_msg->hdr.seqNumber;
        uv.uv_mh.mh_time     = om->om_msg->hdr.time;

        // RAW fields
        uv.uv_last_sale      = om->om_index_value;

        msg    = &uv;
        length = sizeof(uv);
    }
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_END(opra_uv_ls_latency);
    }
#endif
    return fh_opra_ml_send(msg, length);
}

/*
 * fh_opra_msg_uv_bo_send
 *
 * Sends an underlying value bid offer message to the messaging layer.
 */
static FH_STATUS fh_opra_msg_uv_bo_send(fh_opra_msg_uv_t *om, fh_opra_opt_t *opt)
{
    FH_STATUS          rc     = FH_OK;
    void              *msg    = NULL;
    int                length = 0;
    fh_opra_ml_uv_bo_t uv;

    if (msg_uv_bo_pack) {
        msg_uv_bo_pack(&rc, om, opt, &msg, &length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    else {
        // OPRA perf header
        uv.uv_ph.feedType = opra_v2;
        uv.uv_ph.feedCtrl.opra.msgCat  = om->om_msg->hdr.category;
        uv.uv_ph.feedCtrl.opra.msgType = om->om_msg->hdr.type;
        uv.uv_ph.feedCtrl.opra.proc_id = opra_cfg.ocfg_proc_id;
        uv.uv_ph.genTime               = fh_opra_lh_recv_time;
        uv.uv_ph.msgSeqNum   = om->om_msg->hdr.seqNumber;

        // OPRA Header fields
        uv.uv_mh.mh_msg_cat  = om->om_msg->hdr.category;
        uv.uv_mh.mh_msg_type = om->om_msg->hdr.type;
        uv.uv_mh.mh_part_id  = om->om_msg->hdr.participantId;
        uv.uv_mh.mh_seq_num  = om->om_msg->hdr.seqNumber;
        uv.uv_mh.mh_time     = om->om_msg->hdr.time;

        // RAW fields
        uv.uv_bid_value      = om->om_bo_bid_value;
        uv.uv_offer_value    = om->om_bo_offer_value;

        msg    = &uv;
        length = sizeof(uv);
    }
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_END(opra_uv_bo_latency);
    }
#endif
    return fh_opra_ml_send(msg, length);
}


/*
 * fh_opra_msg_uv_process
 *
 * Process a underlying value message from the OPRA feed.
 */
FH_STATUS fh_opra_msg_uv_process(CatYMsg_v2 *msg)
{
    fh_opra_msg_uv_t om;
    fh_opra_opt_t   *opt = NULL;
    FH_STATUS        rc = FH_ERROR;;

    /*
     * Store a reference to the RAW OPRA message
     */
    om.om_msg = msg;

    /*
     * Process the OPRA message
     */
    switch(msg->hdr.type) {
    case ' ': //underlying value
    {
        register uint32_t i;
#if FH_OPRA_MSG_LATENCY
        if (FH_LL_OK(LH,STATS)) {
            FH_PROF_BEG(opra_uv_ls_latency);
        }
#endif
        for (i=0; i<msg->body.numOfIndices; i++) {
            /* compute the OPRA key */
            FH_OPRA_GET_NOOPT(msg, msg->body.indexGroup[i].symbol,
                              sizeof(msg->body.indexGroup[i].symbol), opt);
            if (opt == NULL) {
                continue;
            }

            opt->opt_seq_num  = msg->hdr.seqNumber;
            opt->opt_time     = msg->hdr.time;

            opt->opt_uflags = pp_flags;
            if (opt->opt_init) {
                opt->opt_uflags |= FH_OPRA_MSG_LINE_NUM|FH_OPRA_MSG_PART_ID;
                opt->opt_init    = 0;
            }

            /*
             * Compute index value (ISE price format)
             */
            om.om_index_value   = indexvalue(msg->body.indexGroup[i].group.indexValue,
                                             sizeof(msg->body.indexGroup[i].group.indexValue));

            if (om.om_index_value != opt->opt_last_price) {
                opt->opt_uflags |= FH_OPRA_MSG_LAST;
                opt->opt_last_price = om.om_index_value;
            }

            rc = fh_opra_msg_uv_ls_send(&om, opt);
        }
    }
    break;

    case 'I': // bid offer
    {
        register uint32_t i;
#if FH_OPRA_MSG_LATENCY
        if (FH_LL_OK(LH,STATS)) {
            FH_PROF_END(opra_uv_bo_latency);
        }
#endif
        for (i = 0; i < msg->body.numOfIndices; i++) {
            /* compute the OPRA key */
            FH_OPRA_GET_NOOPT(msg, msg->body.indexGroup[i].symbol,
                              sizeof(msg->body.indexGroup[i].symbol), opt);
            if (opt == NULL) {
                continue;
            }

            FH_OPRA_DROP_DUPS(msg, opt);

            opt->opt_seq_num  = msg->hdr.seqNumber;
            opt->opt_time     = msg->hdr.time;

            opt->opt_uflags = pp_flags;
            if (opt->opt_init) {
                opt->opt_uflags |= FH_OPRA_MSG_LINE_NUM|FH_OPRA_MSG_PART_ID;
                opt->opt_init    = 0;
            }

            /*
             * Compute bid/offer value (ISE price format)
             */
            om.om_bo_bid_value =
                indexvalue(msg->body.indexGroup[i].group.bidOffer.bidValueIndex,
                           sizeof(msg->body.indexGroup[i].group.bidOffer.bidValueIndex));

            if (om.om_bo_bid_value != opt->opt_bid_price) {
                opt->opt_uflags |= FH_OPRA_MSG_BID;
                opt->opt_bid_price = om.om_bo_bid_value;
            }

            om.om_bo_offer_value =
                indexvalue(msg->body.indexGroup[i].group.bidOffer.offerValueIndex,
                           sizeof(msg->body.indexGroup[i].group.bidOffer.offerValueIndex));

            if (om.om_bo_offer_value != opt->opt_offer_price) {
                opt->opt_uflags |= FH_OPRA_MSG_OFFER;
                opt->opt_offer_price = om.om_bo_offer_value;
            }

            rc = fh_opra_msg_uv_bo_send(&om, opt);
        }
    }
    break;

    case 'F':
    case 'C':
	break;
    }

    return rc;
}

/*
 * fh_opra_msg_ls_send
 *
 * Sends a last sale message to the messaging layer.
 */
static FH_STATUS fh_opra_msg_ls_send(fh_opra_msg_ls_t *om, fh_opra_opt_t *opt)
{
    FH_STATUS         rc     = FH_OK;
    void             *msg    = NULL;
    int               length = 0;
    fh_opra_ml_ls_t   ls;

    if (msg_ls_pack) {
        msg_ls_pack(&rc, om, opt, &msg, &length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    else {
        // OPRA perf header
        ls.ls_ph.feedType = opra_v2;
        ls.ls_ph.feedCtrl.opra.msgCat  = om->om_msg->hdr.category;
        ls.ls_ph.feedCtrl.opra.msgType = om->om_msg->hdr.type;
        ls.ls_ph.feedCtrl.opra.proc_id = opra_cfg.ocfg_proc_id;
        ls.ls_ph.genTime               = fh_opra_lh_recv_time;
        ls.ls_ph.msgSeqNum   = om->om_msg->hdr.seqNumber;

        // OPRA Header fields
        ls.ls_mh.mh_msg_cat  = om->om_msg->hdr.category;
        ls.ls_mh.mh_msg_type = om->om_msg->hdr.type;
        ls.ls_mh.mh_part_id  = om->om_msg->hdr.participantId;
        ls.ls_mh.mh_seq_num  = om->om_msg->hdr.seqNumber;
        ls.ls_mh.mh_time     = om->om_msg->hdr.time;

        // RAW fields
        ls.ls_exp_sp         = om->om_exp_sp;
        ls.ls_prem_price     = om->om_prem_price;
        ls.ls_volume         = om->om_msg->volume;

        // Value-Added/Partial publish fields
        memcpy(&ls.ls_exp_date, &opt->opt_exp_date, sizeof(opra_exp_date_t));
        ls.ls_session        = opt->opt_session;
        ls.ls_open_price     = opt->opt_open_price;
        ls.ls_daily_high     = opt->opt_daily_high;
        ls.ls_daily_low      = opt->opt_daily_low;
        ls.ls_unhalttime     = opt->opt_unhalttime;
        ls.ls_cum_volume     = opt->opt_cum_volume;
        ls.ls_cum_value      = opt->opt_cum_value;

        msg    = &ls;
        length = sizeof(ls);
    }
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_END(opra_ls_latency);
    }
#endif
    return fh_opra_ml_send(msg, length);
}

/*
 * fh_opra_msg_ls_process
 *
 * Process a last sale message from the OPRA feed.
 */
FH_STATUS fh_opra_msg_ls_process(CataMsg_v2 *msg)
{
    fh_opra_msg_ls_t om;
    fh_opra_opt_t   *opt = NULL;
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_BEG(opra_ls_latency);
    }
#endif
    /* compute the OPRA key */
    FH_OPRA_GET_OPT(msg, msg->symbol, sizeof(msg->symbol), opt);
    if (opt == NULL) {
        return FH_ERROR;
    }

    FH_OPRA_DROP_DUPS(msg, opt);

    opt->opt_seq_num  = msg->hdr.seqNumber;
    opt->opt_time     = msg->hdr.time;

    opt->opt_uflags = pp_flags;
    if (opt->opt_init) {
        opt->opt_uflags |= FH_OPRA_MSG_LINE_NUM;
        opt->opt_init    = 0;
    }

    /*
     * Store a reference to the RAW OPRA message, and compute other value-added
     * fields (prices in ISE format)
     */
    om.om_msg         = msg;
    om.om_exp_sp      = iseprice(msg->explicitStrike, msg->strikePriceDenomCode);
    om.om_prem_price  = iseprice(msg->premium, msg->premiumPriceDenomCode);

    /*
     * Process the OPRA message
     */
    if (opt->opt_year_v2[0] == 0) {
        /*
         * New option entry
         */
        opt->opt_uflags  |= FH_OPRA_MSG_YEAR|FH_OPRA_MSG_PART_ID;

        memcpy(opt->opt_year_v2, msg->year, sizeof(opt->opt_year_v2));
        memcpy(opt->opt_date, msg->expirationDate, sizeof(opt->opt_date));
    }

    if (msg->hdr.type == 'J') {
        uint64_t now;
        fh_time_get(&now);

        opt->opt_unhalttime = now;
        opt->opt_uflags    |= FH_OPRA_MSG_UNHALTTIME;
    }

    if (opt->opt_session != msg->sessionIndicator) {
        opt->opt_session    = msg->sessionIndicator;
        opt->opt_uflags    |= FH_OPRA_MSG_SESSION;
    }

    if (om.om_prem_price > 0 && opt->opt_open_price == 0) {
        opt->opt_open_price = om.om_prem_price;
        opt->opt_uflags    |= FH_OPRA_MSG_OPENING;
    }

    if (opt->opt_daily_low == 0 || om.om_prem_price < opt->opt_daily_low) {
        opt->opt_daily_low  = om.om_prem_price;
        opt->opt_uflags    |= FH_OPRA_MSG_DAILY_LOW;
    }

    if (opt->opt_daily_high < om.om_prem_price) {
        opt->opt_daily_high = om.om_prem_price;
        opt->opt_uflags    |= FH_OPRA_MSG_DAILY_HIGH;
    }

    opt->opt_cum_volume += (uint64_t)msg->volume;
    opt->opt_cum_value  += (uint64_t)msg->volume * (uint64_t)om.om_prem_price;


    return fh_opra_msg_ls_send(&om, opt);
}

/*
 * fh_opra_msg_eod_send
 *
 * Sends a end-of-day summary message to the messaging layer.
 */
static FH_STATUS fh_opra_msg_eod_send(fh_opra_msg_eod_t *om, fh_opra_opt_t *opt)
{
    FH_STATUS         rc     = FH_OK;
    void             *msg    = NULL;
    int               length = 0;
    fh_opra_ml_eod_t  eod;

    if (msg_eod_pack) {
        msg_eod_pack(&rc, om, opt, &msg, &length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    else {
        // OPRA perf header
        eod.eod_ph.feedType = opra_v2;
        eod.eod_ph.feedCtrl.opra.msgCat  = om->om_msg->hdr.category;
        eod.eod_ph.feedCtrl.opra.msgType = om->om_msg->hdr.type;
        eod.eod_ph.feedCtrl.opra.proc_id = opra_cfg.ocfg_proc_id;
        eod.eod_ph.genTime               = fh_opra_lh_recv_time;
        eod.eod_ph.msgSeqNum   = om->om_msg->hdr.seqNumber;

        // OPRA Header fields
        eod.eod_mh.mh_msg_cat  = om->om_msg->hdr.category;
        eod.eod_mh.mh_msg_type = om->om_msg->hdr.type;
        eod.eod_mh.mh_part_id  = om->om_msg->hdr.participantId;
        eod.eod_mh.mh_seq_num  = om->om_msg->hdr.seqNumber;
        eod.eod_mh.mh_time     = om->om_msg->hdr.time;

        // RAW fields
        eod.eod_exp_sp         = om->om_exp_sp;
        eod.eod_volume         = om->om_msg->volume;
        eod.eod_under_price    = om->om_under_price;
        eod.eod_net_change     = om->om_net_change;

        // Value-Added/Partial publish fields
        memcpy(&eod.eod_exp_date, &opt->opt_exp_date, sizeof(opra_exp_date_t));
        eod.eod_bid_price      = opt->opt_bid_price;
        eod.eod_offer_price    = opt->opt_offer_price;
        eod.eod_open_price     = opt->opt_open_price;
        eod.eod_high_price     = opt->opt_high_price;
        eod.eod_low_price      = opt->opt_low_price;
        eod.eod_last_price     = opt->opt_last_price;
        eod.eod_close_price    = opt->opt_close_price;

        msg    = &eod;
        length = sizeof(eod);
    }
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_END(opra_eod_latency);
    }
#endif
    return fh_opra_ml_send(msg, length);
}

/*
 * fh_opra_msg_eod_process
 *
 * Process a end-of-day message from the OPRA feed.
 */
FH_STATUS fh_opra_msg_eod_process(CatfMsg_v2 *msg)
{
    fh_opra_msg_eod_t om;
    fh_opra_opt_t    *opt = NULL;
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_BEG(opra_eod_latency);
    }
#endif
    /* compute the OPRA key */
    FH_OPRA_GET_OPT(msg, msg->symbol, sizeof(msg->symbol), opt);
    if (opt == NULL) {
        return FH_ERROR;
    }

    FH_OPRA_DROP_DUPS(msg, opt);

    opt->opt_seq_num  = msg->hdr.seqNumber;
    opt->opt_time     = msg->hdr.time;

    opt->opt_uflags = pp_flags;
    if (opt->opt_init) {
        opt->opt_uflags |= FH_OPRA_MSG_LINE_NUM;
        opt->opt_init    = 0;
    }

    /*
     * Store a reference to the RAW OPRA message, and compute other value-added
     * fields (prices in ISE format)
     */
    om.om_msg         = msg;
    om.om_exp_sp      = iseprice(msg->explicitStrike, msg->strikePriceDenomCode);
    om.om_open_price  = iseprice(msg->open, msg->premiumPriceDenomCode);
    om.om_high_price  = iseprice(msg->high, msg->premiumPriceDenomCode);
    om.om_low_price   = iseprice(msg->low,  msg->premiumPriceDenomCode);
    om.om_last_price  = iseprice(msg->last, msg->premiumPriceDenomCode);
    om.om_net_change  = iseprice(msg->netChange, msg->premiumPriceDenomCode);
    om.om_under_price = iseprice(msg->underlyingStockPrice, msg->underlyingPriceDenomCode);
    om.om_bid_price   = iseprice(msg->bidQuote, msg->premiumPriceDenomCode);
    om.om_offer_price = iseprice(msg->askQuote, msg->premiumPriceDenomCode);

    /*
     * Process the OPRA message
     */
    if (opt->opt_year_v2[0] == 0) {
        /*
         * New option entry
         */
        opt->opt_uflags  |= FH_OPRA_MSG_YEAR|FH_OPRA_MSG_PART_ID;

        memcpy(opt->opt_year_v2, msg->year, sizeof(opt->opt_year_v2));
        memcpy(opt->opt_date, msg->expirationDate, sizeof(opt->opt_date));
    }

    if (opt->opt_bid_price != om.om_bid_price) {
        opt->opt_uflags        |= FH_OPRA_MSG_BID;
        opt->opt_bid_price      = om.om_bid_price;
    }

    if (opt->opt_offer_price != om.om_offer_price) {
        opt->opt_uflags        |= FH_OPRA_MSG_OFFER;
        opt->opt_offer_price    = om.om_offer_price;
    }

    if (opt->opt_open_price != om.om_open_price) {
        opt->opt_uflags        |= FH_OPRA_MSG_OPEN;
        opt->opt_open_price     = om.om_open_price;
    }

    if (opt->opt_high_price != om.om_high_price) {
        opt->opt_uflags        |= FH_OPRA_MSG_HIGH;
        opt->opt_high_price     = om.om_high_price;
    }

    if (opt->opt_low_price != om.om_low_price) {
        opt->opt_uflags        |= FH_OPRA_MSG_LOW;
        opt->opt_low_price      = om.om_low_price;
    }

    if (opt->opt_last_price != om.om_last_price) {
        opt->opt_uflags        |= FH_OPRA_MSG_LAST;
        opt->opt_last_price     = om.om_last_price;
    }

    if (opt->opt_close_price != om.om_last_price) {
        opt->opt_uflags        |= FH_OPRA_MSG_CLOSING;
        opt->opt_close_price    = om.om_last_price;
    }

    return fh_opra_msg_eod_send(&om, opt);
}

/*
 * fh_opra_msg_quote_send
 *
 * Sends a quote message to the messaging layer.
 */
static FH_STATUS fh_opra_msg_quote_send(fh_opra_msg_quote_t *om, fh_opra_opt_t *opt)
{
    FH_STATUS          rc     = FH_OK;
    void              *msg    = NULL;
    int                length = 0;
    fh_opra_ml_quote_t quote;

    if (msg_quote_pack) {
        msg_quote_pack(&rc, om, opt, &msg, &length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    else {
        // OPRA perf header
        quote.quote_ph.feedType = opra_v2;
        quote.quote_ph.feedCtrl.opra.msgCat  = om->om_msg->hdr.category;
        quote.quote_ph.feedCtrl.opra.msgType = om->om_msg->hdr.type;
        quote.quote_ph.feedCtrl.opra.proc_id = opra_cfg.ocfg_proc_id;
        quote.quote_ph.genTime               = fh_opra_lh_recv_time;
        quote.quote_ph.msgSeqNum   = om->om_msg->hdr.seqNumber;

        // OPRA Header fields
        quote.quote_mh.mh_msg_cat  = om->om_msg->hdr.category;
        quote.quote_mh.mh_msg_type = om->om_msg->hdr.type;
        quote.quote_mh.mh_part_id  = om->om_msg->hdr.participantId;
        quote.quote_mh.mh_seq_num  = om->om_msg->hdr.seqNumber;
        quote.quote_mh.mh_time     = om->om_msg->hdr.time;

        // RAW fields
        quote.quote_bbo_ind        = om->om_msg->bboIndicator;
        quote.quote_exp_sp         = om->om_exp_sp;
        quote.quote_offer_price    = om->om_offer_price;
        quote.quote_bid_price      = om->om_bid_price;
        quote.quote_offer_size     = om->om_msg->askSize;
        quote.quote_bid_size       = om->om_msg->bidSize;

        // Value-Added/Partial publish fields
        memcpy(&quote.quote_exp_date, &opt->opt_exp_date, sizeof(opra_exp_date_t));
        quote.quote_session        = opt->opt_session;
        quote.quote_halttime       = opt->opt_halttime;
        quote.quote_open_bid       = opt->opt_open_bid;
        quote.quote_open_offer     = opt->opt_open_offer;

        msg    = &quote;
        length = sizeof(quote);
    }

    return fh_opra_ml_send(msg, length);
}

/*
 * fh_opra_msg_quote_bo_send
 *
 * Sends a quote best-offer message to the messaging layer.
 */
static FH_STATUS fh_opra_msg_quote_bo_send(fh_opra_msg_quote_t *om, fh_opra_opt_t *opt)
{
    FH_STATUS             rc     = FH_OK;
    void                 *msg    = NULL;
    int                   length = 0;
    fh_opra_ml_quote_bo_t bo;

    if (msg_quote_bo_pack) {
        msg_quote_bo_pack(&rc, om, opt, &msg, &length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    else {
        // OPRA perf header
        bo.bo_ph.feedType = opra_v2;
        bo.bo_ph.feedCtrl.opra.msgCat  = om->om_msg->hdr.category;
        bo.bo_ph.feedCtrl.opra.msgType = om->om_msg->hdr.type;
        bo.bo_ph.genTime               = fh_opra_lh_recv_time;
        bo.bo_ph.msgSeqNum   = om->om_msg->hdr.seqNumber;

        // OPRA Header fields
        bo.bo_mh.mh_msg_cat  = om->om_msg->hdr.category;
        bo.bo_mh.mh_msg_type = om->om_msg->hdr.type;
        bo.bo_mh.mh_part_id  = om->om_msg->hdr.participantId;
        bo.bo_mh.mh_seq_num  = om->om_msg->hdr.seqNumber;
        bo.bo_mh.mh_time     = om->om_msg->hdr.time;

        // RAW Fields
        bo.bo_bbo_ind        = om->om_msg->bboIndicator;
        bo.bo_exp_sp         = om->om_exp_sp;
        bo.bo_offer_price    = om->om_offer_price;
        bo.bo_bid_price      = om->om_bid_price;
        bo.bo_offer_size     = om->om_msg->askSize;
        bo.bo_bid_size       = om->om_msg->bidSize;

        bo.bo_bo_price       = om->om_bo_price;
        bo.bo_bo_size        = om->om_msg->bbo.bestOffer.size;
        bo.bo_bo_partid      = om->om_msg->bbo.bestOffer.partId;
        bo.bo_bb_price       = 0;
        bo.bo_bb_size        = 0;
        bo.bo_bb_partid      = 0;

        // Value-Added/Partial publish fields
        memcpy(&bo.bo_exp_date, &opt->opt_exp_date, sizeof(opra_exp_date_t));
        bo.bo_session        = opt->opt_session;
        bo.bo_halttime       = opt->opt_halttime;
        bo.bo_open_bid       = opt->opt_open_bid;
        bo.bo_open_offer     = opt->opt_open_offer;

        msg    = &bo;
        length = sizeof(bo);
    }

    return fh_opra_ml_send(msg, length);
}

/*
 * fh_opra_msg_quote_bb_send
 *
 * Sends a quote best-offer message to the messaging layer.
 */
static FH_STATUS fh_opra_msg_quote_bb_send(fh_opra_msg_quote_t *om, fh_opra_opt_t *opt)
{
    FH_STATUS             rc     = FH_OK;
    void                 *msg    = NULL;
    int                   length = 0;
    fh_opra_ml_quote_bb_t bb;


    if (msg_quote_bb_pack) {
        msg_quote_bb_pack(&rc, om, opt, &msg, &length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    else {
        // OPRA perf header
        bb.bb_ph.feedType = opra_v2;
        bb.bb_ph.feedCtrl.opra.msgCat  = om->om_msg->hdr.category;
        bb.bb_ph.feedCtrl.opra.msgType = om->om_msg->hdr.type;
        bb.bb_ph.feedCtrl.opra.proc_id = opra_cfg.ocfg_proc_id;
        bb.bb_ph.genTime               = fh_opra_lh_recv_time;
        bb.bb_ph.msgSeqNum   = om->om_msg->hdr.seqNumber;

        // OPRA Header fields
        bb.bb_mh.mh_msg_cat  = om->om_msg->hdr.category;
        bb.bb_mh.mh_msg_type = om->om_msg->hdr.type;
        bb.bb_mh.mh_part_id  = om->om_msg->hdr.participantId;
        bb.bb_mh.mh_seq_num  = om->om_msg->hdr.seqNumber;
        bb.bb_mh.mh_time     = om->om_msg->hdr.time;

        // RAW Fields
        bb.bb_bbo_ind        = om->om_msg->bboIndicator;
        bb.bb_exp_sp         = om->om_exp_sp;
        bb.bb_offer_price    = om->om_offer_price;
        bb.bb_bid_price      = om->om_bid_price;
        bb.bb_offer_size     = om->om_msg->askSize;
        bb.bb_bid_size       = om->om_msg->bidSize;

        bb.bb_bb_price       = om->om_bb_price;
        bb.bb_bb_size        = om->om_msg->bbo.bestBid.size;
        bb.bb_bb_partid      = om->om_msg->bbo.bestBid.partId;
        bb.bb_bo_price       = 0;
        bb.bb_bo_size        = 0;
        bb.bb_bo_partid      = 0;

        // Value-Added/Partial publish fields
        memcpy(&bb.bb_exp_date, &opt->opt_exp_date, sizeof(opra_exp_date_t));
        bb.bb_session        = opt->opt_session;
        bb.bb_halttime       = opt->opt_halttime;
        bb.bb_open_bid       = opt->opt_open_bid;
        bb.bb_open_offer     = opt->opt_open_offer;

        msg    = &bb;
        length = sizeof(bb);
    }

    return fh_opra_ml_send(msg, length);
}

/*
 * fh_opra_msg_quote_bbo_send
 *
 * Sends a quote best-offer message to the messaging layer.
 */
static FH_STATUS fh_opra_msg_quote_bbo_send(fh_opra_msg_quote_t *om, fh_opra_opt_t *opt)
{
    FH_STATUS              rc     = FH_OK;
    void                  *msg    = NULL;
    int                    length = 0;
    fh_opra_ml_quote_bbo_t bbo;

    if (msg_quote_bbo_pack) {
        msg_quote_bbo_pack(&rc, om, opt, &msg, &length);
        if (rc != FH_OK) {
            return rc;
        }
    }
    else {
        // OPRA perf header
        bbo.bbo_ph.feedType = opra_v2;
        bbo.bbo_ph.feedCtrl.opra.msgCat  = om->om_msg->hdr.category;
        bbo.bbo_ph.feedCtrl.opra.msgType = om->om_msg->hdr.type;
        bbo.bbo_ph.feedCtrl.opra.proc_id = opra_cfg.ocfg_proc_id;
        bbo.bbo_ph.genTime               = fh_opra_lh_recv_time;
        bbo.bbo_ph.msgSeqNum   = om->om_msg->hdr.seqNumber;

        // OPRA Header fields
        bbo.bbo_mh.mh_msg_cat  = om->om_msg->hdr.category;
        bbo.bbo_mh.mh_msg_type = om->om_msg->hdr.type;
        bbo.bbo_mh.mh_part_id  = om->om_msg->hdr.participantId;
        bbo.bbo_mh.mh_seq_num  = om->om_msg->hdr.seqNumber;
        bbo.bbo_mh.mh_time     = om->om_msg->hdr.time;

        // RAW Fields
        bbo.bbo_bbo_ind        = om->om_msg->bboIndicator;
        bbo.bbo_exp_sp         = om->om_exp_sp;
        bbo.bbo_offer_price    = om->om_offer_price;
        bbo.bbo_bid_price      = om->om_bid_price;
        bbo.bbo_offer_size     = om->om_msg->askSize;
        bbo.bbo_bid_size       = om->om_msg->bidSize;

        bbo.bbo_bo_price       = om->om_bo_price;
        bbo.bbo_bo_size        = om->om_msg->bbo.bestBidOffer.bestOffer.size;
        bbo.bbo_bo_partid      = om->om_msg->bbo.bestBidOffer.bestOffer.partId;

        bbo.bbo_bb_price       = om->om_bb_price;
        bbo.bbo_bb_size        = om->om_msg->bbo.bestBidOffer.bestBid.size;
        bbo.bbo_bb_partid      = om->om_msg->bbo.bestBidOffer.bestBid.partId;

        // Value-Added/Partial publish fields
        memcpy(&bbo.bbo_exp_date, &opt->opt_exp_date, sizeof(opra_exp_date_t));
        bbo.bbo_session        = opt->opt_session;
        bbo.bbo_halttime       = opt->opt_halttime;
        bbo.bbo_open_bid       = opt->opt_open_bid;
        bbo.bbo_open_offer     = opt->opt_open_offer;

        msg    = &bbo;
        length = sizeof(bbo);
    }

    return fh_opra_ml_send(msg, length);
}

/*
 * fh_opra_msg_quote_process
 *
 * Process a quote message from the OPRA feed.
 */
FH_STATUS fh_opra_msg_quote_process(CatkMsg_v2 *msg)
{
    fh_opra_msg_quote_t om;
    fh_opra_opt_t      *opt = NULL;
    FH_STATUS           rc = FH_ERROR;
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_BEG(opra_quote_latency);
    }
#endif
    /* compute the OPRA key */
    FH_OPRA_GET_OPT(msg, msg->symbol, sizeof(msg->symbol), opt);
    if (opt == NULL) {
        return FH_ERROR;
    }

    FH_OPRA_DROP_DUPS(msg, opt);

    opt->opt_seq_num  = msg->hdr.seqNumber;
    opt->opt_time     = msg->hdr.time;

    opt->opt_uflags = pp_flags;
    if (opt->opt_init) {
        opt->opt_uflags |= FH_OPRA_MSG_LINE_NUM;
        opt->opt_init    = 0;
    }

    /*
     * Store a reference to the RAW OPRA message, and compute other value-added
     * fields (prices in ISE format)
     */
    om.om_msg         = msg;
    om.om_exp_sp      = iseprice(msg->explicitStrike, msg->strikePriceDenomCode);
    om.om_offer_price = iseprice(msg->askQuote, msg->premiumPriceDenomCode);
    om.om_bid_price   = iseprice(msg->bidQuote, msg->premiumPriceDenomCode);

    if (opt->opt_year_v2[0] == 0) {
        /*
         * New option entry
         */
        opt->opt_uflags  |= FH_OPRA_MSG_YEAR|FH_OPRA_MSG_PART_ID;

        memcpy(opt->opt_year_v2, msg->year, sizeof(opt->opt_year_v2));
        memcpy(opt->opt_date, msg->expirationDate, sizeof(opt->opt_date));
    }

    if (msg->hdr.type == 'T') {
        uint64_t now;
        fh_time_get(&now);

        opt->opt_halttime = now;
        opt->opt_uflags  |= FH_OPRA_MSG_HALTTIME;
    }

    if (opt->opt_open_offer == 0 && om.om_offer_price != 0) {
        opt->opt_open_offer = om.om_offer_price;
        opt->opt_uflags    |= FH_OPRA_MSG_OPEN_OFFER;
    }

    if (opt->opt_open_bid == 0 && om.om_bid_price != 0) {
        opt->opt_open_bid   = om.om_bid_price;
        opt->opt_uflags    |= FH_OPRA_MSG_OPEN_BID;
    }

    if (opt->opt_session != msg->sessionIndicator) {
        opt->opt_session    = msg->sessionIndicator;
        opt->opt_uflags    |= FH_OPRA_MSG_SESSION;
    }

    /*
     * Process the OPRA message
     */
    switch (msg->bboIndicator) {
	case 'A': // No Best Bid Change, No Best Offer Change
	case 'B': // No Best Bid Change, Quote Contains Best Offer
	case 'D': // No Best Bid Change, No Best Offer
	case 'E': // Quote Contains Best Bid, No Best Offer Change
	case 'F': // Quote Contains Best Bid, Quote Contains Best Offer
	case 'H': // Quote Contains Best Bid, No Best Offer
	case 'I': // No Best Bid, No Best Offer Change
	case 'J': // No Best Bid, Quote Contains Best Offer
	case 'L': // No Best Bid, No Best Offer
	case ' ': // Ineligible
    {
        rc = fh_opra_msg_quote_send(&om, opt);
    }
    break;

	case 'G': // Quote Contains Best Bid, Best Offer
	case 'C': // No Best Bid Change, Best Offer
	case 'K': // No Best Bid, Best Offer
    {
        Appendage_v2 *bo = &msg->bbo.bestOffer;

        om.om_bo_price = iseprice(bo->price, bo->denominator);

        if (opt->opt_bo_partid != bo->partId) {
            opt->opt_bo_partid  = bo->partId;
            opt->opt_uflags    |= FH_OPRA_MSG_BO_PART_ID;
        }

        rc = fh_opra_msg_quote_bo_send(&om, opt);
    }
    break;

	case 'N': // Best Bid , Quote Contains Best Offer
        case 'M': // Best Bid , No Best Offer Change
	case 'P': // Best Bid , No Best Offer
    {
        Appendage_v2 *bb = &msg->bbo.bestBid;

        om.om_bb_price = iseprice(bb->price, bb->denominator);

        if (opt->opt_bb_partid != bb->partId) {
            opt->opt_bb_partid  = bb->partId;
            opt->opt_uflags    |= FH_OPRA_MSG_BB_PART_ID;
        }

        rc = fh_opra_msg_quote_bb_send(&om, opt);
    }
    break;

    case 'O': // Best Bid , Best Offer
    {
        Appendage_v2 *bo = &msg->bbo.bestBidOffer.bestOffer;
        Appendage_v2 *bb = &msg->bbo.bestBidOffer.bestBid;

        om.om_bo_price = iseprice(bo->price, bo->denominator);

        if (opt->opt_bo_partid != bo->partId) {
            opt->opt_bo_partid  = bo->partId;
            opt->opt_uflags    |= FH_OPRA_MSG_BO_PART_ID;
        }

        om.om_bb_price = iseprice(bb->price, bb->denominator);

        if (opt->opt_bb_partid != bb->partId) {
            opt->opt_bb_partid  = bb->partId;
            opt->opt_uflags    |= FH_OPRA_MSG_BB_PART_ID;
        }

        rc = fh_opra_msg_quote_bbo_send(&om, opt);
    }
    break;
    }
#if FH_OPRA_MSG_LATENCY
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_END(opra_quote_latency);
    }
#endif
    return rc;
}



/*
 *   Message latency measurements
 */
#if FH_OPRA_MSG_LATENCY
void fh_opra_msg_latency()
{
    if (FH_LL_OK(LH,STATS)) {
        FH_PROF_PRINT(opra_ctrl_latency);
        FH_PROF_PRINT(opra_oi_latency);
        FH_PROF_PRINT(opra_uv_latency);
        FH_PROF_PRINT(opra_uv_ls_latency);
        FH_PROF_PRINT(opra_uv_bo_latency);
        FH_PROF_PRINT(opra_ls_latency);
        FH_PROF_PRINT(opra_eod_latency);
        FH_PROF_PRINT(opra_quote_latency);
    }


}
#endif
