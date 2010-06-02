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

#ifndef __FH_MGMT_SERVICE_H__
#define __FH_MGMT_SERVICE_H__

#include "fh_sysmon.h"

#include "fh_mgmt_conn.h"
#include "fh_mgmt_admin.h"

/*
 * Service definition
 */
struct fh_serv;
struct fh_sg;

typedef TAILQ_ENTRY(fh_serv) serv_le_t;
typedef TAILQ_HEAD(,fh_serv) serv_lh_t;

typedef struct fh_serv {
    serv_le_t       serv_le;        /* Linked-list element                  */
    char           *serv_name;      /* Service name                         */
    char           *serv_command;   /* Service command line                 */
    char           *serv_args;      /* Service command args                 */
    fh_mgmt_conn_t *serv_conn;      /* Service connection                   */
    uint32_t        serv_mask;      /* Service bitmask                      */
    uint32_t        serv_flags;     /* Service flags/states                 */
    struct fh_sg   *serv_group;     /* Back-pointer to the group            */
    fh_mon_cpu_t    serv_cpu;       /* Previous total CPU states            */
    fh_mon_proc_t   serv_proc;      /* Previous Process states              */
} fh_mgmt_serv_t;

/*
 * Service states
 */

#define FH_MGMT_SERV_ENABLED            (0x00000001)
#define FH_MGMT_SERV_DISABLED           (0x00000002)
#define FH_MGMT_SERV_STARTED            (0x00000004)
#define FH_MGMT_SERV_RUNNING            (0x00000008)
#define FH_MGMT_SERV_STOPPED            (0x00000010)
#define FH_MGMT_SERV_ADMIN_STOPPED      (0x00000020)
#define FH_MGMT_SERV_STATS              (0x00000040)
#define FH_MGMT_SERV_WAIT               (0x00000080)
#define FH_MGMT_SERV_SPAWN              (0x00000100)
#define FH_MGMT_SERV_CLI                (0x00000200)
#define FH_MGMT_SERV_RESTART            (0x00000400)
#define FH_MGMT_SERV_STOPPING           (0x00000800)
#define FH_MGMT_SERV_ADMIN_STOPPING     (0x00001000)
#define FH_MGMT_SERV_RESPAWN            (0x00002000)
#define FH_MGMT_SERV_SHUTDOWN           (0x00004000)

/*
 * Define the arbitrary maximum number of lines for a service group
 */
#define SG_MAX_LINES (48)

/*
 * Service group report for all lines
 */
typedef struct {
    char                 sg_rpt_name[16];
    uint32_t             sg_rpt_line_count;
    fh_adm_line_stats_t  sg_rpt_lines[SG_MAX_LINES];
    fh_adm_line_stats_t  sg_rpt_total;
} fh_mgmt_sg_rpt_t;

/*
 * Service group definition
 *
 * A service group is a list of services that have to be started and stopped
 * at the same time, in the order in which they are listed in the configuration.
 */
struct fh_sg;

typedef TAILQ_ENTRY(fh_sg) sg_le_t;
typedef TAILQ_HEAD(,fh_sg) sg_lh_t;

typedef struct fh_sg {
    sg_le_t     sg_le;              /* Linked-list element                  */
    sg_le_t     sg_timer_le;        /* Timer Linked-list element            */
    char       *sg_name;            /* Service group name                   */
    char       *sg_report_name;     /* Service group report name            */
    serv_lh_t   sg_serv_lh;         /* Service list                         */
    uint32_t    sg_serv_count;      /* Number of services in this group     */
    uint32_t    sg_flags;           /* Service group flags/states           */
    uint32_t    sg_up_mask;         /* Up Services bitmask                  */
    uint32_t    sg_restart_time;    /* Restart time in secs after midnight  */
    uint32_t    sg_restart_deadline;/* Restart deadline in secs             */
    uint32_t    sg_restart_ticks;   /* Restart ticks in secs                */
    uint32_t    sg_stats_period;    /* Statistics period in secs            */
    uint32_t    sg_stats_ticks;     /* Statistics ticks in secs             */
    uint32_t    sg_respawn_period;  /* Respawn period in seconds            */
    uint32_t    sg_respawn_ticks;   /* Respawn ticks in seconds             */
    // Reporting data
    fh_mgmt_sg_rpt_t sg_rpt;        /* Reporting snapshot                   */
    pthread_mutex_t  sg_rpt_lock;   /* Reporting lock                       */
} fh_mgmt_sg_t;

/*
 * Service group flags/states
 */
#define FH_MGMT_SG_ENABLED          (0x00000001)
#define FH_MGMT_SG_DISABLED         (0x00000002)
#define FH_MGMT_SG_STARTED          (0x00000004)
#define FH_MGMT_SG_RUNNING          (0x00000008)
#define FH_MGMT_SG_STOPPED          (0x00000010)
#define FH_MGMT_SG_STATS            (0x00000020)
#define FH_MGMT_SG_RESTART          (0x00000040)

/*
 * Service API
 */

void            fh_mgmt_serv_init(uint32_t spawn_delay);

fh_mgmt_serv_t *fh_mgmt_serv_lookup(char *serv_name);

FH_STATUS       fh_mgmt_serv_disable(fh_mgmt_serv_t *serv);
FH_STATUS       fh_mgmt_serv_enable(fh_mgmt_serv_t *serv);

FH_STATUS       fh_mgmt_serv_attach(fh_mgmt_serv_t *serv, fh_mgmt_conn_t *conn);
FH_STATUS       fh_mgmt_serv_detach(fh_mgmt_serv_t *serv);

FH_STATUS       fh_mgmt_serv_start(fh_mgmt_serv_t *serv);
FH_STATUS       fh_mgmt_serv_stop(fh_mgmt_serv_t *serv);

/**
 *  @brief Administratively (manually, from the command line) stop a given service
 *
 *  @param serv service that is being administratively stopped
 *  @return status code indicating success or failure
 */
FH_STATUS fh_mgmt_serv_admin_stop(fh_mgmt_serv_t *serv);

FH_STATUS       fh_mgmt_serv_restart(fh_mgmt_serv_t *serv);
FH_STATUS       fh_mgmt_serv_clrstats(fh_mgmt_serv_t *serv);


FH_STATUS       fh_mgmt_serv_process(fh_mgmt_serv_t *serv);

FH_STATUS       fh_mgmt_serv_request(fh_mgmt_serv_t *serv,
                                     fh_adm_serv_req_t      *serv_req,
                                     fh_adm_serv_resp_t     *serv_resp, char *ptr);

FH_STATUS       fh_mgmt_serv_request_all(fh_adm_serv_req_t  *serv_req,
                                         fh_adm_serv_resp_t *serv_resp, char *ptr);

fh_mgmt_serv_t *fh_mgmt_serv_new(const char *name, const char *command, const char *args,
                                 int disable, int stats_enable, int kill_enabled,
                                 const char *action);

/*
 * Service Group API
 */
fh_mgmt_sg_t   *fh_mgmt_sg_new(const char *name, const char *report_name,
                               uint32_t restart_time, uint32_t stats_period, int disable);

fh_mgmt_sg_t   *fh_mgmt_sg_lookup(char *name);

/**
 *  @brief Provides the complete list of service groups to the requestor
 *  BE VERY CAREFUL TO TREAD THE RETURNED LIST AS READ ONLY
 *
 *  @return a pointer to the service group hash table
 */
sg_lh_t        *fh_mgmt_sg_getall();

FH_STATUS       fh_mgmt_sg_add_serv(fh_mgmt_sg_t *sg, fh_mgmt_serv_t *serv);

FH_STATUS       fh_mgmt_sg_request(fh_mgmt_sg_t *sg,
                                   fh_adm_serv_req_t   *serv_req,
                                   fh_adm_serv_resp_t  *serv_resp, char *ptr);

void            fh_mgmt_sg_dump();
void            fh_mgmt_sg_timers();
void            fh_mgmt_sg_rpt_save(fh_mgmt_sg_t *sg, fh_mgmt_sg_rpt_t *sg_rpt);
void            fh_mgmt_sg_rpt_snapshot(fh_mgmt_sg_t *sg, fh_mgmt_sg_rpt_t *sg_rpt);
void            fh_mgmt_sg_rpt_reset(fh_mgmt_sg_t *sg);
void            fh_mgmt_sg_restart_time(fh_mgmt_sg_t *sg, uint32_t restart_time);
void            fh_mgmt_sg_stats_period(fh_mgmt_sg_t *sg, uint32_t stats_period);

fh_adm_sgt_resp_t *fh_mgmt_sgt_resp();

/**
 *  @brief Perform initialization post-processing to clean up anything changed by a plugin
 */
void fh_mgmt_serv_post_init();

#endif /* __FH_MGMT_SERVICE_H__ */
