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

#ifndef __FH_OPRA_LO_H__
#define __FH_OPRA_LO_H__

#include "fh_errors.h"
#include "fh_config.h"
#include "fh_opra_lo_ext.h"

/*
 * Option types
 */
#define LO_TYPE_EU     (1)   /* Equity Underlying       */
#define LO_TYPE_EB     (2)   /* Equity Bounds           */
#define LO_TYPE_EL     (3)   /* Equity Long Term        */
#define LO_TYPE_EF     (4)   /* Equity FLEX             */
#define LO_TYPE_CU     (5)   /* Currency Underlying     */
#define LO_TYPE_CL     (6)   /* Currency Long Term      */
#define LO_TYPE_CM     (7)   /* Currency Month End      */
#define LO_TYPE_CF     (8)   /* Currency FLEX           */
#define LO_TYPE_IL     (9)   /* Index Long Term         */
#define LO_TYPE_IU    (10)   /* Index Underlying        */
#define LO_TYPE_IF    (11)   /* Index FLEX              */
#define LO_TYPE_GF    (12)   /* Interest Rate Futures   */
#define LO_TYPE_SF    (13)   /* Stock Futures           */
#define LO_TYPE_FC    (14)   /* Futures Cash Index      */
#define LO_TYPE_FP    (15)   /* Futures Physical Index  */
#define LO_TYPE_TU    (16)   /* Treasury Underlying     */
#define LO_TYPE_TL    (17)   /* Treasury Long Term      */

/*
 * OPRA Configuration for the listedoptions file
 */
typedef struct {
    char     loc_src_filename[MAXPATHLEN];
    char     loc_dst_filename[MAXPATHLEN];
    char     loc_scp_script[MAXPATHLEN];
    uint32_t loc_scp_timeout;
    char     loc_scp_username[80];
    char     loc_scp_password[80];
    char     loc_scp_hostname[80];
} fh_opra_lo_cfg_t;

/*
 * Listed Options API
 */
FH_STATUS fh_opra_lo_init(const char *lo_filename);
FH_STATUS fh_opra_lo_cfg_load(fh_cfg_node_t *config, fh_opra_lo_cfg_t *loc);
FH_STATUS fh_opra_lo_lookup(char *lo_root, fh_opra_lo_t **lo);
FH_STATUS fh_opra_lo_download(fh_opra_lo_cfg_t *loc);

#endif /* __FH_OPRA_LO_H__ */
