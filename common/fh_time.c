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

#include <time.h>
#include <sys/time.h>
#include "fh_time.h"

/*
 * fh_time_get
 *
 * Get the current time in microseconds.
 */
FH_STATUS fh_time_get(uint64_t *now)
{
    struct timeval tv;

    gettimeofday(&tv, 0);

    *now = ((uint64_t)tv.tv_sec * 1000000) + (uint64_t)tv.tv_usec;

    return FH_OK;
}

/*
 * fh_time_fmt
 *
 * Format the given time and load it into the buffer
 */
FH_STATUS fh_time_fmt(uint64_t usec, char *buff, int buff_len)
{
    time_t tsec      = (time_t)(usec / 1000000);
    struct tm *ltime = localtime(&tsec);
    size_t res       = strftime(buff, buff_len, "%Y-%m-%d %H:%M:%S", ltime);

    return ((res > 0) ? FH_OK : FH_ERROR);
}
