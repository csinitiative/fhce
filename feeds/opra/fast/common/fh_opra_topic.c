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

/*
 * FH Common includes
 */
#include "fh_log.h"

/*
 * OPRA includes
 */
#include "fh_opra_topic.h"

/*
 * OPRA Topic format API
 */
FH_STATUS fh_opra_topic_fmt(fh_opra_topic_fmt_t *tfmt, fh_opra_opt_key_t *k,
                            char *topic, uint32_t length)
{
    register uint32_t i = 0;
    register uint32_t j = 0;

    for (i = 0; i < tfmt->tfmt_num_stanzas && j < length; i++) {
        register char *ptr = tfmt->tfmt_stanza_fmts[i];
        if (ptr == NULL) {
            FH_LOG(MGMT, ERR, ("Topic format is missing stanza[%d]", i));
            return FH_ERROR;
        }

        FH_LOG(MGMT, DIAG, ("Topic format stanza[%d] = '%s'", i, ptr));

        while (*ptr != '\0' && j < length) {
            if (*ptr != '$') {
                topic[j++] = *ptr++;
            }
            else {
                ptr++;

                if (*ptr == '\0') {
                    FH_LOG(MGMT, ERR, ("Invalid topic format: stanza[%d] = '%s'",
                                       i, tfmt->tfmt_stanza_fmts[i]));
                }

                switch (*ptr++) {
                case FH_OPRA_TOPIC_FMT_SYMBOL:
                {
                    register char *uptr  = k->k_symbol;
                    register int   count = 0;

                    while (*uptr != '\0' && count++ < FH_OPRA_TOPIC_MAX_SYMBOL && j < length) {
                        topic[j++] = *uptr++;
                    }
                }
                break;

                case FH_OPRA_TOPIC_FMT_YEAR:
                {
                    sprintf(&topic[j], "%.2u", k->k_year);
                    j += 2;
                }
                break;

                case FH_OPRA_TOPIC_FMT_MONTH:
                {
                    sprintf(&topic[j], "%.2u", k->k_month);
                    j += 2;
                }
                break;

                case FH_OPRA_TOPIC_FMT_DAY:
                {
                    sprintf(&topic[j], "%.2u", k->k_day);
                    j += 2;
                }
                break;

                case FH_OPRA_TOPIC_FMT_PUTCALL:
                {
                    topic[j++] = k->k_putcall;
                }
                break;

                case FH_OPRA_TOPIC_FMT_DECIMAL:
                {
                    sprintf(&topic[j], "%.5u", k->k_decimal);
                    j += 5;
                }
                break;

                case FH_OPRA_TOPIC_FMT_FRACTION:
                {
                    sprintf(&topic[j], "%.3u", k->k_fraction * 1000);
                    j += 3;
                }
                break;

                case FH_OPRA_TOPIC_FMT_EXCH:
                {
                    topic[j++] = k->k_exchid;
                }
                break;

                default:
                    FH_LOG(MGMT, ERR, ("Invalid topic format variable: $%c - stanza[%d] = '%s'",
                                       *ptr, i, tfmt->tfmt_stanza_fmts[i]));
                    FH_ASSERT(1);
                }
            }
        }

        if (j < length && (i+1) != tfmt->tfmt_num_stanzas) {
            topic[j++] = tfmt->tfmt_stanza_delim;
        }
    }

    if (j == length) {
        topic[j-1] = '\0';

        FH_LOG(MGMT, ERR, ("Topic too long (len:%d): topic: %s", topic));

        return FH_ERROR;
    }

    topic[j] = '\0';

    FH_LOG(MGMT, DIAG, ("Topic formatted successfully: topic: %s (len:%d)", topic, j));

    return FH_OK;
}
