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

// FH headers (required header files for all plugins)
#include "fh_plugin.h"

// super simple hook function that will be registered twice by the constructor function below
void sample_log_msg(FH_STATUS *status, ...) {
    *status = FH_OK;
}

// super simple plugin designed to test behavior when the same hook is registered twice
void __attribute__ ((constructor)) sample_init(void) {
    if (fh_plugin_register(FH_PLUGIN_LOG_MSG, sample_log_msg) != FH_OK) {
        exit(-1);
    }
    if (fh_plugin_register(FH_PLUGIN_LOG_MSG, sample_log_msg) != FH_OK) {
        exit(-1);
    }
}
