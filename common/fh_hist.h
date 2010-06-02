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

#ifndef __FH_HIST_H__
#define __FH_HIST_H__

#include <stdint.h>

typedef struct {
    char      *hg_key;          /* Histogram name               */
    uint64_t   hg_start;        /* Histogram start timestamp    */
    uint64_t   hg_stop;         /* Histogram stop timestamp     */
    int        hg_bincnt;       /* Number of bins in histogram  */
    int        hg_binsize;      /* Bin size                     */
    uint64_t  *hg_bins;         /* Bin array                    */
    uint64_t   hg_total;        /* Total number of samples      */
    int64_t    hg_min;          /* Min sample value             */
    int64_t    hg_max;          /* Max sample value             */
    int64_t    hg_sum;          /* Sum of all sample values     */
    float      hg_avg;          /* Average sample value         */
} fh_hist_t;

/*
 * Exported functions
 */

fh_hist_t *fh_hist_new   (const char *key, int bincnt, int binsize);
void       fh_hist_free  (fh_hist_t *hg);
void       fh_hist_copy  (fh_hist_t *gh_from, fh_hist_t *hg_to);
void       fh_hist_start (fh_hist_t *hg);
void       fh_hist_stop  (fh_hist_t *hg);
void       fh_hist_reset (fh_hist_t *hg);
void       fh_hist_add   (fh_hist_t *hg, int64_t val);
void       fh_hist_print (fh_hist_t *hg);

#endif /* __FH_HIST_H__ */
