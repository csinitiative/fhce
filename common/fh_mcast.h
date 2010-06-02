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

#ifndef __FH_MCAST_H__
#define __FH_MCAST_H__

#include "fh_errors.h"

FH_STATUS fh_mcast_ttl(int s, uint8_t ttl);
FH_STATUS fh_mcast_if(int s, char *ifname);
FH_STATUS fh_mcast_loop(int s, int on);
FH_STATUS fh_mcast_join(int s, uint32_t ifaddr, uint32_t mcaddr);
FH_STATUS fh_mcast_leave(int s, uint32_t ifaddr, uint32_t mcaddr);

#endif /* __FH_MCAST_H__ */
