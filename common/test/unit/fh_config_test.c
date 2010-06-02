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

// System headers
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// FH common headers
#include "fh_config.h"

// FH test headers
#include "fh_test_assert.h"


// create a basic configuration file
const char *create_basic_config_file() {
    char    *filename;
    int      tmpdes;
    FILE    *outfile;

    // create a temp file in which to create the basic configuration file
    filename = (char *)malloc(sizeof(char) * 100);
    strcpy(filename, "/tmp/fhtest.XXXXXX");
    tmpdes = mkstemp(filename);
    outfile = fdopen(tmpdes, "w+");

    // output a basic configuration file with all of the elements available in config files
    fprintf(outfile, "foo = { bar:123 baz:\"string with spaces\" }\n");
    fprintf(outfile, "  # indented comment\n");
    fprintf(outfile, "array = ( 1, \"string with spaces\", 123 )\n");

    // close the tempfile and return the filename
    fclose(outfile);
    return filename;
}

// delete a configuration file at the given path
void delete_config_file(const char *filename)
{
    unlink(filename);
}

// test that parsing of a basic configuration file works properly
void test_basic_config_file_parses_successfully()
{
    const char *filename;

    filename = create_basic_config_file();
    FH_TEST_ASSERT_NOTNULL(fh_cfg_load(filename));
    delete_config_file(filename);
}

// test that parsing of an invalid configuration file returns NULL
void test_invalid_config_file_fails_to_parse()
{
    const char *filename;
    FILE       *file;

    // add some invalid syntax to the config file
    filename = create_basic_config_file();
    file = fopen(filename, "a+");
    fprintf(file, "invalid;abcdefg:123\n");
    fclose(file);

    FH_TEST_ASSERT_NULL(fh_cfg_load(filename));
    delete_config_file(filename);
}

// test that get_node returns a non-null value when a valid node is requested
void test_get_node_returns_non_null_for_valid_path()
{
    const char    *filename;
    fh_cfg_node_t *config = NULL;

    filename = create_basic_config_file();
    config = fh_cfg_load(filename);
    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_NOTNULL(fh_cfg_get_node(config, "foo.baz"));
    delete_config_file(filename);
}

// test that get_node returns a null value when an invalid node is requested
void test_get_node_returns_null_for_invalid_path()
{
    const char    *filename;
    fh_cfg_node_t *config = NULL;

    filename = create_basic_config_file();
    config = fh_cfg_load(filename);
    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_NULL(fh_cfg_get_node(config, "foo.bax"));
    delete_config_file(filename);
}

// test that get_string returns the expected string when a valid node is requested
void test_get_string_returns_expected_string_for_valid_path()
{
    const char    *filename;
    fh_cfg_node_t *config = NULL;

    filename = create_basic_config_file();
    config = fh_cfg_load(filename);
    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_STREQUAL(fh_cfg_get_string(config, "foo.baz"), "string with spaces");
    delete_config_file(filename);
}

// test that get_string returns NULL when an invalid node is requested
void test_get_string_returns_null_for_invalid_path()
{
    const char    *filename;
    fh_cfg_node_t *config = NULL;

    filename = create_basic_config_file();
    config = fh_cfg_load(filename);
    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_NULL(fh_cfg_get_string(config, "foo.bax"));
    delete_config_file(filename);
}

// test that get_array returns the correct array of strings when a valid node is requested
void test_get_array_returns_expected_array_for_valid_path()
{
    const char       *filename;
    const char      **strings;
    fh_cfg_node_t    *config = NULL;

    filename = create_basic_config_file();
    config = fh_cfg_load(filename);
    FH_TEST_ASSERT_NOTNULL(config);

    strings = fh_cfg_get_array(config, "array");
    FH_TEST_ASSERT_STREQUAL(strings[0], "1");
    FH_TEST_ASSERT_STREQUAL(strings[1], "string with spaces");
    FH_TEST_ASSERT_STREQUAL(strings[2], "123");
    delete_config_file(filename);
}

// test that get_array returns NULL when an invalid node is requested
void test_get_array_returns_null_for_invalid_path()
{
    const char       *filename;
    fh_cfg_node_t    *config = NULL;

    filename = create_basic_config_file();
    config = fh_cfg_load(filename);
    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_NULL(fh_cfg_get_array(config, "nonexistent_array"));
    delete_config_file(filename);
}
