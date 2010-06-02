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
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

/* FH common headers */
#include "fh_config.h"
#include "fh_util.h"
#include "fh_log.h"
#include "fh_plugin_internal.h"

/* FH shared "other" headers */
#include "fh_shr_config.h"
#include "fh_shr_cfg_cmdline.h"
#include "fh_shr_cfg_lh.h"
#include "fh_shr_mgmt.h"
#include "fh_shr_lh.h"

/* FH shared mmcast headers */
#include "fh_shr_mmcast.h"

/* static mmcast-global data */
static fh_info_proc_t proc_info;

/**
 *  @brief Exit the feed handler gracefully giving all threads a change to do any required cleanup
 */
void fh_shr_mmcast_exit()
{
    fh_shr_mgmt_exit();
    fh_shr_lh_exit();
}

/** 
 *  Signal handler for the main feed handler thread (parent process).  Sends SIGINT signals
 *  to the other threads (management and line handler) to exit which will cause the entire
 *  process to terminate.
 */
static void fh_shr_mmcast_sig_handle(int32_t signo)
{
    /* handle signals, printing an appropriate log message and returning, where appropriate */
    switch (signo) {
    case SIGTERM:
        FH_LOG(CSI, WARN, ("SIGTERM caught - exiting - "));
        break;

    case SIGHUP:
        FH_LOG(CSI, WARN, ("SIGHUP caught - exiting - "));
        break;

    case SIGINT:
        FH_LOG(CSI, WARN, ("SIGINT caught - exiting - "));
        break;

    default:
        FH_LOG(CSI, WARN, ("unhandled signal received: %d", signo));
    }

    /* send signals to the management and line handler threads to exit */
    fh_shr_mmcast_exit();
}

/**
 *  @brief Initialize signal handling for this process
 */
static void fh_shr_mmcast_sig_init()
{
    if (signal(SIGINT, fh_shr_mmcast_sig_handle) == SIG_ERR) {
        FH_LOG(LH, ERR, ("unable to register for SIGINT...exiting"));
        exit(1);
    }

    if (signal(SIGHUP, fh_shr_mmcast_sig_handle) == SIG_ERR) {
        FH_LOG(LH, ERR, ("unable to register for SIGHUP...exiting"));
        exit(1);
    }

    if (signal(SIGTERM, fh_shr_mmcast_sig_handle) == SIG_ERR) {
        FH_LOG(LH, ERR, ("unable to register for SIGTERM...exiting"));
        exit(1);
    }
}

/**
 *  @brief Dump revision and build information
 *
 *  @param info feed handler version, build, etc. information
 *  @param options structure of options to this process
 */
void fh_shr_mmcast_version(const fh_info_build_t *info, fh_shr_cfg_options_t *options)
{
    /* log a standard version message */
    fh_info_version_msg(info);

    /* if we were asked to display the version only, just exit */
    if (options->display_version) {
        exit(0);
    }
}

/*
 * Return the collected status of various aspects of the feed handler
 */
fh_info_proc_t *fh_shr_mmcast_status()
{
    /* if the currently set LH thread id is invalid, try to fetch it again (sometimes it is
     * not available by the time the proc_info struct is set up) */
    if (proc_info.tid <= 0) {
        proc_info.tid = fh_shr_lh_get_tid();
    }
    return &proc_info;
}

/*
 * "Real" main function for mirrored multicast feed handlers
 */
int fh_shr_mmcast_main(int argc, char **argv, const char *cfg_tag,
                       const fh_info_build_t *info, fh_shr_mmcast_cb_t *cb)
{    
    fh_shr_cfg_options_t     options;
    fh_shr_cfg_lh_proc_t     process_config;
    fh_cfg_node_t           *config;
    char                    *thread_name = NULL;
    
    /* build structure of management callbacks */
    fh_shr_mgmt_cb_t mgmt_callbacks = {
        fh_shr_mmcast_exit,
        fh_shr_lh_get_stats,
        fh_shr_lh_clear_stats,
        fh_shr_lh_snap_stats,
        fh_shr_lh_latency,
        fh_shr_mmcast_status
    };
    
    /* build structure of line handler callbacks */
    fh_shr_lh_cb_t lh_callbacks = {
        cb->init,
        cb->parser
    };
    
    /* set default values for options (to later be modified by command line and config file) */
    fh_shr_cfg_set_defaults(&options, info->name, info->version);
    
    /* parse command line arguments [ depends on ...set_defaults() ] */
    fh_shr_cfg_cmd_parse(argc, argv, info->name, &options);
    
    /* daemonize if not in debug mode [ depends on ...cmd_parse() ] */
    if (!options.debug_level) {
        fh_daemonize();
    }
    
    /* load the configuration file [ depends on ...cmd_parse() ] */
    config = fh_cfg_load(options.config_file);
    if (config == NULL) {
        fprintf(stderr, "ERROR: loading confguration file: %s\n", options.config_file);
        exit(1);
    }
    
    /* initialize logging subsystem [ depends on fh_cfg_load() ] */
    if (fh_shr_cfg_log_init(&options, config) != FH_OK) {
        fprintf(stderr, "ERROR: initializing logging configuration\n");
        exit(1);
    }
    
    /* load any plugins that are in the plugin directory [ depends on ...log_init() ] */
    fh_plugin_load(options.plugin_path);
        
    /* figure out what process configuration to use [ depends on ...log_init() & ...cfg_load() ] */
    if (fh_shr_cfg_lh_get_proc(options.process, cfg_tag, config) != FH_OK) {
        FH_LOG(CSI, ERR, ("invalid process specified or no default process available"));
        exit(1);
    }
    
    /* generate a process configuration structure from parsed configuration nodes */
    if (fh_shr_cfg_lh_load(options.process, cfg_tag, config, &process_config) != FH_OK) {
        FH_LOG(CSI, ERR, ("unable to parse configuration into line handler process configuration"));
        exit(1);
    }
            
    /* free the configuration structure (NO USE OF CONFIG DATA AFTER THIS POINT) */
    fh_cfg_free(config);
    
    /* display some info about the binary [ depends on ...log_init() ] */
    fh_shr_mmcast_version(info, &options);
        
    /* initialize signal handling [ depends on ...log_init() ] */
    fh_shr_mmcast_sig_init();
    
    /* allocate space for, generate, and log a "thread started" message for this thread's name */
    thread_name = fh_util_thread_name("Main", options.process);
    fh_log_thread_start(thread_name);
    
    /* start up the line handler thread */
    if (fh_shr_lh_start(info, &process_config, &lh_callbacks) != FH_OK) {
        FH_LOG(CSI, ERR, ("error starting line handler thread for '%s'", options.process));
        exit(1);
    }
    
    /* start up the management thread */
    if (fh_shr_mgmt_start(options.standalone, info, options.process, &mgmt_callbacks) != FH_OK) {
        FH_LOG(CSI, ERR, ("error starting management thread for '%s'", options.process));
        exit(1);
    }
    
    /* set up values for this process's info structure */
    memset(&proc_info, 0, sizeof(fh_info_proc_t));
    proc_info.pid           = getpid();
    proc_info.tid           = fh_shr_lh_get_tid();
    proc_info.cpu           = process_config.cpu;
    proc_info.start_time    = time(NULL);
    
    /* wait for all threads to exit */
    fh_shr_mgmt_wait();
    fh_shr_lh_wait();

    /* log a "thread stopped" message for the main thread */
    fh_log_thread_stop(thread_name);
    
    /* if we get here, return success code */
    return 0;
}
