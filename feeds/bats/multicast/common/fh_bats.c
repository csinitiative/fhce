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

/* system headers */
#include <stdlib.h>

/* common FH headers */
#include "fh_info.h"
#include "fh_shr_lh.h"
#include "fh_shr_cfg_lh.h"
#include "fh_shr_config.h"
#include "fh_shr_mmcast.h"

/* BATS headers */
#include "fh_bats_revision.h"
#include "fh_bats_parse.h"
#include "fh_bats_cfg.h"

static fh_info_build_t version_info = {
    "BATS",
    1,
    BUILD_USER,
    BUILD_HOST,
    BUILD_ARCH,
    BUILD_KVER,
    BUILD_DATE,
    BUILD_URL,
    BUILD_REV
};

/**
 *  @brief "real" main function for BATS feed handler
 *
 *  @param argc number of command line arguments passed along from main function
 *  @param argv array of command line arguments passed along from main function
 *  @param version BATS feed handler version
 *  @return return value which will in turn be returned by main function (and become the
 *          the application's exit code)
 */
int fh_bats_main(int argc, char **argv, int version)
{
    /* build main structure of callbacks */
    fh_shr_mmcast_cb_t callbacks = {
        fh_bats_parse_init,
        fh_bats_parse_pkt,
    };

    /* set the proper version in the static version info struct */
    version_info.version = version;

    /* start the "real" feed handler */
    return fh_shr_mmcast_main(argc, argv, "bats", &version_info, &callbacks);
}
