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

#ifndef __FH_ERRORS_H__
#define __FH_ERRORS_H__

#include <stdint.h>

/*
 * Error/return-code type definition
 */
typedef int32_t FH_STATUS;

/*
 * Error codes
 */
#define FH_OK                   (0)
#define FH_ERROR                (-1)
#define FH_ERR_NOTFOUND         (1)
#define FH_ERR_DUP              (2)
#define FH_ERR_END_OF_FILE      (3)

#endif /* __FH_ERRORS_H__ */
