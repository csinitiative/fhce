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
#include <syslog.h>
#include <pthread.h>
#include <stdarg.h>
#include "fh_log.h"
#include "fh_util.h"
#include "fh_cpu.h"
#include "fh_time.h"
#include "fh_config.h"
#include "fh_plugin_internal.h"
#include "fh_plugin.h"

uint32_t fh_log_lvl[FH_LC_MAX];
uint32_t fh_log_cfg;

/*
 * Logging modes
 */
#define LOG_MODE_NONE    (0)
#define LOG_MODE_SYSLOG  (1)
#define LOG_MODE_FILE    (2)

static char   fh_log_ident[8] = "FHLOG";
static int    fh_log_mode     = LOG_MODE_NONE;
static FILE  *fh_log_file     = NULL;
static int    fh_log_facility = LOG_USER;

static fh_plugin_hook_t fh_log_hook;

/*
 * Message log memory pool management
 */
#define LOG_NMSGS   (10)
#define LOG_MSGSZ   (1024)
#define LOG_MSGHDR  (1)
#define LOG_MSGMAX  (LOG_MSGSZ - LOG_MSGHDR)

static char            msg_table[LOG_NMSGS * LOG_MSGSZ];
static int             msg_index = 0;
static int             msg_inuse = 0;
static pthread_mutex_t msg_lock;
static int             msg_init = 0;

/*
 * log_msg_init
 *
 * Initialize the log message pool lock
 */
static void log_msg_init()
{
  if (!msg_init) {
    pthread_mutex_init(&msg_lock, NULL);
  }
  msg_init = 1;
}

/*
 * log_msg_clean
 *
 * Cleanup the logging message pool lock
 */
static void log_msg_clean()
{
  msg_init = 0;
  pthread_mutex_destroy(&msg_lock);
}

/*
 * log_msg_new
 *
 * Get a new log message entry from the log message pool to avoid
 * having another thread overwritting the log message before we
 * get a chance to dump it to a file or syslog.
 */
static char *log_msg_new()
{
    char *msg = NULL;

    if (!msg_init) {
        log_msg_init();
    }

    if (pthread_mutex_lock(&msg_lock) != -1) {

        /*
         * If there is still a free buffer in the pool, then
         * we look for it. A free log message contains a 0 in the
         * 1-byte message header, and a 1 if it is already in use
         */

        if (msg_inuse != LOG_NMSGS) {
            int cnt = LOG_NMSGS;
            char *msg_hdr;

            while (cnt) {
                msg_hdr   = msg_table + LOG_MSGSZ * msg_index;
                msg_index = (msg_index + 1) % LOG_NMSGS;

                if (*((uint8_t *)msg_hdr) == 0) {
                    *((uint8_t *)msg_hdr) = 1;
                    break;
                }
                else {
                    msg_hdr = NULL;
                }
                cnt--;
            }

            if (msg_hdr) {
                msg = msg_hdr + LOG_MSGHDR;
                msg_inuse++;
            }
        }

        pthread_mutex_unlock(&msg_lock);
    }

    return msg;
}

/*
 * log_msg_free
 *
 * Return a log message entry to the message pool.
 */
static void log_msg_free(char *msg)
{
    if (!msg_init) {
        log_msg_init();
    }

    if (!msg) {
        return;
    }

    if (pthread_mutex_lock(&msg_lock) != -1) {
        char *msg_hdr = msg - LOG_MSGHDR;

        *((uint8_t *)msg_hdr) = 0;
        msg_inuse--;

        pthread_mutex_unlock(&msg_lock);
    }
}

/*
 * fh_log_set_ident
 *
 * Set a different syslog identifier.
 */
void fh_log_set_ident(char* ident)
{
    if (ident) {
        strncpy(fh_log_ident, ident, sizeof(fh_log_ident));
    }
}


/*
 * fh_log_open
 *
 * Open syslog with the USER facility.
 */
void fh_log_open()
{
    fh_log_open_facility(LOG_USER);
}

/*
 * fh_log_close
 *
 * Close the log
 */
void fh_log_close()
{
    if (fh_log_mode == LOG_MODE_SYSLOG) {
        closelog();
    }
    else if (fh_log_file) {
        fclose(fh_log_file);
        fh_log_file = NULL;
    }

    fh_log_facility = LOG_USER;

    log_msg_clean();
}

/*
 * fh_log_open_facility
 *
 * Open syslog wit the specified facility.
 */
void fh_log_open_facility(int facility)
{
    static int log_init = 0;

    if (!log_init) {
        fh_log_facility = facility;
        fh_log_init(NULL);
    }
}

/*
 * fh_log_init
 *
 * Initialize logging, either syslog if the filename is set to NULL,
 * or to a local file specified by 'filename'.
 */
void fh_log_init(char* filename)
{
    static int log_init = 0;

    if (!log_init) {
        register int i;
        char* lmode;

        for (i=0; i<FH_LC_MAX; i++) {
            fh_log_lvl[i] = FH_LL_DEFAULT;
        }

        log_init = 1;
        log_msg_init();

        fh_log_hook = fh_plugin_get_hook(FH_PLUGIN_LOG_MSG);

        fh_log_mode = LOG_MODE_SYSLOG;

        if ((filename) && (filename[0] != 0)) {
            fh_log_file = fopen(filename, "a");
            if (fh_log_file != NULL) {
                fh_log_mode = LOG_MODE_FILE;
                fh_log_set_cfg(FH_LCF_LOGTIME);
            }
        }

        if (fh_log_mode == LOG_MODE_SYSLOG) {
            openlog(fh_log_ident, LOG_CONS, fh_log_facility);
        }

        if (fh_log_mode == LOG_MODE_SYSLOG) {
            lmode = "Syslog";
        }
        else if (fh_log_mode == LOG_MODE_FILE) {
            lmode = "File";
        }
        else {
            lmode = "?????";
        }

        FH_LOG(CSI, STATE, ("FH> Logging system started - mode: %s ", lmode));
    }
}

/*
 * log_print
 */
static void log_print(char *buffer)
{
    FILE *out = NULL;

    if (fh_log_cfg & FH_LCF_CONSOLE) {
        out = stdout;
    }
    else if (fh_log_mode == LOG_MODE_FILE) {
        out = fh_log_file;
    }

    if (out) {
        if (fh_log_cfg & FH_LCF_LOGTIME) {
            uint64_t now;
            uint32_t sec;
            FH_STATUS rc;
            char time_buffer[64];

            fh_time_get(&now);

            sec = (uint32_t) (now / 1000000);

            rc = fh_time_fmt(now, time_buffer, sizeof(time_buffer));
            if (rc == FH_OK) {
                fprintf(out, "%s.%06d: ", time_buffer, (uint32_t) (now % 1000000));
            }
            else {
                fprintf(out, "%05d.%06d: ", sec, (uint32_t) (now % 1000000));
            }
        }
        fprintf(out, "%s\n", buffer);
    }
    else {
        syslog(LOG_INFO | fh_log_facility, "%s", buffer);
    }
}


/*
 * fh_log_fmt
 *
 * Format a log message.
 */
char *fh_log_fmt(const char *fmt, ...)
{
    char *msg = log_msg_new();

    if (msg) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(msg, LOG_MSGMAX, fmt, args);
        va_end(args);
    }

    return msg;
}

/*
 * fh_log
 *
 * Log a message to a file or syslog.
 */
void fh_log(const char *file, int line, const char *clss, const char *level, char *msg)
{
    char              buffer[LOG_MSGSZ];
    char              *buf = buffer;
    size_t            l, len = sizeof(buffer);
    FH_STATUS         rc;

    if (fh_log_hook) {
        (*fh_log_hook)(&rc, file, line, clss, level, msg);
    }
    else {
        if (level) {
            l = snprintf(buf, len, "%6s: ", level);
            buf += l;
            len -= l;
        }

        if (msg) {
            l = snprintf(buf, len, "%s", msg);
            buf += l;
            len -= l;
        }

        if (fh_log_cfg & FH_LCF_DEBUG) {
            char *ptr = strrchr(file, '/');

            file = ptr ? ptr+1 : file;

            if (clss) {
                l = snprintf(buf, len, " - %s", clss);
                buf += l;
                len -= l;
            }

            l = snprintf(buf, len, " at <%s:%d>", file, line);
            buf += l;
            len -= l;
        }

        log_print(buffer);
    }

    log_msg_free(msg);
}

/*
 * fh_log_thread_start
 *
 * Logs the information about the running thread. This will dump the
 * name of thread along with the thread ID and the current CPU mask.
 */
void fh_log_thread_start(char *name)
{
    uint32_t cpumask;
    char *cpumask_str, buf[32];

    if (fh_cpu_getaffinity(&cpumask) != FH_OK) {
        cpumask_str = "?";
    }
    else {
      fh_cpu_print(cpumask, buf, sizeof(buf));
      cpumask_str = buf;
    }

    FH_LOG(CSI, STATE, ("Start thread: name:%s tid:%d pid:%d cpu:%s",
                         name, gettid(), getpid(), cpumask_str));
}

/*
 * fh_log_thread_stop
 *
 * Log the fact that a thread stopped.
 */
void fh_log_thread_stop(char *name)
{
    FH_LOG(CSI, STATE, ("Stop thread: name:%s tid:%d pid:%d",
                         name, gettid(), getpid()));
}


/*
 * Logging classes
 */
typedef struct {
    char     *lc_name;
    char     *lc_help;
    uint32_t  lc_val;
} log_class_t;

#define LOG_CFG_CLASS(b, h)  { #b, h, FH_LC_ ## b }

static log_class_t log_class_def[] = {
    LOG_CFG_CLASS( CSI    , "Default Class"                        ),
    LOG_CFG_CLASS( NET    , "Channel Layer"                        ),
    LOG_CFG_CLASS( LH     , "Line Handler"                         ),
    LOG_CFG_CLASS( MGMT   , "Management Framework"                 ),
    LOG_CFG_CLASS( CTRL   , "Messaging Control"                    ),
    LOG_CFG_CLASS( PUB    , "Messaging Publication"                ),
    { NULL, NULL, 0 },
};

/*
 * fh_log_get_class
 *
 * Return the logging class value for a given logging class name.
 */
FH_STATUS fh_log_get_class(const char *lc_name, uint32_t *lc_val)
{
    register int i = 0;
    log_class_t *lc = NULL;

    while ((lc = &log_class_def[i]) && lc->lc_name) {
        if (strcmp(lc_name, lc->lc_name) == 0) {
            *lc_val = lc->lc_val;
            return FH_OK;
        }

        i++;
    }

    return FH_ERR_NOTFOUND;
}


/*
 * Logging levels
 */
typedef struct {
    char     *ll_name;
    char     *ll_help;
    uint32_t  ll_val;
} log_level_t;

#define LOG_CFG_LEVEL(b, h)  { #b, h, FH_LL_ ## b }

static log_level_t log_level_def[] = {
    LOG_CFG_LEVEL( ERR      , "errors"                      ),
    LOG_CFG_LEVEL( WARN     , "warnings"                    ),
    LOG_CFG_LEVEL( STATE    , "state transitions"           ),
    LOG_CFG_LEVEL( VSTATE   , "verbose state transitions"   ),
    LOG_CFG_LEVEL( DIAG     , "diagnostics"                 ),
    LOG_CFG_LEVEL( INFO     , "information logging"         ),
    LOG_CFG_LEVEL( STATS    , "statistics logging"          ),
    LOG_CFG_LEVEL( XSTATS   , "extended statistics logging" ),
    { NULL, NULL, 0 },
};

/*
 * fh_log_get_lvl
 *
 * Returns the logging level for a given logging name tag.
 */
FH_STATUS fh_log_get_lvl(const char *ll_name, uint32_t *ll_val)
{
    register int i = 0;
    log_level_t *ll = NULL;

    while ((ll = &log_level_def[i]) && ll->ll_name) {
        if (strcmp(ll_name, ll->ll_name) == 0) {
            *ll_val |= ll->ll_val;
            return FH_OK;
        }

        i++;
    }

    return FH_ERR_NOTFOUND;
}

/*
 * fh_log_fmt_lvl
 *
 * Format a list of logging levels from a logging level bit mask.
 */
char *fh_log_fmt_lvl(uint32_t lvl)
{
    static char   ll_str[256];
    register int  i = 0;
    char         *q = ll_str;
    log_level_t  *ll = NULL;

    *q = 0;

    while((ll = &log_level_def[i]) && ll->ll_name) {
        if(ll->ll_val & lvl) {
            q += sprintf(q, "%c%s", ((i) ? '.' : '-'), ll->ll_name);
        }
        i++;
    }

    *q++ = '-';
    *q = 0;

    return ll_str;
}


/*
 * Logging configuration settings
 */

typedef struct {
    char     *lcf_name;
    char     *lcf_help;
    uint32_t  lcf_val;
} log_config_t;

#define LOG_CFG_CONFIG(b, h)  { #b, h, FH_LCF_ ## b }

static log_config_t log_config_def[] = {
    LOG_CFG_CONFIG( CONSOLE  , "Redirect log to stdout"            ),
    LOG_CFG_CONFIG( DEBUG    , "Debug logging with file/line info" ),
    LOG_CFG_CONFIG( LOGTIME  , "Add console/file time"             ),
    { NULL, NULL, 0 },
};

/*
 * fh_log_get_cfg
 *
 * Return a configuration value from a configuration tag
 */
FH_STATUS fh_log_get_cfg(const char *lcf_name, uint32_t *lcf_val)
{
    register int i = 0;
    log_config_t *lcf = NULL;

    while ((lcf = &log_config_def[i]) && lcf->lcf_name) {
        if (strcmp(lcf_name, lcf->lcf_name) == 0) {
            *lcf_val |= lcf->lcf_val;
            return FH_OK;
        }

        i++;
    }

    return FH_ERR_NOTFOUND;
}

/*
 * fh_log_dump_lvls
 *
 * List all levels for all classes
 */
void fh_log_dump_lvls()
{
    register int i = 0;
    log_class_t *lc = NULL;

    FH_LOG_PGEN(DIAG,  ("-----------------------------------------------------"));
    FH_LOG_PGEN(DIAG,  ("-- Logging levels                                    "));
    FH_LOG_PGEN(DIAG,  ("-----------------------------------------------------"));

    while ((lc = &log_class_def[i]) && lc->lc_name) {
        uint32_t ll = fh_log_lvl[lc->lc_val];

        FH_LOG_PGEN(DIAG,  (" - %-10s : %2d : 0x%08x %s",
                             lc->lc_name, lc->lc_val, ll, fh_log_fmt_lvl(ll)));
        i++;
    }

    FH_LOG_PGEN(DIAG,  ("--------------------------------------------------"));
}

/*-----------------------------------------------------------------------*/
/* -- Config Paramater definition -------------------------------------- */
/*-----------------------------------------------------------------------*/

/*
 * Set logging levels if the passed in config contains logging level information
 *
 * @param  (const fh_cfg_node_t *) config being processed
 * @return FH_STATUS
 */
FH_STATUS fh_log_set_levels(const fh_cfg_node_t *config)
{
    const fh_cfg_node_t *node;
    int                 i, j, rc;
    uint32_t            class = 0;
    uint32_t            level = 0;

    // fetch the appropriate node and return ok if that node does not exist
    node = fh_cfg_get_node(config, "log.level");
    if(node == NULL) return FH_OK;

    // loop through all of the levels that are set
    for(i = 0; i < node->num_children; i++) {
        // convert the class string to an actual class
        rc = fh_log_get_class(node->children[i]->name, &class);
        if(rc != FH_OK) return rc;

        // loop through all values for this class setting each as we go
        for(j = 0; j < node->children[i]->num_values; j++) {
            rc = fh_log_get_lvl(node->children[i]->values[j], &level);
            if(rc != FH_OK) return rc;

            fh_log_lvl[class] |= level;
        }
    }

    return FH_OK;
}

/*
 * Set logging level defaults
 *
 * @param  (const fh_cfg_node_t *) config being processed
 * @return FH_STATUS
 */
FH_STATUS fh_log_set_defaults(const fh_cfg_node_t *config)
{
    const fh_cfg_node_t *node;
    int                 i, rc;
    uint32_t            level = 0;

    // fetch the appropriate node and return ok if that node does not exist
    node = fh_cfg_get_node(config, "log.default");
    if(node == NULL) return FH_OK;

    // loop through all default values adding them to the default level as we go
    for(i = 0; i < node->num_values; i++) {
        rc = fh_log_get_lvl(node->values[i], &level);
        if(rc != FH_OK) return rc;
    }

    // assign the new level to all classes
    for(i = 0; i < FH_LC_MAX; i++) {
        fh_log_lvl[i] |= level;
    }

    return FH_OK;
}

/*
 * Set logging configuration options
 *
 * @param  (const fh_cfg_node_t *) config being processed
 * @return FH_STATUS
 */
FH_STATUS fh_log_set_config(const fh_cfg_node_t *config)
{
    const fh_cfg_node_t *node;
    int                 i, rc;

    // fetch the appropriate node and return ok if that node does not exist
    node = fh_cfg_get_node(config, "log.config");
    if(node == NULL) return FH_OK;

    // loop through all config options setting them as we go
    for(i = 0; i < node->num_values; i++) {
        rc = fh_log_get_cfg(node->values[i], &fh_log_cfg);
        if(rc != FH_OK) return rc;
    }

    return FH_OK;
}

/**
 * Load logging configuration from the provided configuration structure
 *
 * @param  (const fh_cfg_node_t *) configuration structure
 * @return FH_STATUS
 */
FH_STATUS fh_log_cfg_load(const fh_cfg_node_t *config)
{
    FH_STATUS rc;

    // set logging configuration options
    rc = fh_log_set_config(config);
    if(rc != FH_OK) return rc;

    // set logging level defaults
    rc = fh_log_set_defaults(config);
    if(rc != FH_OK) return rc;

    // set logging levels from config
    rc = fh_log_set_levels(config);
    if(rc != FH_OK) return rc;

    return FH_OK;
}

/*
 * fh_log_set_cfg
 *
 * Set a configuration setting for the logging sub-system
 */
void fh_log_set_cfg(uint32_t cfg)
{
    fh_log_cfg |= cfg;
}

/*
 * fh_log_clr_cfg
 *
 * Clear a configuration setting from the logging sub-system
 */
void fh_log_clr_cfg(uint32_t cfg)
{
    fh_log_cfg &= ~cfg;
}

/*
 * fh_log_set_lvl
 *
 * Set log level for all classes.
 */
void fh_log_set_lvl(uint32_t lvl)
{
    register int i;

    for (i=0; i<FH_LC_MAX; i++) {
        fh_log_lvl[i] |= lvl;
    }
}

/*
 * fh_log_clr_lvl
 *
 * Clear log level for all classes.
 */
void fh_log_clr_lvl(uint32_t lvl)
{
    register int i;

    for (i=0; i<FH_LC_MAX; i++) {
        fh_log_lvl[i] &= ~lvl;
    }
}

/*
 * fh_log_set_class
 *
 * Set a logging level for a given logging class.
 */
void fh_log_set_class(uint32_t lc, uint32_t lvl)
{
    if (lc < FH_LC_MAX) {
        fh_log_lvl[lc] |= lvl;
    }
}

/*
 * fh_log_clr_class
 *
 * Clear a logging level for a logging class.
 */
void fh_log_clr_class(uint32_t lc, uint32_t lvl)
{
    if (lc < FH_LC_MAX) {
        fh_log_lvl[lc] &= ~lvl;
    }
}

