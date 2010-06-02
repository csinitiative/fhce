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

/*
 * System includes
 */
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

/*
 * Shared common includes
 */
#include "fh_log.h"
#include "fh_sock.h"
#include "fh_tcp.h"
#include "fh_time.h"

static int should_retry(int err, uint32_t *ts, char * dir);
static int fh_tcp_readex(int s, void *buf, int nbytes, int flags);
static int fh_tcp_writeex(int s, const void *buf, int nbytes, int flags);

#define FH_TCP_TIMEOUT           (2)  /* In secs        */
#define FH_TCP_CLIENT_TIMEOUT  (200)  /* In millisecs   */

/*
 * should_retry
 *
 * Ask if the error is non-fatal or if it is necessary to retry.
 */
static int should_retry(int err, uint32_t *start_ts, char *dir)
{
    uint32_t now_ts = (uint32_t)time(NULL);

    switch (err) {
    case EINTR:
        return 1;

#if EWOULDBLOCK != EAGAIN
    case EWOULDBLOCK:
#endif
    case EAGAIN:
        if (*start_ts == 0) {
            *start_ts = now_ts;
            return 1;
        }
        if ((now_ts - *start_ts) >= FH_TCP_TIMEOUT) {
            FH_LOG(NET, ERR, ("NET> %s: retry timeout after %d secs", dir, FH_TCP_TIMEOUT));
            return 0;
        }
        return 1;

    default:
        break;
    }

    return 0;
}

/*
 * fh_tcp_listen
 *
 * Configure a socket as a listening socket.
 */
FH_STATUS fh_tcp_listen(int s)
{
    if (listen(s, SOMAXCONN) < 0) {
        FH_LOG(NET, ERR, ("NET> listen failed on socket #%d: %s (%d)",
                          s, strerror(errno), errno));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_tcp_accept
 *
 * Performs a TCP accept on a listening socket.
 */
FH_STATUS fh_tcp_accept(int s, uint32_t *daddr, uint16_t *dport, int *cs)
{
    socklen_t peerlen = sizeof(struct sockaddr_in);
    struct sockaddr_in peer;
    int sock;

    sock = accept(s, (struct sockaddr *) &peer, &peerlen);
    if (sock < 0) {
        FH_LOG(NET, ERR, ("NET> accept failed on socket #%d: %s (%d)",
                          s, strerror(errno), errno));
        return FH_ERROR;
    }

    if (fh_tcp_nodelay(sock, 1)) {
        close(sock);
        return FH_ERROR;
    }

    *daddr = peer.sin_addr.s_addr;
    *dport = ntohs(peer.sin_port);
    *cs    = sock;

    return FH_OK;
}

/*
 * fh_tcp_tconnect
 *
 * Timed connect. The connect will timeout after usec microsecs.
 * If usec is set to zero, then timed-connect is equivalent to connect.
 */
FH_STATUS fh_tcp_tconnect(int s, uint32_t daddr, uint16_t dport, uint32_t usec, int quiet)
{
    struct sockaddr_in peer;
    FH_STATUS rc;
    int ret;

    /*
     * For timed-connect, make sure that we are not blocking on connect
     */
    if (usec != 0) {
        rc = fh_sock_block(s, 0);
        if (rc != FH_OK) {
            return rc;
        }
    }
  
    memset(&peer, 0, sizeof(peer));
    peer.sin_addr.s_addr = daddr;
    peer.sin_port        = htons(dport);
    peer.sin_family      = AF_INET;
  
    ret = connect(s, (struct sockaddr *) &peer, sizeof(peer));

    if (usec != 0) {
        if (fh_sock_block(s, 1) < 0) {
            return FH_ERROR;
        }
    }
  
    if (ret != 0) {
        struct timeval *ptv, tv;
        fd_set wrfds;
        int ret;

        if (errno != EINPROGRESS) {
            if (!quiet) {
                FH_LOG(NET, ERR, ("NET> connect failed on socket %d: err=%d", s, errno));
            }
            return FH_ERROR;
        }

        if (usec != 0) {
            tv.tv_sec  = (int) (usec/1000000);
            tv.tv_usec = (int) (usec%1000000);
            ptv = &tv;
        }
        else {
            ptv = NULL;
        }

        FD_ZERO(&wrfds);
        FD_SET(s, &wrfds);

        ret = select((int)(s+1), NULL, &wrfds, NULL, ptv);

        if (ret < 0) {
            if (!quiet) {
                FH_LOG(NET, ERR, ("NET> select failed on socket #%d: %d", s, errno));
            }
            return FH_ERROR;
        }

        if (ret == 0) {
            if (!quiet) {
                FH_LOG(NET, ERR, ("NET> Connect timed out on socket #%d", s));
            }
            return FH_ERROR;
        }

        if (fh_sock_error(s, &ret) != FH_OK) {
            return FH_ERROR;
        }

        if (ret != 0) {
            if (!quiet) {
                FH_LOG(NET, ERR, ("NET> Connect failed on socket #%d: ret=%d", s, ret));
            }
            return FH_ERROR;
        }
    }

    return FH_OK;
}

/*
 * fh_tcp_connect
 */
FH_STATUS fh_tcp_connect(int s, uint32_t daddr, uint16_t dport)
{
    return fh_tcp_tconnect(s, daddr, dport, 0, 0);
}

/*
 * fh_tcp_keepalive
 *
 * Set the TCP socket to send some keepalives to keep socket from being
 * disconnected when connection is idle.
 */
FH_STATUS fh_tcp_keepalive(int s, int on)
{
    if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on)) < 0) {
        FH_LOG(NET, ERR, ("NET> setsockopt SO_KEEPALIVE failed on socket #%d: %d",
                          s, errno));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_tcp_nodelay
 *
 * Configures a socket to not wait before sending data. This reduces overall
 * latency due to TCP buffering.
 */
FH_STATUS fh_tcp_nodelay(int s, int on)
{
    int ret;

    ret = setsockopt(s, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
    if (ret < 0) {
        FH_LOG(NET, ERR, ("NET> setsockopt TCP_NODELAY failed on socket #%d: %d",
                          s, errno));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_tcp_open
 */
FH_STATUS fh_tcp_open(uint32_t addr, uint16_t port, int *s)
{
    int sock;

    sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        FH_LOG(NET, ERR, ("NET> Failed to create TCP socket: %d", errno));
        return FH_ERROR;
    }

    if (fh_sock_block(sock, 0) < 0) {
        goto error;
    }
      
    if (fh_sock_reuse(sock, 1) < 0) {
        goto error;
    }

    if (addr != 0 || port != 0) {
        if (fh_sock_bind(sock, addr, port) < 0) {
            goto error;
        }
    }

    if (fh_tcp_keepalive(sock, 1) != FH_OK) {
        goto error;
    }

    *s = sock;

    return FH_OK;

error:
    close(sock);
    return FH_ERROR;
}

/*
 * fh_tcp_client_ex
 *
 * Creates a TCP Client connection to a given TCP server {daddr, dport}.
 * It is possible to optionally configure the saddr and/or sport to bind
 * to locally.
 */
FH_STATUS fh_tcp_client_ex(uint32_t saddr, uint16_t sport, uint32_t daddr,
                           uint16_t dport, int *s, int quiet)
{
    int sock = -1;
    FH_STATUS rc;

    rc = fh_tcp_open(saddr, sport, &sock);
    if (rc != FH_OK) {
        return rc;
    }
 
    rc = fh_tcp_tconnect(sock, daddr, dport, FH_TCP_CLIENT_TIMEOUT, quiet);
    if (rc != FH_OK) {
        close(sock);
        return rc;
    }

    /*
     * Reset the non-blocking flag if needed since the timed-connect
     * uses the non-blocking flag to never block on connect
     */
    rc = fh_sock_block(sock, 0);
    if (rc != FH_OK) {
        close(sock);
        return rc;
    }
  
    rc = fh_tcp_nodelay(sock, 1);
    if (rc != FH_OK) {
        close(sock);
        return rc;
    }

    *s = sock;

    return FH_OK;
}

/*
 * fh_tcp_client
 *
 * Creates a TCP client connection to the server {daddr,dport}.
 */
FH_STATUS fh_tcp_client(uint32_t daddr, uint16_t dport, int *s, int quiet)
{
    return fh_tcp_client_ex(0, 0, daddr, dport, s, quiet);
}

/*
 * fh_tcp_server
 *
 * Creates a TCP server by opening on TCP listening socket for {saddr,sport}
 */
FH_STATUS fh_tcp_server(uint32_t saddr, uint16_t sport, int *s)
{
    int sock;
    FH_STATUS rc;

    rc = fh_tcp_open(saddr, sport, &sock);
    if (rc != FH_OK) {
        return rc;
    }

    rc = fh_tcp_listen(sock);
    if (rc != FH_OK) {
        return rc;
    }

    *s = sock;

    return 0;
}

/*
 * fh_tcp_readblk
 *
 * Performs the read() system call on the socket, waiting for all the data
 * before returning.
 */
int fh_tcp_readblk(int s, void *buf, int nbytes)
{
    int nleft, flags = MSG_WAITALL;

    nleft = nbytes;
    while (nleft > 0) {
        int nread = recv(s, (char*)buf, nleft, flags);

        if (nread < 0) {
            if (EINTR != errno) {
                FH_LOG(NET, WARN, ("NET> fh_tcp_readblk: exit early from error %d", errno));
                break;
            }

            continue;
        }
        else if (nread == 0) {
            break;
        }

        if (flags != MSG_PEEK) {
            nleft -= nread;
            buf = (char *) buf + nread;
        }
        else if (nleft == nread) {
            nleft = 0;
        }
    }

    FH_LOG(NET, INFO, ("NET> Read %d bytes from socket #%d", nbytes - nleft, s));

    return nbytes - nleft;
}

/*
 * fh_tcp_writeblk
 *
 * Performs the write() system call on the socket, waiting for all the data
 * to be sent before returning.
 */
int fh_tcp_writeblk(int s, void *buf, int nbytes)
{
    int nleft, flags;

    flags = MSG_WAITALL;

    nleft = nbytes;
    while (nleft > 0) {
        int nwritten = send(s, (char*)buf, nleft, flags);

        if (nwritten < 0) {
            if (EINTR != errno) {
                FH_LOG(NET, WARN, ("NET> fh_tcp_writeblk: exit early from error %d", errno));
                break;
            }

            continue;
        }
        else if (nwritten == 0) {
            break;
        }

        nleft -= nwritten;
        buf = (char *) buf + nwritten;
    }

    FH_LOG(NET, INFO, ("NET> fh_tcp_writeblk: written: %d socket # %d left: %d", 
                       nbytes - nleft, s, nleft));

    return nbytes - nleft;
}

/*
 * fh_tcp_readex
 *
 * Performs the read() system call on the socket, waiting for all the data
 * before returning.
 */
static int fh_tcp_readex(int s, void *buf, int nbytes, int flags)
{
    int nleft, nread;
    uint32_t start_ts = 0;
    uint64_t beg_ts   = 0;
    uint64_t end_ts   = 0;
    uint32_t retry    = 0;

    if (FH_LL_OK(NET, STATS)) {
        fh_time_get(&beg_ts);
    }

    nleft = nbytes;
    while (nleft > 0) {
        nread = recv(s, (char*)buf, nleft, flags | MSG_DONTWAIT);

        if (nread < 0) {
            if ((errno == ECONNRESET) || 
                (errno == EPIPE)) {
                FH_LOG(NET, WARN, ("NET> fh_tcp_readex: ret 0 due to error %d", errno));
                return 0;
            }

            if (should_retry(errno, &start_ts, "Rx")) {
                if (beg_ts == 0) {
                    fh_time_get(&beg_ts);
                }

                usleep(1000);
                retry++;
                continue;
            }
            else {
                FH_LOG(NET, ERR, ("NET> fh_tcp_readex failed: %d", errno));
                break;
            }
        }
        else if (nread == 0) {
            break;
        }

        if (flags != MSG_PEEK) {
            nleft -= nread;
            buf = (char *) buf + nread;
        }
        else if (nleft == nread) {
            nleft = 0;
        }
    }

    if (beg_ts) {
        fh_time_get(&end_ts);

        if ((end_ts - beg_ts) > 1000000) {
            FH_LOG_PGEN(WARN, ("NET> Delayed TCP Read: Sock:%d, Elapsed:%lld us, Retry:%d, Pending:%d bytes",
                               s, (long long) (end_ts-beg_ts), retry, fh_sock_pending(s)));
        }
    }

    FH_LOG(NET, INFO, ("NET> Read %d bytes from socket #%d", nbytes - nleft, s));

    return nbytes - nleft;
}

/*
 * fh_tcp_read
 *
 * Performs the read() system call on the socket, waiting for all the data
 * before returning.
 */
int fh_tcp_read(int s, void *buf, int nbytes)
{
    return fh_tcp_readex(s, buf, nbytes, 0);
}

/*
 * fh_tcp_peek
 */
int fh_tcp_peek(int s, void *buf, int nbytes)
{
    return fh_tcp_readex(s, buf, nbytes, MSG_PEEK);
}

/*
 * fh_tcp_writeex
 *
 * Perform the write() system call on the socket, waiting for all the data
 * to be sent before returning.
 */
static int fh_tcp_writeex(int s, const void *buf, int nbytes, int flags)
{
    int nleft, nwritten;
    uint32_t start_ts = 0;
    uint64_t beg_ts   = 0;
    uint64_t end_ts   = 0;
    uint32_t retry    = 0;
   
    if (FH_LL_OK(NET, STATS)) {
        fh_time_get(&beg_ts);
    }

    nleft = nbytes;
    while (nleft > 0) {
        nwritten = send(s, (char*)buf, nleft, flags | MSG_DONTWAIT);

        if (nwritten < 0) {
            if ((errno == ECONNRESET) || 
                (errno == EPIPE)) {
                FH_LOG(NET, WARN, ("NET> fh_tcp_writeex: ret 0 due to error %d", errno));
                return 0;
            }

            if (should_retry(errno, &start_ts, "Tx")) {
                if (beg_ts == 0) {
                    fh_time_get(&beg_ts);
                }

                usleep(1000);
                retry++;
                continue;
            }
            else {
                FH_LOG(NET, ERR, ("NET> fh_tcp_readex failed: %d", errno));
                break;
            }
        }
        else if (nwritten == 0) {
            break;
        }

        nleft -= nwritten;
        buf = (char *) buf + nwritten;
    }

    if (beg_ts) {
        fh_time_get(&end_ts);

        if ((end_ts - beg_ts) > 1000000) {
            FH_LOG_PGEN(WARN, ("NET> Delayed TCP Write: Sock:%d, Elapsed:%lld us, Retry:%d",
                               s, (long long) (end_ts-beg_ts), retry));
        }
    }

    FH_LOG(NET, INFO, ("NET> fh_tcp_writeex: written: %d socket # %d left: %d", 
                       nbytes - nleft, s, nleft));

    return nbytes - nleft;
}

/*
 * fh_tcp_write
 *
 * Perform the write() system call on the socket, waiting for all the data
 * to be sent before returning.
 */
int fh_tcp_write(int s, const void *buf, int nbytes)
{
    return fh_tcp_writeex(s, buf, nbytes, 0);
}

