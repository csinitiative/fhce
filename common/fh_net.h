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

#ifndef __FH_NET_H__
#define __FH_NET_H__

#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <net/ethernet.h>

#define IPHDRLEN        sizeof(struct ip)
#define UDPHDRLEN       sizeof(struct udphdr)
#define ETHHDRLEN       sizeof(struct ether_header)

#define UDP_LEN(len)    (UDPHDRLEN + (len))
#define IP_LEN(len)     (IPHDRLEN  + UDP_LEN(len))
#define PKT_LEN(len)    (ETHHDRLEN + IP_LEN(len))

#define UDP_OVERHEAD     PKT_LEN(0)

/*
 * Network utils API
 */
char *   fh_net_ntoa(uint32_t ipaddr);
char *   fh_net_hwtoa(uint8_t *hwaddr);
uint32_t fh_net_ifaddr(int s, char *ifname);

#endif /* __FH_NET_H__ */
