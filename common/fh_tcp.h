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

#ifndef __FH_TCP_H__
#define __FH_TCP_H__

#include <stdint.h>
#include "fh_errors.h"

FH_STATUS fh_tcp_open(uint32_t addr, uint16_t port, int *s);
FH_STATUS fh_tcp_keepalive(int s, int on);
FH_STATUS fh_tcp_nodelay(int s, int on);
FH_STATUS fh_tcp_accept(int s, uint32_t *daddr, uint16_t *dport, int *cs);
FH_STATUS fh_tcp_listen(int s);
FH_STATUS fh_tcp_tconnect(int s, uint32_t daddr, uint16_t dport, uint32_t usec, int quiet);
FH_STATUS fh_tcp_connect(int s, uint32_t daddr, uint16_t dport);

FH_STATUS fh_tcp_client_ex(uint32_t saddr, uint16_t sport, uint32_t daddr,
                           uint16_t dport, int *s, int quiet);
FH_STATUS fh_tcp_client(uint32_t daddr, uint16_t dport, int *s, int quiet);
FH_STATUS fh_tcp_server(uint32_t saddr, uint16_t sport, int *s);

int       fh_tcp_readblk(int s, void *buf, int nbytes);
int       fh_tcp_writeblk(int s, void *buf, int nbytes);
int       fh_tcp_peek(int s, void *buf, int nbytes);
int       fh_tcp_read(int s, void *buf, int nbytes);
int       fh_tcp_write(int s, const void *buf, int nbytes);

#endif /* __FH_TCP_H__ */
