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

//
/*********************************************************************/
/* file: fh_init_sockets.c                                           */
/* Usage: socket support functions for arca multicast feed handler   */
/* Author: Ross Cooperman of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/*  Intellectual property of Collaborative Software Initiative       */
/*  Copyright Collaborative Software Initiative 2009                 */
/*********************************************************************/

// System headers
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>

// Common FH headers
#include "fh_errors.h"
#include "fh_log.h"
#include "fh_net.h"
#include "fh_udp.h"
#include "fh_mcast.h"

// Arca FH headers
#include "fh_arca_constants.h"
#include "fh_arca_headers.h"
#include "fh_feed_group.h"

/*! \brief Create a socket that is joined to a multicast group and ready to receive
 *
 *  \param mcast_address multicast group address to join
 *  \param mcast_port multicast port
 *  \param intfc_name name of the interface to join on
 *  \return socket on success, 0 on failure
 */
static int fh_arca_sock_build_mcast(const char *mcast_address, 
    const int mcast_port, const char *intfc_name)
{
    static const int     udp_flags  = FH_UDP_FL_MAX_BUFSZ | FH_UDP_FL_MCAST;
    int                  sock       = 0;
    uint32_t             ifaddr     = 0;
    uint32_t             ip_address = 0;

    // create the udp multicast socket
    if (fh_udp_open(INADDR_ANY, mcast_port, udp_flags, &sock) != FH_OK) {
        FH_LOG(LH, ERR, ("failed to create socket for port %d", mcast_port));
        return 0;
    }
    
    // determine the address associated with the specified interface
    ifaddr = fh_net_ifaddr(sock, (char*) intfc_name);
    if (ifaddr == 0) {
        FH_LOG(LH, ERR, ("unable to obtain interface address for %s", intfc_name));
        close(sock);
        return 0;
    }
    
    // join the mcast group
    ip_address = inet_addr(mcast_address);
    if (ip_address == 0xffffffff) {
        FH_LOG(LH, ERR, ("invalid multicast address: %s", mcast_address));
        close(sock);
        return 0;
    }
    
    if (fh_mcast_join(sock, ifaddr, ip_address) != FH_OK) {
        FH_LOG(LH, ERR,("failed to join mcast group %s:%d", mcast_address, mcast_port));
        close(sock);
        return 0;
    }
    
    return sock;
};

/*-------------------------------------------------------------------------*/
/* create a tcp request socket that is not yet connected; deferred until   */
/* we need to make a request                                               */
/*-------------------------------------------------------------------------*/
int build_request_socket(const char* tcp_address, const int tcp_port, 
    const char* intfc_name){

    int sock=0;
    sock = socket(PF_INET,SOCK_SEQPACKET,6);  //6 is tcp
    if (sock==0) {
        FH_LOG(MGMT,ERR,(" Failed to open request for %s : %d : %s",
            tcp_address, tcp_port, intfc_name));
        return 0;
    }
    return sock;
};
/*-------------------------------------------------------------------------*/
/* initialize all the multicast sockets associated with this process       */
/*-------------------------------------------------------------------------*/
int init_mcast_sockets(const struct arca_process_args *args)
{
    int socket_itt=0,primary_or_secondary=0,socket=0;
    struct socket_set *main_sockets=NULL;
    struct socket_set *retrans_sockets=NULL;
    struct feed_group *group = NULL;
    // char *process_name = args->process_name;
    
    main_sockets    = args->main_sockets;
    retrans_sockets = args->retrans_sockets;
    
    socket_itt=0;
    while (socket_itt < main_sockets->socket_count) {
        group = main_sockets->feeds[socket_itt];
        primary_or_secondary = main_sockets->primary_or_secondary[socket_itt];
        if (primary_or_secondary==0) {
            socket = fh_arca_sock_build_mcast(&(group->primary_mcast_ip_addrs[0]),
                group->primary_mcast_port,&(group->primary_mcast_intfc[0]));
            if (socket == 0) {
                return -1;
            }
            group->primary_mcast_socket = socket;
        } else {
            socket = fh_arca_sock_build_mcast(
                &(group->secondary_mcast_ip_addrs[0]),
                group->secondary_mcast_port,
                &(group->secondary_mcast_intfc[0]));
            if (socket==0) {
                 return -1;
            }
            group->secondary_mcast_socket = socket;
        }     
        main_sockets->sockets[socket_itt] = socket;
        socket_itt++;
    }
    //TEMP - WM - future is to have retrans groups built as needed
    socket_itt=0;
    while (socket_itt < retrans_sockets->socket_count) {
        group = retrans_sockets->feeds[socket_itt];
        primary_or_secondary = 
            retrans_sockets->primary_or_secondary[socket_itt];
        if (primary_or_secondary==0) {
            socket = fh_arca_sock_build_mcast(
                &(group->primary_retran_mcast_ip_addrs[0]),
                group->primary_retran_mcast_port,
                &(group->primary_retran_intfc[0]));
            if (socket==0) {
                return -1;
            }
            group->primary_retran_mcast_socket = socket;
            
        } else {
             socket = fh_arca_sock_build_mcast(
                &(group->secondary_retran_mcast_ip_addrs[0]),
                group->secondary_retran_mcast_port,
                &(group->secondary_retran_intfc[0]));
             if (socket==0) {
                 return -1;
             }
             group->secondary_retran_mcast_socket = socket;
        }
        socket_itt++;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* initialize all the request sockets associated with this process         */
/*-------------------------------------------------------------------------*/
int init_request_sockets(const struct arca_process_args* args)
{
    struct socket_set   *retrans_sockets=NULL;
    struct feed_group   *group;
    int                  socket_itt = 0, primary_or_secondary = 0, socket = 0;
    // char                *process_name=args->process_name;
    
    retrans_sockets = args->retrans_sockets;
    socket_itt      = 0;
    
    while (socket_itt<retrans_sockets->socket_count) {
        group =  retrans_sockets->feeds[socket_itt];
        primary_or_secondary = retrans_sockets->primary_or_secondary[socket_itt];
        if (primary_or_secondary==0) {
            socket = build_request_socket(
                &(group->primary_retran_tcp_ip_addrs[0]),
                group->primary_retran_tcp_port,
                &(group->primary_request_intfc[0]));
            if (socket==0) {
                return -1;
            }
        }
        group->primary_retran_tcp_socket = socket;
        socket_itt++;
    }
    return 0;
};
