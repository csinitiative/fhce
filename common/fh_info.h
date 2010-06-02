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

#ifndef __FH_INFO_H__
#define __FH_INFO_H__

/** 
 *  @addtogroup Common
 *  @{
 */

/* system headers */
#include <stdint.h>

/* convenience typedefs for structures in this file */
typedef struct fh_info_stats    fh_info_stats_t;
typedef struct fh_info_build    fh_info_build_t;
typedef struct fh_info_proc     fh_info_proc_t;

/**
 *  @brief Contains build information (user, arch, kernel, subversion info, etc.)
 */
struct fh_info_build {
    char    *name;              /**< Name of the application to which this structure belongs */
    int      version;           /**< Version of the application to which this structure belongs */
    char    *build_user;        /**< Local username of the user who created this build */
    char    *build_host;        /**< Hostname of the host on which this build was done */
    char    *build_arch;        /**< Architecture of this build (x86, SPARC, etc.) */
    char    *build_kernel;      /**< Kernel version on which this build was created */
    char    *build_date;        /**< Date when this build was done */
    char    *build_svn_url;     /**< Subversion URL (if applicable) of this build's source */
    char    *build_svn_rev;     /**< Subversion revision (if applicable) of this build's source */
};

/**
 *  @brief Stores process information such as PID, uptime, etc.
 */
struct fh_info_proc {
    uint32_t    pid;            /**< Process ID (if applicable) */
    uint32_t    cpu;            /**< CPU that the process, thread, etc. is configured to run on */
    uint32_t    tid;            /**< Thread ID (if applicable) */
    uint32_t    start_time;     /**< Uptime, in seconds, of the process, thread, etc. */
};

/**
 *  @brief Struct that contains counters for commonly tracked statistics
 */
struct fh_info_stats {
    uint64_t    packets;            /**< total count of packets */
    uint64_t    messages;           /**< total count of messages */
    uint64_t    bytes;              /**< total count of bytes */
    uint64_t    packet_errors;      /**< count of packets for which there was a packet error */
    uint64_t    message_errors;     /**< count of messages for which a parsing error occured */
    uint64_t    duplicate_packets;  /**< count of duplicate packets */
    uint64_t    gaps;               /**< count of gaps detected */
    uint64_t    lost_messages;      /**< count of messages in gaps that could not be filled */
    uint64_t    recovered_messages; /**< count of messages for which gaps were recovered */
};

/** 
 *  @brief Log a version message containing feed handler version and build information
 *
 *  @param info build, version, etc. information
 */
void fh_info_version_msg(const fh_info_build_t *info);

/*@}*/

#endif  /* __FH_INFO_H__ */
