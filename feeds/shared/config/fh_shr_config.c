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

// System headers
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

// FH common headers
#include "fh_util.h"
#include "fh_log.h"

// FH shared config headers
#include "fh_shr_config.h"

/*
 * Get the configured (or overriden) feed handler home directory
 */
const char *fh_shr_cfg_home()
{
    return (getenv("FH_HOME") == NULL) ? FH_HOME : getenv("FH_HOME");
}

/*! \brief Set the default options for this run (which can be modified by command line options)
 *
 *  \param options the options set where defaults will be set
 *  \param version the name of the feed handler being run
 *  \param version the version of the feed handler that is being run
 */
void fh_shr_cfg_set_defaults(fh_shr_cfg_options_t *options, const char *name, int version)
{
    const char *fh_home;
    char        real_config_dir[PATH_MAX + 1];
    char        real_plugin_dir[PATH_MAX + 1];
    char        real_config_file[NAME_MAX + 1];

    // by default all options are 0 or NULL
    memset(options, 0, sizeof(fh_shr_cfg_options_t));

    // set the version to whatever we were passed
    options->version = version;

    // override default FH_HOME if the FH_HOME env. variable is set
    fh_home = fh_shr_cfg_home();

    // generate "real" config/plugin path information using the name of this feed handler
    sprintf(real_config_dir, FH_SHR_CFG_DIR, name);
    sprintf(real_config_file, FH_SHR_CFG_FILE, name);
    sprintf(real_plugin_dir, FH_SHR_CFG_PLUGIN_DIR, name);
    fh_str_downcase(real_config_dir);
    fh_str_downcase(real_config_file);
    fh_str_downcase(real_plugin_dir);

    // set up paths, rooted at fh_home
    sprintf(options->config_file, "%s/%s/%s", fh_home, real_config_dir, real_config_file);
    sprintf(options->plugin_path, "%s/%s", fh_home, real_plugin_dir);
}

/*! \brief Initialize the logging layer for this process
 *
 *  \param options command line and default options for this process
 *  \param config configuration data
 *  \return success or failure
 */
FH_STATUS fh_shr_cfg_log_init(fh_shr_cfg_options_t *options, const fh_cfg_node_t *config)
{
    int rc = FH_OK;

    // start up logging
    fh_log_open();

    // conditionally set logging to output to the command line
    if (options->debug_level || options->display_version) {
        fh_log_set_cfg(FH_LCF_CONSOLE);
    }

    // increase amount of logging when debug flag is set
    if (options->debug_level > 0) {
        fh_log_set_lvl(FH_LL_STATS);
    }
    if (options->debug_level > 1) {
        fh_log_set_lvl(FH_LL_DIAG | FH_LL_VSTATE);
    }

    // set the logging identifier to this process's name
    fh_log_set_ident(options->logging_tag);

    // load extra logging configuration from process config file
    rc = fh_log_cfg_load(config);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to load logging configuration from '%s'", options->config_file));
        return rc;
    }

    // if we get here, return success
    return rc;
}
