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

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

/* FH common headers */
#include "fh_errors.h"
#include "fh_config.h"
#include "fh_log.h"
#include "fh_plugin_internal.h"
#include "fh_tcp.h"

#include "fh_shr_cfg_lh.h"

/* Dir Edge specific */
#include "SCRATCH_session_mgmt_prot.h"


/* external references     */

extern  uint32_t  nxt_seqNum ;
extern  char cur_session[];


static login_request_msg  login_msg = {'L',
                                       {' ',' ',' ',' ',' ',' '},
                                      {' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
                                       {' ',' ',' ',' ',' ',' ',' ',' ',' ',' '},
                                       {'0','0','0','0','0','0','0','0','0','1'},
                                      0x0A};


void  convert64to10chars(uint64_t value, char *sess)
{
    int i;
    uint64_t temp1,temp2,temp3,temp4;
    uint64_t temp5,temp6,temp7;
    uint64_t temp;
    for ( i = 0; i < 10; i++) {
        sess[i] = ' ';
    }
    i--; // set it to last location
    if ( value < 10 ) {
        sess[i]   = value + '0';
    }
    else if ( value < 100 ) {
        /* 2 chars only  */
        sess[i-1] = value/10 + '0';
        sess[i]   = value%10 + '0';
    } else if ( value < 1000 ) {
        /* 3 chars only */
        temp = (value/100) * 100;
        temp1 = ((value - temp)/10) * 10;
        sess[i-2] = temp/100 + '0';
        sess[i-1] = temp1/10 + '0';
        sess[i]   = (value - (temp + temp1))%10 + '0';
    } else if ( value < 10000 ) {
        /* 4 chars only */
        temp = (value/1000) * 1000;
        temp1= (value - temp)/100 * 100;
        sess[i-3] = temp/1000 + '0';
        sess[i-2] = temp1/100 + '0';
        sess[i-1] = (value - (temp + temp1))/10 + '0';
        sess[i]   = (value - (temp + temp1))%10 + '0';

    } else if ( value < 100000 ) {
        /* 5 chars only */
        temp = (value/10000) * 10000;
        temp1= (value - temp)/1000 * 1000;
        temp2= (value - ( temp + temp1))/100 * 100;
        sess[i-4] = temp/10000 + '0';
        sess[i-3] = temp1/1000 + '0';
        sess[i-2] = temp2/100 + '0';
        sess[i-1] = (value - (temp + temp1 + temp2))/10 + '0';
        sess[i]   = (value - (temp + temp1 + temp2))%10 + '0';
    } else if ( value < 1000000 ) {
        /* 6 chars only */
        temp = (value/100000) * 100000;
        temp1= ((value - temp)/10000) * 10000;
        temp2= ((value - ( temp + temp1))/1000) * 1000;
        temp3= ((value - ( temp + temp1 + temp2))/100) * 100;
        sess[i-5] = temp/100000 + '0';
        sess[i-4] = temp1/10000 + '0';
        sess[i-3] = temp2/1000 + '0';
        sess[i-2] = temp3/100 + '0';
        sess[i-1] = (value - (temp + temp1 + temp2 + temp3))/10  + '0';
        sess[i]   = (value - (temp + temp1 + temp2 + temp3))%10  + '0';
    } else if ( value < 10000000 ) {
        /* 7 chars only */
        temp = (value/1000000) * 1000000;
        temp1= ((value - temp)/100000) * 100000;
        temp2= ((value - ( temp + temp1))/10000) * 10000;
        temp3= ((value - ( temp+temp1+temp2))/1000) * 1000;
        temp4= ((value - ( temp+temp1+temp2+temp3))/100) * 100;
        sess[i-6] = temp/1000000 + '0';
        sess[i-5] = temp1/100000 + '0';
        sess[i-4] = temp2/10000 + '0';
        sess[i-3] = temp3/1000  + '0';
        sess[i-2] = temp4/100   + '0';
        sess[i-1] = (value - (temp + temp1 + temp2 + temp3 + temp4))/10    + '0';
        sess[i]   = (value - (temp + temp1 + temp2 + temp3 + temp4))%10    + '0';
    } else if ( value < 100000000 ) {
        /* 8 chars only */
        temp = (value/10000000) * 10000000;
        temp1= ((value - temp)/1000000) * 1000000;
        temp2= ((value - ( temp + temp1))/100000) * 100000;
        temp3= ((value - ( temp+temp1+temp2))/10000) * 10000;
        temp4= ((value - ( temp+temp1+temp2+temp3))/1000) * 1000;
        temp5= ((value - ( temp+temp1+temp2+temp3+temp4))/100) * 100;
        sess[i-7] = temp/10000000 + '0';
        sess[i-6] = temp1/1000000 + '0';
        sess[i-5] = temp2/100000 + '0';
        sess[i-4] = temp3/10000  + '0';
        sess[i-3] = temp4/1000   + '0';
        sess[i-2] = temp5/100    + '0';
        sess[i-1] = (value - (temp + temp1 + temp2 + temp3 + temp4 + temp5))/10     + '0';
        sess[i]   = (value - (temp + temp1 + temp2 + temp3 + temp4 + temp5))%10     + '0';
    } else if ( value < 1000000000 ){
        /* 9 chars only */
        temp = (value/100000000) * 100000000;
        temp1= ((value - temp)/10000000) * 10000000;
        temp2= ((value - ( temp + temp1))/1000000) * 1000000;
        temp3= ((value - ( temp+temp1+temp2))/100000) * 100000;
        temp4= ((value - ( temp+temp1+temp2+temp3))/10000) * 10000;
        temp5= ((value - ( temp+temp1+temp2+temp3+temp4))/1000) * 1000;
        temp6= ((value - ( temp+temp1+temp2+temp3+temp4+temp5))/100) * 100;
        sess[i-8] = temp/100000000 + '0';
        sess[i-7] = temp1/10000000 + '0';
        sess[i-6] = temp2/1000000 + '0';
        sess[i-5] = temp3/100000  + '0';
        sess[i-4] = temp4/10000   + '0';
        sess[i-3] = temp5/1000    + '0';
        sess[i-2] = temp6/100     + '0';
        sess[i-1] = (value - (temp + temp1 + temp2 + temp3 + temp4 + temp5 + temp6))/10      + '0';
        sess[i]   = (value - (temp + temp1 + temp2 + temp3 + temp4 + temp5 + temp6))%10      + '0';

    } else { // has to be less than 10 billion since we have 10 chars only
        /* all 10 chars  */
        temp = (value/1000000000) * 1000000000;
        temp1= ((value - temp)/100000000) * 100000000;
        temp2= ((value - ( temp + temp1))/10000000) * 10000000;
        temp3= ((value - ( temp+temp1+temp2))/1000000) * 1000000;
        temp4= ((value - ( temp+temp1+temp2+temp3))/100000) * 100000;
        temp5= ((value - ( temp+temp1+temp2+temp3+temp4))/10000) * 10000;
        temp6= ((value - ( temp+temp1+temp2+temp3+temp4+temp5))/1000) * 1000;
        temp7= ((value - ( temp+temp1+temp2+temp3+temp4+temp5+temp6))/100) * 100;
        sess[i-9] = temp/1000000000 + '0';
        sess[i-8] = temp1/100000000 + '0';
        sess[i-7] = temp2/10000000 + '0';
        sess[i-6] = temp3/1000000 + '0';
        sess[i-5] = temp4/100000  + '0';
        sess[i-4] = temp5/10000   + '0';
        sess[i-3] = temp6/1000    + '0';
        sess[i-2] = temp7/100     + '0';
        sess[i-1] = (value - (temp + temp1 + temp2 + temp3 + temp4+temp5+temp6+temp7))/10  + '0';
        sess[i]   = (value - (temp + temp1 + temp2 + temp3 + temp4+temp5+temp6+temp7))%10  + '0';
    }
}


void init_login_msg(login_request_msg *login_msg,fh_shr_cfg_lh_line_t * line,
                    char *cur_session, uint64_t* seq_no)
{
    memset(&login_msg->login_name, 0x20, sizeof(login_msg->login_name));
    memset(&login_msg->password,0x20, sizeof(login_msg->password));

    memcpy(&login_msg->login_name[0], &line[0].primary.login_name[0], 6);
    memcpy(&login_msg->password[0], &line[0].primary.login_passwd[0], 10);

    memcpy(cur_session, &login_msg->session_number, 10);
    convert64to10chars(*seq_no,&login_msg->initial_seq_number[0]);

}

uint64_t  convert10chartoInt(char *seqNum)
{
    uint64_t temp = 0;
    int i;
    int y = 0;
    for ( i = 9; i >= 0; i--) {
        if(seqNum[i] == ' ') break;
        if ( y == 0 )
            y =1;
        else
            y = y *10;
        temp += ((seqNum[i]) - 0x30) * y;
    }
    return temp;
}

inline FH_STATUS login_tcp_server(uint32_t socket, fh_shr_cfg_lh_line_t *  line,
                           uint64_t *seq_no)
{
    ssize_t  count;
    char  rx_one[100];
    login_accept_msg login_accept;


    /*  Clean up the message fields  */

    init_login_msg(&login_msg, line,  cur_session, seq_no);
    if ((count = fh_tcp_write(socket, &login_msg, sizeof(login_request_msg))) != sizeof(login_request_msg)) {
        FH_LOG(LH,ERR,("Login Message has not gone out correctly"));
        return FH_ERROR;
    }

    /* now wait for response and see if we have logged in */
    if((count = fh_tcp_read(socket,&rx_one[0], 1)) != 1) {
        FH_LOG(LH,ERR, (" Error receiving a Login Response"));
        return FH_ERROR;
    } else if ( count == 1) {
        /* we have a response arriving, lets see what it is */
        if ( rx_one[0] == 'A') {
            /* we have a login accept message arriving */
            if((count = fh_tcp_read(socket, &login_accept.session,
                                       sizeof(login_accept_msg)-1)) != (sizeof(login_accept_msg) -1)) {

                FH_LOG(LH,ERR, (" Error receiving a Login Response"));
                return FH_ERROR;
            } else if ( count == 0) {
                FH_LOG(LH, ERR, (" Login error, other end performed a shutdown"));

                return FH_ERROR;
            } else {
                /* we have received a correct response, lets make sure we have  */
                /* logged in successfully and then save the necessary info      */

                memcpy(cur_session, login_accept.session, sizeof(login_accept.session));
                *seq_no = convert10chartoInt(&login_accept.seq_number[0]);
            }

        } else if ( rx_one[0] == 'J' ) {
            char temp[2];
            if((count = fh_tcp_read(socket, temp,2)) != 2) {

                FH_LOG(LH,ERR, (" Error receiving a Login Response"));
                return FH_ERROR;
            } else if ( count == 0) {
                FH_LOG(LH, ERR, (" Login error, other end performed a shutdown"));
                return FH_ERROR;
            } else {
                 /* we have received a correct response, lets make sure we have  */
                 /* logged in successfully and then save the necessary info      */
                 if( (temp[0] == 'A' || temp[0] == 'S') && (temp[1] == 0x0A)) {
                     FH_LOG(LH,ERR, (" Login request rejected reason: %s",
                                     temp[0] == 'A'?"Not Authorized":
                                     "Invalid Session"));
                 } else {
                     FH_LOG(LH, ERR, (" Complete Login Reject message was not received"));
                     return FH_ERROR;
                 }
            }

        } else {
            FH_LOG(LH,ERR, (" Unkown response received to Login Request, msg type %c",
                       rx_one));
            return FH_ERROR;
        }
    } else { // count == 0 so the connection is broken
        return FH_ERROR;
    }

    return FH_OK;

}
