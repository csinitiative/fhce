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

// System headers
#include <stdlib.h>

// FH common headers
#include "fh_info.h"

// FH shared headers
#include "fh_shr_config.h"
#include "fh_shr_mmcast.h"

// FH ITCH headers
#include "fh_itch_revision.h"
#include "fh_itch_parse.h"

static fh_info_build_t version_info = {
    "ITCH",
    1,
    BUILD_USER,
    BUILD_HOST,
    BUILD_ARCH,
    BUILD_KVER,
    BUILD_DATE,
    BUILD_URL,
    BUILD_REV
};

/*! \brief "real" main function for ITCH feed handler
 *
 *  \param argc number of command line arguments passed along from main function
 *  \param argv array of command line arguments passed along from main function
 *  \param version ITCH feed handler version
 *  \return return value which will in turn be returned by main function (and become the
 *          the application's exit code)
 */
int fh_itch_main(int argc, char **argv, int version)
{
    // build main structure of callbacks
    fh_shr_mmcast_cb_t callbacks = {
        fh_itch_parse_init,
        fh_itch_parse_pkt
    };
    
    // set the proper version in the static version info struct
    version_info.version = version;
    
    // start the "real" feed handler
    return fh_shr_mmcast_main(argc, argv, "itch", &version_info, &callbacks);
}
