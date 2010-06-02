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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>

/*
 * FH Includes
 */
#include "fh_log.h"
#include "fh_udp.h"
#include "fh_tcp.h"
#include "fh_net.h"
#include "fh_mcast.h"

/*
 * Messaging Layer includes
 */
#include "fh_msg.h"

/*
 * fh_msg_init
 *
 * Initialize the messaging layer session
 */
FH_STATUS fh_msg_init(fh_msg_sess_t *sess)
{
    FH_ASSERT(sess);
    sess->sess_init = 1;
    return FH_OK;
}

/*
 * fh_msg_send
 *
 * Send some data on the session. If packing is enabled, try to fill in the
 * packet before sending data.
 */
FH_STATUS fh_msg_send(fh_msg_sess_t *sess, void *data, int length)
{
    FH_ASSERT(sess && sess->sess_init && data && length > 0);
    return FH_OK;
}

/*
 * fh_msg_flush
 *
 * Flush the pending data on the session, no matter how much data is pending.
 */
FH_STATUS fh_msg_flush(fh_msg_sess_t *sess)
{
    FH_ASSERT(sess && sess->sess_init);
    return FH_OK;
}

/*
 * fh_msg_recv
 *
 * Receives a channel packet from the session.
 */
FH_STATUS fh_msg_recv(fh_msg_sess_t *sess, void *data, int length)
{
    FH_ASSERT(sess && sess->sess_init && data && length > 0);
    return FH_OK;
}

