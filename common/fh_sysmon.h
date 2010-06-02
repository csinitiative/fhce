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

#ifndef __FH_SYSMON_H__
#define __FH_SYSMON_H__

#include "fh_errors.h"

/*
 * Virtual-Memory statistics
 */
typedef struct {
    uint64_t mem_total;
    uint64_t mem_free;
    uint64_t mem_buffered;
    uint64_t mem_cached;
} fh_mon_mem_t;

/*
 * System statistics
 */
typedef struct {
    uint16_t  sys_clock_rate;
    uint16_t  sys_num_cpus;
    uint32_t  sys_procs_total;
    uint64_t  sys_intr;
    uint64_t  sys_ctxt;
    uint64_t  sys_procs;
    uint64_t  sys_procs_running;
    uint64_t  sys_procs_blocked;
} fh_mon_sys_t;

/*
 * CPU statistics
 */
typedef struct {
    uint32_t  cpu_num;
    int64_t   cpu_user;
    int64_t   cpu_nice;
    int64_t   cpu_idle;
    int64_t   cpu_system;
    int64_t   cpu_wait;
    int64_t   cpu_hintr;
    int64_t   cpu_sintr;
    int64_t   cpu_steal;
} fh_mon_cpu_t;

/*
 * Process statistics
 */
typedef struct {
    int       proc_pid;
    int       proc_tid;
    int32_t   proc_lcpu;
    int64_t   proc_utime;
    int64_t   proc_stime;
    int64_t   proc_size;
    int64_t   proc_resident;
    int64_t   proc_share;
    int64_t   proc_text;
    int64_t   proc_lib;
    int64_t   proc_data;
    int64_t   proc_dirty;
} fh_mon_proc_t;

/*
 * System monitoring API
 */
FH_STATUS fh_mon_mem_stats(fh_mon_mem_t *mem);
FH_STATUS fh_mon_cpu_stats(fh_mon_sys_t *sys, fh_mon_cpu_t *cpu_table, int max_cpus);
FH_STATUS fh_mon_proc_stats(fh_mon_proc_t *proc, int pid, int tid);

#endif /* __FH_SYSMON_H__ */
