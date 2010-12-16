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

/*
 * System includes
 */
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

/*
 * FH Common includes
 */
#include "fh_log.h"
#include "fh_util.h"

/*
 * FH OPRA includes
 */
#include "fh_opra_lh.h"
#include "fh_opra_lh_tap.h"

static lh_tap_t tap_table[OPRA_CFG_MAX_FTLINES * 2];
static uint32_t tap_file_size = 0;

/*
 * fh_opra_lh_tap_init
 *
 * Initialize all the tap table.
 */
void fh_opra_lh_tap_init(uint32_t file_size)
{
    FH_ASSERT(file_size > sizeof(lh_tap_hdr_t));

    memset(tap_table, 0, sizeof(tap_table));
    tap_file_size = file_size;
}

/*
 * fh_opra_lh_tap_open
 *
 * Configure the line tap.
 */
void fh_opra_lh_tap_open(char side, uint32_t index, uint32_t line_cnt)
{
    lh_tap_t *tap = &tap_table[line_cnt];
    int fd;
    int bytes_written;

    FH_ASSERT(tap->tap_address == NULL);

    tap->tap_side  = side;
    tap->tap_index = index;

    // Initialize the tap if not already

    sprintf(tap->tap_filename, "line-%c%d.tap", side, index);

    // Open file and truncate it whatever size it is
    fd = open(tap->tap_filename, O_RDWR|O_CREAT|O_TRUNC, S_IRWXU);
    if (fd == -1) {
        FH_LOG(LH, ERR, ("failed to open file: %s", tap->tap_filename));
        exit(1);
    }

    // Fill in the file with zeros
    {
        static char buffer[1024 * 1024]; //1MB
        static int  buffer_init = 0;
        int bytes = tap_file_size;

        if (buffer_init == 0) {
            memset(buffer, 0, sizeof(buffer));
            buffer_init = 1;
        }

        while (bytes > 0) {
            bytes_written = write(fd, buffer, sizeof(buffer));
            FH_ASSERT(bytes_written == sizeof(buffer));
            bytes -= sizeof(buffer);
        }
    }

    tap->tap_fd = fd;

    // Memory map the file
    tap->tap_address = mmap(NULL, tap_file_size,
                            PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

    if (!tap->tap_address) {
        FH_LOG(LH, ERR, ("Failed to mmap tap file: %s", tap->tap_filename));
        exit(1);
    }

    tap->tap_hdr = (lh_tap_hdr_t *) tap->tap_address;

    tap->tap_num_pkts = 0;
    tap->tap_max_pkts = (tap_file_size - sizeof(lh_tap_hdr_t)) / sizeof(lh_tap_msg_t);

    tap->tap_hdr->hdr_magic    = TAP_FILE_MAGIC;
    tap->tap_hdr->hdr_version  = TAP_FILE_VERSION;
    tap->tap_hdr->hdr_num_msgs = 0;
    tap->tap_hdr->hdr_num_pkts = 0;
    tap->tap_hdr->hdr_max_pkts = tap->tap_max_pkts;
    tap->tap_hdr->hdr_tap_size = tap_file_size;
    tap->tap_hdr->hdr_sn_min   = (uint32_t)~0;
    tap->tap_hdr->hdr_sn_max   = 0;
}

/*
 * fh_opra_lh_tap_close
 *
 * Close the line tap.
 */
void fh_opra_lh_tap_close(lh_line_t *l)
{
    lh_tap_t *tap = &tap_table[l->l_index];
    int done = 1, i;

    if (tap->tap_fd == -1) {
        FH_LOG(LH, WARN, ("Line %s tap is already closed", l->l_name));
        return;
    }

    munmap(tap->tap_address, tap_file_size);
    close(tap->tap_fd);
    tap->tap_fd = -1;

    FH_LOG(LH, STATE, ("Line %s tap is completed", l->l_name));

    /*
     * Check all lines and exit if everyting is done
     */
    for (i=0; i<(OPRA_CFG_MAX_FTLINES * 2); i++) {
        lh_tap_t *tap = &tap_table[l->l_index];

        if (tap->tap_address && tap->tap_fd != -1) {
            done = 0;
            break;
        }
    }

    if (done) {
        FH_LOG(LH, STATE, ("Tap of all lines is done... exiting"));
        exit(0);
    }
}

/*
 * fh_opra_lh_tap
 *
 * Tap the line.
 */
void fh_opra_lh_tap(lh_line_t *l, uint8_t msg_cat, uint8_t msg_type,
                    uint32_t msg_sn, uint32_t num_msgs, uint64_t rxtime)
{
    lh_tap_t *tap = &tap_table[l->l_index];

    FH_ASSERT(l->l_tap && tap->tap_address);

    // Record this message (first message of the packet
    if (tap->tap_num_pkts < tap->tap_max_pkts) {
        lh_tap_msg_t *tap_msg = TAP_SN(tap->tap_address, tap->tap_num_pkts);

        tap_msg->tap_msg_rxtime = rxtime;
        tap_msg->tap_msg_sn     = msg_sn;
        tap_msg->tap_msg_cat    = msg_cat;
        tap_msg->tap_msg_type   = msg_type;
        tap_msg->tap_msg_count  = num_msgs;

        if (msg_sn > tap->tap_hdr->hdr_sn_max) {
            tap->tap_hdr->hdr_sn_max = msg_sn;
        }

        if (msg_sn < tap->tap_hdr->hdr_sn_min) {
            tap->tap_hdr->hdr_sn_min = msg_sn;
        }

        tap->tap_hdr->hdr_num_pkts ++;
        tap->tap_hdr->hdr_num_msgs += num_msgs;
        tap->tap_num_pkts++;
    }
    else if (tap->tap_fd != -1) {
        fh_opra_lh_tap_close(l);
    }
}

/*
 * tap_msg_khash
 *
 * Generate a key hash for the TAP message.
 */
static uint32_t tap_msg_khash(lh_tap_msg_t *tap_msg, int len)
{
    FH_ASSERT(len == sizeof(lh_tap_msg_t));
    return jhash2(&tap_msg->tap_msg_sn, 1, 0);
}

/*
 * tap_msg_kdump
 *
 * Dump a key hash
 */
static char *tap_msg_kdump(lh_tap_msg_t *tap_msg, int len)
{
    static char tap_msg_str[64];
    FH_ASSERT(len == sizeof(lh_tap_msg_t));
    sprintf(tap_msg_str, "SN:%d", tap_msg->tap_msg_sn);
    return tap_msg_str;
}

/*
 * tap_msg_kcmp
 *
 * Compare a key hash
 */
static int tap_msg_kcmp(lh_tap_msg_t *tap_msg_a, lh_tap_msg_t *tap_msg_b, int len)
{
    FH_ASSERT(len == sizeof(lh_tap_msg_t));
    return (tap_msg_a->tap_msg_sn == tap_msg_b->tap_msg_sn);
}

/*
 * lh_tap_rpt_load
 *
 * Load a tap report from a given file.
 */
lh_tap_rpt_t *lh_tap_rpt_load(char *filename)
{
    struct stat   buf;
    int           fd = -1;
    char         *addr = NULL;
    lh_tap_hdr_t *hdr  = NULL;
    lh_tap_rpt_t *rpt  = NULL;
    uint32_t      i;
    uint32_t      file_size = 0;

    fh_ht_kops_t  kops = {
        .kops_khash = (fh_ht_khash_t *) tap_msg_khash,
        .kops_kdump = (fh_ht_kdump_t *) tap_msg_kdump,
        .kops_kcmp  = (fh_ht_kcmp_t  *) tap_msg_kcmp,
    };

    // Verify that the filename is present on disk
    if (stat(filename, &buf) < 0) {
        FH_LOG(LH, ERR, ("Couldn't find file: %s", filename));
        return NULL;
    }

    file_size = buf.st_size;

    // Open the file
    fd = open(filename, O_RDWR);
    if (fd == -1) {
        FH_LOG(LH, ERR, ("failed to open file: %s", filename));
        goto error;
    }

    // Memory map the file
    addr = mmap(NULL, tap_file_size, PROT_READ, MAP_SHARED, fd, 0);
    if (!addr) {
        FH_LOG(LH, ERR, ("Failed to mmap tap file: %s", filename));
        goto error;
    }

    // Verify integrity of file
    hdr = (lh_tap_hdr_t *) addr;

    if (hdr->hdr_magic != TAP_FILE_MAGIC) {
        FH_LOG(LH, ERR, ("Bad magic: 0x%x vs. 0x%x", hdr->hdr_magic, TAP_FILE_MAGIC));
        goto error;
    }

    if (hdr->hdr_version != TAP_FILE_VERSION) {
        FH_LOG(LH, ERR, ("Bad version: %d vs. %d", hdr->hdr_version, TAP_FILE_VERSION));
        goto error;
    }

    if (hdr->hdr_tap_size != file_size) {
        FH_LOG(LH, ERR, ("Bad file size: %d vs. %d", hdr->hdr_tap_size, file_size));
        goto error;
    }


    // Allocate a new report
    rpt = (lh_tap_rpt_t *) malloc(sizeof(lh_tap_rpt_t));
    if (!rpt) {
        FH_LOG(LH, ERR, ("Failed to allocate memory for line tap report"));
        goto error;
    }
    memset(rpt, 0, sizeof(lh_tap_rpt_t));

    // Create the SN htable for all stored messages
    rpt->rpt_htable = fh_ht_new(hdr->hdr_num_pkts, FH_HT_FL_GROW, &kops);
    if (!rpt->rpt_htable) {
        FH_LOG(LH, ERR, ("Failed to allocate htable for line tap report: %d",
                         hdr->hdr_num_pkts));
        goto error;
    }

    // Walk through the two files to generate a diff
    for (i=0; i<hdr->hdr_num_pkts; i++) {
        lh_tap_msg_t *tap_msg = TAP_SN(addr, i);
        FH_STATUS rc;

        // Add the new OPRA msg to the htable
        rc = fh_ht_put(rpt->rpt_htable, tap_msg, sizeof(lh_tap_msg_t), tap_msg);
        if (rc != FH_OK) {
            FH_LOG(LH, DIAG, ("Duplicate detected: %s - sn:%d msg_count:%d",
                              filename, tap_msg->tap_msg_sn, tap_msg->tap_msg_count));
            rpt->rpt_dup_sn++;
            continue;
        }
    }

    rpt->rpt_hdr = hdr;
    rpt->rpt_fd  = fd;

    return rpt;

error:
    if (fd != -1) {
        if (addr) {
            munmap(addr, file_size);
        }

        close(fd);

        if (rpt) {
            if (rpt->rpt_htable) {
                fh_ht_free(rpt->rpt_htable);
            }
            free(rpt);
        }
    }

    return NULL;
}

/*
 * fh_opra_lh_tap_ftdump
 *
 * Dump the tap file statistics about the FT line.
 */
void fh_opra_lh_tap_deltas(char *a_filename, char *b_filename)
{
    lh_tap_rpt_t *rpt_a;
    lh_tap_rpt_t *rpt_b;
    fh_hist_t    *hist_a_first;
    fh_hist_t    *hist_b_first;
    uint32_t      i;

    rpt_a = lh_tap_rpt_load(a_filename);
    rpt_b = lh_tap_rpt_load(b_filename);

    if (!rpt_a || !rpt_b) {
        FH_LOG(LH, ERR, ("Failed to load files"));
        exit(1);
    }

    // Do analysis of the two files

    hist_a_first = fh_hist_new("OPRA SN Delta Distribution (A winners)", 20, 200);
    if (!hist_a_first) {
        FH_LOG(LH, ERR, ("Failed to create histogram for SN distribution A"));
        exit(1);
    }

    hist_b_first = fh_hist_new("OPRA SN Delta Distribution (B winners)", 20, 200);
    if (!hist_b_first) {
        FH_LOG(LH, ERR, ("Failed to create histogram for SN distribution B"));
        exit(1);
    }

    // Analyze A report
    for (i=0; i<rpt_a->rpt_hdr->hdr_num_pkts; i++) {
        lh_tap_msg_t *a_msg = TAP_SN(rpt_a->rpt_hdr, i);
        lh_tap_msg_t *b_msg = NULL;
        FH_STATUS rc;
        void *val = NULL;

        // Lookup for this message in B report
        rc = fh_ht_get(rpt_b->rpt_htable, a_msg, sizeof(lh_tap_msg_t), &val);
        if (rc != FH_OK) {
            rpt_b->rpt_missing_sn++;
            continue;
        }

        b_msg = (lh_tap_msg_t *) val;

        if (a_msg->tap_msg_rxtime > b_msg->tap_msg_rxtime) {
            int64_t delta = a_msg->tap_msg_rxtime - b_msg->tap_msg_rxtime;
            fh_hist_add(hist_b_first, delta);
        }
        else {
            int64_t delta = b_msg->tap_msg_rxtime - a_msg->tap_msg_rxtime;
            fh_hist_add(hist_a_first, delta);
        }
    }

    // Analyze B report for missing SN from A
    for (i=0; i<rpt_b->rpt_hdr->hdr_num_pkts; i++) {
        lh_tap_msg_t *b_msg = TAP_SN(rpt_b->rpt_hdr, i);
        FH_STATUS rc;
        void *val = NULL;

        // Lookup for this message in A report
        rc = fh_ht_get(rpt_a->rpt_htable, b_msg, sizeof(lh_tap_msg_t), &val);
        if (rc != FH_OK) {
            rpt_a->rpt_missing_sn++;
            continue;
        }
    }

    // Dump the reports
    FH_LOG_PGEN(STATS, ("------------------------------------------------------"));
    FH_LOG_PGEN(STATS, ("Report for FT line [ %s : %s ]", a_filename, b_filename));
    FH_LOG_PGEN(STATS, ("------------------------------------------------------"));
    FH_LOG_PGEN(STATS, ("  > A Captured Packets  : %d", rpt_a->rpt_hdr->hdr_num_pkts));
    FH_LOG_PGEN(STATS, ("  > B Captured Packets  : %d", rpt_b->rpt_hdr->hdr_num_pkts));
    FH_LOG_PGEN(STATS, ("  > A Captured Messages : %d", rpt_a->rpt_hdr->hdr_num_msgs));
    FH_LOG_PGEN(STATS, ("  > B Captured Messages : %d", rpt_b->rpt_hdr->hdr_num_msgs));
    FH_LOG_PGEN(STATS, ("  > A SN Range          : [ %d : %d ]",
                        rpt_a->rpt_hdr->hdr_sn_min, rpt_a->rpt_hdr->hdr_sn_max));
    FH_LOG_PGEN(STATS, ("  > B SN Range          : [ %d : %d ]",
                        rpt_b->rpt_hdr->hdr_sn_min, rpt_b->rpt_hdr->hdr_sn_max));
    FH_LOG_PGEN(STATS, ("  > A Duplicate Packets : %d", rpt_a->rpt_dup_sn));
    FH_LOG_PGEN(STATS, ("  > B Duplicate Packets : %d", rpt_b->rpt_dup_sn));
    FH_LOG_PGEN(STATS, ("  > A Missing Packets   : %d", rpt_a->rpt_missing_sn));
    FH_LOG_PGEN(STATS, ("  > B Missing Packets   : %d", rpt_b->rpt_missing_sn));
    FH_LOG_PGEN(STATS, ("------------------------------------------------------"));

    fh_hist_print(hist_a_first);
    fh_hist_print(hist_b_first);
}

/*
 * fh_opra_lh_tap_rates
 *
 * Dump the rates on a per-<period> basis. Period is provided in usecs.
 */
void fh_opra_lh_tap_rates(char *filename, uint64_t period)
{
    struct stat   buf;
    int           fd = -1;
    char         *addr = NULL;
    lh_tap_hdr_t *hdr  = NULL;
    uint32_t      i;
    uint64_t      prev_ts = 0;
    int64_t       pkt_count = 0;
    int64_t       msg_count = 0;
    fh_hist_t    *msg_rates_hist = NULL;
    fh_hist_t    *pkt_rates_hist = NULL;
    uint32_t      bincnt  = 20;
    uint32_t      binsize = 100;
    uint32_t      file_size = 0;

    //printf("binsize %d\n", binsize);

    // Verify that the filename is present on disk
    if (stat(filename, &buf) < 0) {
        FH_LOG(LH, ERR, ("Couldn't find file: %s", filename));
        return;
    }

    file_size = buf.st_size;

    // Open the file
    fd = open(filename, O_RDWR);
    if (fd == -1) {
        FH_LOG(LH, ERR, ("failed to open file: %s", filename));
        exit(1);
    }

    // Memory map the file
    addr = mmap(NULL, file_size, PROT_READ, MAP_SHARED, fd, 0);
    if (!addr) {
        FH_LOG(LH, ERR, ("Failed to mmap tap file: %s", filename));
        exit(1);
    }

    // Verify integrity of file
    hdr = (lh_tap_hdr_t *) addr;

    if (hdr->hdr_magic != TAP_FILE_MAGIC) {
        FH_LOG(LH, ERR, ("Bad magic: 0x%x vs. 0x%x", hdr->hdr_magic, TAP_FILE_MAGIC));
        exit(1);
    }

    if (hdr->hdr_version != TAP_FILE_VERSION) {
        FH_LOG(LH, ERR, ("Bad version: %d vs. %d", hdr->hdr_version, TAP_FILE_VERSION));
        exit(1);
    }

    if (hdr->hdr_tap_size != file_size) {
        FH_LOG(LH, ERR, ("Bad file size: %d vs. %d", hdr->hdr_tap_size, file_size));
        exit(1);
    }

    // Create the msg rate histogram
    msg_rates_hist = fh_hist_new("OPRA Msg Rate Distribution", bincnt, binsize);
    if (!msg_rates_hist) {
        FH_LOG(LH, ERR, ("Failed to create histogram for msg rates distribution"));
        exit(1);
    }

    pkt_rates_hist = fh_hist_new("OPRA Pkt Rate Distribution", bincnt, binsize/10);
    if (!pkt_rates_hist) {
        FH_LOG(LH, ERR, ("Failed to create histogram for pkt rates distribution"));
        exit(1);
    }

    // Walk through the two files to generate a diff
    for (i=0; i<hdr->hdr_num_pkts; i++) {
        lh_tap_msg_t *tap_msg = TAP_SN(addr, i);
        uint64_t elapsed;

        if (prev_ts == 0) {
            prev_ts = tap_msg->tap_msg_rxtime;
        }

        elapsed = tap_msg->tap_msg_rxtime - prev_ts;

        //printf("%lld - %lld = %lld\n", LLI(tap_msg->tap_msg_rxtime),
               //LLI(prev_ts), LLI(elapsed));

        pkt_count++;
        msg_count += tap_msg->tap_msg_count;

        // End of a given period
        if (elapsed > period) {
            fh_hist_add(pkt_rates_hist, pkt_count);
            fh_hist_add(msg_rates_hist, msg_count);

            // Reset the current state
            pkt_count = 0;
            msg_count = 0;
            prev_ts   = tap_msg->tap_msg_rxtime;
        }
    }

    FH_LOG_PGEN(STATS, ("------------------------------------------------------"));
    FH_LOG_PGEN(STATS, ("Report for line : %s", filename));
    FH_LOG_PGEN(STATS, ("------------------------------------------------------"));
    fh_hist_print(pkt_rates_hist);
    fh_hist_print(msg_rates_hist);
}

