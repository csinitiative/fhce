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

// FH test headers
#include "fh_test_assert.h"

// System headers
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// FH common headers

// FH Arca common headers
#include "../../common/fh_arca_cfg.h"

// generate a valid configuration with no index in a temp file and return the filename
const char *no_index_config()
{
    char    *filename;
    int      tmpdes;
    FILE    *outfile;
    
    // create a temp file in which to create the basic configuration file
    filename = (char *)malloc(sizeof(char) * 100);
    strcpy(filename, "/tmp/fhtest.XXXXXX");
    tmpdes = mkstemp(filename);
    outfile = fdopen(tmpdes, "w+");
    
    // output a basic configuration file with all of the elements available in config files
    fprintf(outfile, "arca = {\n");
    fprintf(outfile, "    processes = {\n");
    fprintf(outfile, "        foo = {\n");
    fprintf(outfile, "            lines        = ( \"foo\" )\n");
    fprintf(outfile, "            cpu          = 1\n");
    fprintf(outfile, "            max_sessions = 8\n");
    fprintf(outfile, "            max_symbols  = 8000\n");
    fprintf(outfile, "            max_firms    = 2000\n");
    fprintf(outfile, "            max_orders   = 100000\n");
    fprintf(outfile, "        }\n");
    fprintf(outfile, "    }\n");
    fprintf(outfile, "    lines = {\n");
    fprintf(outfile, "        foo = {\n");
    fprintf(outfile, "            primary = {\n");
    fprintf(outfile, "              address   = \"10.0.0.1\"\n");
    fprintf(outfile, "              port      = 12345\n");
    fprintf(outfile, "              interface = eth0\n");
    fprintf(outfile, "              enable    = yes\n");
    fprintf(outfile, "            }\n");
    fprintf(outfile, "            secondary = {\n");
    fprintf(outfile, "              address   = \"10.0.0.1\"\n");
    fprintf(outfile, "              port      = 54321\n");
    fprintf(outfile, "              interface = eth0\n");
    fprintf(outfile, "              enable    = yes\n");
    fprintf(outfile, "            }\n");
    fprintf(outfile, "            fast            = no\n");
    fprintf(outfile, "            strict_ordering = no\n");
    fprintf(outfile, "        }\n");
    fprintf(outfile, "    }\n");
    fprintf(outfile, "}\n");
    
    // close the tempfile and return the filename
    fclose(outfile);
    return filename;
}

// generate a valid configuration with an index in a temp file and return the filename
const char *index_config()
{
    char    *filename;
    int      tmpdes;
    FILE    *outfile;
    
    // create a temp file in which to create the basic configuration file
    filename = (char *)malloc(sizeof(char) * 100);
    strcpy(filename, "/tmp/fhtest.XXXXXX");
    tmpdes = mkstemp(filename);
    outfile = fdopen(tmpdes, "w+");
    
    // output a basic configuration file with all of the elements available in config files
    fprintf(outfile, "arca = {\n");
    fprintf(outfile, "    processes = {\n");
    fprintf(outfile, "        foo = {\n");
    fprintf(outfile, "            lines        = ( \"foo\" )\n");
    fprintf(outfile, "            cpu          = 1\n");
    fprintf(outfile, "            index        = 5\n");
    fprintf(outfile, "            max_sessions = 8\n");
    fprintf(outfile, "            max_symbols  = 8000\n");
    fprintf(outfile, "            max_firms    = 2000\n");
    fprintf(outfile, "            max_orders   = 100000\n");
    fprintf(outfile, "        }\n");
    fprintf(outfile, "    }\n");
    fprintf(outfile, "    lines = {\n");
    fprintf(outfile, "        foo = {\n");
    fprintf(outfile, "            primary = {\n");
    fprintf(outfile, "              address   = \"10.0.0.1\"\n");
    fprintf(outfile, "              port      = 12345\n");
    fprintf(outfile, "              interface = eth0\n");
    fprintf(outfile, "              enable    = yes\n");
    fprintf(outfile, "            }\n");
    fprintf(outfile, "            secondary = {\n");
    fprintf(outfile, "              address   = \"10.0.0.1\"\n");
    fprintf(outfile, "              port      = 54321\n");
    fprintf(outfile, "              interface = eth0\n");
    fprintf(outfile, "              enable    = yes\n");
    fprintf(outfile, "            }\n");
    fprintf(outfile, "            fast            = no\n");
    fprintf(outfile, "            strict_ordering = no\n");
    fprintf(outfile, "        }\n");
    fprintf(outfile, "    }\n");
    fprintf(outfile, "}\n");
    
    // close the tempfile and return the filename
    fclose(outfile);
    return filename;
}

// delete a configuration file at the given path
void delete_config(const char *filename)
{
    unlink(filename);
}

// test that not including an index in a process configuration will produce a value of -1
void test_no_index_configuration_produces_negative_one()
{
    const char       *filename;
    fh_cfg_node_t    *config;
    
    filename = no_index_config();
    config = fh_cfg_load(filename);
    delete_config(filename);
    
    FH_TEST_ASSERT_EQUAL(fh_arca_cfg_load(config, "foo"), FH_OK);
    FH_TEST_ASSERT_LEQUAL(fh_arca_cfg.index, -1);
}

// test that an included index value in the configuration produces the correct result
void test_index_configuration_produces_correct_index()
{
    const char       *filename;
    fh_cfg_node_t    *config;
    
    filename = index_config();
    config = fh_cfg_load(filename);
    delete_config(filename);
    
    FH_TEST_ASSERT_EQUAL(fh_arca_cfg_load(config, "foo"), FH_OK);
    FH_TEST_ASSERT_LEQUAL(fh_arca_cfg.index, 5);
}
