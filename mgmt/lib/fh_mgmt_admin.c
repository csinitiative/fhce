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

/*
 * System includes
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

/*
 * FH Shared includes
 */
#include "fh_log.h"
#include "fh_tcp.h"
#include "fh_util.h"

/*
 * Admin library includes
 */
#include "fh_mgmt_admin.h"

/*----------------------------------------------------------------------*/
/* Admin Message Definition Table                                       */
/*----------------------------------------------------------------------*/

/* Specification components to define an admin message                  */
#define ADM_DEF_TYPE    (1)
#define ADM_DEF_NAME    (2)
#define ADM_DEF_PACK    (3)
#define ADM_DEF_UNPACK  (4)

/* Specification for one admin message type                             */
#define ADM_DEF(type, pack, unpack)             \
    (void *) ADM_DEF_TYPE,   (void *) type,     \
    (void *) ADM_DEF_NAME,   (void *) #type,    \
    (void *) ADM_DEF_PACK,   (void *) pack,     \
    (void *) ADM_DEF_UNPACK, (void *) unpack

/* Macro equivalent to va_start and va_next stdarg API                  */
#define ADM_DEF_START()     int _defs_idx = 0
#define ADM_DEF_NEXT(type)  (type) defs[_defs_idx++]

/* Specification table                                                  */
static void *defs[] = {
    ADM_DEF(FH_ADM_CMD_REG_REQ,     adm_reg_req_pack,     adm_reg_req_unpack),
    ADM_DEF(FH_ADM_CMD_REG_RESP,    adm_reg_resp_pack,    adm_reg_resp_unpack),
    ADM_DEF(FH_ADM_CMD_GETVER_REQ,  adm_getver_req_pack,  adm_getver_req_unpack),
    ADM_DEF(FH_ADM_CMD_GETVER_RESP, adm_getver_resp_pack, adm_getver_resp_unpack),
    ADM_DEF(FH_ADM_CMD_SERV_REQ,    adm_serv_req_pack,    adm_serv_req_unpack),
    ADM_DEF(FH_ADM_CMD_SERV_RESP,   adm_serv_resp_pack,   adm_serv_resp_unpack),
    ADM_DEF(FH_ADM_CMD_STATUS_REQ,  adm_status_req_pack,  adm_status_req_unpack),
    ADM_DEF(FH_ADM_CMD_STATUS_RESP, adm_status_resp_pack, adm_status_resp_unpack),
    ADM_DEF(FH_ADM_CMD_STATS_REQ,   adm_stats_req_pack,   adm_stats_req_unpack),
    ADM_DEF(FH_ADM_CMD_STATS_RESP,  adm_stats_resp_pack,  adm_stats_resp_unpack),
    ADM_DEF(FH_ADM_CMD_ACTION_REQ,  adm_action_req_pack,  adm_action_req_unpack),
    ADM_DEF(FH_ADM_CMD_SGT_REQ,     adm_sgt_req_pack,     adm_sgt_req_unpack),
    ADM_DEF(FH_ADM_CMD_SGT_RESP,    adm_sgt_resp_pack,    adm_sgt_resp_unpack),
    NULL
};

static fh_adm_def_t adm_defs[FH_ADM_CMD_MAX];

/*
 * fh_adm_init
 *
 * Initialize the admin library.
 */
FH_STATUS fh_adm_init()
{
    fh_adm_def_t *am = NULL;
    size_t def_cmd;

    ADM_DEF_START();

    memset(&adm_defs[0], 0, sizeof(adm_defs));

    while ((def_cmd = ADM_DEF_NEXT(size_t)) != 0) {
        switch (def_cmd) {
        case ADM_DEF_TYPE:
        {
            size_t type = ADM_DEF_NEXT(size_t);

            if (!FH_ADM_ISVALID(type)) {
                /* Invalid log message */
                return FH_ERROR;
            }

            am = &adm_defs[type];

            am->adm_type = (int)type;

            break;
        }

        case ADM_DEF_NAME:
            am->adm_name = ADM_DEF_NEXT(char *);
            break;

        case ADM_DEF_PACK:
            am->adm_pack = ADM_DEF_NEXT(fh_adm_pack_t *);
            break;

        case ADM_DEF_UNPACK:
            am->adm_unpack = ADM_DEF_NEXT(fh_adm_unpack_t *);
            break;

        default:
            /* Error message */
            return FH_ERROR;
        }
    }

    return FH_OK;
}

/*
 * fh_adm_create
 *
 * Create an admin message.
 */
FH_STATUS fh_adm_create(int type, uint32_t tid, void *msg, 
                        char **data, uint32_t *size)
{
    fh_adm_def_t *adm;
    FH_STATUS     rc;
    int           length;
    int           plen;
    fh_adm_cmd_t *cmd;
    char         *buf;

    FH_ASSERT(msg);

    /*
     * Validate the message type
     */

    if (!FH_ADM_ISVALID(type)) {
        FH_LOG(MGMT, ERR, ("ADM_CREATE: invalid message type: %d", type));
        return FH_ERROR;
    }

    adm = &adm_defs[type];

    if (adm->adm_type != type || adm->adm_pack == NULL) {
        FH_LOG(MGMT, ERR, ("ADM_CREATE: packing unsupported for message type: %d", type));
        return FH_ERROR;
    }

    /*
     * Pack the payload
     */

    length = *size;

    rc = adm->adm_pack(msg, 0, &length);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("ADM_CREATE: packing failed for admin message: %s", adm->adm_name));
        return rc;
    }

    /*
     * Build the output buffer now that we know the size
     */

    plen = length + sizeof(fh_adm_cmd_t);
    buf = malloc(plen);
    if(!buf) {
        FH_LOG(MGMT, ERR, ("ADM_CREATE: failed to allocate output buffer: %s : %d", 
                         adm->adm_name, plen));
        return FH_ERROR;
    }

    rc = adm->adm_pack(msg, &buf[sizeof(fh_adm_cmd_t)], &length);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("ADM_CREATE: packing failed for admin message: %s", adm->adm_name));
        free(buf);
        return rc;
    }

    /*
     * Finish up the command packet
     */
    cmd = (fh_adm_cmd_t *) buf;

    cmd->cmd_magic = htonl(FH_ADM_MAGIC);
    cmd->cmd_type  = htonl(type);
    cmd->cmd_tid   = htonl(tid);
    cmd->cmd_len   = htonl(length);

    *data = buf;
    *size = plen;

    return FH_OK;
}

/*
 * fh_adm_unpack
 *
 * Unpack the data only.
 */
FH_STATUS fh_adm_unpack(int type, char *data, void *msg, int length)
{
    FH_STATUS     rc;
    fh_adm_def_t *adm;

    FH_ASSERT(msg);

    if (data == NULL) {
        return FH_ERROR;
    }

    /*
     * Validate the message type
     */
    if (!FH_ADM_ISVALID(type)) {
        FH_LOG(MGMT, ERR, ("ADM_UNPACK: invalid message type: %d", type));
        return FH_ERROR;
    }

    adm = &adm_defs[type];

    if (adm->adm_type != type || adm->adm_unpack == NULL) {
        FH_LOG(MGMT, ERR, ("ADM_UNPACK: unpacking unsupported for message type: %d", type));
        return FH_ERROR;
    }

    /*
     * Compute the offset in the data buffer for the topic
     */
    rc = adm->adm_unpack(msg, data, length);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("ADM_UNPACK: unpacking failed for admin message: %s", adm->adm_name));
        return rc;
    }

    return FH_OK;
}

/*
 * fh_adm_pack
 *
 * Pack the data only.
 */
FH_STATUS fh_adm_pack(int type, char *data, void *msg, int length)
{
    FH_STATUS     rc;
    fh_adm_def_t *adm;

    FH_ASSERT(msg);

    if (data == NULL) {
        return FH_ERROR;
    }

    /*
     * Validate the message type
     */
    if (!FH_ADM_ISVALID(type)) {
        FH_LOG(MGMT, ERR, ("ADM_PACK: invalid message type: %d", type));
        return FH_ERROR;
    }

    adm = &adm_defs[type];

    if (adm->adm_type != type || adm->adm_pack == NULL) {
        FH_LOG(MGMT, ERR, ("ADM_PACK: packing unsupported for message type: %d", type));
        return FH_ERROR;
    }

    rc = adm->adm_pack(msg, data, &length);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("ADM_PACK: packing failed for admin message: %s", adm->adm_name));
        return rc;
    }

    return FH_OK;
}

/*
 * fh_adm_parse
 *
 * Parse an admin message based on type.
 */
FH_STATUS fh_adm_parse(int type, char *data, void *msg)
{
    FH_STATUS     rc;
    fh_adm_def_t *adm;
    int           length;
    fh_adm_cmd_t *cmd;

    FH_ASSERT(msg);

    if (data == NULL) {
        return FH_ERROR;
    }

    /*
     * Validate the message type
     */
    if (!FH_ADM_ISVALID(type)) {
        FH_LOG(MGMT, ERR, ("ADM_PARSE: invalid message type: %d", type));
        return FH_ERROR;
    }

    adm = &adm_defs[type];

    if (adm->adm_type != type || adm->adm_unpack == NULL) {
        FH_LOG(MGMT, ERR, ("ADM_PARSE: unpacking unsupported for message type: %d", type));
        return FH_ERROR;
    }

    cmd = (fh_adm_cmd_t *) data;

    length = cmd->cmd_len;

    /*
     * Compute the offset in the data buffer for the topic
     */
    data += sizeof(fh_adm_cmd_t);

    rc = adm->adm_unpack(msg, data, length);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("ADM_PARSE: unpacking failed for admin message: %s", adm->adm_name));
        return rc;
    }

    return FH_OK;
}


/*
 * fh_adm_getname
 *
 * Returns the name of an admin message based on type.
 */
char *fh_adm_getname(int type)
{
    if (FH_ADM_ISVALID(type)) {
        fh_adm_def_t *adm = &adm_defs[type];

        if (adm->adm_type == type) {
            FH_ASSERT(adm->adm_name);

            return adm->adm_name;
        }
    }

    return "- na -";
}

/*
 * fh_adm_getdef
 *
 * Get the message description from the table
 */
fh_adm_def_t *fh_adm_getdef(int type)
{
    if (FH_ADM_ISVALID(type)) {
        fh_adm_def_t *adm = &adm_defs[type];

        if (adm->adm_type == type) {
            return adm;
        }
    }

    return NULL;
}

/*
 * fh_adm_recv
 *
 * Receives an admin message from a given admin socket. This function
 * returns the message that is dynamically allocated (using malloc). It is the 
 * responsibility of the calling function to free the memory (using free).
 */
FH_STATUS fh_adm_recv(int fd, char **msg)
{
    FH_STATUS      rc = FH_OK;
    int            msize;
    int            numb;
    fh_adm_cmd_t   cmdh;
    fh_adm_cmd_t  *cmd;
    char          *buf;

    *msg = NULL;

    /*
     * Peek the command header data first
     */
    numb = fh_tcp_peek(fd, (char *)&cmdh, sizeof(fh_adm_cmd_t));
    if(numb < 0) {
        FH_LOG(MGMT, ERR, ("fh_adm_recv: failed to peek message header on fd: %d",fd));
        return FH_ERROR;
    }
    else if (numb == 0 || numb != sizeof(fh_adm_cmd_t)) {
        FH_LOG(MGMT, WARN, ("fh_adm_recv: failed to peek admin header - numb: %d", numb));
        return FH_ERROR;
    }

    if (rc != FH_OK) {
        return rc;
    }

    if (ntohl(cmdh.cmd_magic) != FH_ADM_MAGIC) {
        FH_LOG(MGMT, ERR, ("fh_adm_recv: Invalid Magic 0x%08x vs. 0x%08x (fd: %d) ---",
                           ntohl(cmdh.cmd_magic), FH_ADM_MAGIC, fd));
        return FH_ERROR;
    }

    msize = (int) ntohl(cmdh.cmd_len) + sizeof(fh_adm_cmd_t);
    if (msize == 0) {
        FH_LOG_PGEN(DIAG, ("--- MA Read (fd: %d) ---", fd));
        FH_LOG_PGEN(DIAG, (" > Magic Number  : 0x%08x", ntohl(cmdh.cmd_magic)));
        FH_LOG_PGEN(DIAG, (" > Type          : 0x%08x", ntohl(cmdh.cmd_type)));
        FH_LOG_PGEN(DIAG, (" > TID           : %d", ntohl(cmdh.cmd_tid)));
        FH_LOG_PGEN(DIAG, (" > Length        : %d", ntohl(cmdh.cmd_len)));
        return FH_ERROR;
    }

    /*
     * Allocate a buffer for the full command based on size
     */
    buf = (char*)malloc(msize);
    if (!buf) {
        FH_LOG(MGMT, ERR, ("fh_adm_recv: malloc failed for msg - size: %d", msize));
        return FH_ERROR;
    }

    FH_LOG(MGMT, INFO, ("fh_adm_recv: reading fd: %d msize: %d", fd, msize));

    /*
     * Read the data from the socket
     */
    numb = fh_tcp_read(fd, buf, msize);
    if (numb != (int32_t)msize) {
        FH_LOG(MGMT, ERR, ("fh_adm_recv: read failed nb: %d ms: %d", numb, msize));
        free(buf);
        return FH_ERROR;
    }

    cmd = (fh_adm_cmd_t *)buf;

    cmd->cmd_type = ntohl(cmd->cmd_type);
    cmd->cmd_tid  = ntohl(cmd->cmd_tid);
    cmd->cmd_len  = ntohl(cmd->cmd_len);

    FH_LOG(MGMT, VSTATE, ("--- Admin --- RCV: cmd: %08X tid: %08X size: %d : %s",
                          cmd->cmd_type, cmd->cmd_tid, cmd->cmd_len,
                          FH_ADM_CMD_NAME(cmd->cmd_type)));

    *msg = buf;

    return FH_OK;
}

/*
 * fh_adm_send
 *
 * Send a message to a peer.
 */
FH_STATUS fh_adm_send(int fd, uint32_t cmd_type, uint32_t cmd_tid,
                      void *msg, uint32_t size)
{
    FH_STATUS     rc = FH_OK;
    char         *data = NULL;
    int           numb;

    FH_ASSERT(msg && size > 0);

    FH_LOG(MGMT, DIAG, ("Sending admin message on fd: %d size: %d cmd: %08X",
                        fd, size, cmd_type));

    /*
     * Build the full message with the command header, and pack the data
     */
    rc = fh_adm_create(cmd_type, cmd_tid, msg, &data, &size);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to pack admin command: %d tid: %d", cmd_type,
                           cmd_tid));
        return rc;
    }

    /*
     * Send the command body
     */
    numb = fh_tcp_write(fd, data, size);
    if (numb != (int32_t)size) {
        FH_LOG(MGMT, ERR, ("fh_adm_send: write failed numb: %d", numb));
        rc = FH_ERROR;
    }

    /*
     * Free the dynamically allocated memory for the full message
     */
    free(data);

    return rc;
}

/*
 * fh_adm_expect
 *
 * Wait for a given command and verify that it corresponds to a given
 * transaction ID and the command is actually of the expected type.
 */
FH_STATUS fh_adm_expect(int fd, uint32_t cmd_type, uint32_t cmd_tid,
                        void *msg, uint32_t cmd_len)
{
    FH_STATUS      rc;
    fd_set         rfds;
    int            nfds;
    int            n = 0;
    struct timeval tv;
    char          *data = NULL;
    fh_adm_cmd_t  *cmd = NULL;

    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    nfds = fd + 1;

    /*
     * Wait for this message for 2 seconds
     */
    tv.tv_sec  = 2;
    tv.tv_usec = 0;

    n = select(nfds, &rfds, NULL, NULL, &tv);
    if (n < 0) {
        FH_LOG(MGMT, ERR, ("select failed: fd: %d cmd: %d", fd, cmd_type));
        return FH_ERROR;
    }

    if (n == 0) {
        FH_LOG(MGMT, WARN, ("select timed-out: fd: %d cmd: %d", fd, cmd_type));
        return FH_ERROR;
    }

    FH_ASSERT(FD_ISSET(fd, &rfds));

    /*
     * Receive the data from the socket
     */
    rc = fh_adm_recv(fd, &data);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Failed to recv message from peer: fd: %d cmd: %d", fd, cmd_type));
        return rc;
    }

    /*
     * Validate the command against the expected values
     */
    cmd = (fh_adm_cmd_t *) data;
    if (unlikely(cmd->cmd_type != cmd_type)) {
        FH_LOG(MGMT, ERR, ("Invalid command type: %d (expected: %d)",
                           cmd->cmd_type, cmd_type));
        free(data);
        return rc;
    }

    if (unlikely(cmd->cmd_len > cmd_len)) {
        FH_LOG(MGMT, ERR, ("Invalid command length: %d (expected <= %d)",
                           cmd->cmd_len, cmd_len));
        free(data);
        return rc;
    }

    if (unlikely(cmd->cmd_tid != cmd_tid)) {
        FH_LOG(MGMT, ERR, ("Invalid command TID: %d (expected: %d)",
                           cmd->cmd_tid, cmd_tid));
        free(data);
        return rc;
    }

    /*
     * Unpack the actual payload
     */
    rc = fh_adm_parse(cmd_type, data, msg);
    if (rc != FH_OK) {
        FH_LOG(MGMT, ERR, ("Command parsing failed: fd: %d cmd: %d", fd, cmd_type));
    }

    free(data);

    return rc;
}

