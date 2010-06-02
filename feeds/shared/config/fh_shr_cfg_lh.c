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

/* System headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

/* FH common headers */
#include "fh_errors.h"
#include "fh_config.h"
#include "fh_log.h"
#include "fh_plugin_internal.h"

/* FH shared config headers */
#include "fh_shr_cfg_lh.h"

/**
 *  @brief (private) Add a connection configuration to a line configuration
 *
 *  @param config the configuration structure that contains the config. information
 *  @param name the name of the line being added
 *  @param lh_config the line handler config that will receive the added line
 *  @return status code indicating success or failure
 */
static FH_STATUS add_connection(const fh_cfg_node_t *config, const char *name,
                                fh_shr_cfg_lh_conn_t *conn)
{
    const fh_cfg_node_t  *node      = NULL;
    const char           *property  = NULL;
    struct in_addr        address;
    
    /* fetch the connection node for this connection */
    node = fh_cfg_get_node(config, name);

    /* if the node doesn't exist, disable the connection (no configuraton == disabled) */
    if (node == NULL) {
        conn->enabled = 0;
        return FH_OK;
    }

    /*
     * if the "enabled" property is missing the connection is enabled...if it is there it must
     * be yes or no, otherwise log an error and disable it
     */
    switch(fh_cfg_set_yesno(node, "enabled", &conn->enabled)) {
            
    /* if there is a valid enabled property, return if it is set to no */
    case FH_OK:
        if (!conn->enabled) return FH_OK;
        break;
        
    /* if the enabled property was not found log a warning (default = disabled) */
    case FH_ERR_NOTFOUND:
        FH_LOG(CSI, WARN, ("connection %s: missing property 'enabled'...disabling", name));
        return FH_ERR_NOTFOUND;
        
    /* any other error, warn about invalid value for property */
    default:
        FH_LOG(CSI, WARN, ("connection %s: property 'enabled' must be 'yes' or 'no'", name));
        return FH_ERROR;
    }
    
    /* 
     * if we have gotten here either the connection is explicitly enabled or the enabled
     * property was not present...set the address
     */
    property = fh_cfg_get_string(node, "address");
    if (property == NULL || !inet_aton(property, &address)) {
        FH_LOG(CSI, ERR, ("invalid ip address '%s' for %s connection", property, name));
        return FH_ERROR;
    }
    conn->address = address.s_addr;
    
    /* set the connection's port address */
    switch(fh_cfg_set_uint16(node, "port", &conn->port)) {

    case FH_OK:
        break;

    case FH_ERR_NOTFOUND:
        FH_LOG(CSI, WARN, ("connection %s: missing port address property", name));
        return FH_ERR_NOTFOUND;
        
    default:
        FH_LOG(CSI, ERR, ("connection %s: invalid port address", name));
        return FH_ERROR;
    }

    /* set the interface */
    property = fh_cfg_get_string(node, "interface");
    if (property == NULL) {
        FH_LOG(CSI, ERR, ("missing interface specification for %s connection", name));
        return FH_ERR_NOTFOUND;
    }
    strcpy(conn->interface, property);
    
    /* if we manage to get here, connection was enabled and configured */
    conn->enabled = 1;
    return FH_OK;
}                            


/**
 *  @brief (private) Add a line configuration to a process configuration
 *
 *  @param config the configuration structure that contains the config. information
 *  @param name the name of the line being added
 *  @param lh_config the line handler config that will receive the added line
 *  @return status code indicating success or failure
 */
static FH_STATUS add_line(const fh_cfg_node_t *config, const char *name,
                          fh_shr_cfg_lh_proc_t *lh_config)
{
    const fh_cfg_node_t  *lines_node    = NULL;
    const fh_cfg_node_t  *line_node     = NULL;
    fh_shr_cfg_lh_line_t *line;
    
    /* fetch the lines node */
    lines_node = fh_cfg_get_node(config, "lines");
    if (lines_node == NULL) {
        FH_LOG(CSI, ERR, ("missing configuration option 'lines'"));
        return FH_ERROR;
    }
    
    /* fetch the node for the specific line being added */
    line_node = fh_cfg_get_node(lines_node, name);
    if (line_node == NULL) {
        FH_LOG(CSI, ERR, ("missing configuration for line '%d'", name));
        return FH_ERROR;
    }
    
    /* allocate space for the new line */
    lh_config->lines = (fh_shr_cfg_lh_line_t *)realloc(lh_config->lines,
                        sizeof(fh_shr_cfg_lh_line_t) * (lh_config->num_lines + 1));
    if (lh_config->lines == NULL) {
        FH_LOG(CSI, ERR, ("unable to allocate memory for line '%s'", name));
        return FH_ERROR;
    }
    
    /* initialize the data for the new line */
    line = &lh_config->lines[lh_config->num_lines++];
    memset(line, 0, sizeof(fh_shr_cfg_lh_line_t));
    line->process = lh_config;
    strcpy(line->name, name);
    
    /* set up the connections for the new line */
    if (add_connection(line_node, "primary", &line->primary) != FH_OK) {
        line->primary.line = line;
        return FH_ERROR;
    }
    if (add_connection(line_node, "secondary", &line->secondary) != FH_OK) {
        line->secondary.line = line;
        return FH_ERROR;
    }
    
    /* if we get here, success! */
    return FH_OK;
}

/*
 * Load a line handler configuration structure from a general config structure
 */
FH_STATUS fh_shr_cfg_lh_load(const char *process, const char *root,
                             const fh_cfg_node_t *config, fh_shr_cfg_lh_proc_t *lh_config)
{
    char                  process_node_name[MAX_PROPERTY_LENGTH];
    int                   i;
    FH_STATUS             rc;
    const fh_cfg_node_t  *top_node     = NULL;
    const fh_cfg_node_t  *process_node  = NULL;
    const fh_cfg_node_t  *lines_node    = NULL;
    
    /* initialize the process configuration structure we have been passed */
    memset(lh_config, 0, sizeof(fh_shr_cfg_lh_proc_t));

    /* fetch a reference to the top level node that should contain all relevant configuration */
    top_node = fh_cfg_get_node(config, root);
    if (top_node == NULL) {
        FH_LOG(CSI, ERR, ("missing top configuration node, '%s'", root));
        return FH_ERROR;
    }

    /* build the full, expected node name of the process configuration and fetch the node */
    sprintf(process_node_name, "processes.%s", process);
    process_node = fh_cfg_get_node(top_node, process_node_name);
    if (process_node == NULL) {
        FH_LOG(CSI, ERR, ("no process configuration for '%s'", process));
        return FH_ERROR;
    }
    
    /* copy the process name into the process configuration structure */
    strcpy(lh_config->name, process_node->name);

    /* if a proper CPU specification has been made, set it, otherwise default to -1 */
    switch (fh_cfg_set_int(process_node, "cpu", &lh_config->cpu)) {

    case FH_OK:
        break;

    case FH_ERR_NOTFOUND:
        FH_LOG(CSI, WARN, ("%s: missing CPU specification", process));
        lh_config->cpu = -1;
        break;
        
    default:
        FH_LOG(CSI, WARN, ("%s: invalid CPU specification", process));
        lh_config->cpu = -1;
        break;
    }
    
    /* if a proper fill_gaps.max specification has been made, set it, otherwise default to 0 */
    switch (fh_cfg_set_int(top_node, "fill_gaps.max", &lh_config->gap_list_max)) {

    case FH_OK:
        break;

    case FH_ERR_NOTFOUND:
        lh_config->gap_list_max = 0;
        break;
        
    default:
        FH_LOG(CSI, WARN, ("%s: invalid fill_gaps.max option (default = 0)", process));
        lh_config->gap_list_max = 0;
        break;
    }
    
    /* if a proper fill_gaps.timeout specification has been made, set it, otherwise default to 0 */
    switch (fh_cfg_set_int(top_node, "fill_gaps.timeout", &lh_config->gap_timeout)) {

    case FH_OK:
        break;

    case FH_ERR_NOTFOUND:
        lh_config->gap_timeout = 0;
        break;
        
    default:
        FH_LOG(CSI, WARN, ("%s: invalid fill_gaps.timeout option (default = 0)", process));
        lh_config->gap_timeout = 0;
        break;
    }
    
    /* load table configurations */
    fh_shr_cfg_tbl_load(top_node, "symbol_table", &lh_config->symbol_table);
    fh_shr_cfg_tbl_load(top_node, "order_table", &lh_config->order_table);
    
    /* load lines node for this process */
    lines_node = fh_cfg_get_node(process_node, "lines");
    if (lines_node == NULL) {
        FH_LOG(CSI, ERR, ("process configuration '%s' contains no lines", process));
        return FH_ERROR;
    }
    
    /* go through every line entry, loading the line that corresponds */
    for (i = 0; i < lines_node->num_values; i++) {
        if (add_line(top_node, lines_node->values[i], lh_config) != FH_OK) {
            return FH_ERROR;
        }
    }
    
    /* allow a plugin the change to modify the loaded configuration */
    if (fh_plugin_is_hook_registered(FH_PLUGIN_CFG_LOAD)) {
        fh_plugin_get_hook(FH_PLUGIN_CFG_LOAD)(&rc, lh_config);
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("error occured during plugin configuration (%d)", rc));
            return rc;
        }
    }
    
    /* if we get here, success! */
    return FH_OK;
}

/**
 *  @brief Validate that the specified process exists *or* fetch the default process
 *
 *  @param process the process to validate or location in which to store the default process
 *  @param root node name under which all relevant (including process) configuration resides
 *  @param config configuration structure where we are getting process information
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_cfg_lh_get_proc(char *proc, const char *root, const fh_cfg_node_t *config)
{
    char                 node_name[MAX_PROPERTY_LENGTH];
    const fh_cfg_node_t *node = NULL;
    
    /* get the "<root>.processes" node */
    sprintf(node_name, "%s.processes", root);
    node = fh_cfg_get_node(config, node_name);

    /* if we are being asked to fetch the default process name, fetch the <root>.processes node and
       make sure it has at least one child, then copy the name into the provided process string */
    if (proc[0] == '\0') {
        if (node == NULL || node->num_children <= 0) {
            return FH_ERROR;
        }
        strcpy(proc, node->children[0]->name);
    }
    
    /* if we are being asked to validate a process name, build the full node name and make sure */
    /* that node exists */
    else {
        if (fh_cfg_get_node(node, proc) == NULL) {
            return FH_ERROR;
        }
    }
    
    /* if we get here, success! */
    return FH_OK;
}
