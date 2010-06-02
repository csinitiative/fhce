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
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

/*
 * FH Common includes
 */
#include "fh_sock.h"
#include "fh_net.h"
#include "fh_udp.h"
#include "fh_errors.h"
#include "fh_log.h"
#include "fh_util.h"

/*
 * fh_udp_tstamp
 *
 * Configure the UDP socket to attach a timestamp with every ingress packet.
 */
FH_STATUS fh_udp_tstamp(int s, int on)
{
    if (setsockopt(s, SOL_SOCKET, SO_TIMESTAMP, &on, sizeof(on)) < 0) {
        FH_LOG(NET, ERR, ("NET> setsockopt SO_TIMESTAMP failed on socket #%d: %d",
            s, errno));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_udp_pktinfo
 *
 * This option allows to get interface information via OOB messages on each
 * receive message.
 */
FH_STATUS fh_udp_pktinfo(int s, int on)
{
    if (setsockopt(s, IPPROTO_IP, IP_PKTINFO, (char*)&on, sizeof(on)) < 0) {
        FH_LOG(NET, ERR, ("NET> setsockopt IP_PKTINFO failed on socket #%d: %d",
            s, errno));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_udp_set_maxbufsz
 *
 * Set the Rx/Tx socket buffer size to the maximum value possible.
 */
static void fh_udp_set_maxbufsz(int s)
{
    int send_set = 0x2000000, recv_set = 0x2000000;
    int send_get, recv_get;

    /*
     * Set the size of the send buffer
     */

    fh_sock_settxbuf(s, send_set);
    fh_sock_gettxbuf(s, &send_get);

    while ((send_set > send_get) && (send_set > 0x2000)) {
        send_set >>= 1;
        fh_sock_settxbuf(s, send_set);
        fh_sock_gettxbuf(s, &send_get);
    }

    /*
     * Set the size of the recv buffer
     */

    fh_sock_setrxbuf(s, recv_set);
    fh_sock_getrxbuf(s, &recv_get);

    while ((recv_set > recv_get) && (recv_set > 0x2000)) {
        recv_set >>= 1;
        fh_sock_setrxbuf(s, recv_set);
        fh_sock_getrxbuf(s, &recv_get);
    }

    FH_LOG(NET, VSTATE, ("UDP> Send buffer size: 0x%x", send_get));
    FH_LOG(NET, VSTATE, ("UDP> Recv buffer size: 0x%x", recv_get));
}

/*
 * fh_udp_open
 *
 * Open a UDP socket, bind it to a specific port, and do all necessary
 * configuration.
 */
FH_STATUS fh_udp_open(uint32_t addr, uint16_t port, int flags, int *s)
{
    int sock;

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        FH_LOG(NET, ERR, ("NET> Failed to open UDP socket: %d", errno));
        return FH_ERROR;
    }

    /*
     * For multicast sockets
     */
    if (flags & FH_UDP_FL_MCAST) {
        if (fh_sock_reuse(sock, 1) < 0) {
            goto error;
        }
    }

    if (fh_sock_bind(sock, addr, port) < 0) {
        goto error;
    }

    if (fh_udp_pktinfo(sock, 1) < 0) {
        goto error;
    }

    if (fh_udp_tstamp(sock, 1) < 0) {
        goto error;
    }

    if (flags & FH_UDP_FL_MAX_BUFSZ) {
        fh_udp_set_maxbufsz(sock);
    }

    *s = sock;

    return FH_OK;

error:
    close(sock);
    return FH_ERROR;
}

/*
 * fh_udp_send
 *
 * Send a UDP packet to a given destination.
 */
int fh_udp_send(int s, void *buf, int nbytes, struct sockaddr_in *to)
{
    return sendto(s, (char*)buf, nbytes, 0,
                  (struct sockaddr *)to, sizeof(struct sockaddr_in));
}

/*
 * fh_udp_recv
 *
 * Receive a UDP packet. It provides a microsecond timestamp of when the packet
 * entered the stack, what ingress interface, and from what source IP address.
 */
int fh_udp_recv(int s, void *buf, int buflen, struct sockaddr_in *from,
                 uint32_t *ifindex, uint32_t *ifaddr, uint64_t *ts)
{
    int msglen;
    struct msghdr msg;
    struct iovec  iov[1];
    struct cmsghdr  *cmsg;
    union {
        struct cmsghdr just_for_alignment;
        char   ctrl[1024];
    } control_un;

    *ifindex = 0;
    *ifaddr  = 0;

    msg.msg_control    = control_un.ctrl;
    msg.msg_controllen = sizeof(control_un.ctrl);
    msg.msg_flags      = 0;
    msg.msg_name       = (void *) from;
    msg.msg_namelen    = sizeof(struct sockaddr_in);

    msg.msg_iov    = iov;
    msg.msg_iovlen = 1;

    iov[0].iov_base = buf;
    iov[0].iov_len  = buflen;

    msglen = recvmsg(s, &msg, 0);

    if (unlikely(msglen <= 0)) {
        FH_LOG(NET, ERR, ("NET> recvfrom failed on socket #%d :%d/%d: %d",
            s, msglen, buflen, errno));
        return -1;
    }

    if (unlikely(msg.msg_controllen < sizeof(*cmsg))) {
        FH_LOG(NET, ERR, ("NET> Failed to get control data on socket #%d (%d/%d bytes): %d",
            s, msg.msg_controllen, sizeof(*cmsg), errno));
        return -1;
    }

    if (unlikely(msg.msg_flags & MSG_TRUNC)) {
        FH_LOG(NET, WARN, ("NET> Message TRUNC (%d out of %d bytes) on socket #%d: %d",
            msglen, buflen, errno));
        return msglen;
    }

    for (cmsg = CMSG_FIRSTHDR(&msg); cmsg; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
        if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type  == IP_PKTINFO) {
            struct in_pktinfo *ipi;

            ipi = (struct in_pktinfo *) CMSG_DATA (cmsg);

            *ifindex = ipi->ipi_ifindex;
            *ifaddr  = ipi->ipi_addr.s_addr;

            FH_LOG(NET, INFO, ("NET> Control message IP_PKTINFO: ifindex:%d ifaddr:%s",
                *ifindex, fh_net_ntoa(*ifaddr)));

            continue;
        }

        if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMP) {
            struct timeval *tv;

            tv = (struct timeval *) CMSG_DATA(cmsg);

            *ts = (uint64_t) tv->tv_sec * 1000000 + (uint64_t) tv->tv_usec;

            FH_LOG(NET, INFO, ("NET> Control message SCM_TIMESTAMP: %d.%d",
                tv->tv_sec, tv->tv_usec));
        }
    }

    return msglen;
}


