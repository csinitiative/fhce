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

#ifndef __FH_LOG_H__
#define __FH_LOG_H__

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include "fh_errors.h"
#include "fh_config.h"

/*
 * Logging library
 *
 * Each log entry is defined on given logging class and a logging level.
 * For the log entry, to show up in the logs, both the logging class and
 * level need to match the system log settings.
 *
 * By default, there is a list of logging levels that will be enabled for
 * all logging classes.
 */

/*
 * Logging classes
 */
#define FH_LC_CSI       (0)              /* Default Logging class */
#define FH_LC_NET       (1)              /* Networking layer */
#define FH_LC_LH        (2)              /* Line handler */
#define FH_LC_MGMT      (3)              /* Management layer */
#define FH_LC_CTRL      (4)              /* Messaging Control layer */
#define FH_LC_PUB       (5)              /* Messaging Publication layer */
#define FH_LC_MAX       (6)              /* Maximum number of logging classes */

/*
 * Logging levels
 */
#define FH_LL_ERR       (0x00000001)     /* errors */
#define FH_LL_WARN      (0x00000002)     /* warnings */
#define FH_LL_STATE     (0x00000004)     /* state transitions */
#define FH_LL_VSTATE    (0x00000008)     /* verbose state transitions */
#define FH_LL_DIAG      (0x00000010)     /* diagnostic level logging */
#define FH_LL_INFO      (0x00000020)     /* information logging */
#define FH_LL_STATS     (0x00000040)     /* statistics logging */
#define FH_LL_XSTATS    (0x00000080)     /* extended statistics logging */

/*
 * Default logging levels for all classes
 */
#define FH_LL_DEFAULT   (FH_LL_ERR | FH_LL_STATE)

/*
 * Logging configuration levels
 */
#define FH_LCF_CONSOLE  (0x00000001)     /* Redirect log to stdout */
#define FH_LCF_DEBUG    (0x00000002)     /* Set the debug level */
#define FH_LCF_LOGTIME  (0x00000004)     /* Show time on console logging */

/*
 * Logging macros
 *
 * These macros are defined such that the different logging levels for all
 * classes can be compiled in, but only get displayed when the level
 * is enabled for the corresponding class. The performance impact should
 * be minimal.
 */

/*
 * Check the logging level for a given logging class
 */
#define FH_CHK_LVL(c,l) (fh_log_lvl[c] & (l))
#define FH_LL_OK(c,l)   FH_CHK_LVL(FH_LC_ ## c, FH_LL_ ## l)

#define FH_LOG(c,l,a)                                                   \
do {                                                                    \
    if (FH_LL_OK(c,l)) {                                                \
        char *_msg = fh_log_fmt a;                                      \
        fh_log(__FILE__, __LINE__, #c, #l, _msg);                       \
    }                                                                   \
} while (0)

/*
 * Force the logging at a given level.
 */
#define FH_LOG_PGEN(l,a)                                                \
do {                                                                    \
    char *_msg = fh_log_fmt a;                                          \
    fh_log(__FILE__, __LINE__, NULL, #l, _msg);                         \
} while (0)


/*
 * Assertion handling
 */
#define FH_ASSERT(cond)                                                 \
do {                                                                    \
    int nok = !(cond);                                                  \
    if (nok) {                                                          \
        char *_msg = fh_log_fmt("assertion '%s' failed at %s:%d",       \
                                #cond, __FILE__, __LINE__);             \
        fh_log(__FILE__, __LINE__, NULL, "ERR", _msg);                  \
        exit(1);                                                        \
    }                                                                   \
} while (0)


/*
 * Logging initialization/termination API
 */
void      fh_log_init(char* filename);
void      fh_log_open();
void      fh_log_close();
void      fh_log_set_ident(char* ident);
void      fh_log_open_facility(int facility);

/*
 * Logging API
 */
char     *fh_log_fmt(const char *fmt, ...);
void      fh_log(const char *file, int line, const char *clss, const char *level, char *fmt);
void      fh_log_thread_start(char *name);
void      fh_log_thread_stop(char *name);

/*
 * Logging configuration management API
 */
FH_STATUS fh_log_cfg_load(const fh_cfg_node_t *);
FH_STATUS fh_log_get_class(const char *lc_name, uint32_t *lc_val);
FH_STATUS fh_log_get_lvl(const char *ll_name, uint32_t *ll_val);
FH_STATUS fh_log_get_cfg(const char *lcf_name, uint32_t *lcf_val);
void      fh_log_dump_lvls();

/*
 * Set/Clear logging configuration settings
 */
void      fh_log_set_cfg(uint32_t cfg);
void      fh_log_clr_cfg(uint32_t cfg);

/*
 * Set/Clear log level for all classes
 */
void      fh_log_set_lvl(uint32_t lvl);
void      fh_log_clr_lvl(uint32_t lvl);

/*
 * Set/Clear log level for a given class
 */
void      fh_log_set_class(uint32_t lc, uint32_t lvl);
void      fh_log_clr_class(uint32_t lc, uint32_t lvl);

extern uint32_t fh_log_lvl[];

#endif /* __FH_LOG_H__ */
