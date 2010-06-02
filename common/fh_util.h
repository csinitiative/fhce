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

#ifndef __FH_UTIL_H__
#define __FH_UTIL_H__

#include <sys/types.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/syscall.h>
#include <linux/unistd.h>

/*
 * Useful macros
 */
#define likely(x)    __builtin_expect((x), 1)
#define unlikely(x)  __builtin_expect((x), 0)
#define barrier()    __asm__ __volatile__("": : :"memory")

/*
 * Read the TSC CPU timestamp counter
 */
#define rdtscll(val) do {                        \
    uint32_t a,d;                                \
    asm volatile("rdtsc" : "=a" (a), "=d" (d));  \
    (val) = ((uint64_t)a) | (((uint64_t)d)<<32); \
} while(0)

/*
 * Get the thread ID.
 */
static inline pid_t gettid(void)
{
    return syscall(__NR_gettid);
}

#ifndef MAX
# define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifndef MIN
# define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define CPU(n)          (1<<(n))

#define LLI(x)          ((long long int)(x))

#define FH_SIZEOF(t,f)  sizeof(((t *)0L)->f)
#define FH_OFFSET(t,f)  (long)(&((t *)0L)->f)

#define FH_PNAME_GET(_pn, _a) do {      \
    char *ptr = strrchr(_a[0], '/');    \
    _pn = ptr ? ptr + 1 : _a[0];        \
} while (0)

#define FH_ALIGN32(n)   (((uint32_t)(n)+3) & (uint32_t) ~3)
#define FH_ALIGN64(n)   (((uint32_t)(n)+7) & (uint32_t) ~7)

#ifndef htonll
#define htonll(x)                                                   \
    ((((uint64_t)htonl((uint32_t)((x)&0x00000000ffffffff)))<< 32) | \
    ((uint64_t)htonl((uint32_t)(((x)>> 32)&0x00000000ffffffff))))
#endif

#ifndef ntohll
#define ntohll(x)                                                   \
    ((((uint64_t)ntohl((uint32_t)((x)&0x00000000ffffffff)))<< 32) | \
    ((uint64_t)ntohl((uint32_t)(((x)>> 32)&0x00000000ffffffff))))
#endif

void fh_daemonize();
int  fh_strsplit_sep(char *string, char *sep, char **fields, size_t size);
int  fh_strsplit(char *string, char **fields, size_t size);

/**
 *  @brief Convert a string to lower case
 *
 *  @param str string to be converted
 *  @return a pointer to the same string
 */
char *fh_str_downcase(char *str);

/**
 *  @brief Generate a thread name string (such as Main_fhFoo0)
 *
 *  @param base thread name base (Main, Mgmt, etc)
 *  @param proc process for which this name is being generated
 *  @return a fully formatted thread name string
 */
char *fh_util_thread_name(const char *base, const char *proc);

/**
 *  @brief Convert the string constant provided to upper case
 *
 *  @param target string where the result will be stored
 *  @param source string being converted
 */
void fh_util_ucstring(char *target, const char *source);

#endif /* __FH_UTIL_H__ */
