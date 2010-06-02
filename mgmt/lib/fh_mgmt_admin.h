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

#ifndef __FH_MGMT_ADMIN_H__
#define __FH_MGMT_ADMIN_H__

#include <stdint.h>
#include "fh_errors.h"

/*
 * Admin command types
 */
#define FH_ADM_CMD_REG_REQ          (1)
#define FH_ADM_CMD_REG_RESP         (2)
#define FH_ADM_CMD_GETVER_REQ       (3)
#define FH_ADM_CMD_GETVER_RESP      (4)
#define FH_ADM_CMD_SERV_REQ         (5)
#define FH_ADM_CMD_SERV_RESP        (6)
#define FH_ADM_CMD_STATS_REQ        (7)
#define FH_ADM_CMD_STATS_RESP       (8)
#define FH_ADM_CMD_STATUS_REQ       (9)
#define FH_ADM_CMD_STATUS_RESP      (10)
#define FH_ADM_CMD_ACTION_REQ       (11)
#define FH_ADM_CMD_SGT_REQ          (12)
#define FH_ADM_CMD_SGT_RESP         (13)

#define FH_ADM_CMD_MAX              (14)

#define FH_ADM_CMD_NAME(c)          (fh_adm_getname(c))

#define FH_ADM_ISVALID(t)           ((t) > 0 && (t) <= FH_ADM_CMD_MAX)

/*
 * Admin message includes
 */
#include "admin/fh_adm_reg_req.h"
#include "admin/fh_adm_reg_resp.h"
#include "admin/fh_adm_getver_req.h"
#include "admin/fh_adm_getver_resp.h"
#include "admin/fh_adm_serv_req.h"
#include "admin/fh_adm_serv_resp.h"
#include "admin/fh_adm_status_req.h"
#include "admin/fh_adm_status_resp.h"
#include "admin/fh_adm_stats_req.h"
#include "admin/fh_adm_stats_resp.h"
#include "admin/fh_adm_action_req.h"
#include "admin/fh_adm_sgt_req.h"
#include "admin/fh_adm_sgt_resp.h"

/*
 * Admin command definition
 */
typedef struct {
    uint32_t  cmd_magic;           /* Magic number for version and sync     */
    uint32_t  cmd_type;            /* Command                               */
    uint32_t  cmd_tid;             /* Transaction ID                        */
    uint32_t  cmd_len;             /* Total length of the packet w/header   */
} fh_adm_cmd_t;

#define FH_ADM_MAGIC   (0x1a2b3c4d)

/*
 * Packing operation
 */
typedef FH_STATUS (fh_adm_pack_t)
    (
    void  *msg,    /* Admin message content to write                        */
    char  *data,   /* Pointer to the start of the payload                   */
    int   *length  /* Payload length returned after packing                 */
    );

/*
 * Unpacking operation
 */
typedef FH_STATUS (fh_adm_unpack_t)
    (
    void  *msg,    /* Admin message content to fill                         */
    char  *data,   /* Pointer to the start of the payload                   */
    int    length  /* Payload length as read from the msg header            */
    );

/*
 * Admin Message Definitions
 */

typedef struct fh_adm_def {
    int              adm_type;     /* Admin Message Type                  */
    char            *adm_name;     /* Admin Message Name                  */
    fh_adm_pack_t   *adm_pack;     /* Pack function                       */
    fh_adm_unpack_t *adm_unpack;   /* Unpack function                     */
} fh_adm_def_t;

/*
 * FH Manager TCP server port
 */
#define FH_MGR_PORT   (40000)

FH_STATUS     fh_adm_init();
FH_STATUS     fh_adm_create(int type, uint32_t tid, void *msg, 
                            char **data, uint32_t *size);
FH_STATUS     fh_adm_parse(int type, char *inbuf, void *msg);
char         *fh_adm_getname(int type);
fh_adm_def_t *fh_adm_getdef(int type);
FH_STATUS     fh_adm_recv(int fd, char **msg);
FH_STATUS     fh_adm_send(int fd, uint32_t cmd_type, uint32_t tid,
                          void *msg, uint32_t size);
FH_STATUS     fh_adm_expect(int fd, uint32_t cmd_type, uint32_t cmd_tid,
                            void *msg, uint32_t cmd_len);
FH_STATUS     fh_adm_pack(int type, char *data, void *msg, int length);
FH_STATUS     fh_adm_unpack(int type, char *data, void *msg, int length);

#endif /* __FH_MGMT_ADMIN_H__ */
