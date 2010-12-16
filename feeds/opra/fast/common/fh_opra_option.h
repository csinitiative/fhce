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

#ifndef __FH_OPRA_OPTION_H__
#define __FH_OPRA_OPTION_H__

#include "fh_errors.h"
#include "fh_opra_option_ext.h"

/*
 * OPRA option table API
 */
FH_STATUS fh_opra_opt_init();
FH_STATUS fh_opra_opt_lookup(fh_opra_opt_key_t *k, fh_opra_opt_t **optp);
FH_STATUS fh_opra_opt_add(fh_opra_opt_key_t *k, fh_opra_opt_t **optp);
void      fh_opra_opt_memdump();

#endif /* __FH_OPRA_OPTION_H__ */
