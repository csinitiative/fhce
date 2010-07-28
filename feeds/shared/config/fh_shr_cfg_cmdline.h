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

#ifndef __FH_SHR_CFG_CMDLINE_H__
#define __FH_SHR_CFG_CMDLINE_H__

// FH shared headers
#include "fh_shr_config.h"

/*! \brief Parse general feed handler command line arguments
 *
 *  \param argc argument count (generally passed from main function)
 *  \param argv argument array (generally passed from main function)
 *  \param name the name of the feed handler we parsing command line options for
 *  \param options structure that will hold the options found on the command line
 */
void fh_shr_cfg_cmd_parse(int argc, char **argv, const char *name, fh_shr_cfg_options_t *options);

#endif  /* __FH_SHR_CFG_CMDLINE_H__ */
