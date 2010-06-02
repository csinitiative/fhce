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

// System includes
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/param.h>

// FH Includes
#include "fh_log.h"
#include "fh_plugin.h"
#include "fh_plugin_internal.h"


// when the plugins are being loaded, the logging sub-system is not yet initialized
#define FH_PLUGIN_LOG(l, args) \
    do { printf("FH_PLUGIN: %-5s%s", #l":", " "); printf args; printf("\n"); } while (0)
 

fh_plugin_hook_t *hook_table     = NULL;    // table of hook functions
int               allow_override = 0;       // allow existing registrations to be overriden?


/*! \brief Register a function for a plugin hook
 *
 *  \param hook the hook being registered
 *  \param hook_func the function to register
 *  \return status code indicating success or failure
 */
FH_STATUS fh_plugin_register(int hook, fh_plugin_hook_t hook_func)
{
    // if the hook being registered is out of bounds
    if (hook < 0 || hook > FH_PLUGIN_MAX) {
        FH_PLUGIN_LOG(ERR, ("plugin attempted to register an invalid hook %d", hook));
        return FH_ERROR;
    }
    
    // if override is not allowed, and there is already a plugin registered...
    if (fh_plugin_is_hook_registered(hook)) {
        if (!allow_override) {
            FH_PLUGIN_LOG(ERR, ("plugin attempted to re-register a hook (%d)", hook));
            exit(1);
        }
        else {
            FH_PLUGIN_LOG(WARN, ("plugin hook overriden (%d)", hook));
        }
    }
    
    
    // if the hook table has not yet been allocated...
    if (hook_table == NULL) {
        hook_table = (fh_plugin_hook_t *)malloc((FH_PLUGIN_MAX + 1) * sizeof(fh_plugin_hook_t));
        FH_ASSERT(hook_table);
        memset(hook_table, 0, (FH_PLUGIN_MAX + 1) * sizeof(fh_plugin_hook_t));
    }
    
    // register the hook function and return FH_OK
    hook_table[hook] = hook_func;
    return FH_OK;
}

/*! \brief Dump the array of hooks
 */
void fh_plugin_dump_plugins()
{
    int i;
    
    if(hook_table == NULL) return;
    
    for(i = 0; i <= FH_PLUGIN_MAX; i++) {
        FH_LOG_PGEN(DIAG, ("%d => %p", i, (void *)hook_table[i]));
    }
}

/*! \brief Load any plugins that are in the specified directory
 *
 *  \param plugin_dir_name name of the plugin directory being loaded
 */
void fh_plugin_load(const char *plugin_dir_name)
{
    void          *plugin_handle;
    DIR           *plugin_dir = NULL;
    struct dirent *entry = NULL;
    int           len;
    char          name[MAXPATHLEN];

    // open the plugin directory
    plugin_dir = opendir(plugin_dir_name);
    if(plugin_dir == NULL) {
        FH_PLUGIN_LOG(WARN, ("Invalid plugin directory or out of memory"));
        return;
    }
    
    while((entry = readdir(plugin_dir))) {
        len = strlen(entry->d_name);
        if(len > 3 && entry->d_name[len - 3] == '.' && entry->d_name[len - 2] == 's' &&
                      entry->d_name[len - 1] == 'o') {
            snprintf(name, sizeof(name), "%s/%s", plugin_dir_name, entry->d_name);
            plugin_handle = dlopen(name, RTLD_NOW | RTLD_LOCAL);
            if(plugin_handle == NULL) {
                FH_PLUGIN_LOG(WARN, ("Failed to load plugin: %s | Error: %s", 
                                     entry->d_name, dlerror()));
            }
            else {
                FH_PLUGIN_LOG(INFO, ("Loaded plugin: %s", entry->d_name));
            }
        }
    }
}

/*! \brief Check to see if a particular hook has been registered
 *
 *  \param hook hook being checked
 *  \return true if the hook is registered, false if not
 */
bool fh_plugin_is_hook_registered(int hook)
{
    return (hook_table != NULL      && 
            hook >= 0               &&
            hook <= FH_PLUGIN_MAX   &&
            hook_table[hook] != NULL);
}

/*! \brief Returns the hook function for the given hook
 *
 *  \param hook the hook function to fetch
 *  \return hook function
 */
fh_plugin_hook_t fh_plugin_get_hook(int hook)
{
    if(hook_table == NULL || hook < 0 || hook > FH_PLUGIN_MAX) return NULL;
    return hook_table[hook];
}

/*! \brief Set the flag that determines whether two plugins can register for the same hook
 *
 *  \param flag value to set
 */
void fh_plugin_allow_override(int flag)
{
    if (flag) {
        FH_PLUGIN_LOG(INFO, ("plugin hook override turned ON"));
    }
    allow_override = flag;
}
