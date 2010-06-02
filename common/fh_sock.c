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

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include "fh_log.h"
#include "fh_sock.h"
#include "fh_net.h"

/*
 * fh_sock_setbuf
 *
 * Sets the socket buffer size to the 'bufsz' value.
 */
FH_STATUS fh_sock_setbuf(int s, int opt, int bufsz)
{
    if (bufsz == 0) {
        return FH_OK;
    }

    if (setsockopt(s, SOL_SOCKET, opt, (char *)&bufsz, sizeof(bufsz)) < 0) {
        FH_LOG(NET, WARN, ("NET> setsockopt %s failed on socket #%d: %d",
            opt == SO_RCVBUF ? "SO_RCVBUF" : "SO_SNDBUF", s, errno));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_sock_getbuf
 *
 * Get the current socket buffer size.
 */
FH_STATUS fh_sock_getbuf(int s, int opt, int *bufsz)
{
    int len = sizeof(int);

    if (getsockopt(s, SOL_SOCKET, opt,(char *)bufsz,(socklen_t*)&len) < 0) {
        FH_LOG(NET, ERR, ("NET> setsockopt %s failed on socket #%d: %d",
            opt == SO_RCVBUF ? "SO_RCVBUF" : "SO_SNDBUF", s, errno));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_sock_setrxbuf
 *
 * Set the current socket Rx buffer size.
 */
FH_STATUS fh_sock_setrxbuf(int s, int bufsz)
{
    return fh_sock_setbuf(s, SO_RCVBUF, bufsz);
}

/*
 * fh_sock_getrxbuf
 *
 * Get the current socket Rx buffer size.
 */
FH_STATUS fh_sock_getrxbuf(int s, int *bufsz)
{
    return fh_sock_getbuf(s, SO_RCVBUF, bufsz);
}

/*
 * fh_sock_settxbuf
 *
 * Set the current socket Tx buffer size.
 */
FH_STATUS fh_sock_settxbuf(int s, int bufsz)
{
    return fh_sock_setbuf(s, SO_SNDBUF, bufsz);
}

/*
 * fh_sock_gettxbuf
 *
 * Get the current socket Tx buffer size.
 */
FH_STATUS fh_sock_gettxbuf(int s, int *bufsz)
{
    return fh_sock_getbuf(s, SO_SNDBUF, bufsz);
}

/*
 * fh_sock_error
 *
 * Retrieve the socket error.
 */
FH_STATUS fh_sock_error(int s, int *ret)
{
    int err;
    int len = sizeof(err);

    if (getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err, (socklen_t*)&len) < 0) {
        FH_LOG(NET, ERR, ("NET> setsockopt SO_ERROR failed on socket #%d: %d", s, errno));
        return FH_ERROR;
    }

    *ret = err;

    return FH_OK;
}

/*
 * fh_sock_reuse
 *
 * Mark the socket reusable, so, for multicast multiple applications can bind
 * to the same UDP port. And, for TCP, the socket can be reused after being
 * closed but still in TIME_WAIT state.
 */
FH_STATUS fh_sock_reuse(int s, int on)
{
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&on, sizeof(on)) < 0) {
        FH_LOG(NET, ERR, ("NET> setsockopt SO_REUSEADDR(%s) failed on socket #%d: %d",
            on ? "on" : "off", s, errno));
        return FH_ERROR;
    }

    return FH_OK;
} 

/*
 * fh_sock_bind
 *
 * Bind the socket to a particular address and port, either UDP or TCP.
 */
FH_STATUS fh_sock_bind(int s, uint32_t addr, uint16_t port)
{
    struct sockaddr_in sin;

    memset(&sin, 0, sizeof(struct sockaddr_in));

    sin.sin_addr.s_addr = addr;
    sin.sin_port        = htons(port);
    sin.sin_family      = AF_INET;

    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        FH_LOG(NET, ERR, ("NET> bind failed on socket #%d (%s:%d): %d",
            s, fh_net_ntoa(addr), port, errno));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_sock_block
 *
 * Mark the socket blocking or non-blocking.
 */
FH_STATUS fh_sock_block(int s, int on)
{
    int flags;

    flags = fcntl(s, F_GETFL, 0);
    if (flags < 0) {
        FH_LOG(NET, ERR, ("NET> fcntl F_GETFL failed on socket #%d: %d", s, errno));
        return FH_ERROR;
    }

    if (on) {
        flags &= ~O_NONBLOCK;
    }
    else {
        flags |= O_NONBLOCK;
    }

    if (fcntl(s, F_SETFL, flags) < 0) {
        FH_LOG(NET, ERR, ("NET> fcntl F_SETFL (0x%x) failed on socket #%d: %d",
            flags, s, errno));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_sock_pending
 *
 * Returns the numbers of bytes pending on the socket for TCP, and whether
 * there is a pending packet for UDP, or -1 on error.
 */
int32_t fh_sock_pending(int s)
{
    long pending = -1;

    if (ioctl(s, FIONREAD, &pending) < 0) {
        FH_LOG(NET, DIAG, ("NET> ioctl FIONREAD failed on socket #%d: %d",
            s, errno));
        return -1;
    }

    return pending;
}

/*
 * fh_sock_getdst
 */
FH_STATUS fh_sock_getdst(int s, uint32_t *daddr, uint16_t *dport)
{
    socklen_t sinlen = sizeof(struct sockaddr_in);
    struct sockaddr_in sin;

    if (getpeername(s, (struct sockaddr *)&sin, &sinlen) < 0) {
        FH_LOG(NET, ERR, ("NET> getpeername failed on socket #%d: %d", s, errno));
        return FH_ERROR;
    }

    *daddr = sin.sin_addr.s_addr;
    *dport = ntohs(sin.sin_port);

    return FH_OK;
}

/*
 * fh_sock_getsrc
 */
FH_STATUS fh_sock_getsrc(int s, uint32_t *saddr, uint16_t *sport)
{
    socklen_t sinlen = sizeof(struct sockaddr_in);
    struct sockaddr_in sin;

    if (getsockname(s, (struct sockaddr *)&sin, &sinlen) < 0) {
        FH_LOG(NET, ERR, ("NET> getsockname failed on socket #%d: %d", s, errno));
        return FH_ERROR;
    }

    *saddr = sin.sin_addr.s_addr;
    *sport = ntohs(sin.sin_port);

    return FH_OK;
}
