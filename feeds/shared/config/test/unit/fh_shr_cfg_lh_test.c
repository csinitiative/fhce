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

/* system headers */
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

/* FH unit test framework headers */
#include "fh_test_assert.h"

/* FH common headers */
#include "fh_errors.h"
#include "fh_config.h"

/* FH shared config headers */
#include "fh_shr_cfg_lh.h"

/* generate a valid configuration in a temp file and return the filename */
const char *valid_config()
{
    char    *filename;
    int      tmpdes;
    FILE    *outfile;
    
    /* create a temp file in which to create the basic configuration file */
    filename = (char *)malloc(sizeof(char) * 100);
    strcpy(filename, "/tmp/fhtest.XXXXXX");
    tmpdes = mkstemp(filename);
    outfile = fdopen(tmpdes, "w+");
    
    /* output a basic configuration file with all of the elements available in config files */
    fprintf(outfile,
        "itch = {"
        "    processes = {"
        "        foo = {"
        "            lines       = ( \"foo\" )"
        "        }"
        "    }"
        "    lines = {"
        "        foo = {"
        "            primary = { address:\"10.0.0.1\" port:12345 interface:eth0 enabled: yes }"
        "        }"
        "    }"
        "}"
    );
    
    /* close the tempfile and return the filename */
    fclose(outfile);
    return filename;
}

/* generate an invalid configuration in a temp file and return the filename */
const char *empty_config()
{
    char    *filename;
    int      tmpdes;
    FILE    *outfile;
    
    /* create a temp file in which to create the basic configuration file */
    filename = (char *)malloc(sizeof(char) * 100);
    strcpy(filename, "/tmp/fhtest.XXXXXX");
    tmpdes = mkstemp(filename);
    outfile = fdopen(tmpdes, "w+");
    
    /* print an empty string to the file */
    fprintf(outfile, "\n");
    
    /* close the tempfile and return the filename */
    fclose(outfile);
    return filename;
}

/* delete a configuration file at the given path */
void delete_config(const char *filename)
{
    unlink(filename);
}

/* test that ...get_process() returns FH_ERROR when given a "\0" process name and config with no */
/* default process */
void test_get_proc_failure_with_no_name_and_invalid_config()
{
    const char       *filename;
    fh_cfg_node_t    *config;
    char              process[MAX_PROPERTY_LENGTH] = "\0";
    
    filename = empty_config();
    config = fh_cfg_load(filename);
    delete_config(filename);
    
    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_lh_get_proc(process, "itch", config), FH_ERROR);
}

/* test that ...get_process() returns FH_OK and sets the process argument correctly when given a */
/* "\0" process name and config with no default process */
void test_get_proc_success_with_no_name_and_valid_config()
{
    const char       *filename;
    fh_cfg_node_t    *config;
    char              process[MAX_PROPERTY_LENGTH] = "\0";
    
    filename = valid_config();
    config = fh_cfg_load(filename);
    delete_config(filename);

    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_lh_get_proc(process, "itch", config), FH_OK);
    FH_TEST_ASSERT_STREQUAL(process, "foo");
}

/* test that ...get_process() returns FH_ERROR when given a process name that doesn't exist in */
/* the configuration */
void test_get_proc_failure_with_nonexistent_name()
{
    const char       *filename;
    fh_cfg_node_t    *config;
    char              process[MAX_PROPERTY_LENGTH] = "bar";
    
    filename = valid_config();
    config = fh_cfg_load(filename);
    delete_config(filename);

    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_lh_get_proc(process, "itch", config), FH_ERROR);
}

/* test that ...get_process() returns FH_OK when given a process name that exists */
void test_get_proc_success_with_existent_name()
{
    const char       *filename;
    fh_cfg_node_t    *config;
    char              process[MAX_PROPERTY_LENGTH] = "foo";
    
    filename = valid_config();
    config = fh_cfg_load(filename);
    delete_config(filename);

    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_lh_get_proc(process, "itch", config), FH_OK);
}

/* test that ...load() returns FH_ERROR when given a process name that doesn't exist */
void test_load_failure_with_nonexistent_name()
{
    const char              *filename;
    fh_shr_cfg_lh_proc_t     lh_config;
    fh_cfg_node_t           *config;
    
    filename = valid_config();
    config = fh_cfg_load(filename);
    delete_config(filename);

    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_lh_load("bar", "itch", config, &lh_config), FH_ERROR);
}

/* test that ...load() fills in the lh_config structure with process info when given a valid */
/* process name */
void test_load_success_with_existent_name()
{
    const char              *filename;
    fh_shr_cfg_lh_proc_t     lh_config;
    fh_cfg_node_t           *config;
    
    memset(&lh_config, 0, sizeof(fh_shr_cfg_lh_proc_t));
    filename = valid_config();
    config = fh_cfg_load(filename);
    delete_config(filename);

    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_lh_load("foo", "itch", config, &lh_config), FH_OK);
    FH_TEST_ASSERT_STREQUAL(lh_config.name, "foo");
}

/* test that loading a config without and lines produces FH_ERROR */
void test_failure_when_loading_config_with_no_lines()
{
    const char              *filename;
    fh_shr_cfg_lh_proc_t     lh_config;
    fh_cfg_node_t           *config;
    FILE                    *outfile;
    
    filename = empty_config();
    outfile = fopen(filename, "a");
    fprintf(outfile, "itch = {\n");
    fprintf(outfile, "    processes = {\n");
    fprintf(outfile, "        foo = {\n");
    fprintf(outfile, "        }\n");
    fprintf(outfile, "    }\n");
    fprintf(outfile, "}\n");
    fclose(outfile);
    
    config = fh_cfg_load(filename);
    delete_config(filename);
    memset(&lh_config, 0, sizeof(fh_shr_cfg_lh_proc_t));

    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_lh_load("foo", "itch", config, &lh_config), FH_ERROR);
}

/* test that loading a config without a line configuration for process lines produces FH_ERROR */
void test_failure_when_loading_config_with_no_line_configuration_for_process_lines()
{
    const char              *filename;
    fh_shr_cfg_lh_proc_t     lh_config;
    fh_cfg_node_t           *config;
    FILE                    *outfile;
    
    filename = empty_config();
    outfile = fopen(filename, "a");
    fprintf(outfile, "itch = {\n");
    fprintf(outfile, "    processes = {\n");
    fprintf(outfile, "        foo = {\n");
    fprintf(outfile, "            lines    = ( \"foo\" )\n");
    fprintf(outfile, "        }\n");
    fprintf(outfile, "    }\n");
    fprintf(outfile, "    lines = {\n");
    fprintf(outfile, "        bar = {\n");
    fprintf(outfile, "            primary = { address:\"10.0.0.1\" port:12345 interface:eth0 }\n");
    fprintf(outfile, "        }\n");
    fprintf(outfile, "    }\n");
    fprintf(outfile, "}\n");
    fclose(outfile);
    
    config = fh_cfg_load(filename);
    delete_config(filename);
    memset(&lh_config, 0, sizeof(fh_shr_cfg_lh_proc_t));

    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_lh_load("foo", "itch", config, &lh_config), FH_ERROR);
}

/* test that the correct line configuration is created for a valid config file */
void test_load_correct_line_configuration_for_valid_config()
{
    const char              *filename;
    unsigned long            address;
    fh_shr_cfg_lh_proc_t     lh_config;
    fh_cfg_node_t           *config;
    
    memset(&lh_config, 0, sizeof(fh_shr_cfg_lh_proc_t));
    filename = valid_config();
    config = fh_cfg_load(filename);
    delete_config(filename);

    /* load configuration and make some basic assertions */
    FH_TEST_ASSERT_NOTNULL(config);
    FH_TEST_ASSERT_EQUAL(fh_shr_cfg_lh_load("foo", "itch", config, &lh_config), FH_OK);
    FH_TEST_ASSERT_EQUAL(lh_config.num_lines, 1);
    
    /* assert that the proper lines are enabled/disabled */
    FH_TEST_ASSERT_TRUE(lh_config.lines[0].primary.enabled);
    FH_TEST_ASSERT_FALSE(lh_config.lines[0].secondary.enabled);
    
    /* assert that the proper options are set for the line(s) that is/are enabled */
    address = inet_addr(fh_cfg_get_string(config, "itch.lines.foo.primary.address"));
    FH_TEST_ASSERT_LEQUAL((long int)lh_config.lines[0].primary.address, (long int)address);
    FH_TEST_ASSERT_LEQUAL((long int)lh_config.lines[0].primary.port,  (long int)12345);
    FH_TEST_ASSERT_STREQUAL(lh_config.lines[0].primary.interface, "eth0");
    
}
