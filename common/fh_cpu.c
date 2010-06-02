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
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#define __USE_GNU /* For sched_getaffinity */
#include <sched.h>
#include <errno.h>
#include "fh_cpu.h"
#include "fh_log.h"
#include "fh_util.h"
#include "fh_time.h"

static uint32_t num_cpus = 0;

/*
 * fh_cpu_count
 *
 * Returns the number of CPU in the system.
 */
uint32_t fh_cpu_count()
{
    num_cpus = sysconf(_SC_NPROCESSORS_CONF);

    if ((uint32_t) num_cpus > __NCPUBITS) {
        num_cpus = __NCPUBITS;
    }

    return num_cpus;
}

/*
 * fh_cpu_print
 *
 * Dump the CPU list to the passed buffer. This function will fail if the
 * number of CPUs is greater than 32.
 */
void fh_cpu_print(uint32_t cpu_mask, char *buf, int size)
{
    char *ptr = buf;
    int len = 0, i, first = 1;

    if (cpu_mask) {
        for (i=0; i<32; i++) {
            if (cpu_mask & (1<<i)) {
               len = snprintf(ptr, size, "%s%d", first ? "" : ",", i);
               size  -= len;
               ptr   += len;
               first  = 0;
            }
        }
    }
    else {
        strcpy(buf, "?");
    }
}

/*
 * fh_cpu_getaffinity
 *
 * Return the CPU affinity of the running thread.
 */
FH_STATUS fh_cpu_getaffinity(uint32_t *cpu_mask)
{
    cpu_set_t cpu_set;
    unsigned int len = sizeof(cpu_set_t);
    uint32_t i, mask = 0, pid;

    __CPU_ZERO(&cpu_set);

    pid = gettid();

    if (num_cpus == 0) {
        num_cpus = fh_cpu_count();
    }

    if (sched_getaffinity(pid, len, &cpu_set) < 0) {
        FH_LOG(CSI, ERR, ("sched_getaffinity failed: %s (%d)", strerror(errno), errno));
        return FH_ERROR;
    }

    for (i=0; i<num_cpus; i++) {
        FH_LOG(CSI, INFO, ("  %d: %s", i, __CPU_ISSET(i, &cpu_set) ? "set" : "noset"));

        if (__CPU_ISSET(i, &cpu_set)) {
            mask |= (1<<i);
        }
    }

    *cpu_mask = mask;

    return FH_OK;
}

/*
 * fh_cpu_setaffinity
 *
 * Set CPU affinity for the running thread to the provided CPU mask.
 */
FH_STATUS fh_cpu_setaffinity(int cpu_mask)
{
    cpu_set_t cpu_set;
    int i = 0, pid;

    pid = gettid();

    __CPU_ZERO(&cpu_set);

    while (cpu_mask) {
        if (cpu_mask & 0x1) {
            FH_LOG(CSI, INFO, ("set affinity on cpu %d", i));
            __CPU_SET(i, &cpu_set);
        }
        i++;
        cpu_mask >>= 1;
    }

    if (sched_setaffinity(pid, sizeof(cpu_set_t), &cpu_set)) {
        FH_LOG(CSI, WARN, ("sched_setaffinity failed: %s (%d)", strerror(errno), errno));
        return FH_ERROR;
    }

    return FH_OK;
}

/*
 * fh_cpu_rdspeed
 *
 * Compute the CPU speed (in MHz) by looking at the elapsed time in microseconds
 * that it takes to get through 500M CPU cycles.
 */
uint32_t fh_cpu_rdspeed()
{
    uint64_t tsc_start = 0;
    uint64_t tsc_stop  = 0;
    uint64_t tsc_total = 0;
    uint64_t ts2, ts1;
    uint32_t cpu_speed;

    fh_time_get(&ts1);
    rdtscll(tsc_start);
    while (1) {
        barrier();

        rdtscll(tsc_stop);

        if (likely(tsc_stop > tsc_start)) {
            tsc_total += tsc_stop - tsc_start;
        }
        else {
            tsc_total += (uint64_t)~0 - tsc_start + tsc_stop;
        }

        if (tsc_total > (uint64_t)500000000) {
            break;
        }
        tsc_start = tsc_stop;
    }
    fh_time_get(&ts2);

    cpu_speed = (uint32_t) (tsc_total / (ts2-ts1));

    return cpu_speed;
}
