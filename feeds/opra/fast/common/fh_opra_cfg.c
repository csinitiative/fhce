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

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "fh_log.h"
#include "fh_config.h"
#include "fh_net.h"
#include "fh_cpu.h"
#include "fh_plugin.h"

#include "fh_opra_cfg.h"
#include "fh_opra_lo.h"

/*
 * Global OPRA configuration structure
 */
fh_opra_cfg_t           opra_cfg;
static int              opra_cfg_init = 0;
static fh_plugin_hook_t opra_cfg_load = NULL;

static fh_opra_topic_fmt_t opra_topic_fmt  = {
    .tfmt_num_stanzas     = 4,
    .tfmt_stanza_delim    = '.',
    .tfmt_stanza_fmts     = {
        "OPRA",
        "$S",
        "$Y$M$D$C$I$F",
        "$X"
    },
};



/*
 * OPRA Configuration parsing functions
 */

/*
 * fh_opra_cfg_yesno
 *
 * Parse the yes/no value string and return 1 for yes, 0 for no, and -1 for
 * errors
 */
static int fh_opra_cfg_yesno(const char *strval)
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
 * fh_opra_cfg_load_options
 *
 * Load OPRA options.
 */
FH_STATUS fh_opra_cfg_load_options(const fh_cfg_node_t *config, fh_opra_cfg_t *opra_cfg)
{
    const fh_cfg_node_t *node;
    const char          *strval;
    int                  line_status_enable       = 0;
    int                  line_status_period       = 0;
    int                  lo_scp_enable            = 0;
    int                  jitter_stats             = 0;
    uint32_t             partial_publish          = OPRA_CFG_PP_VALUE_ADDED;
    int                  periodic_stats           = 0;
    int                  periodic_stats_interval  = 0;


    node = fh_cfg_get_node(config, "opra.options");
    if (node) {
        /* Retrieve the line_status_enable boolean */
        strval = fh_cfg_get_string(node, "line_status_enable");
        if (strval) {
            line_status_enable = fh_opra_cfg_yesno(strval);
            if (line_status_enable == -1) {
                line_status_enable = 0;
            }
        }
        else {
            FH_LOG(MGMT, WARN, ("line_status_enable parameter is missing"));
        }

        if (line_status_enable) {
            /* Retrieve the line_status_period value */
            strval = fh_cfg_get_string(node, "line_status_period");
            if (strval) {
                line_status_period = atoi(strval);
                if (line_status_period <= 0) {
                    FH_LOG(MGMT, ERR, ("line_status_period must be numeric (was '%s')", strval));
                    return FH_ERROR;
                }
            }
            else {
                /* Default to 10 second line_status period */
                line_status_period = 10;
            }
        }

        /* Retrieve the listed_options_scp boolean */
        strval = fh_cfg_get_string(node, "listed_options_scp");
        if (strval) {
            lo_scp_enable = fh_opra_cfg_yesno(strval);
            if (lo_scp_enable == -1) {
                lo_scp_enable = 0;
            }
        }
        else {
            FH_LOG(MGMT, WARN, ("listed_options_scp parameter is missing"));
        }

        /* Retrieve the jitter stats boolean */
        strval = fh_cfg_get_string(node, "jitter_stats");
        if (strval) {
            jitter_stats = fh_opra_cfg_yesno(strval);
            if (jitter_stats == -1) {
                jitter_stats = 0;
            }
        }
        else {
            FH_LOG(MGMT, WARN, ("jitter_stats parameter is missing"));
        }

        /* Retrieve the partial publish value */
        strval = fh_cfg_get_string(node, "partial_publish");
        if (strval) {
            if (strcmp(strval, "value-added") == 0) {
                partial_publish = OPRA_CFG_PP_VALUE_ADDED;
            }
            else if (strcmp(strval, "all") == 0) {
                partial_publish = OPRA_CFG_PP_ALL;
            }
            else {
                FH_LOG(MGMT, WARN, ("partial_publish invalid value: %s "
                                    "(defaulting to value-added", strval));
            }
        }
        else {
            FH_LOG(MGMT, WARN, ("partial_publish is missing"));
        }

        /* retrieve the partial publish (yes/no) value */
        strval = fh_cfg_get_string(node, "periodic_stats");
        if (strval) {
            periodic_stats = fh_opra_cfg_yesno(strval);
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
    }

    opra_cfg->ocfg_line_status_enable      = line_status_enable;
    opra_cfg->ocfg_line_status_period      = line_status_period;
    opra_cfg->ocfg_lo_scp_enable           = lo_scp_enable;
    opra_cfg->ocfg_jitter_stats            = jitter_stats;
    opra_cfg->ocfg_partial_publish         = partial_publish;
    opra_cfg->ocfg_periodic_stats          = periodic_stats;
    opra_cfg->ocfg_periodic_stats_interval = periodic_stats_interval;

    return FH_OK;
}

/*
 * fh_opra_cfg_laod_topic_fmt
 *
 * Load generic OPRA configuration information.
 */
FH_STATUS fh_opra_cfg_load_topic_fmt(const fh_cfg_node_t *config)
{
    const fh_cfg_node_t *node;
    long                 value;
    char                *endptr;
    const char          *strval;
    FH_STATUS            rc;
    int                  i;

    node = fh_cfg_get_node(config, "opra.topic_fmt");
    if (!node) {
        FH_LOG(MGMT, DIAG, ("topic_fmt section not present"));
        return FH_OK;
    }

    /* Retrive the table_size limit */
    strval = fh_cfg_get_string(node, "num_stanzas");
    if (!strval) {
        FH_LOG(MGMT, ERR, ("num_stanzas limit is missing"));
        return FH_ERROR;
    }

    value = strtol(strval, &endptr , 0);
    if (*strval == '\0' || *endptr != '\0') {
        FH_LOG(MGMT, ERR, ("num_stanzas must be numeric (was '%s')", strval));
        return FH_ERROR;
    }

    if (value >= FH_OPRA_TOPIC_MAX_STANZAS) {
        FH_LOG(MGMT, ERR, ("num_stanzas cannot exceed %d (value:%d)",
                          FH_OPRA_TOPIC_MAX_STANZAS, value));
        return FH_ERROR;
    }

    opra_topic_fmt.tfmt_num_stanzas = value;

    strval = fh_cfg_get_string(node, "stanza_delim");
    if (strval && strlen(strval) == 1) {
        opra_topic_fmt.tfmt_stanza_delim = *strval;
    }

    node = fh_cfg_get_node(node, "stanza_list");
    if (!node) {
        FH_LOG(MGMT, ERR, ("stanza_list is missing"));
        return FH_ERROR;
    }

    /* Walk through all stanzas */
    for(i = 0; i < node->num_values; i++) {
        char *stanza = node->values[i];
        char *ptr = stanza;

        do {
            ptr = strchr(ptr, '$');
            if (ptr) {
                ptr++;
                switch (*ptr) {
                case FH_OPRA_TOPIC_FMT_SYMBOL:
                case FH_OPRA_TOPIC_FMT_YEAR:
                case FH_OPRA_TOPIC_FMT_MONTH:
                case FH_OPRA_TOPIC_FMT_DAY:
                case FH_OPRA_TOPIC_FMT_PUTCALL:
                case FH_OPRA_TOPIC_FMT_DECIMAL:
                case FH_OPRA_TOPIC_FMT_FRACTION:
                case FH_OPRA_TOPIC_FMT_EXCH:
                    break;
                default:
                    FH_LOG(MGMT, ERR, ("Stanza format: invalid variable in stanza '%s'", stanza));
                    return FH_ERROR;
                }
            }
        } while (ptr);

        ptr = strdup(stanza);
        FH_ASSERT(ptr);

        opra_topic_fmt.tfmt_stanza_fmts[i] = ptr;
    }

    /*
     * Run a simple test before declaring victory
     */

    {
        fh_opra_opt_key_t key;
        char topic[32];

        strcpy(key.k_symbol, "WIB");
        key.k_year     = 8;
        key.k_month    = 1;
        key.k_day      = 1;
        key.k_putcall  = 'P';
        key.k_decimal  = 100;
        key.k_fraction = 50;
        key.k_exchid   = 'X';

        rc = fh_opra_topic_fmt(&opra_topic_fmt, &key, topic, sizeof(topic));
        if (rc != FH_OK) {
            return rc;
        }

        FH_LOG(MGMT, DIAG, ("New Opra topic format: generated topic: %s", topic));
    }

    return FH_OK;
}

/*
 * fh_opra_cfg_load_limits
 *
 * Load generic OPRA configuration information.
 */
FH_STATUS fh_opra_cfg_load_limits(const fh_cfg_node_t *config, fh_opra_cfg_t *opra_cfg)
{
    const fh_cfg_node_t *node;
    long                 value;
    char                *endptr;
    const char          *strval;

    node = fh_cfg_get_node(config, "opra.limits");
    if (!node) {
        FH_LOG(MGMT, ERR, ("Limits section not present"));
        return FH_ERROR;
    }

    /* Retrive the table_size limit */
    strval = fh_cfg_get_string(node, "table_size");
    if (!strval) {
        FH_LOG(MGMT, ERR, ("table_size limit is missing"));
        return FH_ERROR;
    }

    value = strtol(strval, &endptr , 0);
    if (*strval == '\0' || *endptr != '\0') {
        FH_LOG(MGMT, ERR, ("table_size limit must be numeric (was '%s')", strval));
        return FH_ERROR;
    }

    opra_cfg->ocfg_table_size = value;

    /* Retrive the opra wrap sequence number limit */
    strval = fh_cfg_get_string(node, "wrap_limit_high");
    if (!strval) {
        FH_LOG(MGMT, ERR, ("wrap_limit_high is missing"));
        return FH_ERROR;
    }

    value = strtol(strval, &endptr , 0);
    if (*strval == '\0' || *endptr != '\0') {
        FH_LOG(MGMT, ERR, ("wrap_limit_high must be numeric (was '%s')", strval));
        return FH_ERROR;
    }

    opra_cfg->ocfg_wrap_limit_high = value;

    /* Retrive the opra wrap sequence low number limit */
    strval = fh_cfg_get_string(node, "wrap_limit_low");
    if (!strval) {
        FH_LOG(MGMT, ERR, ("wrap_limit_low is missing"));
        return FH_ERROR;
    }

    value = strtol(strval, &endptr , 0);
    if (*strval == '\0' || *endptr != '\0') {
        FH_LOG(MGMT, ERR, ("wrap_limit_low must be numeric (was '%s')", strval));
        return FH_ERROR;
    }

    opra_cfg->ocfg_wrap_limit_low = value;

   /* Retrieve the sequence jump threshold number */
    strval = fh_cfg_get_string(node, "seq_jump_threshold");
    if (!strval) {
        FH_LOG(MGMT, ERR, ("seq_jump_threshold is missing"));
        return FH_ERROR;
    }

    value = strtol(strval, &endptr , 0);
    if (*strval == '\0' || *endptr != '\0') {
        FH_LOG(MGMT, ERR, ("seq_jump_threshold must be numeric (was '%s')", strval));
        return FH_ERROR;
    }

    opra_cfg->ocfg_seq_jump_threshold = value;

    return FH_OK;
}


/*
 * Internal function for parsing opra process information from a feed handler config structure
 *
 * @param  (const fh_cfg_node_t *)   config structure generated by config file parser
 * @param  (fh_opra_cfg_t *opra_cfg) opra config structure needing to be filled out
 * @return FH_STATUS
 */
FH_STATUS fh_opra_cfg_load_processes(const fh_cfg_node_t *config, fh_opra_cfg_t *opra_cfg)
{
    const fh_cfg_node_t *node, *process;
    fh_opra_proc_t      *proc_cfg;
    int                  i;
    long                 value;
    char                *endptr;
    const char          *strval;

    /* fetch opra process data from config structure and loop through each defined process */
    node = fh_cfg_get_node(config, "opra.processes");
    for(i = 0; i < node->num_children; i++) {
        /* determine which process this configuration data belongs to */
        value = strtol(node->children[i]->name, &endptr , 0);
        if(*node->children[i]->name == '\0' || *endptr != '\0') {
            FH_LOG(MGMT, ERR, ("Process name must be numeric (was '%s')", node->children[i]->name));
            return FH_ERROR;
        }

        proc_cfg = &opra_cfg->ocfg_procs[value - 1];
        proc_cfg->op_idx = value;
        process = node->children[i];

        /* configure the cpu parameter of this process */
        strval = fh_cfg_get_string(process, "cpu");
        if (!strval) {
            FH_LOG(MGMT, ERR, ("cpu configuration not found (node '%s')",
                              node->children[i]->name));
            return FH_ERROR;
        }

        value = strtol(strval, &endptr , 0);
        if(*strval == '\0' || *endptr != '\0') {
            FH_LOG(MGMT, ERR, ("cpu must be numeric (was '%s')", strval));
            return FH_ERROR;
        }
        proc_cfg->op_cpu = value - 1;

        /* configure the line_from parameter of this process */
        strval = fh_cfg_get_string(process, "line_from");
        if (!strval) {
            FH_LOG(MGMT, ERR, ("line_from configuration not found (node '%s')",
                              node->children[i]->name));
            return FH_ERROR;
        }

        value = strtol(strval, &endptr , 0);
        if(*strval == '\0' || *endptr != '\0') {
            FH_LOG(MGMT, ERR, ("line_from must be numeric (was '%s')", strval));
            return FH_ERROR;
        }
        proc_cfg->op_line_from = value - 1;

        /* configure the line_to parameter of this process */
        strval = fh_cfg_get_string(process, "line_to");
        if (!strval) {
            FH_LOG(MGMT, ERR, ("line_to configuration not found (node '%s')",
                              node->children[i]->name));
            return FH_ERROR;
        }

        value = strtol(strval, &endptr , 0);
        if(*strval == '\0' || *endptr != '\0') {
            FH_LOG(MGMT, ERR, ("line_to must be numeric (was '%s')", strval));
            return FH_ERROR;
        }
        proc_cfg->op_line_to = value - 1;

        /* mark this process configuration as initialized */
        proc_cfg->op_init = 1;

        /* increment the number of configured processes */
        opra_cfg->ocfg_num_procs++;

        /* log addition of this process */
        FH_LOG(MGMT, DIAG, ("Loaded OPRA process: %d cpu:%2d lines:[ %2d - %2d ]",
                            proc_cfg->op_idx, proc_cfg->op_cpu, proc_cfg->op_line_from,
                            proc_cfg->op_line_to));
    }

    return FH_OK;
}

/*
 * Internal function for parsing opra line information from a feed handler config structure
 *
 * @param  (const fh_cfg_node_t *)   config structure generated by config file parser
 * @param  (fh_opra_cfg_t *opra_cfg) opra config structure needing to be filled out
 * @return FH_STATUS
 */
FH_STATUS fh_opra_cfg_load_lines(const fh_cfg_node_t *config, fh_opra_cfg_t *opra_cfg)
{
    const fh_cfg_node_t *node, *line;
    fh_opra_ftline_t    *ftline_cfg = NULL;
    fh_opra_line_t      *line_cfg;
    int                  i;
    long                 index, value;
    char                *endptr = NULL, side;
    const char          *address, *port, *interface;
    int                  enable;

    /* a lines and b lines */
    for(side = 'a'; side < 'c'; side++) {
        /* fetch opra line data for the current side */
        switch(side) {
        case 'a':
            node = fh_cfg_get_node(config, "opra.a_lines");
            break;
        case 'b':
            node = fh_cfg_get_node(config, "opra.b_lines");
            break;
        default:
            FH_LOG(MGMT, ERR, ("Invalid OPRA line FT side: %c", side));
            return FH_ERROR;
        }

        if (!node) {
            FH_LOG(MGMT, ERR, ("Failed to find node for line %c", side));
            return FH_ERROR;
        }

        /* loop through all configurations for this side */
        for(i = 0; i < node->num_children; i++) {
            /* determine which line this configuration data belongs to */
            index = strtol(node->children[i]->name, &endptr , 0);
            if(*node->children[i]->name == '\0' || *endptr != '\0') {
                FH_LOG(MGMT, ERR, ("Line name, %s, is not numeric", node->children[i]->name));
                return FH_ERROR;
            }
            line = node->children[i];

            /* make sure the line number is in range */
            if(index > OPRA_CFG_MAX_FTLINES) {
                FH_LOG(MGMT, ERR, ("OPRA line ID is out of range: %d (max:%d)",
                                   index, OPRA_CFG_MAX_FTLINES));
                return FH_ERROR;
            }

            /* get the ftline and line structures */
            ftline_cfg = &opra_cfg->ocfg_lines[index - 1];
            switch(side) {
            case 'a':
                line_cfg = &ftline_cfg->oftl_line_a;
                line_cfg->ol_side = OPRA_CFG_LINE_A;
                break;
            case 'b':
                line_cfg = &ftline_cfg->oftl_line_b;
                line_cfg->ol_side = OPRA_CFG_LINE_B;
                break;
            default:
                FH_LOG(MGMT, ERR, ("Invalid OPRA line FT side: %c", side));
                return FH_ERROR;
            }

            /* make sure this line has not already been initialized */
            if (line_cfg->ol_init > 0) {
                FH_LOG(MGMT, ERR, ("OPRA process ID: line %c%d already initialized (%d)",
                                   side, index, line_cfg->ol_init));
                return FH_ERROR;
            }

            /* if line's 'enable' property is missing */
            enable = fh_opra_cfg_yesno(fh_cfg_get_string(line, "enable"));
            if (enable < 0) {
                FH_LOG(MGMT, ERR, ("enable attribute not found or invalid for line %c%d "
                                   "(default = disabled)", side, index));
            }

            /* if the line's 'enable' property is explicitly set to 'no' */
            if (enable <= 0) {
                line_cfg->ol_enable = 0;
                line_cfg->ol_init = 1;
                continue;
            }
            else {
                line_cfg->ol_enable = 1;
            }

            /* configure the port parameter of this line */
            port = fh_cfg_get_string(line, "port");
            if (!port) {
                FH_LOG(MGMT, ERR, ("port attribute not found for line %c%d", side, index));
                return FH_ERROR;
            }

            value = strtol(port, &endptr , 0);
            if(*port == '\0' || *endptr != '\0') {
                FH_LOG(MGMT, ERR, ("port must be numeric (was '%s')", port));
                return FH_ERROR;
            }
            line_cfg->ol_port = value;

            /* configure the mcaddr parameter of this line */
            address = fh_cfg_get_string(line, "address");
            if(address == NULL) {
                FH_LOG(MGMT, ERR, ("address attribute not found for line %c%d", side, index));
                return FH_ERROR;
            }
            line_cfg->ol_mcaddr = inet_addr(address);

            /* configure the interface parameter of this line */
            interface = fh_cfg_get_string(line, "interface");
            if(interface == NULL) {
                FH_LOG(MGMT, ERR, ("interface attribute not found for line %c%d", side, index));
                return FH_ERROR;
            }
            strcpy(line_cfg->ol_ifname, interface);

            /* mark this line as initialized */
            line_cfg->ol_init = 1;

            /* set this ftline index only once (not for both sides) */
            if(ftline_cfg->oftl_index == 0) {
                ftline_cfg->oftl_index = index;
                opra_cfg->ocfg_num_lines++;
            }

            /* log that line has been loaded */
            FH_LOG(MGMT, DIAG, ("Loaded OPRA line: %c%-2d addr:%-15s port:%s ifname:%s",
                                side, index, address, port, interface));
        }
    }

    return FH_OK;
}

/*
 * fh_opra_cfg_load
 *
 * Load the OPRA configuration, with all the process configuration and
 * the line configurations.
 */
FH_STATUS fh_opra_cfg_load(fh_cfg_node_t *config, int proc_id)
{
    FH_STATUS   rc;
    int         i;

    FH_ASSERT(config);

    if (opra_cfg_init) {
        FH_LOG(MGMT, WARN, ("OPRA configuration already loaded"));
        return FH_OK;
    }

    /*
     * Initialize the memory for the global OPRA configuration
     */
    memset(&opra_cfg, 0, sizeof(opra_cfg));

    /*
     * Save the OPRA process ID in the configuration structure
     */
    opra_cfg.ocfg_proc_id = proc_id;

    /*
     * Configuration of the topic format (hardcoded for now, until we have the more
     * advanced configuration file
     */
    memcpy(&opra_cfg.ocfg_topic_fmt, &opra_topic_fmt, sizeof(opra_topic_fmt));

    /*
     * Load opra general limits from generated config structure
     */
    rc = fh_opra_cfg_load_limits(config, &opra_cfg);
    if (rc != FH_OK) {
        return rc;
    }

    /*
     * Load the OPRA options if present
     */
    rc = fh_opra_cfg_load_options(config, &opra_cfg);
    if (rc != FH_OK) {
        return rc;
    }

    /*
     * Load the OPRA topic format if present
     */
    rc = fh_opra_cfg_load_topic_fmt(config);
    if (rc != FH_OK) {
        return rc;
    }

    /*
     * Load opra process information from generated config structure
     */
    rc = fh_opra_cfg_load_processes(config, &opra_cfg);
    if (rc != FH_OK) {
        return rc;
    }

    /*
     * Load opra line information from generated config structure
     */
    rc = fh_opra_cfg_load_lines(config, &opra_cfg);
    if (rc != FH_OK) {
        return rc;
    }

    /*
     * Load the listedoptions information from the generated config structure
     *
     */
    rc = fh_opra_lo_cfg_load(config, &opra_cfg.ocfg_lo_config);
    if (rc != FH_OK) {
    	return rc;
    }

    /*
     * Now load the configuration from hook if defined, so we can overwrite any
     * of the values that have been pre-loaded via configuration file.
     */
    opra_cfg_load = fh_plugin_get_hook(FH_PLUGIN_OPRA_CFG_LOAD);
    if (opra_cfg_load) {
        opra_cfg_load(&rc, &opra_cfg);
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("Plugin config loader failed: rc:%d", rc));
            return rc;
        }
    }

    /* check to make sure that lines that are enabled/disabled are done in FT pairs */
    for (i = 0; i < OPRA_CFG_MAX_FTLINES; i++) {
        fh_opra_line_t *a = &opra_cfg.ocfg_lines[i].oftl_line_a;
        fh_opra_line_t *b = &opra_cfg.ocfg_lines[i].oftl_line_b;

        if ((a->ol_enable && !b->ol_enable) || (!a->ol_enable && b->ol_enable)) {
            FH_LOG(CSI, ERR, ("A and B lines must be enabled/disabled together (line %d)", i + 1));
            return FH_ERROR;
        }
    }

    /*
     * Ensure that the process id passed to this function is a valid opra process number
     */
    if (proc_id >= opra_cfg.ocfg_num_procs) {
        FH_LOG(MGMT, ERR, ("Invalid OPRA process ID: %d must be less than %d",
                           proc_id, opra_cfg.ocfg_num_procs));
        return FH_ERROR;
    }

    /*
     * For development only, dump the entire configuration, once it
     * is fully loaded.
     */
    if (FH_LL_OK(MGMT, DIAG)) {
        fh_opra_cfg_dump();
    }

    /*
     * Set configuration as initialized and return success status code
     */
    opra_cfg_init = 1;

    return FH_OK;
}

/*
 * fh_opra_cfg_dump
 *
 * Dump teh configuration to the logs.
 */
void fh_opra_cfg_dump()
{
    fh_opra_cfg_t *ocfg = &opra_cfg;
    register uint32_t i;

    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("> OPRA Configuration:"));
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("> Number of processes    : %d", ocfg->ocfg_num_procs));
    FH_LOG_PGEN(DIAG, ("> Number of FT lines     : %d", ocfg->ocfg_num_lines));
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("> OPRA Processes:"));

    for (i=0; i<ocfg->ocfg_num_procs; i++) {
        fh_opra_proc_t *op = &ocfg->ocfg_procs[i];

        FH_LOG_PGEN(DIAG, ("   - Proc %2d: CPU: %2d Lines [ %2d - %2d ]",
                           op->op_idx,
                           op->op_cpu,
                           op->op_line_from,
                           op->op_line_to));
    }

    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("> OPRA Lines:"));

    for (i=0; i<ocfg->ocfg_num_lines; i++) {
        fh_opra_ftline_t *oftl = &ocfg->ocfg_lines[i];
        fh_opra_line_t *ol = NULL;

        FH_LOG_PGEN(DIAG, ("   - FT Line: %d", oftl->oftl_index));

        ol = &oftl->oftl_line_a;
        FH_LOG_PGEN(DIAG, ("     - A: %-15s %d - %s",
                           fh_net_ntoa(ol->ol_mcaddr), ol->ol_port, ol->ol_ifname));

        ol = &oftl->oftl_line_b;
        FH_LOG_PGEN(DIAG, ("     - B: %-15s %d - %s",
                           fh_net_ntoa(ol->ol_mcaddr), ol->ol_port, ol->ol_ifname));
    }
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
}

