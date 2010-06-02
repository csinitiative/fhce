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

#ifndef __FH_UDP_H__
#define __FH_UDP_H__

#include <stdint.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include "fh_errors.h"

/*
 * UDP configuration flags
 */
#define FH_UDP_FL_MCAST       (0x00000001)  /* Multicast socket             */
#define FH_UDP_FL_MAX_BUFSZ   (0x00000002)  /* Max out socket buffer size   */

/*
 * UDP socket API
 */
FH_STATUS fh_udp_tstamp(int s, int on);
FH_STATUS fh_udp_pktinfo(int s, int on);
FH_STATUS fh_udp_open(uint32_t addr, uint16_t port, int flags, int *s);
int       fh_udp_send(int s, void *buf, int nbytes, struct sockaddr_in *to);
int       fh_udp_recv(int s, void *buf, int buflen, struct sockaddr_in *from,
                      uint32_t *ifindex, uint32_t *ifaddr, uint64_t *ts);

#endif /* __FH_UDP_H__ */
