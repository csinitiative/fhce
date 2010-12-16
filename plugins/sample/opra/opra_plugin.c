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

/* system includes */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>

/* common feed handler includes */
#include "fh_log.h"
#include "fh_util.h"
#include "fh_plugin.h"
#include "fh_errors.h"
#include "fh_adm_stats_resp.h"

/* OPRA feed handler headers */
#include "fh_opra_lh.h"
#include "fh_opra_stats.h"
#include "fh_opra_msg_ext.h"
#include "fh_opra_option_ext.h"

#define OPRA_PLUGIN_REGISTER(x, h)                                                      \
do {                                                                                    \
    printf("Loading OPRA hook: %s (%d) hook=%s...", #x, x, #h);                         \
    if (fh_plugin_register((x), (fh_plugin_hook_t)(h)) != FH_OK) {                      \
        fprintf(stderr, "OPRA_PLUGIN: Unable to register %s (%d) hook\n", #x, x);       \
        exit(1);                                                                        \
    }                                                                                   \
    printf("done\n");                                                                   \
} while (0)

/* plugin global data */
char topic_filter[100];


/**
 * @brief Determine whether the given topic should be filtered out
 *
 * @param topic to check
 * @return true if topic should be filtered, false otherwise
 */
static inline int is_filtered(char *topic)
{
    if (strstr(topic, topic_filter) == NULL) return 1;
    return 0;
}

/**
 * @brief Load any required external configuration
 */
void opra_cfg_load(FH_STATUS *rc, fh_opra_cfg_t *config)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_OPRA_CFG_LOAD called: configuration at 0x%x", config));
    *rc = FH_OK;
}

/**
 * @brief OPRA hook to initialize the messaging layer.
 */
void opra_ml_init(FH_STATUS *rc, fh_opra_cfg_t *config)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_OPRA_MSG_INIT called: configuration at 0x%x", config));
    printf("topic filter => ");
    if (scanf("%s", topic_filter) < 1) {
        FH_LOG(CSI, ERR, ("error reading topic filter"));
        exit(1);
    }
    FH_LOG(CSI, WARN, ("filtering on topic '%s'", topic_filter));
    *rc = FH_OK;
}

/**
 * @brief OPRA hook to flush outstanding data from the messaging layer.
 */
void opra_ml_flush(FH_STATUS *rc)
{
    *rc = FH_OK;
}

/**
 * @brief OPRA hook to send a message to the messaging layer.
 */
void opra_ml_send(FH_STATUS *rc, void *msg, uint32_t length)
{
    /* send a message */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_MSG_SEND called: data of length %d at 0x%x", length, msg));
    *rc = FH_OK;
}

/**
 * @brief OPRA hook to add an option to the messaging layer.
 */
void opra_ml_opt_add(FH_STATUS *rc, fh_opra_opt_t *opt)
{
    /* set the messaging layer private option context */
    opt->opt_priv = NULL;
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_OPRA_OPT_ADD called: topic %s", opt->opt_topic));
    *rc = FH_OK;
}

/**
 * @brief Pack an OPRA control message.
 */
void opra_msg_ctrl_pack(FH_STATUS *rc, fh_opra_msg_ctrl_t *om, void **msg, int *len)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_OPRA_CTRL_PACK called: data of length %d at 0x%x", *len, msg));
    FH_LOG(CSI, VSTATE, ("  header => { category => %c, type => %c, seq_no => %u }",
                         om->om_msg->hdr.category, om->om_msg->hdr.type,
                         om->om_msg->hdr.seqNumber));
    FH_LOG(CSI, VSTATE, ("  fh_opra_msg_ctrl_t => { text => '%s' }", om->om_msg->text));
    *rc = FH_OK;
}

/**
 * @brief Pack an OPRA open-interest message.
 */
void opra_msg_oi_pack(FH_STATUS         *rc,
                      fh_opra_msg_oi_t  *om,
                      fh_opra_opt_t     *opt,
                      void             **msg,
                      int               *len)
{
    if (!is_filtered(opt->opt_topic)) {
        FH_LOG(CSI, STATE, ("FH_PLUGIN_OPRA_OI_PACK called: data of length %d at 0x%x", *len, msg));
        FH_LOG(CSI, STATE, ("  header => { category => %c, type => %c, seq_no => %u }",
                            om->om_msg->hdr.category, om->om_msg->hdr.type,
                            om->om_msg->hdr.seqNumber));
        FH_LOG(CSI, STATE, ("  fh_opra_msg_oi_t => { symbol => '%.5s' }", om->om_msg->symbol));
        FH_LOG(CSI, STATE, ("  fh_opra_opt_t => { topic => '%s' }", opt->opt_topic));
    }
    *rc = FH_OK;
}

/**
 * @brief Packs an OPRA underlying-value last sale message.
 */
void opra_msg_uv_ls_pack(FH_STATUS         *rc,
                         fh_opra_msg_uv_t  *om,
                         fh_opra_opt_t     *opt,
                         void             **msg,
                         int               *len)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_OPRA_UV_LS_PACK called: data of length %d at 0x%x", *len, msg));
    FH_LOG(CSI, VSTATE, ("  header => { category => %c, type => %c, seq_no => %u }",
                         om->om_msg->hdr.category, om->om_msg->hdr.type,
                         om->om_msg->hdr.seqNumber));
    FH_LOG(CSI, VSTATE, ("  fh_opra_msg_uv_ls_t => { index_count => %u }",
                         om->om_msg->body.numOfIndices));
    FH_LOG(CSI, VSTATE, ("  fh_opra_opt_t => { topic => '%s' }", opt->opt_topic));
    *rc = FH_OK;
}

/**
 * @brief Packs an OPRA underlying-value bid/offer message.
 */
void opra_msg_uv_bo_pack(FH_STATUS         *rc,
                         fh_opra_msg_uv_t  *om,
                         fh_opra_opt_t     *opt,
                         void             **msg,
                         int               *len)
{
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_OPRA_UV_BO_PACK called: data of length %d at 0x%x", *len, msg));
    FH_LOG(CSI, VSTATE, ("  header => { category => %c, type => %c, seq_no => %u }",
                         om->om_msg->hdr.category, om->om_msg->hdr.type,
                         om->om_msg->hdr.seqNumber));
    FH_LOG(CSI, VSTATE, ("  fh_opra_msg_uv_bo_t => { index_count => %u }",
                         om->om_msg->body.numOfIndices));
    FH_LOG(CSI, VSTATE, ("  fh_opra_opt_t => { topic => '%s' }", opt->opt_topic));
    *rc = FH_OK;
}

/**
 * @brief Packs an OPRA last-sale message.
 */
void opra_msg_ls_pack(FH_STATUS         *rc,
                      fh_opra_msg_ls_t  *om,
                      fh_opra_opt_t     *opt,
                      void             **msg,
                      int               *len)
{
    if (!is_filtered(opt->opt_topic)) {
        FH_LOG(CSI, STATE, ("FH_PLUGIN_OPRA_LS_PACK called: data of length %d at 0x%x", *len, msg));
        FH_LOG(CSI, STATE, ("  header => { category => %c, type => %c, seq_no => %u }",
                            om->om_msg->hdr.category, om->om_msg->hdr.type,
                            om->om_msg->hdr.seqNumber));
        FH_LOG(CSI, STATE, ("  fh_opra_msg_ls_t => { symbol => '%.5s' }", om->om_msg->symbol));
        FH_LOG(CSI, STATE, ("  fh_opra_opt_t => { topic => '%s' }", opt->opt_topic));
    }
    *rc = FH_OK;
}

/**
 * @brief Packs an OPRA end-of-day message.
 */
void opra_msg_eod_pack(FH_STATUS          *rc,
                       fh_opra_msg_eod_t  *om,
                       fh_opra_opt_t      *opt,
                       void              **msg,
                       int                *len)
{
    if (!is_filtered(opt->opt_topic)) {
        FH_LOG(CSI, STATE, ("FH_PLUGIN_OPRA_EOD_PACK called: data of length %d at 0x%x",
                            *len, msg));
        FH_LOG(CSI, STATE, ("  header => { category => %c, type => %c, seq_no => %u }",
                            om->om_msg->hdr.category, om->om_msg->hdr.type,
                            om->om_msg->hdr.seqNumber));
        FH_LOG(CSI, STATE, ("  fh_opra_msg_eod_t => { symbol => '%.5s' }", om->om_msg->symbol));
        FH_LOG(CSI, STATE, ("  fh_opra_opt_t => { topic => '%s' }", opt->opt_topic));
    }
    *rc = FH_OK;
}

/**
 * @brief Packs an OPRA quote message.
 */
void opra_msg_quote_pack(FH_STATUS            *rc,
                         fh_opra_msg_quote_t  *om,
                         fh_opra_opt_t        *opt,
                         void                **msg,
                         int                  *len)
{
    if (!is_filtered(opt->opt_topic)) {
        FH_LOG(CSI, STATE, ("FH_PLUGIN_OPRA_QUOTE_PACK called: data of length %d at 0x%x",
                            *len, msg));
        FH_LOG(CSI, STATE, ("  header => { category => %c, type => %c, seq_no => %u, "
                            "participant => %c }",
                            om->om_msg->hdr.category, om->om_msg->hdr.type,
                            om->om_msg->hdr.seqNumber, om->om_msg->hdr.participantId));
        FH_LOG(CSI, STATE, ("  fh_opra_msg_quote_t => { symbol => '%.5s', strikePricecode = %c, "
                            "explicitStrike = %u, strikePriceDenomCode = %c",
                            om->om_msg->symbol, om->om_msg->strikePriceCode,
                            om->om_msg->explicitStrike, om->om_msg->strikePriceDenomCode));
        FH_LOG(CSI, STATE, ("  fh_opra_opt_t => { topic => '%s' }", opt->opt_topic));
        *rc = FH_OK;
    }
}

/**
 * @brief Packs an OPRA quote best offer message.
 */
void opra_msg_quote_bo_pack(FH_STATUS            *rc,
                            fh_opra_msg_quote_t  *om,
                            fh_opra_opt_t        *opt,
                            void                **msg,
                            int                  *len)
{
    if (!is_filtered(opt->opt_topic)) {
        FH_LOG(CSI, STATE, ("FH_PLUGIN_OPRA_BO_PACK called: data of length %d at 0x%x", *len, msg));
        FH_LOG(CSI, STATE, ("  header => { category => %c, type => %c, seq_no => %u }",
                            om->om_msg->hdr.category, om->om_msg->hdr.type,
                            om->om_msg->hdr.seqNumber));
        FH_LOG(CSI, STATE, ("  fh_opra_msg_bo_t => { symbol => '%.5s' }", om->om_msg->symbol));
        FH_LOG(CSI, STATE, ("  fh_opra_opt_t => { topic => '%s' }", opt->opt_topic));
    }
    *rc = FH_OK;
}

/**
 * @brief Packs an OPRA quote best bid message.
 */
void opra_msg_quote_bb_pack(FH_STATUS            *rc,
                            fh_opra_msg_quote_t  *om,
                            fh_opra_opt_t        *opt,
                            void                **msg,
                            int                  *len)
{
    if (!is_filtered(opt->opt_topic)) {
        FH_LOG(CSI, STATE, ("FH_PLUGIN_OPRA_BB_PACK called: data of length %d at 0x%x", *len, msg));
        FH_LOG(CSI, STATE, ("  header => { category => %c, type => %c, seq_no => %u }",
                            om->om_msg->hdr.category, om->om_msg->hdr.type,
                            om->om_msg->hdr.seqNumber));
        FH_LOG(CSI, STATE, ("  fh_opra_msg_bb_t => { symbol => '%.5s' }", om->om_msg->symbol));
        FH_LOG(CSI, STATE, ("  fh_opra_opt_t => { topic => '%s' }", opt->opt_topic));
    }
    *rc = FH_OK;
}

/**
 * @brief Packs an OPRA quote best bid and offer message.
 */
void opra_msg_quote_bbo_pack(FH_STATUS            *rc,
                             fh_opra_msg_quote_t  *om,
                             fh_opra_opt_t        *opt,
                             void                **msg,
                             int                  *len)
{
    if (!is_filtered(opt->opt_topic)) {
        FH_LOG(CSI, STATE, ("FH_PLUGIN_OPRA_BBO_PACK called: data of length %d at 0x%x",
                            *len, msg));
        FH_LOG(CSI, STATE, ("  header => { category => %c, type => %c, seq_no => %u }",
                            om->om_msg->hdr.category, om->om_msg->hdr.type,
                            om->om_msg->hdr.seqNumber));
        FH_LOG(CSI, STATE, ("  fh_opra_msg_bbo_t => { symbol => '%.5s' }", om->om_msg->symbol));
        FH_LOG(CSI, STATE, ("  fh_opra_opt_t => { topic => '%s' }", opt->opt_topic));
    }
    *rc = FH_OK;
}


/**
 * @brief Packs an OPRA line events plugin.
 */
void opra_ftline_event(FH_STATUS *rc, lh_ftline_t *ftl, uint32_t event)
{
    /* prevent "unused parameter" warnings */
    FH_LOG(CSI, VSTATE, ("FH_PLUGIN_OPRA_FTLINE_EVENT called: event %s on line %x",
                         OPRA_FTLINE_EVENT_NAME(event), ftl->ftl_config->oftl_index));
    *rc = FH_OK;
}

/**
 * @brief This hook will receive all process stats at configurable interval
 *
 * @param rc "return value"
 * @param stats process stats structure for the process that we are running in
 */
void opra_periodic_stats(FH_STATUS *rc, fh_adm_stats_resp_t *stats)
{
	uint32_t i;

	for (i = 0; i < stats->stats_line_cnt; i++) {
	    fh_adm_line_stats_t *line = &stats->stats_lines[i];

	    FH_LOG(LH, XSTATS, ("Periodic stats on line %s", line->line_name));
	    FH_LOG(LH, XSTATS, ("- Packets             : %lld", LLI(line->line_pkt_rx)));
        FH_LOG(LH, XSTATS, ("- Packet errors       : %lld", LLI(line->line_pkt_errs)));
	    FH_LOG(LH, XSTATS, ("- Packet dups         : %lld", LLI(line->line_pkt_dups)));
        FH_LOG(LH, XSTATS, ("- Packet late         : %lld", LLI(line->line_pkt_late)));
        FH_LOG(LH, XSTATS, ("- Packet bad time     : %lld", LLI(line->line_pkt_bad_times)));
        FH_LOG(LH, XSTATS, ("- Packet seq jumps    : %lld", LLI(line->line_pkt_seq_jump)));
        FH_LOG(LH, XSTATS, ("- Packet wrap noreset : %lld", LLI(line->line_pkt_wrap_noreset)));
        FH_LOG(LH, XSTATS, ("- Lost messages       : %lld", LLI(line->line_msg_loss)));
        FH_LOG(LH, XSTATS, ("- Recovered messages  : %lld", LLI(line->line_msg_recovered)));
        FH_LOG(LH, XSTATS, ("- Late messages       : %lld", LLI(line->line_msg_late)));
        FH_LOG(LH, XSTATS, ("- Received messages   : %lld", LLI(line->line_msg_rx)));
        FH_LOG(LH, XSTATS, ("- Bytes               : %lld", LLI(line->line_bytes)));
	}
	*rc = FH_OK;
}

/**
 *  @brief Startup function
 *
 *  The "__attribute__ ((constructor))" syntax will cause this function to be run
 *  whenever the shared object, compiled from this .c file, is loaded.  It is in this
 *  function that all registration of hook functions must be done.
 */
void __attribute__ ((constructor)) opra_plugin_init(void)
{
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_MSG_INIT,         opra_ml_init);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_MSG_FLUSH,             opra_ml_flush);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_MSG_SEND,              opra_ml_send);

    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_CFG_LOAD,         opra_cfg_load);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_OPT_ADD,          opra_ml_opt_add);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_CTRL_PACK,        opra_msg_ctrl_pack);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_OI_PACK,          opra_msg_oi_pack);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_UV_LS_PACK,       opra_msg_uv_ls_pack);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_UV_BO_PACK,       opra_msg_uv_bo_pack);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_LS_PACK,          opra_msg_ls_pack);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_EOD_PACK,         opra_msg_eod_pack);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_QUOTE_PACK,       opra_msg_quote_pack);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_QUOTE_BO_PACK,    opra_msg_quote_bo_pack);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_QUOTE_BB_PACK,    opra_msg_quote_bb_pack);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_QUOTE_BBO_PACK,   opra_msg_quote_bbo_pack);

    OPRA_PLUGIN_REGISTER(FH_PLUGIN_OPRA_FTLINE_EVENT,     opra_ftline_event);
    OPRA_PLUGIN_REGISTER(FH_PLUGIN_PERIODIC_STATS,        opra_periodic_stats);
}
