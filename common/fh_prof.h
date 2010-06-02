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

#ifndef __FH_PROF_H__
#define __FH_PROF_H__

#include "fh_log.h"
#include "fh_hist.h"
#include "fh_cpu.h"

/*
 * Profiling context
 */
typedef struct {
    char        *prof_name;
    fh_hist_t   *prof_hist;
    fh_hist_t   *prof_hist_cpy;
    uint64_t     prof_beg;
    uint64_t     prof_end;
    uint64_t     prof_count;
    uint32_t     prof_samples;
    uint16_t     prof_bincnt;
    uint16_t     prof_binsize;
    uint32_t     prof_cpuspeed;
    uint16_t     prof_ready;
    uint16_t     prof_init;
} fh_prof_t;

/*
 * fh_prof_init
 *
 * Profiling context initialization. Inlining is done for performance reason.
 */
static inline void fh_prof_init(fh_prof_t *prof)
{
    if (prof->prof_init == 0) {
        prof->prof_hist     = fh_hist_new(prof->prof_name,
                                          prof->prof_bincnt,
                                          prof->prof_binsize);
        FH_ASSERT(prof->prof_hist);

        prof->prof_hist_cpy = fh_hist_new(prof->prof_name,
                                          prof->prof_bincnt,
                                          prof->prof_binsize);
        FH_ASSERT(prof->prof_hist_cpy);

        prof->prof_cpuspeed = fh_cpu_rdspeed();

        fh_hist_start(prof->prof_hist);

        prof->prof_init = 1;
    }
}

/*
 * fh_prof_beg
 *
 * Mark the beginning of the profiling section
 */
static inline void fh_prof_beg(fh_prof_t *prof)
{
    rdtscll(prof->prof_beg);
}

/*
 * fh_prof_end
 *
 * Mark the end of the profiling section
 */
static inline void fh_prof_end(fh_prof_t *prof)
{
    uint64_t diff;

    if (prof->prof_beg > 0) {

        rdtscll(prof->prof_end);

        if (likely(prof->prof_end > prof->prof_beg)) {
            diff = prof->prof_end - prof->prof_beg;
        }
        else {
            diff = (uint64_t)~0 - prof->prof_beg + prof->prof_end;
        }

        diff /= prof->prof_cpuspeed;

        fh_hist_add(prof->prof_hist, diff);

        prof->prof_count++;

        if ((prof->prof_count % prof->prof_samples) == 0) {
            fh_hist_stop(prof->prof_hist);
            fh_hist_copy(prof->prof_hist, prof->prof_hist_cpy);
            fh_hist_reset(prof->prof_hist);
            fh_hist_start(prof->prof_hist);
            prof->prof_ready = 1;
        }

        prof->prof_beg = 0;
    }
}

/*
 * fh_prof_print
 *
 * Print the profiling histogram if it is ready. This can be called out of line
 * on a periodic basis for instance.
 */
static inline void fh_prof_print(fh_prof_t *prof)
{
    if (prof->prof_ready) {
        fh_hist_print(prof->prof_hist_cpy);
        prof->prof_ready = 0;
    }
}

/*
 * Profiling API is done via macro, here is an example on how to use it
 *
 * FH_PROF_DECL(foo);
 * 
 * void bar()
 * {
 *     FH_PROF_INIT(foo);
 *
 *     code;
 *
 *     FH_PROF_BEG(foo);
 *     code_to_profile;
 *     FH_PROF_END(foo);
 *
 *     code;
 *
 *     FH_PROF_PRINT(foo);
 * }
 *
 * int main()
 * {
 *     while (1) {
 *         sleep(1);
 *     }
 * }
 */
#define FH_PROF_DECL(tag,n,c,s)                     \
    fh_prof_t tag##_prof_ctxt = {                   \
        .prof_name     = #tag,                      \
        .prof_hist     = NULL,                      \
        .prof_hist_cpy = NULL,                      \
        .prof_init     = 0,                         \
        .prof_count    = 1,                         \
        .prof_samples  = (n),                       \
        .prof_bincnt   = (c),                       \
        .prof_binsize  = (s),                       \
        .prof_ready    = 0,                         \
        .prof_beg      = 0,                         \
        .prof_end      = 0,                         \
        .prof_cpuspeed = 0,                         \
    }

#define FH_PROF_EXTERN_DECL(tag)                    \
    extern fh_prof_t tag##_prof_ctxt

#define FH_PROF_INIT(tag)                           \
    fh_prof_init(&tag##_prof_ctxt)

#define FH_PROF_BEG(tag)                            \
    fh_prof_beg(&tag##_prof_ctxt)

#define FH_PROF_END(tag)                            \
    fh_prof_end(&tag##_prof_ctxt)

#define FH_PROF_PRINT(tag)                          \
    fh_prof_print(&tag##_prof_ctxt)

#endif /* __FH_PROF_H__ */
