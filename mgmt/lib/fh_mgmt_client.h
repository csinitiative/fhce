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

#ifndef __FH_MGMT_CLIENT_H__
#define __FH_MGMT_CLIENT_H__

#include <stdint.h>
#include "fh_errors.h"

/*
 * Client types
 */
#define FH_MGMT_CL_CLI          (0x00000001)    /* Command-line interface   */
#define FH_MGMT_CL_SERVICE      (0x00000002)    /* Feed-Handler service     */

/*
 * Service control Asynchronous events from CLI to FH Manager
 */
#define FH_MGMT_CL_CTRL_MASK      (0x80000000)

#define FH_MGMT_CL_CTRL_ENABLE    (FH_MGMT_CL_CTRL_MASK|0x00000001)
#define FH_MGMT_CL_CTRL_DISABLE   (FH_MGMT_CL_CTRL_MASK|0x00000002)
#define FH_MGMT_CL_CTRL_START     (FH_MGMT_CL_CTRL_MASK|0x00000004)
#define FH_MGMT_CL_CTRL_STOP      (FH_MGMT_CL_CTRL_MASK|0x00000008)
#define FH_MGMT_CL_CTRL_RESTART   (FH_MGMT_CL_CTRL_MASK|0x00000010)
#define FH_MGMT_CL_CTRL_CLRSTATS  (FH_MGMT_CL_CTRL_MASK|0x00000020)


/*
 * Max number of services supported
 */
#define FH_MGMT_MAX_SERVICES    (30)

/*
 * Max lines per service (OPRA: (A+B) * 6 = 12)
 */
#define FH_MGMT_MAX_LINES       (12)

/*
 * FH Manager request handler for all client services
 */
typedef FH_STATUS (fh_mgmt_cl_cb_t)(char *cmd_buf, int cmd_len);

/*
 * Client service context for the FH Manager connection
 */
typedef struct {
    int              mcl_fd;
    uint32_t         mcl_conn_id;
    fh_mgmt_cl_cb_t *mcl_process;
} fh_mgmt_cl_t;

/*
 * Management client API
 */

FH_STATUS fh_mgmt_cl_init(fh_mgmt_cl_t *mcl, fh_mgmt_cl_cb_t *process,
                          uint32_t client_type, uint32_t serv_addr,
                          uint16_t serv_port, char *service, int quiet);
FH_STATUS fh_mgmt_cl_process(fh_mgmt_cl_t *mcl, uint64_t usec);
FH_STATUS fh_mgmt_cl_reqresp(fh_mgmt_cl_t *mcl,
                             int req_cmd,  void *req_data,  int req_len,
                             int resp_cmd, void *resp_data, int resp_len);
FH_STATUS fh_mgmt_cl_post(fh_mgmt_cl_t *mcl,
                          int req_cmd,  void *req_data,  int req_len);


#endif /* __FH_MGMT_CLIENT_H__ */
