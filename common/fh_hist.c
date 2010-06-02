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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fh_log.h"
#include "fh_util.h"
#include "fh_time.h"
#include "fh_errors.h"
#include "fh_hist.h"

/*
 * fh_hist_new
 *
 * Allocate and initialize a new histogram.
 */
fh_hist_t *fh_hist_new(const char *key, int bincnt, int binsize)
{
    fh_hist_t *hg = NULL;

    hg = (fh_hist_t *) malloc(sizeof(fh_hist_t));
    if (!hg) {
        return NULL;
    }

    memset(hg, 0, sizeof(fh_hist_t));

    hg->hg_binsize = binsize;
    hg->hg_bincnt  = bincnt;

    hg->hg_min     = binsize * bincnt;
    hg->hg_max     = 0;

    hg->hg_key     = strdup(key);
    FH_ASSERT(hg->hg_key);

    /* Add 1 bin for outliers */
    bincnt++;

    hg->hg_bins = (uint64_t *) malloc(bincnt * sizeof(int64_t));
    if (!hg->hg_bins) {
        fh_hist_free(hg);               // release all memory
        return NULL;
    }

    memset(hg->hg_bins, 0, bincnt * sizeof(int64_t));

    return hg;
}

/*
 * fh_hist_free
 *
 * Free and release all associated resources for the histogram.
 */
void fh_hist_free(fh_hist_t *hg)
{
    if (hg) {
        if (hg->hg_bins) {
            free(hg->hg_bins);
        }
        if (hg->hg_key) {
            free(hg->hg_key);
        }
        free(hg);
    }
}

/*
 * fh_hist_add
 *
 * Add a sample to the histogram.
 */
void fh_hist_add(fh_hist_t *hg, int64_t val)
{
    int64_t bin;

    FH_ASSERT(hg);

    if (val >= 0) {
        if (val < hg->hg_min) {
            hg->hg_min = val;
        }

        if (val > hg->hg_max) {
            hg->hg_max = val;
        }

        bin = val / hg->hg_binsize;
        if (bin > hg->hg_bincnt) {
            bin = hg->hg_bincnt;
        }

        hg->hg_bins[bin]++;

        hg->hg_sum += val;

        hg->hg_total++;
    }
}

/*
 * fh_hist_reset
 *
 * Reset the histogram.
 */
void fh_hist_reset(fh_hist_t *hg)
{
    FH_ASSERT(hg);

    hg->hg_total = 0;
    hg->hg_sum   = 0;
    hg->hg_min   = hg->hg_binsize * hg->hg_bincnt;
    hg->hg_max   = 0;

    memset(hg->hg_bins, 0, (hg->hg_bincnt+1) * sizeof(int64_t));
}

/*
 * fh_hist_start
 *
 * Grab a timestamp to mark the start of the sampling period.
 */
void fh_hist_start(fh_hist_t *hg)
{
    FH_ASSERT(hg);

    fh_time_get(&hg->hg_start);
}

/*
 * fh_hist_stop
 *
 * Grab a timestamp to mark the end of the sampling period.
 */
void fh_hist_stop(fh_hist_t *hg)
{
    FH_ASSERT(hg);

    fh_time_get(&hg->hg_stop);
}



/*
 * fh_hist_copy
 *
 * Copy the current histogram to another histogram structure.
 */
void fh_hist_copy(fh_hist_t *hg_from, fh_hist_t *hg_to)
{
    FH_ASSERT(hg_from && hg_to);
    FH_ASSERT(hg_from->hg_bincnt == hg_to->hg_bincnt);
  
    hg_to->hg_total = hg_from->hg_total;
    hg_to->hg_sum   = hg_from->hg_sum;
    hg_to->hg_min   = hg_from->hg_min;
    hg_to->hg_max   = hg_from->hg_max;

    hg_to->hg_start = hg_from->hg_start;
    hg_to->hg_stop  = hg_from->hg_stop;

    memcpy(hg_to->hg_bins, hg_from->hg_bins, (hg_from->hg_bincnt+1) * sizeof(int64_t));
}

#define HG_PERCENT(hg, i)  ((float) 100*(float)(hg)->hg_bins[i]/(hg)->hg_total)
#define HG_PRINT(args)     FH_LOG_PGEN(STATS, args)

/*
 * fh_hist_print
 *
 * Print the histogram to the logs.
 */
void fh_hist_print(fh_hist_t *hg)
{
    int64_t total = 0;
    int i;

    FH_ASSERT(hg);

    if (hg->hg_total == 0) {
        return;
    }

    HG_PRINT(("------------------------------------------------------"));
    HG_PRINT(("Histogram for '%s':", hg->hg_key));
    HG_PRINT(("------------------------------------------------------"));
    HG_PRINT(("        Buckets         Counts   Dist(%%) Perc'ile(th)"));
    HG_PRINT(("------------------------------------------------------"));

    for (i=0; i<hg->hg_bincnt; i++) {

        total += hg->hg_bins[i];

        if ((((i+1) * hg->hg_binsize) >= hg->hg_min) &&
            ((i * hg->hg_binsize) <= hg->hg_max)) {

            HG_PRINT((" > [ %5d - %5d ]: %8lld   %6.2f     %3d",
                i * hg->hg_binsize, ((i+1) * hg->hg_binsize) - 1, LLI(hg->hg_bins[i]),
                HG_PERCENT(hg, i), (int) 100 * total / hg->hg_total));
        }
    }

    total += hg->hg_bins[i];

    if ((i * hg->hg_binsize) <= hg->hg_max) {
        HG_PRINT((" > [ %5d - ..... ]: %8lld   %6.2f     %3d",
            i * hg->hg_binsize, LLI(hg->hg_bins[i]), HG_PERCENT(hg, i),
            (int) 100 * total / hg->hg_total));
    }

 
    hg->hg_avg = (float) hg->hg_sum / hg->hg_total;

    HG_PRINT((" > Total:             %8lld", LLI(total)));
    HG_PRINT((" > min %lld max %lld avg %.2f", LLI(hg->hg_min), LLI(hg->hg_max), hg->hg_avg));

    if (hg->hg_start && hg->hg_stop) {
        uint64_t elapsed;
        float rate;

        elapsed = hg->hg_stop - hg->hg_start;

        rate = (float) hg->hg_total/elapsed * 1000000;

        HG_PRINT((" > total processed %lld (rate %.2f /sec)", LLI(hg->hg_total), rate));
        HG_PRINT((" > elapsed processing time %d.%d",
            (uint32_t) (elapsed/1000000), (uint32_t) (elapsed%1000000)));
    }
}


