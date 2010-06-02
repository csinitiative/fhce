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

/*
 * System includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sysinfo.h>

/*
 * FH includes
 */
#include "fh_log.h"
#include "fh_util.h"
#include "fh_sysmon.h"

/*
 * fh_mon_mem_stats
 *
 * Get the VM stats for the system.
 */
FH_STATUS fh_mon_mem_stats(fh_mon_mem_t *mem)
{
#define NUM_FIELDS (4)
    char *fields[NUM_FIELDS];
    FILE *fh;
    char  buffer[1024];
    int   numfields;

    fh = fopen("/proc/meminfo", "r");
    if (!fh) {
        FH_LOG(CSI, ERR, ("Failed to open /proc/meminfo: %m"));
        return FH_ERROR;
    }

    while (fgets(buffer, sizeof(buffer), fh) != NULL) {
        uint64_t *val;

        if (strncasecmp(buffer, "MemTotal:", 9) == 0)
            val = &mem->mem_total;
        else if (strncasecmp(buffer, "MemFree:", 8) == 0)
            val = &mem->mem_free;
        else if (strncasecmp (buffer, "Buffers:", 8) == 0)
            val = &mem->mem_buffered;
        else if (strncasecmp(buffer, "Cached:", 7) == 0)
            val = &mem->mem_cached;
        else
            break;

        numfields = fh_strsplit(buffer, fields, NUM_FIELDS);

        if (numfields < 2) continue;

        *val = atoll(fields[1]) * 1024LL;
    }

    fclose(fh);

    return FH_OK;
#undef NUM_FIELDS
}

/*
 * fh_mon_cpu_stats
 *
 * System and CPU statistics
 */
FH_STATUS fh_mon_cpu_stats(fh_mon_sys_t *sys, fh_mon_cpu_t *cpu_table, int max_cpus)
{
#define NUM_FIELDS (10)
    FILE            *fh;
    char             buffer[1024];
    char            *fields[NUM_FIELDS];
    int              numfields;
    int              numcpus = 0;
    static uint16_t  cpu_count  = 0;
    static uint32_t  clock_rate = 0;

    /*
     * Read the CPU clock rate and total number of CPUs in the system only once
     */
    if (cpu_count == 0) {
        clock_rate = sysconf(_SC_CLK_TCK);
        cpu_count  = sysconf(_SC_NPROCESSORS_CONF);
    }

    memset(sys, 0, sizeof(fh_mon_sys_t));
    memset(cpu_table, 0, sizeof(fh_mon_cpu_t) * max_cpus);

    sys->sys_num_cpus   = cpu_count;
    sys->sys_clock_rate = clock_rate;

    {
        struct sysinfo info;
        sysinfo(&info);
        sys->sys_procs_total = info.procs;
    }

    fh = fopen("/proc/stat", "r");
    if (!fh) {
        FH_LOG(CSI, ERR, ("Failed to open /proc/stat: %m"));
        return FH_ERROR;
    }

    while (fgets(buffer, sizeof(buffer), fh) != NULL) {
        numfields = fh_strsplit(buffer, fields, NUM_FIELDS);

        if (strncmp(buffer, "cpu", 3)) {
            if (strncmp(buffer, "intr", 4) == 0) {
                sys->sys_intr = atoll(fields[1]);
            }
            else if (strncmp(buffer, "ctxt", 4) == 0) {
                sys->sys_ctxt = atoll(fields[1]);
            }
            else if (strncmp(buffer, "procs_running", 13) == 0) {
                sys->sys_procs_running = atoll(fields[1]);
            }
            else if (strncmp(buffer, "procs_blocked", 13) == 0) {
                sys->sys_procs_blocked = atoll(fields[1]);
            }
            else if (strncmp(buffer, "procs", 6) == 0) {
                sys->sys_procs = atoll(fields[1]);
            }
            continue;
        }

        if (buffer[3] < '0' || buffer[3] > '9') {
            continue;
        }

        if (numcpus < max_cpus) {
            fh_mon_cpu_t *cpu;

            cpu = &cpu_table[numcpus];

            cpu->cpu_num     = atoi (fields[0] + 3);
            cpu->cpu_user    = atoll(fields[1]);
            cpu->cpu_nice    = atoll(fields[2]);
            cpu->cpu_system  = atoll(fields[3]);
            cpu->cpu_idle    = atoll(fields[4]);
            cpu->cpu_wait    = atoll(fields[5]);
            cpu->cpu_hintr   = atoll(fields[6]);
            cpu->cpu_sintr   = atoll(fields[7]);

            if (numfields == 9) {
                cpu->cpu_steal = atoll(fields[8]);
            }
            else {
                cpu->cpu_steal = 0;
            }

            numcpus ++;
        }
    }

    fclose(fh);

    return FH_OK;
#undef NUM_FIELDS
}

/*
 * fh_mon_proc_stats
 *
 * Get the process statistics for a given PID and Thread ID (TID).
 */
FH_STATUS fh_mon_proc_stats(fh_mon_proc_t *proc, int pid, int tid)
{
#define NUM_FIELDS (100)
    static int64_t  pagesize = 0;
    char            buffer[1024];
    char            filename[256];
    char           *fields[NUM_FIELDS];
    FILE           *fp = NULL;

    if (pagesize == 0) {
        pagesize = sysconf(_SC_PAGESIZE);
    }

    memset(proc, 0, sizeof(fh_mon_proc_t));

    /*
     * Fill in the process PID and TID
     */
    proc->proc_pid = pid;
    proc->proc_tid = tid;

    /*
     * Look for CPU statistics for this particular process
     */
    sprintf(filename, "/proc/%d/task/%d/stat",  pid, tid);

    fp = fopen(filename, "r");
    if (!fp) {
        FH_LOG(CSI, ERR, ("Failed to open %s: %m", filename));
        return FH_ERROR;
    }

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        int numfields = fh_strsplit(buffer, fields, NUM_FIELDS);

        if (numfields > 15) {
            proc->proc_lcpu  = atoi(fields[38]);
            proc->proc_utime = atoll(fields[13]);
            proc->proc_stime = atoll(fields[14]);
        }
    }

    fclose(fp);

    /*
     * Get the memory usage of the process
     */
    sprintf(filename, "/proc/%d/task/%d/statm", pid, tid);

    fp = fopen(filename, "r");
    if (!fp) {
        FH_LOG(CSI, ERR, ("Failed to open %s: %m", filename));
        return FH_ERROR;
    }

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        int numfields = fh_strsplit(buffer, fields, NUM_FIELDS);

        if (numfields == 7) {
            proc->proc_size     = atoll(fields[0]) * pagesize;
            proc->proc_resident = atoll(fields[1]) * pagesize;
            proc->proc_share    = atoll(fields[2]) * pagesize;
            proc->proc_text     = atoll(fields[3]) * pagesize;
            proc->proc_lib      = atoll(fields[4]) * pagesize;
            proc->proc_data     = atoll(fields[5]) * pagesize;
            proc->proc_dirty    = atoll(fields[6]) * pagesize;
        }
    }

    fclose(fp);

    return FH_OK;
#undef NUM_FIELDS
}


