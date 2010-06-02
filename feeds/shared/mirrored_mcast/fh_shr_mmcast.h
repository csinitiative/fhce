/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef FH_SHR_MMCAST_H
#define FH_SHR_MMCAST_H

/* FH common headers */
#include "fh_info.h"

/* FH shared config headers */
#include "fh_shr_config.h"
#include "fh_shr_cfg_lh.h"
#include "fh_shr_mgmt.h"
#include "fh_shr_lh.h"

/* structure used to pass necessary callbacks to the "real" main function */
typedef struct {
    fh_shr_lh_init_cb_t     *init;
    fh_shr_lh_parse_cb_t    *parser;
} fh_shr_mmcast_cb_t;

/**
 *  @brief "Real" main function for mirrored multicast feed handlers
 *
 *  @param argc number of command line arguments passed along from main function
 *  @param argv array of command line arguments passed along from main function
 *  @param cfg_tag top level configuration block inside which all relevant config resides
 *  @param info feed handler build, version, etc. information
 *  @return return value which will in turn be returned by main function (and become the
 *          the application's exit code)
 */
int fh_shr_mmcast_main(int argc, char **argv, const char *cfg_tag,
                       const fh_info_build_t *info, fh_shr_mmcast_cb_t *cb);

#endif  /* FH_SHR_MMCAST_H */
