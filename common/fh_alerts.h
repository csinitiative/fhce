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

#ifndef __FH_ALERTS_H__
#define __FH_ALERTS_H__

/* typedef FH_ALERT to uint8_t */
typedef uint8_t FH_ALERT;

/* list of all alerts that a feed handler can generate */
#define FH_ALERT_GAP                        (1)
#define FH_ALERT_NOGAP                      (2)
#define FH_ALERT_LOSS                       (3)
#define FH_ALERT_SESSION_TERMINATED         (4)
#define FH_ALERT_TCP_CONNECTION_ESTABLISHED (5)
#define FH_ALERT_TCP_CONNECTION_BROKEN      (6)
#define FH_ALERT_SERVER_HB_MISSING          (7)

#endif /* __FH_ALERTS_H__ */
