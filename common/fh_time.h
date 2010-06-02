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

#ifndef __FH_TIME_H__
#define __FH_TIME_H__

#include <stdint.h>
#include "fh_errors.h"

/*
 * Get a 64bit microsecond timestamp
 */
FH_STATUS fh_time_get(uint64_t *now);
FH_STATUS fh_time_fmt(uint64_t usec, char *buff, int buff_len);

#endif /* __FH_TIME_H__ */
