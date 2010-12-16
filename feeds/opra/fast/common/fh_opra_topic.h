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

#ifndef __FH_OPRA_TOPIC_H__
#define __FH_OPRA_TOPIC_H__

#include "fh_opra_option.h"

/*
 * These are the possible variables that can be used by the administrator to
 * build the OPRA topic for each option update message.
 *
 * Example: OPRA.$S.$Y$M$D$C$I$F.$X
 */
#define FH_OPRA_TOPIC_FMT_SYMBOL            ('S')
#define FH_OPRA_TOPIC_FMT_YEAR              ('Y')
#define FH_OPRA_TOPIC_FMT_MONTH             ('M')
#define FH_OPRA_TOPIC_FMT_DAY               ('D')
#define FH_OPRA_TOPIC_FMT_PUTCALL           ('C')
#define FH_OPRA_TOPIC_FMT_DECIMAL           ('I')
#define FH_OPRA_TOPIC_FMT_FRACTION          ('F')
#define FH_OPRA_TOPIC_FMT_EXCH              ('X')

#define FH_OPRA_TOPIC_MAX_STANZAS           (10)
#define FH_OPRA_TOPIC_MAX_SYMBOL            (5)

/*
 * OPRA topic format definition
 */
typedef struct {
    uint32_t    tfmt_num_stanzas;       /* Number of stanzas in the topic   */
    char        tfmt_stanza_delim;      /* Stanza delimiter character       */
    char       *tfmt_stanza_fmts[FH_OPRA_TOPIC_MAX_STANZAS];
} fh_opra_topic_fmt_t;

/*
 * OPRA Topic format API
 */
FH_STATUS fh_opra_topic_fmt(fh_opra_topic_fmt_t *tfmt, fh_opra_opt_key_t *k,
                            char *topic, uint32_t length);

void      fh_opra_topic_test();

#endif /* __FH_OPRA_TOPIC_H__ */
