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

#ifndef __FH_OPRA_MSG_H__
#define __FH_OPRA_MSG_H__

#include "fh_opra_msg_ext.h"

/*
 * OPRA RAW message processing API
 */

FH_STATUS fh_opra_msg_init();
FH_STATUS fh_opra_msg_ctrl_process(CatHMsg_v2 *msg);
FH_STATUS fh_opra_msg_oi_process(CatdMsg_v2 *msg);
FH_STATUS fh_opra_msg_uv_process(CatYMsg_v2 *msg);
FH_STATUS fh_opra_msg_ls_process(CataMsg_v2 *msg);
FH_STATUS fh_opra_msg_eod_process(CatfMsg_v2 *msg);
FH_STATUS fh_opra_msg_quote_process(CatkMsg_v2 *msg);

#endif /* __FH_OPRA_MSG_H__ */
