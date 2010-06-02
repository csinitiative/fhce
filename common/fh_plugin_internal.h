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

#ifndef __FH_PLUGIN_INTERNAL_H__
#define __FH_PLUGIN_INTERNAL_H__

// System includes
#include <stdbool.h>

// FH Includes
#include "fh_plugin.h"


/*! \brief Dump the array of hooks
 */
void fh_plugin_dump_plugins();

/*! \brief Check to see if a particular hook has been registered
 *
 *  \param hook hook being checked
 *  \return true if the hook is registered, false if not
 */
bool fh_plugin_is_hook_registered(int hook);

#endif  /* __FH_PLUGIN_INTERNAL_H__ */
