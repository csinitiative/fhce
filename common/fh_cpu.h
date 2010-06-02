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

#ifndef __FH_CPU_H__
#define __FH_CPU_H__

#include <stdint.h>
#include "fh_util.h"
#include "fh_errors.h"

// for *nixes that use CPU_ZERO, etc. rather than __CPU_ZERO
#ifndef __CPU_ZERO
#ifdef CPU_ZERO
#define __CPU_ZERO(x) CPU_ZERO(x)
#define __CPU_ISSET(x, y) CPU_ISSET(x, y)
#define __CPU_SET(x, y) CPU_SET(x, y)
#endif
#endif

/*
 * Read the CPU speed.
 */
uint32_t fh_cpu_rdspeed();

/*
 * Return the number of CPUs in the system.
 */
uint32_t fh_cpu_count();

/*
 * CPU Affinity API
 */

/*
 * Get/Set CPU affinity for the calling thread. The CPU mask is a bitmask
 * where each bit represents a CPU (CPU N corresponds to bit (1 << N))
 */
FH_STATUS fh_cpu_getaffinity(uint32_t *cpu_mask);
FH_STATUS fh_cpu_setaffinity(int cpu_mask);

/*
 * Generate a CPU list string from the provided CPU mask (e.g.: 1,3,5 means
 * that the process/thread is running on CPU 1, 3 and 5)
 */
void      fh_cpu_print(uint32_t cpu_mask, char *buf, int size);

#endif /* __FH_CPU_H__ */
