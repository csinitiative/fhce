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

#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

#include "fh_util.h"
#include "fh_log.h"
#include "fh_net.h"
#include "fh_tcp.h"
#include "fh_time.h"

#include "fh_mgmt_service.h"
#include "fh_mgmt_cfg.h"


/*
 * fh_mgmt_cfg_yesno
 *
 * Parse the yes/no value string and return 1 for yes, 0 for no, and -1 for
 * errors
 */
static int fh_mgmt_cfg_yesno(const char *strval)
{
    if (strval) {
        if (strcmp(strval, "yes") == 0) {
            return 1;
        }

        if (strcmp(strval, "no") == 0) {
            return 0;
        }
    }

    FH_LOG(MGMT, ERR, ("Invalid yes/no value: %s", strval ? strval : "NULL"));

    return -1;
}
 
/*
 * cfg_sg_load
 *
 * Load the service group from this configuration node
 */
static fh_mgmt_sg_t *cfg_sg_load(const fh_cfg_node_t *group)
{
    fh_mgmt_sg_t    *sg             = NULL;
    char            *endptr         = NULL;
    const char      *strval         = NULL;
    const char      *report_name    = NULL;
    int              disable        = 0;
    int              stats          = 0;
    uint32_t         restart_time   = 0;
    uint32_t         stats_interval = 0;
    int              restart        = 0;

    FH_LOG(MGMT, DIAG, ("Parsing service group: %s", group->name));

    // Get the report name for the service group
    report_name  = fh_cfg_get_string(group, "report_name");
    if (!report_name) {
        FH_LOG(CSI, ERR, ("report_name configuration not found (node '%s')",
                           group->name));
        return NULL;
    }

    // Check whether the service group is enabled
    strval  = fh_cfg_get_string(group, "disable");
    disable = fh_mgmt_cfg_yesno(strval);
    if (disable == -1) {
        disable = 1;
    }


    // Check whether statistics monitoring is enabled
    strval  = fh_cfg_get_string(group, "stats");
    stats   = fh_mgmt_cfg_yesno(strval);
    if (stats == -1) {
        stats = 0;
    }

    if (stats) {
        strval = fh_cfg_get_string(group, "stats_interval");
        if (!strval) {
            FH_LOG(CSI, ERR, ("stats_interval configuration not found (node '%s')",
                               group->name));
            return NULL;
        }

        stats_interval = strtol(strval, &endptr , 0);
        if (*strval == '\0' || *endptr != '\0') {
            FH_LOG(MGMT, ERR, ("Invalid stats_interval second value: '%s'", strval));
            return NULL;
        }
    }

    // Check whether automatic restart is enabled
    strval  = fh_cfg_get_string(group, "restart");
    restart = fh_mgmt_cfg_yesno(strval);
    if (restart == -1) {
        restart = 0;
    }

    if (restart) {
        strval = fh_cfg_get_string(group, "restart_time");
        if (!strval) {
            FH_LOG(CSI, ERR, ("restart_time configuration not found (node '%s')",
                              group->name));
            return NULL;
        }

        restart_time = strtol(strval, &endptr , 0);
        if (*strval == '\0' || *endptr != '\0') {
            FH_LOG(MGMT, ERR, ("Invalid restart_time second value: %s", strval));
            return NULL;
        }
    }

    // Create the new service group
    sg = fh_mgmt_sg_new(group->name, report_name, restart_time, stats_interval, disable);
    if (!sg) {
        FH_LOG(MGMT, ERR, ("Failed to allocate a new service group: %s", group->name));
        return NULL;
    }

    return sg;
}

/*
 * cfg_serv_load
 *
 * Load the service from this configuration node
 */
static fh_mgmt_serv_t *cfg_serv_load(const fh_cfg_node_t *serv_node)
{
    fh_mgmt_serv_t  *serv           = NULL;
    const char      *command        = NULL;
    const char      *args           = NULL;
    const char      *action         = NULL;
    int              disable        = 0;
    int              stats          = 0;
    int              shutdown       = 0;
    int              cli            = 0;
    const char      *strval         = NULL;

    FH_LOG(MGMT, DIAG, ("Parsing service: %s", serv_node->name));

    // Check whether the service group is enabled
    strval  = fh_cfg_get_string(serv_node, "disable");
    disable = fh_mgmt_cfg_yesno(strval);
    if (disable == -1) {
        disable = 1;
    }

    // Check whether the service group is enabled
    strval  = fh_cfg_get_string(serv_node, "cli");
    cli     = fh_mgmt_cfg_yesno(strval);
    if (cli == -1) {
        cli = 0;
    }

    // Check whether statistics monitoring is enabled
    strval  = fh_cfg_get_string(serv_node, "stats");
    stats   = fh_mgmt_cfg_yesno(strval);
    if (stats == -1) {
        stats = 0;
    }
    
    // Check killing process on fhmgr exit is enabled
    strval      = fh_cfg_get_string(serv_node, "shutdown");
    shutdown = fh_mgmt_cfg_yesno(strval);
    if (shutdown == -1) {
        shutdown = 0;
    }

    // Get the action for the service
    action = fh_cfg_get_string(serv_node, "action");
    if (!action) {
        FH_LOG(MGMT, ERR, ("Missing 'action' configuration for service: %s", serv_node->name));
        return NULL;
    }

    // Get the command for the service
    command = fh_cfg_get_string(serv_node, "command");
    if (!command) {
        FH_LOG(MGMT, ERR, ("Missing 'command' configuration for service: %s", serv_node->name));
        return NULL;
    }

    // Fetch the arguments configuration
    args = fh_cfg_get_string(serv_node, "args");

    // Create the new service
    serv = fh_mgmt_serv_new(serv_node->name, command, args, disable, stats, shutdown, action);
    if (!serv) {
        FH_LOG(MGMT, ERR, ("Failed to allocate a new service: %s", serv_node->name));
        return NULL;
    }

    if (cli) {
        serv->serv_flags |= FH_MGMT_SERV_CLI;
    }

    return serv;
} 

/*
 * fh_mgmt_cfg_load
 *
 * Loads the FH manager configuration
 */
FH_STATUS fh_mgmt_cfg_load(fh_cfg_node_t *config)
{
    const fh_cfg_node_t *node;
    int                  i, j;
    const char          *strval         = NULL;
    char                *endptr         = NULL;
    uint32_t             spawn_delay    = 0;
    FH_STATUS            rc             = FH_ERROR;
    
    /* fetch (or default) the spawn delay value from the config structure */
    strval = fh_cfg_get_string(config, "fhmgr.spawn_delay");
    if (!strval) {
        FH_LOG(MGMT, WARN, ("spawn_delay option not found (default = 0)"));
    }
    else {
        spawn_delay = strtol(strval, &endptr , 0);
        if (*strval == '\0' || *endptr != '\0') {
            FH_LOG(MGMT, ERR, ("spawn_delay value must be numeric (default = 0)"));
            spawn_delay = 0;
        }
        else {
            FH_LOG(MGMT, INFO, ("spawn_delay initialized to %u", spawn_delay));
        }
    }

    /* initialization of the service library */
    fh_mgmt_serv_init(spawn_delay);
    
    // Look for the service_groups configuration
    node = fh_cfg_get_node(config, "fhmgr.service_groups");
    if (!node) {
        FH_LOG(CSI, ERR, ("Missing required configuration value: service_groups"));
        return FH_ERROR;
    }

    for (i = 0; i < node->num_children; i++) {
        const fh_cfg_node_t *group    = node->children[i];
        const fh_cfg_node_t *services = NULL;
        fh_mgmt_sg_t *sg = NULL;

        // Load each service group
        sg = cfg_sg_load(group);
        if (!sg) {
            FH_LOG(MGMT, ERR, ("Failed to parse service group: %s", group->name)); 
            return rc;
        }

        services = fh_cfg_get_node(group, "services");
        if (!services) {
            FH_LOG(MGMT, WARN, ("No services for service group: %s", group->name));
            continue;
        }

        // Load and add all its services 
        for (j=0; j < services->num_children; j++) {
            const fh_cfg_node_t *serv_node = services->children[j];
            fh_mgmt_serv_t *serv = NULL;

            // Load service from the configuration node
            serv = cfg_serv_load(serv_node);
            if (!serv) {
                FH_LOG(MGMT, ERR, ("failed to load service: %s group:%s",
                                   serv_node->name, group->name));
                return FH_ERROR;
            }

            // Add service to the service group
            rc = fh_mgmt_sg_add_serv(sg, serv);
            if (rc != FH_OK) {
                FH_LOG(MGMT, ERR, ("failed to add service: %s to group:%s",
                                   serv_node->name, group->name));
                return FH_ERROR;
            }
        }
    }

    // Dump the service group configuration for debugging purpose
    if (FH_LL_OK(MGMT, DIAG)) {
        fh_mgmt_sg_dump();
    }

    return FH_OK;
}
