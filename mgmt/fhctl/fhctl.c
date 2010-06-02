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
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "fh_cli.h"
#include "fh_cli_hist.h"
#include "fh_cli_cmd.h"

#include "fh_time.h"
#include "fh_tcp.h"
#include "fh_log.h"
#include "fh_util.h"

#include "fh_mgmt_client.h"
#include "fh_mgmt_service.h"
#include "fh_mgmt_admin.h"

static char         *fhctl_cmd = NULL;
static char         *pname = NULL;
static int           debug = 0;
static int           quiet = 0;
static int           showUpTime;
static fh_mgmt_cl_t cli_cl = { . mcl_fd = -1 };

#define DBG (0)

#if DBG
# define DPRINTF(args...) printf(args)
#else
# define DPRINTF(args...)
#endif

/*
 * date_cb
 *
 * Date callback.
 */
int date_cb(char *full_cmd, char **argv, int argc)
{
    FH_STATUS rc;
    uint64_t now;
    char now_buffer[64];

    FH_ASSERT(full_cmd && argv);
    argc = 0;

    fh_time_get(&now);

    rc = fh_time_fmt(now, now_buffer, sizeof(now_buffer));
    FH_ASSERT(rc == FH_OK);

    fh_cli_write("%s\n", now_buffer);

    return 0;
}

/*
 * cli_serv_request
 *
 * Sends a service request to the FH Manager, and wait for the response.
 */
static fh_adm_serv_resp_t *cli_serv_request(char *serv_name, uint32_t req_cmd,
                                            uint32_t resp_cmd, uint32_t resp_size)
{
    fh_adm_serv_req_t   serv_req;
    fh_adm_serv_resp_t *serv_resp = NULL;
    char               *resp;
    uint32_t            max_resp_len;
    FH_STATUS           rc;

    // Prepare the service request
    strcpy(serv_req.serv_name, serv_name);
    serv_req.serv_cmd = req_cmd;

    max_resp_len = sizeof(*serv_resp) + FH_MGMT_MAX_SERVICES * resp_size;

    resp = malloc(max_resp_len);
    FH_ASSERT(resp);

    /*
     * Send the service request and wait for the response
     */
    rc = fh_mgmt_cl_reqresp(&cli_cl,
                            FH_ADM_CMD_SERV_REQ,  &serv_req, sizeof(serv_req),
                            FH_ADM_CMD_SERV_RESP, resp, max_resp_len);
    if (rc != FH_OK) {
        fh_cli_write("Failed to send service request command for service '%s'\n", serv_name);
    }
    else {
        /*
         * Process the service response
         */
        serv_resp = (fh_adm_serv_resp_t *)resp;

        if (serv_resp->serv_resp_cnt == 0) {
            fh_cli_write("Service not found: %s\n", serv_resp->serv_name);
            free(resp);
            return NULL;
        }

        if (serv_resp->serv_resp_cmd != resp_cmd) {
            fh_cli_write("Unexpected response command: %d vs. %d\n",
                         serv_resp->serv_resp_cmd, resp_cmd);
            free(resp);
            return NULL;
        }
    }

    return serv_resp;
}

/*
 * dump_service_status
 *
 * Dump the status of a given service
 */
static void dump_service_status(int i, fh_adm_status_resp_t *status_resp)
{
    char     uptime_buffer[64];
    uint64_t uptime;
    uint32_t putime = 0;
    uint32_t pstime = 0;
    uint32_t pcpu = 0;
    uint32_t pmem = 0;
    uint32_t pid = 0;
    uint32_t fp_cpu = 0;
    char    *state;

    /*
     * Dump the table header only once
     */
    if (i == 0) {
        if (showUpTime)
        {
            fh_cli_write("%-16s %-10s %-6s %-5s %-4s %-4s %-4s %-4s %s\n",
                     "Service",
                     "Status",
                     "PID",
                     "CPU",
                     "\%MEM",
                     "\%CPU",
                     "\%Usr",
                     "\%Sys",
                     "Up time");
        }
        else
        {
            fh_cli_write("%-16s %-10s %-6s %-5s %-4s %-4s %-4s %-4s %s\n",
                     "Service",
                     "Status",
                     "PID",
                     "CPU",
                     "\%MEM",
                     "\%CPU",
                     "\%Usr",
                     "\%Sys",
                     "Start time");
        }
        fh_cli_write("---------------- ---------- ------ ----- ---- ---- ---- ---- "
                     "-----------------------\n");
    }

    if (status_resp->status_state & FH_MGMT_SERV_RUNNING) {
        pid    = status_resp->status_pid;
        fp_cpu = status_resp->status_fp_cpu;
        pmem   = status_resp->status_pmem;
        putime = status_resp->status_putime;
        pstime = status_resp->status_pstime;
        pcpu   = putime + pstime;

        if (status_resp->status_state & (FH_MGMT_SERV_STOPPING | FH_MGMT_SERV_ADMIN_STOPPING)) {
            state = "Stopping";

            strcpy(uptime_buffer, "N/A");
        }
        else {
            uint64_t now;

            state = "Running";

            if (showUpTime)
            {
                fh_time_get(&now);

                uptime = (uint64_t)status_resp->status_uptime * 1000000;
                uptime = (now - uptime) / 1000000;

                sprintf(uptime_buffer, "%llu", (long long unsigned int)uptime);
            }   
            else
            {
                uptime = (uint64_t)status_resp->status_uptime * 1000000;
                fh_time_fmt(uptime, uptime_buffer, sizeof(uptime_buffer));
            }
        }                                           
    }
    else {
        strcpy(uptime_buffer, "N/A");

        state = (status_resp->status_state & FH_MGMT_SERV_DISABLED)         ? "Disabled" :
                (status_resp->status_state & FH_MGMT_SERV_STARTED)          ? "Started"  :
                (status_resp->status_state & FH_MGMT_SERV_STOPPED)          ? "Stopped"  :
                (status_resp->status_state & FH_MGMT_SERV_ADMIN_STOPPED)    ? "Stopped"  :
                "Unknown";
    }

    fh_cli_write("%-16s %-10s %-6d %-5d %-4d %-4d %-4d %-4d %s\n",
                 status_resp->status_service,
                 state, pid, fp_cpu, pmem, pcpu, putime, pstime, uptime_buffer);
}

/*
 * dump_service_stats
 *
 * Dump the service statistics for all lines
 */
static void dump_service_stats(fh_adm_stats_resp_t *stats_resp)
{
    uint32_t i;

    if (stats_resp->stats_state & FH_MGMT_SERV_RUNNING) {
        fh_cli_write("Service            : %s\n", stats_resp->stats_service);

        for (i=0; i<stats_resp->stats_line_cnt; i++) {
            fh_adm_line_stats_t *line = &stats_resp->stats_lines[i];

            fh_cli_write(" > Line: %s\n", line->line_name);
            fh_cli_write("   - Packets            : %lld\n", LLI(line->line_pkt_rx));
            fh_cli_write("   - Packet errors      : %lld\n", LLI(line->line_pkt_errs));
            fh_cli_write("   - Packet dups        : %lld\n", LLI(line->line_pkt_dups));
            fh_cli_write("   - Packet late        : %lld\n", LLI(line->line_pkt_late));
            fh_cli_write("   - Packet bad time    : %lld\n", LLI(line->line_pkt_bad_times));
            fh_cli_write("   - Packet seq jumps   : %lld\n", LLI(line->line_pkt_seq_jump));
            fh_cli_write("   - Packet wrap noreset: %lld\n", LLI(line->line_pkt_wrap_noreset));
            fh_cli_write("   - Lost messages      : %lld\n", LLI(line->line_msg_loss));
            fh_cli_write("   - Recovered messages : %lld\n", LLI(line->line_msg_recovered));
            fh_cli_write("   - Late messages      : %lld\n", LLI(line->line_msg_late));
            fh_cli_write("   - Received messages  : %lld\n", LLI(line->line_msg_rx));
            fh_cli_write("   - Bytes              : %lld\n", LLI(line->line_bytes));
        }
    }
    else {
        char *state;

        state = (stats_resp->stats_state & FH_MGMT_SERV_DISABLED) ? "Disabled" :
                (stats_resp->stats_state & FH_MGMT_SERV_STARTED)  ? "Started"  :
                (stats_resp->stats_state & FH_MGMT_SERV_STOPPED)  ? "Stopped"  :
                "Unknown";

        fh_cli_write("Service            : %s (%s)\n", stats_resp->stats_service, state);
    }
    fh_cli_write("\n");
}

/*
 * dump_service_version
 *
 * Dump the version information for a given service
 */
static void dump_service_version(int i, fh_adm_getver_resp_t *getver_resp)
{
    fh_cli_write("--- Service [ %d ] ---\n", i+1);

    if (getver_resp->getver_state & FH_MGMT_SERV_RUNNING) {
        fh_cli_write("Service            : %s\n", getver_resp->getver_service);
        fh_cli_write("Build Date         : %s\n", getver_resp->getver_build_date);
        fh_cli_write("Build Git Revision : %s\n", getver_resp->getver_build_rev);
    }
    else {
        char *state;

        state = (getver_resp->getver_state & FH_MGMT_SERV_DISABLED) ? "Disabled" :
                (getver_resp->getver_state & FH_MGMT_SERV_STARTED)  ? "Started"  :
                (getver_resp->getver_state & FH_MGMT_SERV_STOPPED)  ? "Stopped"  :
                "Unknown";

        fh_cli_write("Service            : %s (%s)\n", getver_resp->getver_service, state);
    }
    fh_cli_write("\n");
}

/*
 * show_serv_status_cb
 *
 * Show service status CLI callback.
 */
static int show_serv_status_cb(char *full_cmd, char **argv, int argc)
{
    fh_adm_status_resp_t   *status_resp = NULL;
    fh_adm_serv_resp_t     *serv_resp = NULL;
    char                    serv_name[16];
    uint32_t                i; 

    if (argc > 0) {
        char *last_arg = argv[argc-1];

        if (last_arg[strlen(last_arg)-1] == '?') {
            fh_cli_write("\n");
        }
        fh_cli_write("Usage: %s\n", full_cmd);
        return 0;
    }

    // Find the service name
    sscanf(full_cmd, "show service %s status", serv_name);

    // Send the status request for the service group
    serv_resp = cli_serv_request(serv_name,
                                 FH_ADM_CMD_STATUS_REQ,
                                 FH_ADM_CMD_STATUS_RESP,
                                 sizeof(fh_adm_status_resp_t));
    if (serv_resp == NULL) {
        return 0;
    }

    status_resp = (fh_adm_status_resp_t *)(serv_resp+1);

    for (i=0; i<serv_resp->serv_resp_cnt; i++) {
        dump_service_status(i, status_resp);
        status_resp++;
    }

    free(serv_resp);

    return 0;
}

/*
 * show_serv_stats_cb
 *
 * Show service statistics CLI callback.
 */
static int show_serv_stats_cb(char *full_cmd, char **argv, int argc)
{
    fh_adm_stats_resp_t    *stats_resp = NULL;
    fh_adm_serv_resp_t     *serv_resp = NULL;
    char                    serv_name[16];
    uint32_t                i; 

    if (argc > 0) {
        char *last_arg = argv[argc-1];

        if (last_arg[strlen(last_arg)-1] == '?') {
            fh_cli_write("\n");
        }
        fh_cli_write("Usage: %s\n", full_cmd);
        return 0;
    }

    // Find the service name
    sscanf(full_cmd, "show service %s stats", serv_name);

    // Send the stats request for the service group
    serv_resp = cli_serv_request(serv_name,
                                 FH_ADM_CMD_STATS_REQ,
                                 FH_ADM_CMD_STATS_RESP,
                                 sizeof(fh_adm_stats_resp_t));
    if (serv_resp == NULL) {
        return 0;
    }

    stats_resp = (fh_adm_stats_resp_t *)(serv_resp+1);

    for (i=0; i<serv_resp->serv_resp_cnt; i++) {
        fh_cli_write("--- Service [ %d ] ---\n", i+1);
        dump_service_stats(stats_resp);

        stats_resp++;
    }

    return 0;
}

/*
 * show_serv_version_cb
 *
 * Show service version CLI callback.
 */
static int show_serv_version_cb(char *full_cmd, char **argv, int argc)
{
    fh_adm_getver_resp_t   *getver_resp = NULL;
    fh_adm_serv_resp_t     *serv_resp = NULL;
    char                    serv_name[16];
    uint32_t                i; 

    if (argc > 0) {
        char *last_arg = argv[argc-1];

        if (last_arg[strlen(last_arg)-1] == '?') {
            fh_cli_write("\n");
        }
        fh_cli_write("Usage: %s\n", full_cmd);
        return 0;
    }

    // Find the service name
    sscanf(full_cmd, "show service %s version", serv_name);

    // Send the stats request for the service group
    serv_resp = cli_serv_request(serv_name,
                                 FH_ADM_CMD_GETVER_REQ,
                                 FH_ADM_CMD_GETVER_RESP,
                                 sizeof(fh_adm_getver_resp_t));
    if (serv_resp == NULL) {
        return 0;
    }

    getver_resp = (fh_adm_getver_resp_t *)(serv_resp+1);

    for (i=0; i<serv_resp->serv_resp_cnt; i++) {
        dump_service_version(i, getver_resp);
        getver_resp++;
    }


    return 0;
}

/*
 * serv_control_cb
 *
 * Generic control command callback.
 */
static int serv_control_cb(char *full_cmd, char **argv, int argc,
                           char *control, int control_cmd)
{
    char                serv_name[16];
    fh_adm_serv_req_t   serv_req;
    FH_STATUS           rc;
    char                show_cmd[64];

    if (argc > 0) {
        char *last_arg = argv[argc-1];

        if (last_arg[strlen(last_arg)-1] == '?') {
            fh_cli_write("\n");
        }
        fh_cli_write("Usage: %s\n", full_cmd);
        return 0;
    }

    // Find the service name
    sscanf(full_cmd, "service %s", serv_name);

    strcpy(serv_req.serv_name, serv_name);
    serv_req.serv_cmd = control_cmd;

    rc = fh_mgmt_cl_post(&cli_cl, FH_ADM_CMD_SERV_REQ, &serv_req, sizeof(serv_req));
    if (rc != FH_OK) {
        fh_cli_write("Failed to send service group %s to FH Manager for %s\n",
                     control, serv_name);
        return rc;
    }

    fh_cli_write("--------------------------------------------------------------------------\n");
    fh_cli_write("> Successfully sent async %s request for %s\n", control, serv_name);
    fh_cli_write("--------------------------------------------------------------------------\n");

    sprintf(show_cmd, "show service %s status", serv_name);

    return show_serv_status_cb(show_cmd, argv, argc);
}

/*
 * serv_enable_cb
 *
 * Sends an enable async request to FH manager for the service group or
 * service.
 */
static int serv_enable_cb(char *full_cmd, char **argv, int argc)
{
    return serv_control_cb(full_cmd, argv, argc, "enable", FH_MGMT_CL_CTRL_ENABLE);
}

/*
 * serv_disable_cb
 *
 * Sends a disable async request to FH manager for the service/service group.
 */
static int serv_disable_cb(char *full_cmd, char **argv, int argc)
{
    return serv_control_cb(full_cmd, argv, argc, "disable", FH_MGMT_CL_CTRL_DISABLE);
}

/*
 * serv_restart_cb
 *
 * Sends a restart async request to FH manager for the service/service group.
 */
static int serv_restart_cb(char *full_cmd, char **argv, int argc)
{
    return serv_control_cb(full_cmd, argv, argc, "restart", FH_MGMT_CL_CTRL_RESTART);
}

/*
 * serv_start_cb
 *
 * Sends a start async request to FH manager for the service/service group.
 */
static int serv_start_cb(char *full_cmd, char **argv, int argc)
{
    return serv_control_cb(full_cmd, argv, argc, "start", FH_MGMT_CL_CTRL_START);
}

/*
 * serv_stop_cb
 *
 * Sends a stop async request to FH manager for the service/service group.
 */
static int serv_stop_cb(char *full_cmd, char **argv, int argc)
{
    return serv_control_cb(full_cmd, argv, argc, "stop", FH_MGMT_CL_CTRL_STOP);
}

/*
 * serv_clrstats_cb
 *
 * Sends a clrstats async request to FH manager for the service/service group.
 */
static int serv_clrstats_cb(char *full_cmd, char **argv, int argc)
{
    return serv_control_cb(full_cmd, argv, argc, "clrstats", FH_MGMT_CL_CTRL_CLRSTATS);
}


/*----------------------------------------------------------------------*/
/* Service group and service commands                                   */
/*----------------------------------------------------------------------*/

typedef struct {
    char            *ccl_name;
    fh_cli_cmd_cb_t *ccl_cb;
} cmd_cb_list_t;

static cmd_cb_list_t show_serv_cmd_list[] = {
    { "status",     show_serv_status_cb },
    { "stats",      show_serv_stats_cb  },
    { "version",    show_serv_version_cb },
    { NULL, NULL}
};

static cmd_cb_list_t serv_cmd_list[] = {
    { "enable",     serv_enable_cb  },
    { "disable",    serv_disable_cb },
    { "start",      serv_start_cb   },
    { "stop",       serv_stop_cb    },
    { "restart",    serv_restart_cb },
    { "clrstats",   serv_clrstats_cb },
    { NULL, NULL}
};

/*
 * add_cli_cmd
 *
 * Add the CLI command to the CLI command tree for a given type of commands, and
 * a given service group or service.
 */
static void add_cli_cmd(fh_cli_cmd_t *parent, cmd_cb_list_t *ccl,
                        char *name, char *action, char *type)
{
    char           help[128];
    fh_cli_cmd_t  *child;
    int            j = 0;

    // Add the 'name' CLI service node
    sprintf(help, "%s %s %s information", action, name, type);

    child = fh_cli_cmd_register(parent, name, FH_CLI_CMD_MODE_DEFAULT, NULL, help);
    FH_ASSERT(child);

    // Add the CLI node children for this 'name' service node
    while (ccl[j].ccl_name) {
        sprintf(help, "%s %s %s %s", action, name, type, ccl[j].ccl_name);

        fh_cli_cmd_register(child, ccl[j].ccl_name, FH_CLI_CMD_MODE_DEFAULT, 
                            ccl[j].ccl_cb, help);

        j++;
    }
}

/*
 * get_service_group_tree
 *
 * Request the service group tree from FH Manager.
 */
static FH_STATUS get_service_group_tree()
{
    char buffer[2048];
    fh_adm_sgt_req_t   sgt_req;
    fh_adm_sgt_resp_t *sgt_resp = (fh_adm_sgt_resp_t *) buffer;
    fh_cli_cmd_t      *show_serv_c = NULL;
    fh_cli_cmd_t      *serv_c = NULL;
    int                i;
    FH_STATUS          rc;

    sgt_req.sgt_reserved = 0;

    rc = fh_mgmt_cl_reqresp(&cli_cl,
                            FH_ADM_CMD_SGT_REQ,  &sgt_req, sizeof(sgt_req),
                            FH_ADM_CMD_SGT_RESP, sgt_resp, sizeof(buffer));

    if (rc != FH_OK) {
        fh_cli_write("Failed to send service group tree request to FH Manager\n");
        return rc;
    }

    // Load the different service groups and services
    char show_service_node[64] = "show.service";
    char service_node[64]      = "service";

    show_serv_c = fh_cli_cmd_search(NULL, show_service_node);
    if (!show_serv_c) {
        fh_cli_write("Failed to find CLI command node for 'show service'\n");
        return FH_ERROR;
    }

    serv_c = fh_cli_cmd_search(NULL, service_node);
    if (!serv_c) {
        fh_cli_write("Failed to find CLI command node for 'service'\n");
        return FH_ERROR;
    }

    // Add the CLI commands for all service groups
    add_cli_cmd(show_serv_c, show_serv_cmd_list, "all", "Show", "service group");
    add_cli_cmd(serv_c, serv_cmd_list, "all", "Perform", "service group");

    // For each service group, add the service group and the child services
    char *ptr = (char *)(sgt_resp+1);

    for (i=0; i<sgt_resp->sgt_count; i++) {
        fh_adm_sg_resp_t *sg = (fh_adm_sg_resp_t *)ptr;
        uint32_t j = 0;

        if (sg->sg_serv_count > 1) {
            add_cli_cmd(show_serv_c, show_serv_cmd_list, sg->sg_name, "Show", "service group");
            add_cli_cmd(serv_c, serv_cmd_list, sg->sg_name, "Perform", "service group");
        }

        ptr += sizeof(fh_adm_sg_resp_t);

        for (j=0; j<sg->sg_serv_count; j++) {
            fh_adm_sg_serv_resp_t *sg_serv = (fh_adm_sg_serv_resp_t *)ptr;

            add_cli_cmd(show_serv_c, show_serv_cmd_list, sg_serv->serv_name, "Show", "service");
            add_cli_cmd(serv_c, serv_cmd_list, sg_serv->serv_name, "Perform", "service");

            ptr += sizeof(fh_adm_sg_serv_resp_t);
        }
    }

    return FH_OK;
}



/*
 * connect_cb
 * 
 * Connect callback. Connects to the FH manager daemon.
 */
int connect_cb(char *full_cmd, char **argv, int argc)
{
    char              *last_arg = argv[argc-1];
    struct hostent    *host;
    struct sockaddr_in addr;
    uint32_t           ipaddr;
    FH_STATUS          rc;

    if (argc != 1) {
        fh_cli_write("\nUsage: %s <host>\n", full_cmd);
        return 0;
    }

    if (last_arg[strlen(last_arg)-1] == '?') {
        fh_cli_write("\n\n");
        fh_cli_write("Available Arguments:\n");
        fh_cli_write("--------------------\n");
        fh_cli_print("<host>", "Client IP Address <a.b.c.d> or Hostname");
        fh_cli_write("\n");
        return 0;
    }

    host = gethostbyname(argv[0]);
    if (host) {
        memcpy(&addr.sin_addr, host->h_addr, host->h_length);
        ipaddr = addr.sin_addr.s_addr;
    }
    else {
        fh_cli_write("Bad host address specified\n");
        return 0;
    }

    /*
     * Connect to the FH manager
     */
    rc = fh_mgmt_cl_init(&cli_cl, NULL, FH_MGMT_CL_CLI, ipaddr, FH_MGR_PORT, "fhctl", 0);
    if (rc != FH_OK) {
        fh_cli_write("Failed to connect to FH manager\n");
        return 1;
    }

    /*
     * Request the service group tree, and all its services, and add them to the
     * CLI command tree for all service-related commands.
     */
    rc = get_service_group_tree();
    if (rc != FH_OK) {
        return 1;
    }

    {
        char prompt[64];
        sprintf(prompt, "fhctl[%s]", argv[0]);
        fh_cli_set_prompt(prompt);
    }

    return 0;
}

/*
 * show_version_cb
 *
 * Show the version infomration from FH manager
 */
int show_version_cb(char *full_cmd, char **argv, int argc)
{
    fh_adm_getver_req_t  getver_req;
    fh_adm_getver_resp_t getver_resp;
    FH_STATUS rc;

    if (argc > 0) {
        char *last_arg = argv[argc-1];

        if (last_arg[strlen(last_arg)-1] == '?') {
            fh_cli_write("\n");
        }

        fh_cli_write("Usage: %s\n", full_cmd);

        return 0;
    }

    FH_ASSERT(argv);

    /*
     * Request to get the FH manager revision information
     */
    strcpy(getver_req.getver_service, "FH Manager");

    rc = fh_mgmt_cl_reqresp(&cli_cl,
                            FH_ADM_CMD_GETVER_REQ, &getver_req, sizeof(getver_req),
                            FH_ADM_CMD_GETVER_RESP, &getver_resp, sizeof(getver_resp));
    if (rc != FH_OK) {
        fh_cli_write("Failed to retrieve version information from FH Manager\n");
        return 0;
    }

    dump_service_version(0, &getver_resp);

    return 0;
}


/*--------------------------------------------------------------*/
/* CLI Command Tree ------------------------------------------- */
/*--------------------------------------------------------------*/
static void *show_tree[] = {
    FH_CLI_CMD_DEF("version", FH_CLI_CMD_MODE_DEFAULT,
                   show_version_cb, "Show FH manager version information", NULL),
    FH_CLI_CMD_DEF("service", FH_CLI_CMD_MODE_DEFAULT,
                   NULL, "Show service commands", NULL),
    NULL
};

static void *top_tree[] = {
    FH_CLI_CMD_DEF("date",    FH_CLI_CMD_MODE_DEFAULT,
                   date_cb, "Show the current date", NULL),
    FH_CLI_CMD_DEF("connect", FH_CLI_CMD_MODE_DEFAULT,
                   connect_cb, "Connect to FH manager", NULL),
    FH_CLI_CMD_DEF("service", FH_CLI_CMD_MODE_DEFAULT,
                   NULL, "Service Commands (restart, ...)",  NULL),
    FH_CLI_CMD_DEF("show",    FH_CLI_CMD_MODE_DEFAULT,
                   NULL, "Show Commands (logging, reports, ...)",  show_tree),
    NULL
};

/*
 * fh_ctl_usage
 *
 * Dump the command line paramaters and help message.
 */
static void fh_ctl_usage()
{
  printf("Usage: %s ARGS\n\n"
         "   -u             Show Uptime instead of start time for service commands\n"
         "   -c '<CMD>'     Run the <CMD> in non-interactive mode\n"
         "   -d             Debug mode (doesn't daemonize)\n"
         "   -q             Quiet mode (doesn't log anything)\n"
         "   -h, -?         Display this help message\n", pname);
  exit(1);
}

/*
 * fh_ctl_parse_args
 *
 * Parse command line arguments
 */
FH_STATUS fh_ctl_parse_args(int argc, char *argv[])
{
    extern int   optind;  /* index of first unused arg */
    extern char *optarg;  /* pointer to option string  */
    int          c;
 
    while ((c = getopt(argc, argv, "uc:dqh?")) != EOF) {
        switch (c) {
        case 'c':
            fhctl_cmd = optarg;
            break;
        case 'u':
            showUpTime = 1;
            break;

        case 'd':
            debug = 1;
            break;

        case 'q':
            quiet = 1;
            break;

        case '?':
        case 'h':
        default:
            return FH_ERROR;
        }
    }

    return FH_OK;
}


/*--------------------------------------------------------------*/
/* Shell API -------------------------------------------------- */
/*--------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    FH_STATUS rc;

    FH_PNAME_GET(pname, argv);

    /*
     * Parse the command line arguments
     */
    rc = fh_ctl_parse_args(argc, argv);
    if (rc != FH_OK) {
        fh_ctl_usage();
    }

    if (!quiet) {
        if (debug) {
            fh_log_open();
            fh_log_set_cfg(FH_LCF_CONSOLE);
            fh_log_set_class(FH_LC_MGMT, FH_LL_WARN|FH_LL_VSTATE);
        }
        else {
            fh_log_init("/tmp/fhctl.log");
        }
    }

    /* Unused parameters */
    argv = NULL;
    argc = 0;

    /* --- Initialize Command Line Interface -------------------- */

    fh_cli_init("fhctl");

    fh_cli_set_prompt("fhctl");
    fh_cli_set_prompt_char('>');

    fh_cli_tree(top_tree);

    /*
     * Try to connect to the local host
     */
    {
        char *argv_tmp[] = { "127.0.0.1", NULL };

        if (connect_cb("connect", argv_tmp, 1) == 0) {
            char prompt[64], *ptr = NULL;
            char hostname[255];

            int rc = gethostname(hostname, sizeof(hostname));
            if (rc != 0) {
                strcpy(hostname, "127.0.0.1");
            }
            else if ((ptr = strchr(hostname, '.')) != NULL) {
                *ptr = '\0';
            }

            sprintf(prompt, "fhctl[%s]", hostname);
            fh_cli_set_prompt(prompt);
        }
    }
    if (fhctl_cmd) {
        int ret;

        if (debug) {
            fh_cli_write("Run command: %s\n", fhctl_cmd);
        }
        if ((ret = fh_cli_cmd_process(fhctl_cmd)) != 0) {
            fh_cli_write("Invalid command: '%s'\n", fhctl_cmd);
        }

        fh_cli_exit();

        exit(ret);
    }


    /* --- Start Command Line Interface ------------------------- */

    fh_cli_loop();

    /* --- Exit Command Line Interface -------------------------- */

    fh_cli_exit();

    return 0;
}

