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
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

/*
 * FH Common includes
 */
#include "fh_log.h"

/*
 * FH OPRA includes
 */
#include "fh_opra_lh.h"
#include "fh_opra_lh_tap.h"

/*
 * Fast API
 */
#include "fast_wrapper.h"
#include "fast_process.h"

/*
 * fh_opra_pkt_process
 *
 * Process a packet from a given line. For Fast V1, we don't have the packet
 * header, so we need to read the first message sequence number, and we only
 * know how many messages were in the packet at the end of the processing.
 */
FH_STATUS fh_opra_pkt_process(Fast *fast, lh_line_t *l, uint8_t *buffer, uint32_t len)
{
    uint32_t num_msgs = 0;
    uint32_t parsed_num_msgs = 1;
    uint32_t msg_sn = 0;
    uint32_t msg_time = 0;
    char     msg_cat;
    char     msg_type;
    uint8_t *seq_num_ptr  = buffer + 2;
    uint8_t *num_msgs_ptr = buffer + 12;
    int      i;

    /*
     * Make sure that we are looking at a V2 OPRA Fast packet
     */
    if (buffer[VERSION_OFFSET] != VERSION_2) {
        static int pkt_count = 0;

        // Only log an error every 100k packets
        if ((pkt_count % 100000) == 0) {
            pkt_count++;

            FH_LOG(LH, ERR, ("FastOpraV2: Invalid OPRA Fast version: %d pkts",
                             pkt_count));
        }

        l->l_stats->lst_pkt_errs++;

        return FH_ERROR;
    }

    /*
     * Compute the sequence number
     */
    for (i=0; i<SEQ_OFFSET; i++) {
        msg_sn *= 10;
        msg_sn += (seq_num_ptr[i] - '0');
    }

    /*
     * Compute the number of messages in packet
     */
    for (i=0; i<NUM_MSG_OFFSET; i++) {
        num_msgs *= 10;
        num_msgs += (num_msgs_ptr[i] - '0');
    }

    /*
     * Get the msg cat and type (Optimization, I could do that only if hte
     * number of messages is equal to 1 -- admin messages are single message
     * packet.)
     */
    if (fast_opra_hdr_info(fast, buffer, len, &msg_cat, &msg_type, NULL, &msg_time) < 0) {
        l->l_stats->lst_pkt_errs++;
        return FH_ERROR;
    }

    /*
     * Check if we are in tap-mode
     */
    if (l->l_tap) {
        l->l_stats->lst_msg_rx += num_msgs;
        fh_opra_lh_tap(l, msg_cat, msg_type, msg_sn, num_msgs, fh_opra_lh_recv_time);
        return FH_OK;
    }

    /*
     * Perform the duplicate detection now.
     */
    if (fh_opra_lh_is_dup(l, msg_cat, msg_type, msg_sn, num_msgs, msg_time)) {
        return FH_ERROR;
    }

    /*
     * Process the packet since it is not a duplicate on this line
     */
    parsed_num_msgs = fast_opra_decode(fast, buffer, len);

    if (parsed_num_msgs != num_msgs) {
        FH_LOG(LH, ERR, ("Line %s parsed messages %d packet header %d",
                         l->l_name, parsed_num_msgs, num_msgs));
    }

    /*
     * Update the line statistics
     */
    l->l_stats->lst_msg_rx += parsed_num_msgs;

    return FH_OK;
}


