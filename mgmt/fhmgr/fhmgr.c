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

/*
 * System includes
 */
#include <stdio.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include <sys/param.h>

/*
 * Common includes
 */
#include "queue.h"
#include "fh_util.h"
#include "fh_log.h"
#include "fh_net.h"
#include "fh_tcp.h"
#include "fh_time.h"
#include "fh_plugin.h"

/*
 * FH Management includes
 */
#include "fh_mgmt_admin.h"
#include "fh_mgmt_revision.h"
#include "fh_mgmt_client.h"
#include "fh_mgmt_conn.h"
#include "fh_mgmt_service.h"
#include "fh_mgmt_cfg.h"

/*
 * Local static variables
 */
static char *pname = NULL;
static int   debug = 0;
static fh_plugin_hook_t mgmt_init_complete = NULL;

/*
 * Make sure that we wake-up enough to catch up the restart command
 */
#define FH_MGR_HZ    (1)
#define FH_MGR_SLEEP (1000000 / FH_MGR_HZ)

#define FH_HOME        "/opt/csi/fh"
#define FH_MGR_CONFIG  "mgmt/etc/fhmgr.conf"
#define FH_MGR_PLUGINS "mgmt/plugins"

/*
 * Defaults
 */
static char *fh_home = NULL;
static char  fhmgr_config_file[MAXPATHLEN];
static char  fhmgr_plugins_dir[MAXPATHLEN];

/*
 * fh_mgr_run
 *
 * FH Manager server
 */
static void *fh_mgr_run(void *arg)
{
    FH_STATUS  rc;
    int        srv_fd = 0;
    int        ticks = 0;
    uint64_t   timeout = (uint64_t)1000000;

    FH_ASSERT(arg == NULL);

    fh_log_thread_start("FH_Mgr");

    /*
     * Initialize the connection library
     */
    fh_mgmt_conn_init();

    /*
     * Initialize the admin message library
     */
    rc = fh_adm_init();
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to initialize admin message library"));
        return NULL;
    }

    /*
     * Start the TCP server for client connections
     */
    rc = fh_tcp_server(INADDR_ANY, FH_MGR_PORT, &srv_fd);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to start the FH Manager on port:%d", FH_MGR_PORT));
        return NULL;
    }

    /*
     * Create the new server connection entry
     */
    rc = fh_mgmt_conn_new(FH_MGMT_CONN_SERVER, srv_fd, INADDR_ANY, FH_MGR_PORT);
    if (rc != FH_OK) {
        FH_LOG(MGMT, WARN, ("failed to create the FH Manager connection"));
        return NULL;
    }

    /*
     * Call the management init complete call back
     * to make sure we can accept and manage other
     * services if that plugin was needed.
     */
    if (mgmt_init_complete) {
        FH_LOG(MGMT, STATE, ("Plugin Management initialization complete"));
        mgmt_init_complete(&rc);
        if (rc != FH_OK) {
             FH_LOG(MGMT, WARN, ("Plugin Management initialization complete failed"));
        }
    }

    while (1) {
        ticks++;

        /*
         * Enter the select loop for all connections
         */
        fh_mgmt_conn_loop(timeout);

        /*
         * 1sec-timer handling
         */
        if ((ticks % FH_MGR_HZ) == 0) {
            fh_mgmt_sg_timers();
        }
    }

    fh_log_thread_stop("FH_Mgr");

    return NULL;
}

/*
 * fh_mgr_usage
 *
 * Dump the command line paramaters and help message.
 */
static void fh_mgr_usage()
{
  printf("Usage: %s ARGS\n\n"
         "   -d             Debug mode (doesn't daemonize)\n"
         "   -h?            Display this help message\n", pname);
  exit(1);
}

/*
 * fh_mgr_parse_args
 *
 * Parse command line arguments
 */
FH_STATUS fh_mgr_parse_args(int argc, char *argv[])
{
    extern int   optind;  /* index of first unused arg */
    extern char *optarg;  /* pointer to option string  */
    int          c;

    while ((c = getopt(argc, argv, "dh?")) != EOF) {
        switch (c) {
        case 'd':
            debug = 1;
            break;

        case '?':
        case 'h':
        default:
            return FH_ERROR;
        }
    }

    return FH_OK;
}


/*
 * main - Main program
 */
int main(int argc, char *argv[])
{
    fh_cfg_node_t *config = NULL;
    pthread_t tid;
    FH_STATUS rc;
    fh_plugin_hook_t mgmt_init = NULL;

    FH_PNAME_GET(pname, argv);

    /*
     * Parse the command line arguments
     */
    rc = fh_mgr_parse_args(argc, argv);
    if (rc != FH_OK) {
        fh_mgr_usage();
    }

    if ((fh_home = getenv("FH_HOME")) == NULL) {
        fh_home = FH_HOME;
    }
    sprintf(fhmgr_config_file, "%s/%s", fh_home, FH_MGR_CONFIG);
    sprintf(fhmgr_plugins_dir, "%s/%s", fh_home, FH_MGR_PLUGINS);

    /*
     * Load the plugins
     */
    if (fhmgr_plugins_dir[0] != '\0') {
        fh_plugin_load(fhmgr_plugins_dir);
    }

    /*
     * Load the configuration
     */
    config = fh_cfg_load(fhmgr_config_file);
    if (!config) {
        fprintf(stderr, "ERROR: Failed to load FH Manager configuration: %s", fhmgr_config_file);
        return 1;
    }

    /*
     * Load the FH manager configuration
     */
    fh_log_open();

    if (debug) {
        fh_log_set_cfg(FH_LCF_CONSOLE);
        fh_log_set_class(FH_LC_MGMT, FH_LL_VSTATE|FH_LL_WARN|FH_LL_DIAG);
    }
    else {
        fh_daemonize();
    }

    fh_log_set_ident("FHMgr");

    /*
     * Load the logging sub-system configuration from the FH configuration file
     */
    rc = fh_log_cfg_load(config);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to load logging configuration from file: %d", rc));
        return 1;
    }

    /*
     * Load the FH Manager configuration
     */
    rc = fh_mgmt_cfg_load(config);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to load FH Manager configuration from file: %d", rc));
        return 1;
    }

    /*
     * Load the plugins for the management interface
     */
    mgmt_init = fh_plugin_get_hook(FH_PLUGIN_MGMT_INIT);
    if (mgmt_init) {
        FH_LOG(MGMT, STATE, ("Plugin Management initialization"));
        mgmt_init(&rc);
        if (rc != FH_OK) {
            return 1;
        }
    }

    /* Perform initialization post-processing to deal with any plugin configuration changes */
    fh_mgmt_serv_post_init();

    mgmt_init_complete = fh_plugin_get_hook(FH_PLUGIN_MGMT_INIT_COMPLETE);
    if (mgmt_init_complete) {
        FH_LOG(MGMT, STATE, ("Plugin Management initialization complete"));
    }

    /*
     * Start the FH Manager connection engine
     */
    if (pthread_create(&tid, NULL, &fh_mgr_run, NULL) != 0) {
        FH_LOG(MGMT, ERR, ("failed to start FH Manager thread"));
        return 1;
    }

    pthread_join(tid, NULL);

    return 0;
}

