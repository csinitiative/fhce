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
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <wait.h>

/*
 * FH Common includes
 */
#include "fh_log.h"
#include "fh_util.h"
#include "fh_mpool.h"
#include "fh_htable.h"

/*
 * FH OPRA includes
 */
#include "fh_opra_lo.h"
#include "fh_opra_cfg.h"

typedef struct {
    char lof_root[LO_ROOT_SIZE];
    char lof_sec[LO_SEC_SIZE];
    char lof_company[LO_COMPANY_SIZE];
    char lof_exch[LO_EXCH_SIZE];
    char lof_xxx[LO_XXX_SIZE];
    char lof_cusip[LO_CUSIP_SIZE];
    char lof_type[LO_TYPE_SIZE];
} lo_format_t;

typedef union {
    lo_format_t lob_format;
    char        lob_string[LO_TOTAL_SIZE + 10];
} lo_buffer_t;

/*
 * Null-Terminate the field 'f' string
 */
#define LO_BUF_TERMINATE(p,f) \
    (p)[FH_OFFSET(lo_format_t,f) + FH_SIZEOF(lo_format_t,f) - 1] = '\0'

/*
 * Listed options database
 */
typedef struct {
    fh_mpool_t  *lodb_mpool;
    fh_ht_t     *lodb_htable;
    uint32_t     lodb_skip;
    uint32_t     lodb_count;
    uint32_t     lodb_init;
} lo_db_t;

/*
 * Initial listed options DB size (growable)
 */
#define LO_DB_SIZE   (1024)

/*
 * LO DB Key operations
 */
static uint32_t lo_khash (char *root, int klen);
static char *   lo_kdump (char *root, int klen);
static int      lo_kcmp  (char *root_a, char *root_b, int klen);

static fh_ht_kops_t lodb_kops = {
    .kops_khash = (fh_ht_khash_t *) lo_khash,
    .kops_kcmp  = (fh_ht_kcmp_t  *) lo_kcmp,
    .kops_kdump = (fh_ht_kdump_t *) lo_kdump,
};

static lo_db_t lo_db = { .lodb_init = 0 };

/*
 * lo_db_init
 *
 * Initialize the listed options database.
 */
static FH_STATUS lo_db_init(lo_db_t *lodb)
{
    FH_ASSERT(lodb->lodb_init == 0);

    /*
     * Initialize the growable memory pool.
     */
    lodb->lodb_mpool = fh_mpool_new("ListedOptions", sizeof(fh_opra_lo_t),
                                    LO_DB_SIZE, FH_MPOOL_FL_GROW);
    if (!lodb->lodb_mpool) {
        FH_LOG(MGMT, ERR, ("Failed to initialize the listed options mem pool"));
        return FH_ERROR;
    }

    /*
     * Initialize the growable H-Table.
     */
    lodb->lodb_htable = fh_ht_new(LO_DB_SIZE, FH_HT_FL_GROW, &lodb_kops);
    if (!lodb->lodb_htable) {
        FH_LOG(MGMT, ERR, ("Failed to initialize the listed options H-table"));
        fh_mpool_free(lodb->lodb_mpool);
        return FH_ERROR;
    }

    lodb->lodb_count  = 0;
    lodb->lodb_init   = 1;

    return FH_OK;
}

/*
 * lo_db_memdump
 *
 * Dump the memory usage of the listed-options database.
 */
static void lo_db_memdump(lo_db_t *lodb)
{
    FH_ASSERT(lodb->lodb_init);

    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("> Listed-options Database Memory footprint:"));
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
    FH_LOG_PGEN(DIAG, ("LO DB number of options   : %d", lodb->lodb_count));
    FH_LOG_PGEN(DIAG, ("LO DB skipped options     : %d", lodb->lodb_skip));
    FH_LOG_PGEN(DIAG, ("LO DB H-table usage ratio : %d / %d",
                       lodb->lodb_htable->ht_count, lodb->lodb_htable->ht_size));
    FH_LOG_PGEN(DIAG, ("LO DB H-table memory      : %.2fK bytes",
                       (float) fh_ht_memuse(lodb->lodb_htable)/1000));
    FH_LOG_PGEN(DIAG, ("LO DB Mem pool memory     : %.2fK bytes",
                       (float) fh_mpool_memuse(lodb->lodb_mpool)/1000));
    FH_LOG_PGEN(DIAG, ("--------------------------------------------------------"));
}

/*
 * lo_db_add
 *
 * Add a new option entry to the listed option table.
 */
static FH_STATUS lo_db_add(lo_db_t *lodb, lo_format_t *lof, fh_opra_lo_t **lop)
{
    fh_opra_lo_t *lo = NULL;
    char         *ptr = NULL;
    FH_STATUS     rc;
    int           klen;

    FH_ASSERT(lodb->lodb_init);

    FH_LOG(MGMT, INFO, (" > lof_root     : %s", lof->lof_root));
    FH_LOG(MGMT, INFO, (" > lof_sec      : %s", lof->lof_sec));
    FH_LOG(MGMT, INFO, (" > lof_company  : %s", lof->lof_company));
    FH_LOG(MGMT, INFO, (" > lof_exch     : %s", lof->lof_exch));
    FH_LOG(MGMT, INFO, (" > lof_xxx      : %s", lof->lof_xxx));
    FH_LOG(MGMT, INFO, (" > lof_cusip    : %s", lof->lof_cusip));
    FH_LOG(MGMT, INFO, (" > lof_type     : %s", lof->lof_type));

    /*
     * Get a new listed-options entry from the memory pool.
     */
    lo = (fh_opra_lo_t *) fh_mpool_get(lodb->lodb_mpool);
    if (!lo) {
        FH_LOG(MGMT, ERR, ("Failed to get a new listed options entry"));
        return FH_ERROR;
    }

    /* Copy the root symbol */
    strcpy(lo->lo_root, lof->lof_root);

    /* Remove the trailing white-spaces */
    ptr = strchr(lo->lo_root, ' ');
    if (ptr) { *ptr = '\0'; }

    /* Copy the security symbol */
    strcpy(lo->lo_sec, lof->lof_sec);

    /* Remove the trailing white-spaces */
    ptr = strchr(lo->lo_sec, ' ');
    if (ptr) { *ptr = '\0'; }

    /*
     * Special case of the FC where the underlying is provided as opposed to the
     * option symbol.
     */
    if (strcmp(lof->lof_type, "FC") == 0) {
        strcpy(lo->lo_root, lo->lo_sec);
    }

    if (strlen(lo->lo_root) > 5) {
        FH_LOG(MGMT, WARN, ("Root symbol for %s option is bigger than 5 characters: %s",
                            lof->lof_type, lo->lo_root));
        fh_mpool_put(lodb->lodb_mpool, lo);
        return FH_ERROR;
    }

#if FH_OPRA_DBG
    strcpy(lo->lo_company, lof->lof_company);
    strcpy(lo->lo_exch,    lof->lof_exch);
    strcpy(lo->lo_cusip,   lof->lof_cusip);
    strcpy(lo->lo_type,    lof->lof_type);
#endif

    klen = strlen(lo->lo_root);
    if (klen == 0) {
        FH_LOG(MGMT, ERR, ("Empty root symbol! root:%s", lo->lo_root));
        fh_mpool_put(lodb->lodb_mpool, lo);
        return FH_ERROR;
    }

    /*
     * Add the LO entry to the htable
     */
    rc = fh_ht_put(lodb->lodb_htable, lo->lo_root, klen, lo);
    if (rc == FH_ERR_DUP) {
        FH_LOG(MGMT, WARN, ("Skip duplicate %s root in LO DB: '%s %s'",
                           lof->lof_type, lof->lof_root, lo->lo_sec));
        fh_mpool_put(lodb->lodb_mpool, lo);
        return rc;
    }

    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to add root '%s' to LO DB (size:%d)",
                           lo->lo_root, lodb->lodb_count));
        fh_mpool_put(lodb->lodb_mpool, lo);
        return rc;
    }

    *lop = lo;

    lodb->lodb_count++;

    return FH_OK;
}

/*
 * lo_db_lookup
 *
 * Lookup a given root symbol in the LO database.
 */
static FH_STATUS lo_db_lookup(lo_db_t *lodb, char *root, fh_opra_lo_t **lop)
{
    FH_STATUS rc;
    void *val = NULL;

    FH_ASSERT(lodb->lodb_init);

    /*
     * Lookup the root in the H-Table
     */
    rc = fh_ht_get(lodb->lodb_htable, root, strlen(root), &val);
    if (rc != FH_OK) {
        return rc;
    }

    *lop = (fh_opra_lo_t *) val;

    return FH_OK;
}

/*
 * lo_khash
 *
 * Hash a given listed option entry.
 */
static uint32_t lo_khash(char *root, int klen)
{
    FH_ASSERT(klen);
    return jhash(root, klen, 0);
}

/*
 * lo_kdump
 *
 * Dump a given listed option key.
 */
static char *lo_kdump(char *root, int klen)
{
    FH_ASSERT(klen);
    return root;
}

/*
 * lo_kcmp
 *
 * Compare two listed options keys.
 */
static int lo_kcmp(char *root_a, char *root_b, int klen)
{
    FH_ASSERT(klen);
    return (strcmp(root_a, root_b) == 0);
}

/*
 * fh_opra_lo_init
 *
 * Initialize the listed-options directory from the provided
 * listedoptions.txt file.
 */
FH_STATUS fh_opra_lo_init(const char *lo_filename)
{
    struct stat  stat_buf;
    lo_buffer_t  lo_buf;
    char        *lo_str = lo_buf.lob_string;
    FILE        *fp = NULL;
    int          line_num = 0;
    FH_STATUS     rc;

    /*
     * Make sure that the union above has the correct size of the
     * listedoptions.txt line format.
     */
    FH_ASSERT(sizeof(lo_buffer_t) > LO_TOTAL_SIZE);

    /*
     * Verify that the file exists.
     */
    if (!lo_filename || stat(lo_filename, &stat_buf) < 0) {
        FH_LOG(MGMT, ERR, ("Listed options file doesn't exist: %s",
                           lo_filename ? : "?"));
        return FH_ERROR;
    }

    /*
     * Initialize the listed options database
     */
    rc = lo_db_init(&lo_db);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to initialize LO database"));
        return FH_ERROR;
    }

    /*
     * Open the listedoptions.txt file
     */
    fp = fopen(lo_filename, "r");
    if (fp == NULL) {
        FH_LOG(MGMT, ERR, ("Failed to open listed options file: %s", lo_filename));
        return FH_ERROR;
    }

    /*
     * Parse the file now.
     */
    while (fgets(lo_str, sizeof(lo_buf), fp) != NULL) {
        fh_opra_lo_t *lo = NULL;
        lo_format_t  *lof = &lo_buf.lob_format;

        /* Trim the trailing newline character */
        lo_str[strlen(lo_str) - 1] = '\0';

        line_num++;

        FH_LOG(MGMT, INFO, ("Read line: <%s>", lo_str));

        LO_BUF_TERMINATE(lo_str, lof_root);
        LO_BUF_TERMINATE(lo_str, lof_sec);
        LO_BUF_TERMINATE(lo_str, lof_company);
        LO_BUF_TERMINATE(lo_str, lof_exch);
        LO_BUF_TERMINATE(lo_str, lof_xxx);
        LO_BUF_TERMINATE(lo_str, lof_cusip);
        LO_BUF_TERMINATE(lo_str, lof_type);

        /*
         * Only add the equity underlying and equity long term.
         *
         * EU = Equity Underlying
         * EB = Equity Bounds
         * EL = Equity Long Term
         * EF = Equity FLEX
         * CU = Currency Underlying
         * CL = Currency Long Term
         * CM = Currency Month End
         * CF = Currency FLEX
         * IL = Index Long Term
         * IU = Index Underlying
         * IF = Index FLEX
         * GF = Interest Rate Futures
         * SF = Stock Futures
         * FC = Futures Cash Index
         * FP = Futures Physical Index
         * TU = Treasury Underlying
         * TL = Treasury Long Term
         */

        if (strcmp(lof->lof_type, "EL") != 0 &&
            strcmp(lof->lof_type, "EU") != 0 &&
            strcmp(lof->lof_type, "FC") != 0 &&
            strcmp(lof->lof_type, "IU") != 0) {
            FH_LOG(MGMT, INFO, ("Skip option: %s (%s)", lof->lof_root, lof->lof_type));
            lo_db.lodb_skip++;
            continue;
        }

        rc = lo_db_add(&lo_db, &lo_buf.lob_format, &lo);
        if (rc != FH_OK && rc != FH_ERR_DUP) {
            FH_LOG(MGMT, WARN, ("Failed to add option entry from %s:%d",
                                lo_filename, line_num));
        }
    }

    fclose(fp);

    if (FH_LL_OK(MGMT, DIAG)) {
        lo_db_memdump(&lo_db);
    }

    return FH_OK;
}

/*
 * fh_opra_lo_lookup
 *
 * Lookup a given listed options from a given root symbol. If the root symbol
 * is not found, this functions will add a new entry to the table with the
 * corresponding root symbol, and a fake security symbol set to the root
 * symbol.
 */
FH_STATUS fh_opra_lo_lookup(char *root, fh_opra_lo_t **lop)
{
    FH_STATUS     rc;
    char          root_copy[6];
    fh_opra_lo_t *lo = NULL;

    /* make a copy of the root symbol that is always null-terminated (5 character root
       symbols don't come in null-terminated) */
    strncpy(root_copy, root, 5);
    root_copy[5] = '\0';

    /* lookup the root from the listed option DB. */
    rc = lo_db_lookup(&lo_db, root_copy, &lo);
    if (rc != FH_OK) {
        lo_format_t lof;

        /*
         * If the listed option is not found, then we will add it to the
         * table. It means that it is a dynamic entry that is not listed
         * in the listedoptions.txt
         */

        FH_LOG(MGMT, VSTATE, ("Add dynamic option to listed options DB: %s", root_copy));

        memset(&lof, 0, sizeof(lo_format_t));

        strcpy(lof.lof_root, root_copy);
        strcpy(lof.lof_sec,  root_copy);

        rc = lo_db_add(&lo_db, &lof, &lo);
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("Failed to add dynamic entry to listed options: %s", root_copy));
            return rc;
        }
    }

    *lop = lo;

    return FH_OK;
}

static int child_rc = 0;

/*
 * opra_lo_scp_child
 *
 * Signal handler for the child process that scp's the listedoptions.txt
 */
static void opra_lo_scp_child(int signal)
{
    int status = 0;

    // Suppress compiler warning
    if (signal) {};

    wait(&status);
    child_rc = status;
}

/*
 * fh_opra_lo_download
 *
 * Download the new listedoptions.txt file
 */
FH_STATUS fh_opra_lo_download(fh_opra_lo_cfg_t *loc)
{
    char backup[MAXPATHLEN];
    char command[300];
    int  status = 0;
    struct stat stat_buf;

    /*
     * Verify that the source filename contains the listedoptions.txt name
     */
    if (strstr(loc->loc_src_filename, "listedoptions.txt") == NULL) {
        FH_LOG(MGMT, ERR, ("Source Path %s incorrect, make sure complete PATH "
                           "and FILE name are included", loc->loc_src_filename));
        return FH_ERROR;
    }

    if (stat(loc->loc_dst_filename, &stat_buf) < 0) {
        FH_LOG(MGMT, ERR, ("File(%s) is missing: %s", loc->loc_dst_filename,
                           strerror(errno)));
        return FH_ERROR;
    }

    /*
     * Save a backup of the destination listedoptions file.
     */
    sprintf(backup, "%s.bak", loc->loc_dst_filename);

    if (rename(loc->loc_dst_filename, backup) < 0) {
        FH_LOG(MGMT, ERR, ("Rename(%s,%s) failed: %s", loc->loc_dst_filename,
                           backup, strerror(errno)));
        return FH_ERROR;
    }

    /*
     * Download the new listedoptions file
     */
    sprintf(command, "%s %s %s %s %s %s %d",
            loc->loc_scp_script,
            loc->loc_scp_username,
            loc->loc_scp_password,
            loc->loc_scp_hostname,
            loc->loc_src_filename,
            loc->loc_dst_filename,
            loc->loc_scp_timeout);

    signal(SIGCHLD, opra_lo_scp_child);

    FH_LOG_PGEN(DIAG, ("Spawning command script = %s, username = %s, password = XXXX, hostname = %s, src_filename = %s, dest_filename = %s,timeout = %d",
                       loc->loc_scp_script,
                       loc->loc_scp_username,
                       loc->loc_scp_hostname,
                       loc->loc_src_filename,
                       loc->loc_dst_filename,
                       loc->loc_scp_timeout));

    status = system(command);

    if (status != 0) {
        if (errno == ECHILD) {
            status = child_rc;
        }

        FH_LOG(MGMT, ERR, ("system(script = %s, username = %s, password = XXXX, hostname = %s, src_filename = %s, dest_filename = %s,timeout = %d), returned 0x%X -- recovering: errno:%d",
                       loc->loc_scp_script,
                       loc->loc_scp_username,
                       loc->loc_scp_hostname,
                       loc->loc_src_filename,
                       loc->loc_dst_filename,
                       loc->loc_scp_timeout, status, errno));

        /*
         * Restore the listedoptions file
         */
        if (rename(backup, loc->loc_dst_filename) < 0) {
            FH_LOG(MGMT, ERR, ("Rename(%s,%s) (recovery) failed: %s",
                               backup, loc->loc_dst_filename, strerror(errno)));
        }

        return FH_ERROR;
    }

    FH_LOG(MGMT, STATE, ("New listed options file successfully obtained.  (status:%d)", status));

    return(FH_OK);
}

/*
 * fh_opra_lo_cfg_load
 *
 * Load the listedoptions configuration from the OPRA configuration.
 */
FH_STATUS fh_opra_lo_cfg_load(fh_cfg_node_t *config, fh_opra_lo_cfg_t *loc)
{
    const fh_cfg_node_t *node;
    long                 value;
    char                *endptr;
    const char          *strval = NULL;

    // Look for the listed_options configuration
    node = fh_cfg_get_node(config, "opra.listed_options");
    if (!node) {
        FH_LOG(CSI, ERR, ("Missing listed_options configuration"));
        return FH_ERROR;
    }

    // Get the source location of the listedoptions.txt
    strval = fh_cfg_get_string(node, "src_filename");
    if (!strval) {
        FH_LOG(CSI, ERR, ("Missing src_filename for listedoptions.txt configuration"));
        return FH_ERROR;
    }

    strncpy(loc->loc_src_filename, strval, sizeof(loc->loc_src_filename));

    // Get the destination location of the listedoptions.txt
    strval = fh_cfg_get_string(node, "dst_filename");
    if (!strval) {
        FH_LOG(CSI, ERR, ("Missing dst_filename for listedoptions.txt configuration"));
        return FH_ERROR;
    }

    strncpy(loc->loc_dst_filename, strval, sizeof(loc->loc_dst_filename));

    // Get the SCP script path
    strval = fh_cfg_get_string(node, "scp_script");
    if (!strval) {
        FH_LOG(CSI, ERR, ("Missing scp_script for listedoptions.txt configuration"));
        return FH_ERROR;
    }

    strncpy(loc->loc_scp_script, strval, sizeof(loc->loc_scp_script));

    // Get the SCP timeout value
    strval = fh_cfg_get_string(node, "scp_timeout");
    if (!strval) {
        FH_LOG(CSI, ERR, ("Missing scp_timeout for listedoptions.txt configuration"));
        return FH_ERROR;
    }

    value = strtol(strval, &endptr , 0);
    if(*strval == '\0' || *endptr != '\0') {
        FH_LOG(CSI, ERR, ("scp timeout must be numeric (was '%s')", strval));
        return FH_ERROR;
    }

    loc->loc_scp_timeout = value;

    // Get the SCP username
    strval = fh_cfg_get_string(node, "scp_username");
    if (!strval) {
        FH_LOG(CSI, ERR, ("Missing scp_username for listedoptions.txt configuration"));
        return FH_ERROR;
    }

    strncpy(loc->loc_scp_username, strval, sizeof(loc->loc_scp_username));

    // Get the SCP password
    strval = fh_cfg_get_string(node, "scp_password");
    if (!strval) {
        FH_LOG(CSI, ERR, ("Missing scp_password for listedoptions.txt configuration"));
        return FH_ERROR;
    }

    strncpy(loc->loc_scp_password, strval, sizeof(loc->loc_scp_password));

    // Get the SCP host
    strval = fh_cfg_get_string(node, "scp_hostname");
    if (!strval) {
        FH_LOG(CSI, ERR, ("Missing scp_hostname for listedoptions.txt configuration"));
        return FH_ERROR;
    }

    strncpy(loc->loc_scp_hostname, strval, sizeof(loc->loc_scp_hostname));

    return FH_OK;
}

