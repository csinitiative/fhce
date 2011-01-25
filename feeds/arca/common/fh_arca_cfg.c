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

//
/*********************************************************************/
/* file: fh_arca_cfg.c                                               */
/* Usage: configuration support for arca feed handler                */
/* Author: Wally Matthews & Ross Cooperman of                        */
/*   Collaborative Software Initiative                               */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>

// FH common headers
#include "fh_log.h"
#include "fh_config.h"

// FH Arca headers
#include "fh_arca_cfg.h"

// global configuration for current process
fh_arca_cfg_process_t   fh_arca_cfg;

/*! \brief Parse a yes/no value string
 *
 *  \param str yes/no string being parsed
 *  \return 1 for yes, 0 for no (or anything else)
 */
static int fh_arca_cfg_yesno(const char *str)
{
    // if str is NULL, log a warning and return no
    if (str == NULL) {
        FH_LOG(CSI, WARN, ("null yes/no value, defaulting to no"));
        return 0;
    }
    
    // if a yes or no is parsed, simply return the appropriate value
    if (str && !strcmp(str, "yes")) return 1;
    if (str && !strcmp(str, "no")) return 0;

    // otherwise, log an invalid yes/no and return no
    FH_LOG(CSI, WARN, ("invalid yes/no value \"%s\", defaulting to no", str));
    return 0;
}

/*! \brief Fetch a long value from the parsed configuration
 *
 *  \param config configuration structure to fetch value from
 *  \param property configuration property being fetched
 *  \param where pointer to a long where the converted value will be stored
 *  \return status code indicating success or failure
 */
static FH_STATUS fh_arca_cfg_get_long(const fh_cfg_node_t *config, const char *property,
                                      long *where)
{
    const char   *value  = NULL;
    char        **endptr = NULL;
    
    // fetch the string version of the property and return FH_ERROR if the value does not exist
    value = fh_cfg_get_string(config, property);
    if (value == NULL) return FH_ERROR;
    
    // convert string version to long and check that conversion was successful
    *where = strtol(value, endptr, 0);
    if (value[0] == '\0' || endptr != NULL) return FH_ERROR;
    
    // return success
    return FH_OK;
}

/*! \brief Configure the specified connection for the specified line
 *
 *  \param config configuration node for the line
 *  \param name connection name being configured
 *  \param connection pointer to the connection structure being populated
 *  \return status code indicating success or failure
 */
static FH_STATUS fh_arca_cfg_load_connection(const fh_cfg_node_t *config, char *name,
                                             fh_arca_cfg_connection_t *connection)
{
    const fh_cfg_node_t      *node   = NULL;
    long                      port   = 0;
    const char               *value  = NULL;
    char                    **endptr = NULL;
    
    // get the config node for the connection we are interested in
    node = fh_cfg_get_node(config, name);
    if (node == NULL) {
        FH_LOG(CSI, ERR, ("missing configuration for connection: %s", name));
        return FH_ERROR;
    }
    
    // convert dotted address into network byte order address and copy to connection config
    if (!inet_aton(fh_cfg_get_string(node, "address"), &connection->address)) {
        FH_LOG(CSI, ERR, ("missing or invalid address property for connection: %s", name));
        return FH_ERROR;
    }
    
    // convert the port from a string to a long and transfer it into the connection structure
    value = fh_cfg_get_string(node, "port");
    port = strtol(value, endptr, 0);
    if (value == NULL || endptr != NULL || port < 1 || port > 65536) {
        FH_LOG(CSI, ERR, ("missing or invalid port property for connection: %s", name));
        return FH_ERROR;
    }
    connection->port = (uint16_t)port;
    
    // copy the specified interface into the connection configuration
    strcpy(connection->interface, fh_cfg_get_string(node, "interface"));
    if (connection->interface == NULL) {
        FH_LOG(CSI, ERR, ("missing or invalid interface property for connection: %s", name));
        return FH_ERROR;
    }
    
    // set the enabled flag for this connection
    connection->enabled = fh_arca_cfg_yesno(fh_cfg_get_string(node, "enable"));
    
    // by the time we get here, success
    return FH_OK;
}

/*! \brief Add a line configuration to the established process configuration
 *
 *  \param config general configuration structure
 *  \param line being added
 *  \return status code indicating success or failure
 */
static FH_STATUS fh_arca_cfg_add_line(const fh_cfg_node_t *config, char *line)
{
    char                  line_cfg_property[MAX_PROPERTY_LENGTH];
    const fh_cfg_node_t  *node     = NULL;
    fh_arca_cfg_line_t   *line_cfg = NULL;
        
    // build a property string to fetch the appropriate line config, then fetch it
    sprintf(line_cfg_property, "arca.lines.%s", line);
    node = fh_cfg_get_node(config, line_cfg_property);
    if (node == NULL) {
        FH_LOG(CSI, ERR, ("no configuration data for line: %s", line));
        return FH_ERROR;
    }
    
    // allocate space for one more line in the process config struct
    fh_arca_cfg.lines = (fh_arca_cfg_line_t *)realloc(fh_arca_cfg.lines, sizeof(fh_arca_cfg_line_t)
                                                      * (fh_arca_cfg.num_lines + 1));
    if (fh_arca_cfg.lines == NULL) {
        FH_LOG(CSI, ERR, ("unable to allocate memory for Arca line configuration list"));
        return FH_ERROR;
    }
    line_cfg = &fh_arca_cfg.lines[fh_arca_cfg.num_lines++];
    
    // copy the line name into the name property of the line structure
    strcpy(&line_cfg->name[0], line);
    
    // set numeric line values
    line_cfg->fast = fh_arca_cfg_yesno(fh_cfg_get_string(node, "fast"));
    
    // configure connections for this line
    if (fh_arca_cfg_load_connection(node, "primary", &line_cfg->primary) != FH_OK) {
        FH_LOG(CSI, ERR, ("unable to load primary connection information for line: %s", line));
        return FH_ERROR;
    }
    if (fh_arca_cfg_load_connection(node, "secondary", &line_cfg->secondary) != FH_OK) {
        FH_LOG(CSI, ERR, ("unable to load secondary connection information for line: %s", line));
        return FH_ERROR;
    }
   
    // copy reference plugin parameters so that customer plugins may over-ride
    line_cfg->maximum_sessions = (uint32_t) fh_arca_cfg.max_sessions;
    line_cfg->maximum_symbols = (uint32_t) fh_arca_cfg.max_symbols;
    line_cfg->maximum_firms = (uint32_t) fh_arca_cfg.max_firms;
    line_cfg->maximum_orders = (uint32_t) fh_arca_cfg.max_orders;
    // if we get here, success
    return FH_OK; 
}

/*! \brief Load the global process configuration structure for this process
 *
 *  \param config parsed configuration node to load data from
 *  \param process process string for this process
 *  \return status code indicating success or failure
 */
FH_STATUS fh_arca_cfg_load(const fh_cfg_node_t *config, const char *process)
{
    int                   i;
    char                  property[MAX_PROPERTY_LENGTH];
    const fh_cfg_node_t  *node      = NULL;

    // make sure we have not been passed a null pointer
    FH_ASSERT(config);
    
    // initialize configuration structure
    memset((void *)&fh_arca_cfg, 0, sizeof(fh_arca_cfg));
    
    // set the process name in the configuration structure
    strcpy(fh_arca_cfg.name, process);
    
    // find the configuration node for this process
    sprintf(property, "arca.processes.%s", process);
    node = fh_cfg_get_node(config, property);
    if (node == NULL) {
        FH_LOG(CSI, ERR, ("no configuration data for process: %s", process));
        return FH_ERROR;
    }
    
    // get several required long values from the general config structure
    if (fh_arca_cfg_get_long(node, "cpu", &fh_arca_cfg.cpu) != FH_OK) {
        FH_LOG(CSI, ERR, ("missing or invalid cpu configuration parameter: %s", process));
        return FH_ERROR;
    }
    if (fh_arca_cfg_get_long(node, "index", &fh_arca_cfg.index) != FH_OK) {
        FH_LOG(CSI, WARN, ("missing or invalid index configuration parameter: %s", process));
        fh_arca_cfg.index = -1;
    }
    if (fh_arca_cfg_get_long(node, "max_sessions", &fh_arca_cfg.max_sessions) != FH_OK) {
        FH_LOG(CSI, WARN, ("missing or invalid max_sessions configuration parameter: %s", process));
        fh_arca_cfg.max_sessions = 10;
    }
    if (fh_arca_cfg_get_long(node, "max_symbols", &fh_arca_cfg.max_symbols) != FH_OK) {
        FH_LOG(CSI, WARN, ("missing or invalid max_symbols configuration parameter: %s", process));
        fh_arca_cfg.max_symbols = 20000;
    }
    if (fh_arca_cfg_get_long(node, "max_firms", &fh_arca_cfg.max_firms) != FH_OK) {
        FH_LOG(CSI, WARN, ("missing or invalid max_firms configuration parameter: %s", process));
        fh_arca_cfg.max_firms = 2000;
    }
    if (fh_arca_cfg_get_long(node, "max_orders", &fh_arca_cfg.max_orders) != FH_OK) {
        FH_LOG(CSI, WARN, ("missing or invalid max_orders configuration parameter: %s", process));
        fh_arca_cfg.max_orders = 10000000;
    }
    
    // fetch the lines config parameter checking that it exists
    node = fh_cfg_get_node(node, "lines");
    if (node == NULL || node->num_values <= 0) {
        FH_LOG(CSI, ERR, ("you must specify at least one line per process: %s", process));
        return FH_ERROR;
    }
    
    // create a line config structure for each line 
    for (i = 0; i < node->num_values; i++) {
        if(fh_arca_cfg_add_line(config, node->values[i]) != FH_OK) return FH_ERROR;
    }
    
    // if we get this far, success
    return FH_OK;
}


/*! \brief Dump all configuration data to the logs
 */
void fh_arca_cfg_dump()
{
    fh_arca_cfg_process_t *config = &fh_arca_cfg;
    register uint32_t i;

    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("> Arca Process Configuration:"));
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("> Name         : %s", config->name));
    FH_LOG_PGEN(DIAG, ("> CPU          : %d", config->cpu));
    FH_LOG_PGEN(DIAG, ("> Max Sessions : %d", config->max_sessions));
    FH_LOG_PGEN(DIAG, ("> Max Symbols  : %d", config->max_symbols));
    FH_LOG_PGEN(DIAG, ("> Max Firms    : %d", config->max_firms));
    FH_LOG_PGEN(DIAG, ("> Max Orders   : %d", config->max_orders));
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("> Process Lines:"));
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));

    for (i = 0; i < config->num_lines; i++) {
        fh_arca_cfg_line_t *line = &config->lines[i];
        FH_LOG_PGEN(DIAG, ("> %s (fast: %s)", line->name,
                          ((line->fast) ? "yes" : "no")));
        FH_LOG_PGEN(DIAG, ("    + primary   - %s:%d @ %s - %s", inet_ntoa(line->primary.address),
                           line->primary.port, line->primary.interface,
                           ((line->primary.enabled) ? "enabled " : "disabled")));
        FH_LOG_PGEN(DIAG, ("    + secondary - %s:%d @ %s - %s", inet_ntoa(line->secondary.address),
                          line->secondary.port, line->secondary.interface,
                          ((line->secondary.enabled) ? "enabled " : "disabled")));
    }

    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
}
