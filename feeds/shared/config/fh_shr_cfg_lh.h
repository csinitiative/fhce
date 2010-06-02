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

#ifndef FH_SHR_CFG_LH_H
#define FH_SHR_CFG_LH_H

/* System headers */
#include <stdint.h>

/* FH common headers */
#include "fh_config.h"

/* shared FH module headers */
#include "fh_shr_cfg_table.h"

/* some convenience typedefs to avoid having to use the "struct" keyword all over the place */
typedef struct fh_shr_cfg_lh_proc fh_shr_cfg_lh_proc_t;
typedef struct fh_shr_cfg_lh_line fh_shr_cfg_lh_line_t;
typedef struct fh_shr_cfg_lh_conn fh_shr_cfg_lh_conn_t;

/* structure that contains the configuration for a line handler connection */
struct fh_shr_cfg_lh_conn {
    fh_shr_cfg_lh_line_t        *line;
    uint8_t                      enabled;
    uint16_t                     port;
    uint32_t                     address;
    char                         login_name[6];
    char                         login_passwd[10];
    char                         interface[MAX_PROPERTY_LENGTH];
    void                        *context;
};

/* structure that contains the configuration for a line handler line */
struct fh_shr_cfg_lh_line {
    fh_shr_cfg_lh_proc_t        *process;
    char                         name[MAX_PROPERTY_LENGTH];
    fh_shr_cfg_lh_conn_t         primary;
    fh_shr_cfg_lh_conn_t         secondary;
    void                        *context;
};

/* structure that contains the configuration for a line handler process */
struct fh_shr_cfg_lh_proc {
    char                         name[MAX_PROPERTY_LENGTH];
    int                          cpu;
    fh_shr_cfg_lh_line_t        *lines;
    int                          num_lines;
    int                          gap_list_max;
    int                          gap_timeout;
    fh_shr_cfg_tbl_t             symbol_table;
    fh_shr_cfg_tbl_t             order_table;
    void                        *context;
};

/**
 *  @brief Load a line handler configuration structure from a general config structure
 *
 *  @param process the process that is being run (and whose configuration is being loaded)
 *  @param root node name (the name of the node under which all relevant configuration resides)
 *  @param config a configuration node from which to load the line handler configure
 *  @param lh_config the line handler structure in which to load the configuration
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_cfg_lh_load(const char *process, const char *root,
                             const fh_cfg_node_t *config, fh_shr_cfg_lh_proc_t *lh_config);

/**
 *  @brief Validate that the specified process exists *or* fetch the default process
 *
 *  @param process the process to validate or location in which to store the default process
 *  @param root node name under which all relevant (including process) configuration resides
 *  @param config configuration structure where we are getting process information
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_cfg_lh_get_proc(char *proc, const char *root, const fh_cfg_node_t *config);

/**
 *  @brief Load a (TCP) line handler configuration structure from a general config structure
 *
 *  @param process the process that is being run (and whose configuration is being loaded)
 *  @param config a configuration node from which to load the line handler configure
 *  @param lh_config the line handler structure in which to load the configuration
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_cfg_tcp_lh_load(const char *process, const fh_cfg_node_t *config,
                                 fh_shr_cfg_lh_proc_t *lh_config);

/**
 *  @brief Validate that the specified (TCP) process exists *or* fetch the default process
 *
 *  @param process the process to validate or location in which to store the default process
 *  @param config configuration structure where we are getting process information
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_cfg_tcp_lh_get_proc(char *process, const fh_cfg_node_t *config);

#endif  /* FH_SHR_CFG_LH_H */
