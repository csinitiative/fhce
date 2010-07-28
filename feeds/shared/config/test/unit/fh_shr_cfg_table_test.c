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
#include "fh_shr_cfg_table.h"

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

/* generate a valid configuration in a temp file and return the filename */
fh_cfg_node_t *valid_config()
{
    const char          *filename;
    fh_cfg_node_t       *config;


    filename = create_config_file(
        "foo_table = {"
        "   size        = 100"
        "}"
    );
    config = fh_cfg_load(filename);
    delete_config_file(filename);
    return config;
}

/* test to make sure that the proper error is returned when an invalid table name is given */
void test_invalid_table_name_produces_error()
{
    fh_shr_cfg_tbl_t     tbl_config;
    fh_cfg_node_t       *config = valid_config();

    memset(&tbl_config, 0, sizeof(fh_shr_cfg_tbl_t));

    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_tbl_load(config, "foo", &tbl_config), FH_ERR_NOTFOUND);
}

/* make sure that the absence of an enabled property returns the correct error */
void test_missing_enabled_property()
{
    fh_shr_cfg_tbl_t     tbl_config;
    const char          *filename;
    fh_cfg_node_t       *config;

    filename = create_config_file(
        "foo_table = {"
        "}"
    );

    config = fh_cfg_load(filename);
    delete_config_file(filename);
    memset(&tbl_config, 1, sizeof(fh_shr_cfg_tbl_t));

    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_tbl_load(config, "foo_table", &tbl_config), FH_ERR_NOTFOUND);
    FH_TEST_ASSERT_FALSE(tbl_config.enabled);
}

/* make sure that a valid 'enabled' property is reflected in resulting configuration */
void test_invalid_size_property_produces_disabled_config()
{
    fh_shr_cfg_tbl_t     tbl_config;
    const char          *filename;
    fh_cfg_node_t       *config;

    filename = create_config_file(
        "foo_table = {"
        "   size    = blah"
        "}"
    );

    config = fh_cfg_load(filename);
    delete_config_file(filename);
    memset(&tbl_config, 0, sizeof(fh_shr_cfg_tbl_t));

    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_tbl_load(config, "foo_table", &tbl_config), FH_ERROR);
    FH_TEST_ASSERT_FALSE(tbl_config.enabled);
}

/* make sure that a valid 'enabled' property is reflected in resulting configuration */
void test_size_property_sets_table_config_properly()
{
    fh_shr_cfg_tbl_t     tbl_config;
    fh_cfg_node_t       *config = valid_config();

    memset(&tbl_config, 0, sizeof(fh_shr_cfg_tbl_t));

    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_tbl_load(config, "foo_table", &tbl_config), FH_OK);
    FH_TEST_ASSERT_TRUE(tbl_config.enabled);
    FH_TEST_ASSERT_EQUAL(tbl_config.size, 100);
}
