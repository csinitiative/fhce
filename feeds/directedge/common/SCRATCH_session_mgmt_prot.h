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

#ifndef SCRATCH_SESSION_MGMT_PROT_H
#define SCRATCH_SESSION_MGMT_PROT_H

#define SOUPTCP2_0
/********************************************************************/
//  Server to client Messages as defined in the SCRATCH spec
/********************************************************************/

// Login Accepted Message

typedef struct{
    char  msg_type;       // always 0x41 ('A')
    char  session[10];    // the session the client has joined
#ifdef SOUPTCP2_0
    char  seq_number[10]; // 10 char ascii numbers with spaces
#else
    char  seq_number[20]; // For SOUPTCP 3.0 it is 20 character ascii numeric
#endif
    char  term_char;
}login_accept_msg;


typedef struct{
    char msg_type;        // always = 0x4A('J') a login rejected message
    char reason;          // reason for rejection ('A'): Not Authiorized,
                          // ('S'): Invalid Session
    char msg_term;        // always 0x0A(LF)
}login_reject_msg;

// Server heartbeat message format

typedef struct{
    char msg_type;        // always 0x48 ('H')
    char term_char;       // always 0x0A (LF)
}server_hb_msg;

typedef struct{
    char msg_type;        // Always 0x48 'H' char
    char term_char;       // Always 0x0A  (LF) character
}end_of_session_msg;

/**************************************************************************/
//  Client to Server messages
/**************************************************************************/

// Login Request message format

typedef struct{
    char msg_type ;       // Always 0x4C ('L') character
    char login_name[6];   // Assigned by DirectEdge staff
    char password[10];    // 10 character max, left justified, right SP padded
    char session_number[10]; // session number the client wants to join
#ifdef SOUPTCP2_0
    char initial_seq_number[10]; // 10 character numeric ascii field
#else
    char initial_seq_number[20]; // 20 character numeric ascii for SOUPTCP3.0
#endif
    char msg_term;        // always 0x0A (LF)
}login_request_msg;

// Logout request message format

typedef struct{
    char msg_type;        // always = 0x4F('O')
    char term_char;       // always 0x0A a LF character
}logout_request_msg;

// Client Heartbeat messages

typedef struct{
    char msg_type;        // always = 0x52 ('R')
    char term_char;       // always 0x0A (LF)
}client_hb_msg;


#endif
