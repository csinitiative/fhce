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

#ifndef __FH_SHR_CONFIG_H__
#define __FH_SHR_CONFIG_H__

// System headers
#include <limits.h>

// FH common headers
#include "fh_config.h"
#include "fh_errors.h"

#define FH_HOME                   "/opt/csi/fh"
#define FH_SHR_CFG_DIR            "%s/etc"
#define FH_SHR_CFG_FILE           "%s.conf"
#define FH_SHR_CFG_PLUGIN_DIR     "%s/plugins"

// structure to contain command line arguments to the ITCH feed handler
typedef struct {
    int   standalone;
    int   debug_level;
    int   display_version;
    int   version;
    char  process[MAX_PROPERTY_LENGTH];
    char  logging_tag[MAX_PROPERTY_LENGTH];
    char  config_file[PATH_MAX + 1];
    char  plugin_path[PATH_MAX + 1];
    char *program_name;
} fh_shr_cfg_options_t;

/**
 *  @brief Get the configured (or overriden) feed handler home directory
 *
 *  @return feed handler home directory
 */
const char *fh_shr_cfg_home();

/*! \brief Set the default options for this run (which can be modified by command line options)
 *
 *  \param options the options set where defaults will be set
 *  \param version the name of the feed handler being run
 *  \param version the version of the feed handler that is being run
 */
void fh_shr_cfg_set_defaults(fh_shr_cfg_options_t *options, const char *name, int version);

/*! \brief Initialize the logging layer for this process
 *
 *  \param options command line and default options for this process
 *  \param config configuration data
 *  \return success or failure
 */
FH_STATUS fh_shr_cfg_log_init(fh_shr_cfg_options_t *options, const fh_cfg_node_t *config);

#endif  /* __FH_SHR_CONFIG_H__ */
