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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// FH common headers
#include "fh_log.h"
#include "fh_config.h"


// Global variables
fh_cfg_node_t *fh_priv_config = NULL;

// External references
extern FILE *yyin;


/*! \brief Load a configuration file
 *
 *  \param filename configuration file to load
 *  \return a pointer to the head node of the generated config structure (or NULL on error)
 */
fh_cfg_node_t *fh_cfg_load(const char *filename)
{
    // allocate memory for the head node
    fh_priv_config = (fh_cfg_node_t *)malloc(sizeof(fh_cfg_node_t));
    if (fh_priv_config == NULL) {
        fprintf(stderr, "%s: out of memory (%d)\n", __FILE__, __LINE__);
        return NULL;
    }
    memset(fh_priv_config, 0, sizeof(fh_cfg_node_t));

    // open the file
    yyin = fopen(filename, "r");
    if (yyin == NULL) {
        fprintf(stderr, "Unable to open specified configuration file: %s\n", filename);
        return NULL;
    }

    // parse the file
    while (!feof(yyin)) {
        if (yyparse() != 0) return NULL;
    }

    // close the file
    fclose (yyin);

    return fh_priv_config;
}

/*! \brief Recursive internal function to actually dump the contents of a config
 *
 *  \param config configuration node being dumped
 *  \param indent the current level of indentation being applied to this node
 */
void _fh_cfg_dump(fh_cfg_node_t *config, int indent)
{
    int i;

    // print spaces for the indent, then the node name
    for (i = 0; i <= indent; i++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "%s ", ((strlen(config->name) <= 0) ? "<ROOT>" : config->name));

    // if this node has children
    if (config->num_children > 0) {
        // print the opening brace for this node's children
        fprintf(stderr, "=> {\n");

        // run this function recursively for its children
        for (i = 0; i < config->num_children; i++) {
            _fh_cfg_dump(config->children[i], indent + 4);
        }

        // print the appropriate amount of indent space and then the closing brace
        for (i = 0; i <= indent; i++) {
            fprintf(stderr, " ");
        }
        fprintf(stderr, "}\n");
    }
    // if this node has values (can't have children and values)
    else if(config->num_values > 0) {
        // print the number of values and an opening square brace
        fprintf(stderr, "(%d) [ ", config->num_values);

        // print each value, separated by a comma
        for (i = 0; i < config->num_values; i++) {
            if (i > 0) {
                fprintf(stderr, ", ");
            }
            fprintf(stderr, "'%s'", config->values[i]);
        }

        // print the closing square brace
        fprintf(stderr, " ]\n");
    }
}

/*! \brief Function to dump the contents of a configuration to stderr.
 *
 *  \param head configuration node to dump
 */
void fh_cfg_dump(fh_cfg_node_t *config)
{
    // call the recursive dump function on the root node with an indent value of 0
    _fh_cfg_dump(config, 0);
}

/*! \brief (private) Returns a pointer to the child with the given name
 *
 *  \param node top not from which search will be initiated
 *  \param name the name of the node that we are looking for
 *  \return pointer to the requested node or NULL if the reuquested node could not be found
 */
const fh_cfg_node_t *_fh_cfg_find_child(const fh_cfg_node_t *node, const char *name)
{
    int i;

    // locate the child that corresponds to the current segment
    for(i = 0; i < node->num_children; i++) {
        if (!strcmp(node->children[i]->name, name)) {
            return node->children[i];
        }
    }

    // return NULL if we have gotten here
    return NULL;
}

/*! \brief Return the configuration node at the given path of the configuration
 *
 *  \param config the root configuration node where search will begin
 *  \param path the path to the node we are looking for
 *  \return a pointer to the node requested or NULL if the path is not found
 */
const fh_cfg_node_t *fh_cfg_get_node(const fh_cfg_node_t *config, const char *path)
{
    int                   num_chars;
    char                  segment[MAX_PROPERTY_LENGTH];
    char                 *marker;
    const char           *remains = path;
    const fh_cfg_node_t  *curr    = config;

    // initialize marker to the location of the first '.' in the path string
    marker = strchr(remains, '.');

    // loop through segments of the path until no segments remain
    while (marker != NULL) {
        // extract the current segment from the path string
        num_chars = (marker - remains) / sizeof(char);
        strncpy(segment, remains, num_chars);
        segment[num_chars] = '\0';

        // advance the curr pointer to the appropriate child
        curr = _fh_cfg_find_child(curr, segment);
        if (curr == NULL) {
            return NULL;
        }

        // set up for the next segment
        remains = marker + sizeof(char);
        marker = strchr(remains, '.');
    }

    // advance the pointer to the final node
    curr = _fh_cfg_find_child(curr, remains);
    if (curr == NULL) {
        return NULL;
    }

    // if we have gotten here, the current node is the one we are searching for, return it
    return curr;
}

/*! \brief Return the string value at the given path of the configuration
 *
 *  \param config the root configuration node where search will begin
 *  \param path the path to the node whose string value we are looking for
 *  \return a pointer to the string value requested or NULL if the path is not found
 */
const char *fh_cfg_get_string(const fh_cfg_node_t *config, const char *path)
{
    const fh_cfg_node_t *node;

    // search, starting at the passed in config node, for the requested path
    node = fh_cfg_get_node(config, path);

    // if the search returned a NULL, return NULL
    if(node == NULL || node->num_values < 1) {
        return NULL;
    }

    // return the first value
    return node->values[0];
}

/**
 * Return the array of values at the given path of the configuration
 *
 * @param  (const fh_cfg_node_t *) config to be looked through
 * @param  (const char *)          path of the option (in dotted notation) we are looking for
 * @return (const char **)
 */
const char **fh_cfg_get_array(const fh_cfg_node_t *config, const char *path)
{
    const fh_cfg_node_t *node;

    node = fh_cfg_get_node(config, path);
    if(node == NULL) return NULL;

    // return the first value
    return (const char **)node->values;
}

/**
 * Free a configuration node and its children
 *
 * @param (fh_cfg_node_t *) configure node to be freed
 */
void fh_cfg_free(fh_cfg_node_t *node)
{
    int i;

    // call fh_cfg_free recursively on all children, then free the children array
    for(i = 0; i < node->num_children; i++) {
        fh_cfg_free(node->children[i]);
    }
    free(node->children);

    // free all values in the values array, then free the values array itself
    for(i = 0; i < node->num_values; i++) {
        free(node->values[i]);
    }
    free(node->values);

    // free the node
    free(node);
}

/*
 *  Set a yes/no field value
 */
FH_STATUS fh_cfg_set_yesno(const fh_cfg_node_t *config, const char *property, uint8_t *value)
{
    const char *str;

    /* get the value of the property and return false if it was not found */
    str = fh_cfg_get_string(config, property);
    if (str == NULL) {
        return FH_ERR_NOTFOUND;
    }

    /* if the value is yes, set value to 1 */
    if ((str[0] == 'y' || str[0] == 'Y') && (str[1] == 'e' || str[1] == 'E') &&
        (str[2] == 's' || str[2] == 'S')) {
        *value = 1;
    }
    /* if the value is no, set value to 0 */
    else if ((str[0] == 'n' || str[0] == 'N') && (str[1] == 'o' || str[1] == 'O')) {
        *value = 0;
    }
    /* if the value is neither, log an error and return false */
    else {
        return FH_ERROR;
    }

    /* if we get here, success */
    return FH_OK;
}

/*
 * Set a uint32_t field value
 */
FH_STATUS fh_cfg_set_uint32(const fh_cfg_node_t *config, const char *property, uint32_t *value)
{
    const char  *str;
    char        *last;
    uint32_t     val;

    /* get the value of the property and return false if it was not found */
    str = fh_cfg_get_string(config, property);
    if (str == NULL) {
        return FH_ERR_NOTFOUND;
    }

    /* convert the value to a uint32_t, logging an error and returning false if the conversion */
    /* fails on any character */
    val = (uint32_t)strtoul(str, &last, 0);
    if (str[0] == '\0' || last[0] != '\0') {
        return FH_ERROR;
    }

    /* success, set value and return true */
    *value = val;
    return FH_OK;
}

/*
 * Set a uint16_t field value
 */
FH_STATUS fh_cfg_set_uint16(const fh_cfg_node_t *config, const char *property, uint16_t *value)
{
    const char  *str;
    char        *last;
    uint16_t     val;

    /* get the value of the property and return false if it was not found */
    str = fh_cfg_get_string(config, property);
    if (str == NULL) {
        return FH_ERR_NOTFOUND;
    }

    /* convert the value to a uint32_t, logging an error and returning false if the conversion */
    /* fails on any character */
    val = (uint16_t)strtoul(str, &last, 0);
    if (str[0] == '\0' || last[0] != '\0') {
        return FH_ERROR;
    }

    /* success, set value and return true */
    *value = val;
    return FH_OK;
}

/*
 *  Set an int field value
 */
FH_STATUS fh_cfg_set_int(const fh_cfg_node_t *config, const char *property, int *value)
{
    const char  *str;
    char        *last;
    int          val;

    /* get the value of the property and return false if it was not found */
    str = fh_cfg_get_string(config, property);
    if (str == NULL) {
        return FH_ERR_NOTFOUND;
    }

    /* convert the value to a uint32_t, logging an error and returning false if the conversion */
    /* fails on any character */
    val = (int)strtoul(str, &last, 0);
    if (str[0] == '\0' || last[0] != '\0') {
        return FH_ERROR;
    }

    /* success, set value and return true */
    *value = val;
    return FH_OK;
}
