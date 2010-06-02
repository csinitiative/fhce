/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

/* FH test headers */
#include "fh_test_assert.h"

/* System headers */
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/* FH common headers */
#include "fh_config.h"
#include "fh_log.h"

/* FH shared config headers */
#include "fh_shr_config.h"

/* create a *very* basic configuration file */
const char *create_config_file(const char *contents) {
    char    *filename;
    int      tmpdes;
    FILE    *outfile;

    /* create a temp file in which to create the basic configuration file */
    filename = (char *)malloc(sizeof(char) * 100);
    strcpy(filename, "/tmp/fhtest.XXXXXX");
    tmpdes = mkstemp(filename);
    outfile = fdopen(tmpdes, "w+");

    /* output contents to the newly created config file */
    fprintf(outfile, "%s", contents);

    /* close the tempfile and return the filename */
    fclose(outfile);
    return filename;
}

/* delete a configuration file at the given path */
void delete_config_file(const char *filename)
{
    unlink(filename);
}


/* test that providing a config structure with no logging configuration produces FH_OK and no */
/* change in logging configuration */
void test_no_logging_config_produces_no_change()
{
    /* options and config file stuff */
    fh_shr_cfg_options_t     options;
    const fh_cfg_node_t  *config;
    const char           *filename;

    /* log level stuff */
    extern uint32_t       fh_log_cfg;
    extern uint32_t       fh_log_lvl[FH_LC_MAX];
    uint32_t              old_log_cfg;
    uint32_t              old_log_lvl[FH_LC_MAX];

    /* initialize options */
    memset(&options, 0, sizeof(fh_shr_cfg_options_t));

    /* get a very basic configuration structure and make sure temp config file is deleted */
    /* before any tests are run. */
    filename = create_config_file("foo = bar");
    config   = fh_cfg_load(filename);
    delete_config_file(filename);

    /* initialize logging so that all default settings are applied */
    fh_log_init(NULL);

    /* store the values of logging variables before call to fh_shr_cfg_log_init */
    old_log_cfg = fh_log_cfg;
    memcpy(&old_log_lvl, &fh_log_lvl, sizeof(old_log_lvl));

    /* test that we get an FH_OK value back */
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_log_init(&options, config), FH_OK);

    /* test that log values have not changed */
    FH_TEST_ASSERT_EQUAL(old_log_cfg, fh_log_cfg);
    FH_TEST_ASSERT_EQUAL(memcmp(&old_log_lvl, &fh_log_lvl, sizeof(old_log_lvl)), 0);
}

/* test that FH_LCF_CONSOLE is set when debug_level > 0 */
void test_console_logging_when_in_debug_mode()
{
    fh_shr_cfg_options_t     options;
    const fh_cfg_node_t  *config;
    const char           *filename;
    uint32_t              config_val;

    /* initialize options */
    memset(&options, 0, sizeof(fh_shr_cfg_options_t));
    options.debug_level = 1;

    /* get a very basic configuration structure and make sure temp config file is deleted */
    /* before any tests are run. */
    filename = create_config_file("foo = bar");
    config   = fh_cfg_load(filename);
    delete_config_file(filename);

    /* test that we get an FH_OK value back and that console logging is on */
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_log_init(&options, config), FH_OK);
    FH_TEST_ASSERT_EQUAL(fh_log_get_cfg("CONSOLE", &config_val), FH_OK);
    FH_TEST_ASSERT_TRUE(config_val);
}

/* test that FH_LCF_CONSOLE is set when display_version */
void test_console_logging_when_in_display_version_mode()
{
    fh_shr_cfg_options_t     options;
    const fh_cfg_node_t  *config;
    const char           *filename;
    uint32_t              config_val;

    /* initialize options */
    memset(&options, 0, sizeof(fh_shr_cfg_options_t));
    options.display_version = 1;

    /* get a very basic configuration structure and make sure temp config file is deleted */
    /* before any tests are run. */
    filename = create_config_file("foo = bar");
    config   = fh_cfg_load(filename);
    delete_config_file(filename);

    /* test that we get an FH_OK value back and that console logging is on */
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_log_init(&options, config), FH_OK);
    FH_TEST_ASSERT_EQUAL(fh_log_get_cfg("CONSOLE", &config_val), FH_OK);
    FH_TEST_ASSERT_TRUE(config_val);
}

/* test that config file-specific log configuration is loaded */
void test_config_file_logging_info_is_loaded()
{
    fh_shr_cfg_options_t     options;
    const fh_cfg_node_t  *config;
    const char           *filename;
    extern uint32_t       fh_log_lvl[FH_LC_MAX];
    int                   i;

    /* initialize options */
    memset(&options, 0, sizeof(fh_shr_cfg_options_t));

    /* get a very basic configuration structure and make sure temp config file is deleted */
    /* before any tests are run. */
    filename = create_config_file("log = {\ndefault = WARN\nlevel = { MGMT:STATS }\n}");
    config   = fh_cfg_load(filename);
    delete_config_file(filename);

    /* check that we get an FH_OK value back and that the MGMT class has STATS level set */
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_log_init(&options, config), FH_OK);
    FH_TEST_ASSERT_TRUE(fh_log_lvl[FH_LC_MGMT] & FH_LL_STATS);

    /* check that all logging classes have level WARN set */
    for (i = 0; i < FH_LC_MAX; i++) {
        FH_TEST_ASSERT_TRUE(fh_log_lvl[i] & FH_LL_WARN);
    }
}
