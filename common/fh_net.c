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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/if_arp.h>
#include "fh_net.h"
#include "fh_log.h"

#define NB_BUF	10					/* # of buffers used */
#define UC(b)	(((int)b)&0xff)		/* Extract char from 'b' */

/*
 * fh_net_ntoa
 *
 * Return an ascii-version of an IP address, but use multiple static buffers,
 * so, you can print multiple IP addresses in the same log entry.
 */
char *fh_net_ntoa(uint32_t ipaddr)
{
#define NTOA_BUF_SZ  18
    static char b[NTOA_BUF_SZ * NB_BUF];
    static int buf_index = 0;

    register char *src, *dst;
    register int i = buf_index++ % NB_BUF;

    src = (char *) &ipaddr;
    dst = (char *) b + i * NTOA_BUF_SZ;

    sprintf(dst, "%d.%d.%d.%d",
        UC(src[0]), UC(src[1]), UC(src[2]), UC(src[3]));

    return dst;
}

/*
 * fh_net_hwtoa
 *
 * Return an ascii-version of a MAC address, but use multiple static buffers,
 * so, you can print multiple addresses in the same log entry.
 */
char *fh_net_hwtoa(uint8_t *hwaddr)
{
#define HWTOA_BUF_SZ  32
    static char b[HWTOA_BUF_SZ * NB_BUF];
    static int buf_index = 0;

    register char *src, *dst;
    register int i = buf_index++ % NB_BUF;

    src = (char *) hwaddr;
    dst = (char *) b + i * HWTOA_BUF_SZ;

    sprintf(dst, "%02X:%02X:%02X:%02X:%02X:%02X",
        UC(src[0]), UC(src[1]), UC(src[2]), UC(src[3]), UC(src[4]), UC(src[5]));

    return dst;
}

/*
 * fh_net_ifaddr
 *
 * Returns the interface IP address provided the interface name.
 */
uint32_t fh_net_ifaddr(int s, char *ifname)
{
    struct ifreq ifr;

    memset(&ifr, 0, sizeof(ifr));

    strncpy(ifr.ifr_name, ifname, sizeof(ifr.ifr_name));

    if (ioctl(s, SIOCGIFADDR, (caddr_t)&ifr) < 0) {
        FH_LOG(NET, ERR, ("NET> ioctl SIOCGIFADDR failed: %d", errno));
        return 0;
    }

    return ((struct sockaddr_in *) &ifr.ifr_addr)->sin_addr.s_addr;
}


