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
/* file: fh_arca.c                                                   */
/* Usage: Process Manager for the Arca Multicast feed handler        */
/* Author: Wally Matthews of  Collaborative Software Initiative      */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <stdio.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <errno.h>

// Common FH headers
#include "fh_errors.h"
#include "fh_util.h"
#include "fh_log.h"
#include "fh_cpu.h"
#include "fh_plugin.h"
#include "fh_config.h"

// Arca FH headers
#include "fh_arca.h"
//#include "fh_arca_revision.h"
#include "fh_arca_cfg.h"
#include "fh_arca_mgmt.h"
#include "fh_arca_lh.h"
#include "fh_arca_util.h"
//TODO - these will need to be factored out into more modular header files
#include "fh_arca_headers.h"
#include "fh_arca_constants.h"

// default path constants
#define FH_HOME                   "/opt/csi/fh"
// plugin directory comes from call to specific feedhandler code
// config file path comes from call to specific feedhandler code

// global data
struct arca_process_args     fh_arca_proc_args;
sem_t                        fh_arca_thread_init;
int                          fh_arca_stopped    = 0;

// static data
static char                 *pname              = "?";
static char                 *fh_home            = NULL;
static char                  logging_tag[16]    = "";

// variables to store the arguments (default or command line specified) to the program
static int                   args_standalone    = 0;
static int                   args_debug         = 0;
static int                   args_version       = 0;
static char                 *args_process       = NULL;

// paths used in this module
//  these paths are filled via calls to feedhandler specific 
//  polymorphic functions (ie. see fh_arcabook.c)
static char                  arca_plugins_dir[MAXPATHLEN];
static char                  arca_config_file[MAXPATHLEN];

// plugin hooks called by functions in this shared file
//  some of these hooks may not apply 
//  for example, arcatrade has no hooks for firm table and order table
static fh_plugin_hook_t      init_hook;
static fh_plugin_hook_t      shutdown_hook;
static fh_plugin_hook_t      config_tables_hook;
static fh_plugin_hook_t      build_symbol_table_hook;
static fh_plugin_hook_t      free_symbol_table_hook;
static fh_plugin_hook_t      build_firm_table_hook;
static fh_plugin_hook_t      free_firm_table_hook;
static fh_plugin_hook_t      build_order_table_hook;
static fh_plugin_hook_t      free_order_table_hook;

/*! \brief Print the usage message for the Arca feed handler
 */
// currently not specialized for arcabook vs arcatrade
static void fh_arca_usage()
{
    fprintf(stderr,
            "Usage: %s [OPTION]\n"
            "Start the ArcaBook Multicast feed handler\n\n"
            " -h, -?            Print this usage message\n"
            " -d                Debugging mode (prints all log messages to console)\n"
            " -s                Standalone mode (do not attach to central FH manager)\n"
            " -p <process>      Process configuration to use\n"
            " -v                Display the version information\n"
            "\n",
            pname);
    exit(1);
}

/*! \brief Validate command line arguments
 *
 *  \return status code indicating success or failure
 */
static FH_STATUS fh_arca_validate_args()
{
    struct stat stat_buf;

    if (stat(arca_config_file, &stat_buf) < 0) {
        fprintf(stderr, "ERROR: Arca configuration file doesn't exist: %s\n", arca_config_file);
        return FH_ERROR;
    }
    
    return FH_OK;
}

/*! \brief Parse Arca feed handler command line arguments
 *
 *  \param argc argument count (generally passed from main function)
 *  \param argv argument array (generally passed from main function)
 *  \return status code indicating success or failure
 */
static FH_STATUS fh_arca_parse_args(int argc, char **argv)
{
    int          op;
    extern char *optarg;

    while ((op = getopt(argc, argv, "h?dsp:v")) != EOF) {

        switch (op) {
        case 'h':
        case '?':
            fh_arca_usage();
            break;

        case 'd':
            args_debug++;
            break;

        case 's':
            args_standalone = 1;
            break;
            
        case 'p':
            args_process = (char *)malloc(sizeof(optarg));
            if (!args_process) {
                fprintf(stderr, "ERROR: unable to allocate memory for process identifier.\n");
                exit(1);
            }
            strcpy(args_process, optarg);
            break;
    
        case 'v':
            args_version = 1;
            break;

        default:
            fprintf(stderr, "ERROR: Unknown command line option: %c\n", op);
            return FH_ERROR;
        }
    }
    
    if (!args_version) {
        fh_arca_validate_args();
    }

    return FH_OK;
}

/*! \brief Initialize the logging layer for this process
 *
 *  \param config configuration data
 *  \param process_name unique name for this process
 *  \return success or failure
 */
FH_STATUS fh_arca_log_init(fh_cfg_node_t *config)
{
    int rc = 0;
    
    // start up logging
    fh_log_open();
    
    // conditionally set logging to output to the command line
    if (args_debug || args_version) {
        fh_log_set_cfg(FH_LCF_CONSOLE);
    }

    // increase amount of logging when debug flag is set
    if (args_debug > 0) {
        fh_log_set_lvl(FH_LL_STATS);
    }
    if (args_debug > 1) {
        fh_log_set_lvl(FH_LL_DIAG | FH_LL_VSTATE);
    }
    
    // set the logging identifier to this process's name
    fh_log_set_ident(logging_tag);

    // load extra logging configuration from process config file 
    rc = fh_log_cfg_load(config);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to load logging configuration from '%s'", arca_config_file));
        return rc;
    }
    
    // if we get here, return success
    return FH_OK;
}

/*! Cache plugin hooks that will be called by functions in this file
 */
static void fh_arca_cache_hooks()
{
    init_hook               = fh_plugin_get_hook(FH_PLUGIN_ARCA_INIT);
    shutdown_hook           = fh_plugin_get_hook(FH_PLUGIN_ARCA_SHUTDOWN);
    config_tables_hook      = fh_plugin_get_hook(FH_PLUGIN_CONFIG_TABLE);
    build_symbol_table_hook = fh_plugin_get_hook(FH_PLUGIN_BUILD_SYM_TABLE);
    free_symbol_table_hook  = fh_plugin_get_hook(FH_PLUGIN_FREE_SYM_TABLE);
    build_firm_table_hook   = fh_plugin_get_hook(FH_PLUGIN_BUILD_FIRM_TABLE);
    free_firm_table_hook    = fh_plugin_get_hook(FH_PLUGIN_FREE_FIRM_TABLE);
    build_order_table_hook  = fh_plugin_get_hook(FH_PLUGIN_BUILD_ORDER_TABLE);
    free_order_table_hook   = fh_plugin_get_hook(FH_PLUGIN_FREE_ORDER_TABLE);
}

/*! \brief Signal handler for all signals that this process will receive
 *
 *  \param signo the signal that is being handled
 */
static void fh_arca_sig_handle(int32_t signo)
{
    switch (signo) {
    case SIGTERM:
        FH_LOG(MGMT, WARN, ("SIGTERM caught - exiting - "));
        fh_arca_stopped = 1;
        break;

    case SIGHUP:
        FH_LOG(MGMT, WARN, ("SIGHUP caught - exiting - "));
        fh_arca_stopped = 1;
        break;

    case SIGINT:
        FH_LOG(MGMT, WARN, ("SIGINT caught - exiting - "));
        fh_arca_stopped = 1;
        break;

    default:
        FH_LOG(MGMT, WARN, ("Unhandled Signal received: %d", signo));
    }
}

/*! \brief Initialize signal handling for this process
 */
void fh_arca_sig_init()
{
    if (signal(SIGINT, fh_arca_sig_handle) == SIG_ERR) {
        FH_LOG(LH, ERR, ("Init for SIGINT failed"));
    }

    if (signal(SIGHUP, fh_arca_sig_handle) == SIG_ERR) {
        FH_LOG(LH, ERR, ("Init for SIGHUP failed"));
    }

    if (signal(SIGTERM, fh_arca_sig_handle) == SIG_ERR) {
        FH_LOG(LH, ERR, ("Init for SIGTERM failed"));
    }
}

/*! \brief Build the sockets sets that the process will need to get data
 *
 *  \param p_map structure to map process to line
 *  \param main_sockets primary/secondary socket pair
 *  \param refresh_sockets primary/secondary refresh socket pair
 *  \param retrans_sockets primary/secondary reftrans socket pair
 *  \param 
 */
void fh_arca_socket_init(struct process_maps *p_map, struct socket_set *main_sockets,
                         struct socket_set *refresh_sockets, struct socket_set *retrans_sockets,
                         struct arca_process_args *p_args)
{
    int                  i;
    struct feed_group   *group = NULL;

    if(refresh_sockets) {}
    // add socket sets to the process argument structure
    p_args->main_sockets            = main_sockets;
    p_args->retrans_sockets         = retrans_sockets;
    
    // set cpu affinities in the process argument structure
    p_args->feed_cpu                = p_map->feed_cpu;
    p_args->refresh_cpu             = p_map->refresh_cpu;
    p_args->retrans_cpu             = p_map->retrans_cpu;
    
    // initialize the socket counts
    main_sockets->socket_count      = 0;
    refresh_sockets->socket_count   = 0;
    retrans_sockets->socket_count   = 0;
    
    // associate per-line feed group structures with the associated socket sets
    for (i = 0; i < p_map->line_count; i++) {
        group = p_map->groups[i];
        main_sockets->feeds[main_sockets->socket_count] = group;
        main_sockets->primary_or_secondary[main_sockets->socket_count] = 0;
        main_sockets->socket_count++;

        if (group->secondary_mcast_port > 0) {
            main_sockets->feeds[main_sockets->socket_count] = group;
            main_sockets->primary_or_secondary[main_sockets->socket_count] = 1;
            main_sockets->socket_count++;
        }
        
        if (group->primary_retran_mcast_port > 0) {
            retrans_sockets->feeds[retrans_sockets->socket_count] = group;
            retrans_sockets->primary_or_secondary[retrans_sockets->socket_count] = 0;
            retrans_sockets->socket_count++;
        }

        if (group->secondary_retran_mcast_port > 0) {
            retrans_sockets->feeds[retrans_sockets->socket_count] = group;
            retrans_sockets->primary_or_secondary[retrans_sockets->socket_count] = 1;
            retrans_sockets->socket_count++;
        }
    }
}

/*! \brief Build reference plugin tables
 *
 *  \param p_map structure mapping processes -> lines
 */
static void fh_arca_build_tables(struct process_maps *p_map)
{
    FH_STATUS   rc = FH_OK;

    // call hook to configure table sizes if one is registered
    if (config_tables_hook) {
        config_tables_hook(&rc, &arca_config_file[0], &p_map->max_symbols, 
            &p_map->max_sessions,
            &p_map->max_firms,
            &p_map->max_orders);
        if (rc != FH_OK) {
            FH_LOG(LH, ERR, ("table size configuration failed (%d)", rc));
        }
    }
        
    // call hook to build the symbol table if one is registered
    if (build_symbol_table_hook) {
        build_symbol_table_hook(&rc, p_map->max_symbols, p_map->max_sessions);
        if (rc != FH_OK) {
            FH_LOG(LH, ERR, ("symbol map build failed (%d)", rc));
        }
    }
    
    // call hook to build the firm table if one is registered
    if (build_firm_table_hook) {
        build_firm_table_hook(&rc, p_map->max_firms);
        if (rc != FH_OK) {
            FH_LOG(LH, ERR, ("firm map build failed (%d)", rc));
        }
    }

    // call hook to build the order table if one is registered
    if (build_order_table_hook) {
        build_order_table_hook(&rc, p_map->max_orders);
        if (rc != FH_OK) {
            FH_LOG(LH, ERR, ("order table build failed (%d)", rc));
        }
    }    
}

/*! \brief Give plugins a chance to clean up after themselves
 */
static void fh_arca_free_tables() {
    FH_STATUS rc;
    
    // call any registered cleanup plugin hooks
    if (free_symbol_table_hook) {
        free_symbol_table_hook(&rc);
    }
    if (free_firm_table_hook) {
        free_firm_table_hook(&rc);
    }
    if (free_order_table_hook) {
        free_order_table_hook(&rc);
    }
}

/*! \brief Get the default (first) process from the config file
 *
 *  \param config config structure to look in
 *  \return status code indicating success or failure
 */
FH_STATUS fh_arca_default_process(fh_cfg_node_t *config)
{
    const fh_cfg_node_t *processes = NULL;
    
    // get the arca.processes node and return FH_ERROR if there are no processes configured
    processes = fh_cfg_get_node(config, "arca.processes");
    if (processes == NULL || processes->num_children <= 0) return FH_ERROR;
        
    // set args_process to the name of "arca.processes" first child
    args_process = (char *)malloc((strlen(processes->children[0]->name) + 1) * sizeof(char));
    if (args_process == NULL) {
        fprintf(stderr, "ERROR: unable to allocate memory for process name\n");
        return FH_ERROR; 
    }
    strcpy(args_process, processes->children[0]->name);
    
    // return success
    return FH_OK;
}

/*! \brief Check to make sure that a configuration exists for the selected process
 *
 *  \param config configuration structure to check
 *  \return status code indicating whether or not the selected process is configured
 */
FH_STATUS fh_arca_process_exists(fh_cfg_node_t *config)
{    
    const fh_cfg_node_t *processes = NULL;

    // check for an arca.processes property and a child of that property with the name of the
    // configured process
    processes = fh_cfg_get_node(config, "arca.processes");
    if (processes == NULL || fh_cfg_get_node(processes, args_process) == NULL) return FH_ERROR;
    
    // if we get to the end of the function the process does exists
    return FH_OK;
}

/*! \brief Entry point for all Arca feed handlers
 *
 *  \param argc number of command line arguments
 *  \param argv content of command line arguments
 *  \param version Arca feed handler version
 *  \return boolean value indicating success or failure
 */
int fh_arca_main(int argc, char *argv[], int version)
{
    struct process_maps          p_map;
    struct socket_set            main_sockets, refresh_sockets, retrans_sockets;
    struct stat                  plugin_desc;
    char                        *thread_name = NULL;
    FH_STATUS                    rc          = 0;
    fh_cfg_node_t               *config      = NULL;
    int                          i;
    
    // initialize variables
    memset(&fh_arca_proc_args, 0, sizeof(struct arca_process_args));
    memset(&p_map, 0, sizeof(struct process_maps));
    memset(&main_sockets, 0, sizeof(struct socket_set));
    memset(&refresh_sockets, 0, sizeof(struct socket_set));
    memset(&retrans_sockets, 0, sizeof(struct socket_set));

    // get the program name by removing any path information
    FH_PNAME_GET(pname, argv);

    // override default FH_HOME if the FH_HOME env. variable is set and build FH_HOME rooted paths
    if ((fh_home = getenv("FH_HOME")) == NULL) {
        fh_home = FH_HOME;
    }
    sprintf(arca_plugins_dir, "%s/%s", fh_home, get_plugin_dir_name());
    sprintf(arca_config_file, "%s/%s", fh_home, get_config_path());

    // parse command line arguments
    rc = fh_arca_parse_args(argc, argv);
    if (rc != FH_OK) {
        fh_arca_usage();
    }
    
    // daemonize if not in debug mode
    if (!args_debug) {
        fh_daemonize();
    }
    
    // get the default config file
    config = fh_cfg_load(arca_config_file);
    if (config == NULL) {
        fprintf(stderr, "Failed to load confguration file: %s\n", arca_config_file);
        goto main_loop_exit;
    }
    
    // if a process was specified on the command line, make sure such a process config exists
    if (args_process != NULL && fh_arca_process_exists(config) != FH_OK) {
        fprintf(stderr, "ERROR: specified process %s does not exists\n", args_process);
        fh_cfg_free(config);
        goto main_loop_exit;
    }
    
    // if no process was specified on the command line, get the default process instead
    if (args_process == NULL && fh_arca_default_process(config) != FH_OK) {
        fprintf(stderr, "ERROR: failed to load a process configuration\n");
        fh_cfg_free(config);
        goto main_loop_exit;
    }
    
    // initialize logging -> depends on fh_cfg_load()
    rc = fh_arca_log_init(config);
    if (rc != FH_OK) {
        fh_cfg_free(config);
        goto main_loop_exit;
    }
    
    // parse config data into Arca process configuration structure
    rc = fh_arca_cfg_load(config, args_process);
    if (rc != FH_OK) {
        fh_cfg_free(config);
        goto main_loop_exit;
    }
    
    //TODO - replace use of this configuration system with new one (see previous chunk of code)
    // for now get_config() will fill the p_map structure using data from the new configuration
    // structure
    strcpy(fh_arca_proc_args.process_name, args_process);
    fh_arca_proc_args.arca_version = version;
    if (get_config(config, &p_map) != FH_OK) {
        FH_LOG(MGMT, ERR, ("configuration failed"));
        fh_cfg_free(config);
        goto main_loop_exit;
    }
    
    // free the configuration structure
    fh_cfg_free(config);
    
    // initialize signal handling -> depends on fh_arca_log_init()
    fh_arca_sig_init();
    
    // dump some information about the program
    fh_arca_version_info(version);
    if(args_version) exit(0); //only looking for version info
    
    // allocate space for, generate, and log a "thread started" message for this thread's name
    thread_name = fh_arca_util_thread_name("Main");
    fh_log_thread_start(thread_name);
    
    // load plugins and cache any hooks that this module will call
    if ((stat(arca_plugins_dir,&plugin_desc))==0) {
        fh_plugin_load(arca_plugins_dir);
        fh_arca_cache_hooks();
        fh_arca_msgparse_cache_hooks();
        fh_arca_pub_cache_hooks();
    }
        
    // build the tables to size and initialize messaging plugins
    fh_arca_build_tables(&p_map);
    chk_cfg(&p_map);
    if (init_hook) {
        init_hook(&rc, p_map.line_count, &p_map.groups);
    }
    
    // build the set of sockets that the line handler will use to receive data
    fh_arca_socket_init(&p_map, &main_sockets, &refresh_sockets, &retrans_sockets,
                        &fh_arca_proc_args);
                      
    // join multicast groups
    if (init_mcast_sockets(&fh_arca_proc_args) != FH_OK) {
        goto main_loop_exit;
    }
    
    // create request sockets; not connected yet
    if (init_request_sockets(&fh_arca_proc_args) != FH_OK) {
        goto main_loop_exit;
    }
    
    // initialize a semaphore to ensure that only one thread is initialized at a time
    if(sem_init(&fh_arca_thread_init, 0, 1) == -1) {
        FH_LOG(MGMT, ERR, ("failed to init thread_init semaphore: %s", strerror(errno)));
        goto main_loop_exit;
    } 
        
    // start the management framework
    rc = fh_arca_mgmt_start(args_standalone);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to start management sub-system"));
        goto main_loop_exit;
    }

    // create the main line handler thread
    rc = fh_arca_lh_start();
    if (rc != FH_OK) {
        FH_LOG(CSI, ERR, ("Failed to start line-handler sub-system"));
        goto main_loop_exit;
    }
    
    // wait for all threads to exit
    fh_arca_mgmt_wait();
    fh_arca_lh_wait();

main_loop_exit:
    //tell all threads to end and give them a second to do so
    fh_arca_stopped = 1;
    sleep(1);
    
    // give messaging system plugin a chance to clean up
    if (shutdown_hook) {
        shutdown_hook(&rc);
    }
    
    // clean up reference plugin tables
    fh_arca_free_tables();  
    
    // shut down logging
    fh_log_thread_stop(thread_name);
    fh_log_close();
    
    // free all the groups in the p_map
    for (i = 0; i < p_map.line_count; i++) {
        destroy_group(p_map.groups[i]);
    }
    
    // return the last return code we have seen (0 if we traversed the main loop without incident)
    return rc;
}
