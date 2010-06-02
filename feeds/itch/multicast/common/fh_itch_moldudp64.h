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

#ifndef __FH_ITCH_MOLDUDP64_H__
#define __FH_ITCH_MOLDUDP64_H__

/* system headers */
#include <stdint.h>

/* FH common headers */
#include "fh_ntoh.h"

/* constants related to MoldUDP64 headers */
#define FH_ITCH_MOLDUDP64_SIZE  (20)

/* convenience typedefs of structure(s) below */
typedef struct fh_itch_moldudp64 fh_itch_moldudp64_t;

/**
 *  @brief Structure which represents a MoldUDP64 header
 *  MoldUDP64 is a protocol, developed by NASDAQ, for transporting an arbitrary number of market
 *  data messages, efficiently, in the same packet.
 */
struct fh_itch_moldudp64 {
    char        session[10];    /**< session ID (in ascii digits) of this trading session */
    uint64_t    seq_no;         /**< sequence number of the first message in the packet */
    uint16_t    msg_count;      /**< count of messages in the packet */
};

/**
 *  @brief Inline function to extract a fh_itch_moldudp64 from a byte buffer
 */
inline void fh_itch_moldudp64_extract(uint8_t *buffer, fh_itch_moldudp64_t *header)
{
    /* copy the session ID as a 10 character byte array */
    memcpy(&header->session, buffer, 10);
    
    /* convert the sequence number and message count to little-endian */
    header->seq_no    = ntoh64(*(uint64_t *)(buffer + 10));
    header->msg_count = ntoh16(*(uint16_t *)(buffer + 18));
}

#endif /* __FH_ITCH_MOLDUDP64_H__ */
