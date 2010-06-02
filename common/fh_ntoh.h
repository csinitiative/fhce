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

#ifndef __FH_NTOH_H__
#define __FH_NTOH_H__

/* system headers */
#include <arpa/inet.h>

/* just a few convenience macros to make big -> little endian conversions more readable */
#define ntoh8(x)     ((x) & 0xff)
#define ntoh16(x)    htons(x)
#define ntoh32(x)    htonl(x)
#define ntoh64(x)    ((((x) & 0xff00000000000000LL) >> 56)  | \
                     (((x)  & 0x00ff000000000000LL) >> 40)  | \
                     (((x)  & 0x0000ff0000000000LL) >> 24)  | \
                     (((x)  & 0x000000ff00000000LL) >> 8)   | \
                     (((x)  & 0x00000000ff000000LL) << 8)   | \
                     (((x)  & 0x0000000000ff0000LL) << 24)  | \
                     (((x)  & 0x000000000000ff00LL) << 40)  | \
                     (((x)  & 0x00000000000000ffLL) << 56))

#endif /* __FH_NTOH_H__ */
