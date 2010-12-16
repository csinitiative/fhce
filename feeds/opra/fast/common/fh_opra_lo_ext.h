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

#ifndef __FH_OPRA_LO_EXT_H__
#define __FH_OPRA_LO_EXT_H__

#include <sys/param.h>

#define FH_OPRA_DBG   (0)

/*
 * Listed-options file format
 */
#define LO_ROOT_SIZE            (7)
#define LO_SEC_SIZE             (7)
#define LO_COMPANY_SIZE         (31)
#define LO_EXCH_SIZE            (12)
#define LO_XXX_SIZE             (13)
#define LO_CUSIP_SIZE           (10)
#define LO_TYPE_SIZE            (3)

#define LO_TOTAL_SIZE           (LO_ROOT_SIZE    + \
                                 LO_SEC_SIZE     + \
                                 LO_COMPANY_SIZE + \
                                 LO_EXCH_SIZE    + \
                                 LO_XXX_SIZE     + \
                                 LO_CUSIP_SIZE   + \
                                 LO_TYPE_SIZE)

/*
 * Listed options information
 */
typedef struct {
    char lo_root[LO_ROOT_SIZE];         /* Option Root symbol   */
    char lo_sec[LO_SEC_SIZE];           /* Security symbol      */
#if FH_OPRA_DBG
    char lo_company[LO_COMPANY_SIZE];   /* Company name         */
    char lo_exch[LO_EXCH_SIZE];         /* Listed Exchanges     */
    char lo_cusip[LO_CUSIP_SIZE];       /* CUSIP                */
    char lo_type[LO_TYPE_SIZE];         /* Option type          */
#endif
} fh_opra_lo_t;

#endif /* __FH_OPRA_LO_EXT_H__ */
