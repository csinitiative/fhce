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
 * OS Header files
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <semaphore.h>

/*
 * FH Common Header files
 */
#include "fh_errors.h"
#include "fh_util.h"
#include "fh_log.h"
#include "fh_cpu.h"
#include "fh_plugin.h"
#include "fh_config.h"

/*
 * FH OPRA Header files
 */
#include "fh_opra_cfg.h"
#include "fh_opra_mgmt.h"
#include "fh_opra_lh.h"
#include "fh_opra_lh_tap.h"
#include "fh_opra_topic.h"
#include "fh_opra_revision.h"

/*
 * External function defined for either V1 or V2 OPRA FAST.
 */
extern FH_STATUS fh_opra_msg_init();

/*
 * Global varaibles
 */
int opra_stopped = 0;

#define FH_HOME          "/opt/csi/fh"
#define FH_OPRA_CONFIG   "opra/etc/opra.conf"
#define FH_OPRA_LO_FILE  "opra/etc/listedoptions.txt"
#define FH_OPRA_PLUGINS  "opra/plugins"

/*
 * Static variables
 */
static char *   pname              = "?";
static int      display_version    = 0;
static char    *fh_home            = NULL;

static char     opra_tag[16]       = "";
static char *   opra_log_file      = NULL;
static int      opra_instance      = 0;
static int      opra_cpu_num       = -1;
static int      opra_debug         = 0;
static int      opra_lo_download   = 0;
static int      opra_standalone    = 0;
static int      opra_tap_bytes     = 0;
static char *   opra_report_ftline = NULL;
static char *   opra_report_lrates = NULL;
static int      opra_report        = 0;

/*
 * Defaults
 */
static char     opra_plugins_dir[MAXPATHLEN];
static char     opra_lo_file[MAXPATHLEN];
static char     opra_config_file[MAXPATHLEN];

/*
 * usage
 *
 * Dump the usage help message re the available command line arguments.
 */
static void fh_opra_usage()
{
    fprintf(stderr,
            "Usage: %s [ OPTS ]\n"
            "   OPTS:\n"
            "   -d                    Debugging mode (prints to console).\n"
            "   -s                    Standalone mode (do not attach to FH manager).\n"
            "   -r <TAP_BYTES>        Record statistics on lines (Dump files in working directory).\n"
            "   -x '<TAP> <PERIOD>'   Report the msg and pkt rates from a tap file (Period in microseconds)\n"
            "   -y '<TAP_A> <TAP_B>'  Report the FT line A,B statistics from the two tap files\n"
            "   -t <OPRA_TAG>         OPRA logging identification.\n"
            "   -i <OPRA_INSTANCE>    OPRA Instance number.\n"
            "   -l <LOG_FILE>         Logging to a file instead of syslog\n"
            "   -f <CONFIG_FILE>      Configuration file (default: fh.config)\n"
            "   -o <LISTED_OPTIONS>   Listed options file (default: listedoptions.txt)\n"
            "   -c <CPU>              Fast-Path CPU affinity (default: 1+Instance)\n"
            "   -p <PLUGINS_DIR>      Plugins directory\n"
            "   -g                    Download listed options and exit\n"
            "   -V                    Display the version information\n",
            pname);
    exit(1);
}

/*
 * fh_opra_validate_args
 *
 * Validate the arguments.
 */
static FH_STATUS fh_opra_validate_args()
{
    struct stat  stat_buf;

    if (stat(opra_config_file, &stat_buf) < 0) {
        fprintf(stderr, "ERROR: OPRA configuration file doesn't exist: %s\n",
                opra_config_file);
        return FH_ERROR;
    }

    if (opra_instance < 0) {
        fprintf(stderr, "ERROR: Invalid instance number or missing -i <ID> option\n");
        return FH_ERROR;
    }

    if (opra_tag[0] == '\0') {
        sprintf(opra_tag, "OPRA%d", opra_instance);
    }

    return FH_OK;
}

/*
 * fh_opra_parse_args
 *
 * Parse the command line arguments
 */
static FH_STATUS fh_opra_parse_args(int argc, char **argv)
{
    int          op;
    extern char *optarg;

    while ((op = getopt(argc, argv, "sr:o:f:t:i:l:gp:c:dh?Vx:y:")) != EOF) {

        switch (op) {
        case 'V':
            display_version = 1;
            break;

        case 'h':
        case '?':
            fh_opra_usage();
            break;

        case 's':
            opra_standalone = 1;
            break;

        case 'g':
            opra_lo_download = 1;
            break;

        case 'x':
            opra_report = 1;
            opra_report_lrates = optarg;
            break;

        case 'y':
            opra_report = 1;
            opra_report_ftline = optarg;
            break;

        case 'r':
            opra_tap_bytes = atoi(optarg);
            if (opra_tap_bytes == -1) {
                opra_tap_bytes = 0;
            }
            break;

        case 'd':
            opra_debug++;
            break;

        case 'i':
            opra_instance = atoi(optarg);
            break;

        case 'o':
            strcpy(opra_lo_file, optarg);
            break;

        case 'p':
            strcpy(opra_plugins_dir, optarg);
            break;

        case 'l':
            opra_log_file = optarg;
            break;

        case 'c':
            opra_cpu_num = atoi(optarg);

            if (opra_cpu_num < 0) {
                fprintf(stderr, "ERROR: Invalid cpu number: %s\n", argv[1]);
                return FH_ERROR;
            }
            break;

        case 'f':
            strcpy(opra_config_file, optarg);
            break;

        case 't':
            strcpy(opra_tag, optarg);
            break;

        default:
            fprintf(stderr, "ERROR: Unknown command line option: %c\n", op);
            return FH_ERROR;
        }
    }

    if (!display_version) {
        return fh_opra_validate_args();
    }

    return FH_OK;
}

/*
 * fh_opra_log_init
 *
 * Initialize and configure the logging sub-system
 */
static void fh_opra_log_init(fh_cfg_node_t *config)
{
    FH_STATUS            rc;

    /*
     * Initialize the logging sub-system.
     */
    if (opra_log_file) {
        fh_log_init(opra_log_file);
    }
    else {
        fh_log_open();
    }

    // if output should be going to the console...
    if (opra_debug || display_version || opra_report) {
        // tell the logging subsystem to send log output to stdout
        fh_log_set_cfg(FH_LCF_CONSOLE);

        // since output is going to the console, make sure stdout and stderr are line buffered
        setlinebuf(stdout);
        setlinebuf(stderr);
    }

    if (opra_debug > 1) {
        fh_log_set_lvl(FH_LL_DIAG | FH_LL_VSTATE);
    }

    /*
     * Change the logging identifier
     */
    fh_log_set_ident(opra_tag);

    // Load the logging sub-system configuration from the FH configuration file
    rc = fh_log_cfg_load(config);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("failed to load logging configuration from file: %d", rc));
    }

    return;
}

/*
 * fh_opra_os_tuning
 *
 * This function takes care of the OS-level tuning.
 */
static void fh_opra_os_tuning()
{
    FH_STATUS rc;

    /*
     * Lock all current and future VM pages in RAM to avoid being
     * swapped out to disk.
     */
    if (mlockall(MCL_CURRENT |MCL_FUTURE) < 0) {
        FH_LOG(MGMT, WARN, ("mlockall call failed: %s (%d)", strerror(errno), errno));
    }

    /*
     * Set the CPU affinity of the process. All FH threads defaults to the
     * last CPU (num_cpus - 1) because of the OS shielding configuration.
     *
     * In order to shield all cores from all processes, the script
     * /etc/rc.sysinit will assign itself and the init process (1) to the
     * last CPU of the box. All spawn processes from init will inherit
     * the CPU affinity mask of init. Effectively this shields all other
     * CPUs from all processes running on the system.
     *
     * Here are the commands that have to be placed at the top of this
     * script:
     *
     * fh_num_cpus=$(grep processor /proc/cpuinfo | wc -l)
     * fh_last_cpu=$((num_cpus - 1))
     * /bin/taskset -pc $last_cpu $$
     * /bin/taskset -pc $last_cpu 1
     *
     * Then, we will assign specific threads to specific CPUs, so they will
     * not incur scheduling jitter due to context switching.
     */
    if (opra_cpu_num == -1) {
        opra_cpu_num = fh_cpu_count() - 1;
    }

    rc = fh_cpu_setaffinity(CPU(opra_cpu_num));
    if (rc != FH_OK) {
        FH_LOG(MGMT, WARN, ("Failed to load OPRA configuration"));
    }

    return;
}

/*
 * fh_opra_sig_handle
 *
 * Global handler for all signals.
 */
static void fh_opra_sig_handle(int32_t signo)
{
    switch (signo) {
    case SIGTERM:
        FH_LOG(MGMT, WARN, ("SIGTERM caught - exiting - "));
        opra_stopped = 1;
        break;

    case SIGHUP:
        FH_LOG(MGMT, WARN, ("SIGHUP caught - exiting - "));
        opra_stopped = 1;
        break;

    case SIGINT:
        FH_LOG(MGMT, WARN, ("SIGINT caught - exiting - "));
        opra_stopped = 1;
        break;

    default:
        FH_LOG(MGMT, WARN, ("Unhandled Signal received: %d", signo));
        return;
    }
}

/*
 * fh_opra_sig_init
 *
 * This function takes care of the signal handling.
 */
static void fh_opra_sig_init()
{
    if (signal(SIGINT, fh_opra_sig_handle) == SIG_ERR) {
        FH_LOG(MGMT, ERR, ("Init for SIGINT failed"));
    }

    if (signal(SIGHUP, fh_opra_sig_handle) == SIG_ERR) {
        FH_LOG(MGMT, ERR, ("Init for SIGHUP failed"));
    }

    if (signal(SIGTERM, fh_opra_sig_handle) == SIG_ERR) {
        FH_LOG(MGMT, ERR, ("Init for SIGTERM failed"));
    }
}

/*
 * fh_opra_revision
 *
 * Dump the OPRA revision and build information
 */
void fh_opra_revision(int opra_version)
{
    FH_LOG(MGMT, STATE, ("---------------------------------------------------------"));
    FH_LOG(MGMT, STATE, ("Starting Fast OPRA version %d", opra_version));
    FH_LOG(MGMT, STATE, ("---------------------------------------------------------"));
    FH_LOG(MGMT, STATE, ("  > Generated by %s on host %s:", BUILD_USER, BUILD_HOST));
    FH_LOG(MGMT, STATE, ("    - Architecture     : %s", BUILD_ARCH));
    FH_LOG(MGMT, STATE, ("    - Kernel version   : %s", BUILD_KVER));
    FH_LOG(MGMT, STATE, ("    - Build date       : %s", BUILD_DATE));
    FH_LOG(MGMT, STATE, ("    - Git URL          : %s", BUILD_URL));
    FH_LOG(MGMT, STATE, ("    - Git Revision     : %s", BUILD_REV));
    FH_LOG(MGMT, STATE, ("---------------------------------------------------------"));

    /*
     * If we were asked to display the version only, just exit
     */
    if (display_version) {
        exit(0);
    }
}

/*
 * OPRA FH Main process loop.
 */
int fh_opra_main(int argc, char *argv[], int opra_version)
{
    fh_cfg_node_t *config = NULL;
    FH_STATUS rc;

    /*
     * Get the program name by skipping the leading path name if necessary
     */
    FH_PNAME_GET(pname, argv);

    /*
     * Check the environment
     */
    if ((fh_home = getenv("FH_HOME")) == NULL) {
        fh_home = FH_HOME;
    }
    sprintf(opra_plugins_dir, "%s/%s", fh_home, FH_OPRA_PLUGINS);
    sprintf(opra_lo_file,     "%s/%s", fh_home, FH_OPRA_LO_FILE);
    sprintf(opra_config_file, "%s/%s", fh_home, FH_OPRA_CONFIG);

    /*
     * Parse command line arguments
     */
    rc = fh_opra_parse_args(argc, argv);
    if (rc != FH_OK) {
        fh_opra_usage();
    }

    /*
     * Daemonize this process (if not in debug mode)
     */
    if (!opra_debug && !opra_lo_download && !opra_report) {
        fh_daemonize();
    }

    config = fh_cfg_load(opra_config_file);
    if (!config) {
        fprintf(stderr, "ERROR: Failed to load OPRA configuration: %s", opra_config_file);
        return 1;
    }

    /*
     * Load the plugins
     */
    if (opra_plugins_dir[0] != '\0' && !opra_report) {
        fh_plugin_load(opra_plugins_dir);
    }

    /*
     * Initialize the logging sub-system
     */
    fh_opra_log_init(config);

    /*
     * Handle reporting cases
     */
    if (opra_report_ftline) {
        char a_filename[MAXPATHLEN];
        char b_filename[MAXPATHLEN];

        int n = sscanf(opra_report_ftline, "%s %s", a_filename, b_filename);
        if (n != 2) {
            FH_LOG(MGMT, ERR, ("Invalid report argument: %s", opra_report_ftline));
            exit(1);
        }

        fh_opra_lh_tap_deltas(a_filename, b_filename);
        exit(0);
    }

    if (opra_report_lrates) {
        char     filename[MAXPATHLEN];
        uint32_t period = 0;

        int n = sscanf(opra_report_lrates, "%s %d", filename, &period);
        if (n != 2) {
            FH_LOG(MGMT, ERR, ("Invalid report argument: %s", opra_report_lrates));
            exit(1);
        }

        fh_opra_lh_tap_rates(filename, (uint64_t)period);
        exit(0);
    }

    /*
     * Load the configuration from the line
     */
    rc = fh_opra_cfg_load(config, opra_instance);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to load OPRA configuration"));
        return 0;
    }

    /*
     * If this is a listedoptions download, then we just download it and exit
     */
    if (opra_lo_download) {
        rc = fh_opra_lo_download(&opra_cfg.ocfg_lo_config);
        if (rc != FH_OK) {
            return 1;
        }

        return 0;
    }

    /*
     * We are done with the OPRA configuration, just free the memory
     */
    fh_cfg_free(config);

    /*
     * Dump some information about the program
     */
    fh_opra_revision(opra_version);

    fh_log_thread_start("OPRA_Main");

    /*
     * OS Tuning of the OPRA process.
     */
    fh_opra_os_tuning();

    /*
     * Take care of the signal handling for the process.
     */
    fh_opra_sig_init();

    /*
     * Management framework start
     */
    rc = fh_opra_mgmt_start(opra_lo_file, opra_standalone);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to start management sub-system"));
        exit(1);
    }

    /*
     * Messaging layer initialization
     */
    rc = fh_opra_msg_init();
    if (rc != FH_OK) {
        exit(1);
    }

    /*
     * Line Handler initialization
     */
    rc = fh_opra_lh_start(opra_tap_bytes);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to start line-handler sub-system"));
        exit(1);
    }

    /*
     * Wait for all the OPRA process threads to exit
     */
    fh_opra_lh_wait();
    fh_opra_mgmt_wait();

    /*
     * Terminate the main process now and exit.
     */
    fh_log_thread_stop("OPRA_Main");

    /*
     * Release all resources
     */
    fh_log_close();

    return 0;
}
