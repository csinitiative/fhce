/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 * 
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "fh_log.h"
#include "fh_util.h"
#include "fh_adm_stats_resp.h"

/*
 * adm_stats_resp_pack
 *
 * Packs a STATS response before sending it to the client.
 */
FH_STATUS adm_stats_resp_pack(void *msg, char *data, int *length)
{
    fh_adm_stats_resp_t *m_stats = (fh_adm_stats_resp_t *) msg;
    fh_adm_stats_resp_t *d_stats = (fh_adm_stats_resp_t *) data;
    register uint32_t i;

    *length = sizeof(fh_adm_stats_resp_t);

    if (data == NULL) {
        return FH_OK;
    }

    strcpy(d_stats->stats_service, m_stats->stats_service);
    d_stats->stats_line_cnt = htonl(m_stats->stats_line_cnt);
    d_stats->stats_state    = htonl(m_stats->stats_state);

    /*
     * For each line stats response, pack 
     */
    for (i=0; i<m_stats->stats_line_cnt; i++) {
        fh_adm_line_stats_t *m_line = &m_stats->stats_lines[i];
        fh_adm_line_stats_t *d_line = &d_stats->stats_lines[i];

        strcpy(d_line->line_name, m_line->line_name);
        d_line->line_pkt_rx          = htonll(m_line->line_pkt_rx);
        d_line->line_pkt_errs        = htonll(m_line->line_pkt_errs);
        d_line->line_pkt_dups        = htonll(m_line->line_pkt_dups);
        d_line->line_pkt_late        = htonll(m_line->line_pkt_late);
        d_line->line_pkt_until_reset = htonll(m_line->line_pkt_until_reset);
        d_line->line_pkt_bad_times   = htonll(m_line->line_pkt_bad_times);
        d_line->line_pkt_seq_jump    = htonll(m_line->line_pkt_seq_jump);
        d_line->line_pkt_wrap_noreset = htonll(m_line->line_pkt_wrap_noreset);
        d_line->line_msg_rx          = htonll(m_line->line_msg_rx);
        d_line->line_msg_loss        = htonll(m_line->line_msg_loss);
        d_line->line_msg_recovered   = htonll(m_line->line_msg_recovered);
        d_line->line_msg_late        = htonll(m_line->line_msg_late);
        d_line->line_bytes           = htonll(m_line->line_bytes);
    }


    return FH_OK;
}

/*
 * adm_stats_resp_unpack
 *
 * Unpacks a STATS response received from the server.
 */
FH_STATUS adm_stats_resp_unpack(void *msg, char *data, int length)
{
    fh_adm_stats_resp_t *m_stats = (fh_adm_stats_resp_t *) msg;
    fh_adm_stats_resp_t *d_stats = (fh_adm_stats_resp_t *) data;
    register uint32_t i;

    FH_ASSERT(length == sizeof(fh_adm_stats_resp_t));

    strcpy(m_stats->stats_service, d_stats->stats_service);
    m_stats->stats_line_cnt = ntohl(d_stats->stats_line_cnt);
    m_stats->stats_state    = ntohl(d_stats->stats_state);

    /*
     * For each line stats response, pack 
     */
    for (i=0; i<m_stats->stats_line_cnt; i++) {
        fh_adm_line_stats_t *m_line = &m_stats->stats_lines[i];
        fh_adm_line_stats_t *d_line = &d_stats->stats_lines[i];

        strcpy(m_line->line_name, d_line->line_name);
        m_line->line_pkt_rx          = ntohll(d_line->line_pkt_rx);
        m_line->line_pkt_errs        = ntohll(d_line->line_pkt_errs);
        m_line->line_pkt_dups        = ntohll(d_line->line_pkt_dups);
        m_line->line_pkt_late        = ntohll(d_line->line_pkt_late);
        m_line->line_pkt_until_reset = ntohll(d_line->line_pkt_until_reset);
        m_line->line_pkt_bad_times   = htonll(d_line->line_pkt_bad_times);
        m_line->line_pkt_seq_jump    = htonll(d_line->line_pkt_seq_jump);
        m_line->line_pkt_wrap_noreset = htonll(d_line->line_pkt_wrap_noreset);
        m_line->line_msg_rx          = ntohll(d_line->line_msg_rx);
        m_line->line_msg_loss        = ntohll(d_line->line_msg_loss);
        m_line->line_msg_recovered   = ntohll(d_line->line_msg_recovered);
        m_line->line_msg_late        = ntohll(d_line->line_msg_late);
        m_line->line_bytes           = ntohll(d_line->line_bytes);
    }

    return FH_OK;
}

