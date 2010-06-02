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

#ifndef __FH_SOCK_H__
#define __FH_SOCK_H__

#include <stdint.h>
#include "fh_errors.h"

/*
 * Socket API
 */
FH_STATUS fh_sock_getsrc(int s, uint32_t *saddr, uint16_t *sport);
FH_STATUS fh_sock_getdst(int s, uint32_t *daddr, uint16_t *dport);
FH_STATUS fh_sock_setbuf(int s, int opt, int bufsz);
FH_STATUS fh_sock_getbuf(int s, int opt, int *bufsz);
FH_STATUS fh_sock_setrxbuf(int s, int bufsz);
FH_STATUS fh_sock_getrxbuf(int s, int *bufsz);
FH_STATUS fh_sock_settxbuf(int s, int bufsz);
FH_STATUS fh_sock_gettxbuf(int s, int *bufsz);
FH_STATUS fh_sock_error(int s, int *ret);
FH_STATUS fh_sock_reuse(int s, int on);
FH_STATUS fh_sock_block(int s, int on);
FH_STATUS fh_sock_bind(int s, uint32_t addr, uint16_t port);
int32_t   fh_sock_pending(int s);

#endif /* __FH_SOCK_H__ */
