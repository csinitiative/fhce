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

#ifndef __FH_MSG_H__
#define __FH_MSG_H__

#include "fh_errors.h"

#define FH_MSG_CH_HDR_MAGIC (0xa1b2c3d4)
#define FH_MSG_HDR_MAGIC    (0x1234dcba)

/*
 * Messaging channel header
 */
typedef struct {
    uint32_t   ch_magic;        /* Magic to validate data       */
    uint16_t   ch_pkt_size;     /* Packet size                  */
    uint16_t   ch_num_msgs;     /* Number of messages in packet */
} fh_msg_ch_t;

/*
 * Messaging messag header
 */
typedef struct {
    uint32_t   mh_magic;        /* Magic to validate data       */
    uint32_t   mh_size;         /* Packet size                  */
} fh_msg_mh_t;

/*
 * Messaging session
 */
typedef struct {
    uint32_t   sess_saddr;      /* Source IP address            */
    uint32_t   sess_daddr;      /* Destination IP address       */
    uint16_t   sess_sport;      /* Source port                  */
    uint16_t   sess_dport;      /* Destination port             */
    uint32_t   sess_flags;      /* Configuration flags          */
    char       sess_pkt[1400];  /* Packet to batch messages     */
    int        sess_offset;     /* Current offset in the packet */
    int        sess_fd;         /* Socket FD                    */
    int        sess_init;       /* Initialized or not           */
} fh_msg_sess_t;

/*
 * Messaging session configuration flags
 */
#define FH_MSG_SESS_UDP         (0x00000001)
#define FH_MSG_SESS_TCP         (0x00000002)
#define FH_MSG_SESS_MCAST_RX    (0x00000004)
#define FH_MSG_SESS_MCAST_TX    (0x00000008)
#define FH_MSG_SESS_PACKING     (0x00000010)

/*
 * FH Messaging Layer
 */
FH_STATUS fh_msg_init(fh_msg_sess_t *sess);
FH_STATUS fh_msg_send(fh_msg_sess_t *sess, void *data, int length);
FH_STATUS fh_msg_recv(fh_msg_sess_t *sess, void *data, int length);
FH_STATUS fh_msg_flush(fh_msg_sess_t *sess);

#endif /* __FH_MSG_H__ */
