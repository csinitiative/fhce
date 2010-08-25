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


/* System headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FH common headers */
#include "fh_info.h"
#include "fh_log.h"

/*
 * fh_edge_cfg_yesno
 *
 * Parse the yes/no value string and return 1 for yes, 0 for no, and -1 for
 * errors
 */
static int fh_edge_cfg_yesno(const char *strval)
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
 * fh_edge_cfg_load
 *
 * Load the DirectEdge configuration, with all the process configuration and
 * the line configurations.
 */
FH_STATUS fh_edge_cfg_load(fh_cfg_node_t *config, int proc_id)
{
    const fh_cfg_node_t *node;
    const char          *strval;
    int                  jitter_stats             = 0;
    int                  periodic_stats           = 0;
    int                  periodic_stats_interval  = 0;

    //TEMP (RC) - suppress unused parameter warning
    if (proc_id) {}

    node = fh_cfg_get_node(config, "edge.options");
    if (node) {
        /* Retrieve the periodic_stats enable field */
        strval = fh_cfg_get_string(node, "periodic_stats");
        if (strval) {
            periodic_stats = fh_edge_cfg_yesno(strval);
            if (periodic_stats == -1) {
                periodic_stats = 0;
                FH_LOG(MGMT, WARN, ("periodic_stats parameter is invalid: %s", strval));
            }
        }
        else {
            FH_LOG(MGMT, WARN, ("periodic_stats is missing"));
        }
        /* retrieve the periodic stats interval value if periodic stats is on */
        if (periodic_stats) {
            strval = fh_cfg_get_string(node, "periodic_stats_interval");
            if (strval) {
                periodic_stats_interval = atoi(strval);
                if (periodic_stats_interval <= 0) {
                    FH_LOG(MGMT, ERR, ("periodic_stats_interval must be numeric: %s", strval));
                    return FH_ERROR;
                }
            }
            else {
                /* it is an error if periodic stats is 'yes' and no interval is set */
                FH_LOG(MGMT, ERR, ("periodic_stats_interval is required if periodic_stats = yes"));
                return FH_ERROR;
            }
        }
        /* Retrieve the jitter stats boolean */
        strval = fh_cfg_get_string(node, "jitter_stats");
        if (strval) {
            jitter_stats = fh_edge_cfg_yesno(strval);
            if (jitter_stats == -1) {
                jitter_stats = 0;
            }
        }
        else {
            FH_LOG(MGMT, WARN, ("jitter_stats parameter is missing"));
        }

    }
    return FH_OK;
}
