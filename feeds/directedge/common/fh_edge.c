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
#include <stdio.h>
#include <stdlib.h>

/* common FH headers */
#include "fh_info.h"
#include "fh_shr_tcp_lh.h"
#include "fh_shr_tcp.h"

/* Direct Edge headers */
#include "fh_edge_revision.h"
#include "fh_edge_parse.h"
#include "fh_edge_connect.h"

/* structure to describe the build environment of the binary produced from this code */
static fh_info_build_t version_info = {
    "DIRECTEDGE",
    0,
    BUILD_USER,
    BUILD_HOST,
    BUILD_ARCH,
    BUILD_KVER,
    BUILD_DATE,
    BUILD_URL,
    BUILD_REV
};

/**
 *  @brief "Real" main function for DirectEdge feed handler
 *
 *  @param argc number of command line arguments passed along from main function
 *  @param argv array of command line arguments passed along from main function
 *  @param version ITCH feed handler version
 *  @return return value which will in turn be returned by main function (and become the
 *          the application's exit code)
 */
int fh_edge_main(int argc, char **argv, int version)
{
    fh_shr_tcp_cb_t callbacks = {
        fh_edge_parse_init,
        fh_edge_parse_msg,
        fh_edge_conn_login,
        fh_edge_alarm,
    };

    /* set the proper version number in the version info struct */
    version_info.version = version;

    /* start the "real" feed handler */
    return fh_shr_tcp_main(argc, argv, "edge", &version_info, &callbacks) ;
}
