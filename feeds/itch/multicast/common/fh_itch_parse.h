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

#ifndef __FH_ITCH_PARSE_H__
#define __FH_ITCH_PARSE_H__

/* FH shared component headers */
#include "fh_shr_lh.h"

/** 
 *  @brief Entry point for parsing of an ITCH packet
 *
 *  @param packet the array of bytes that contains the incoming packet
 *  @param length the length (in bytes) of the incoming packet
 *  @param conn the connection structure on which the packet came in
 *  @return response code indicating success or failure
 */
FH_STATUS fh_itch_parse_pkt(uint8_t *packet, int length, fh_shr_lh_conn_t *conn);

/**
 *  @brief Function to initialize the message parser
 *
 *  @param process data for the currently executing process
 *  @return status code indicating success or failure
 */
FH_STATUS fh_itch_parse_init(fh_shr_lh_proc_t *process);

#endif /* __FH_ITCH_PARSE_H__ */
