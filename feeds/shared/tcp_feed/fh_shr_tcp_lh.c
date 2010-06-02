/*
 * This file is part of Collaborative Software Initiative Feed Handlers (CSI FH).
 *
 * CSI FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * CSI FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CSI FH.  If not, see <http://www.gnu.org/licenses/>.
 */

/* system headers */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

/* common FH headers */
#include "fh_errors.h"
#include "fh_config.h"
#include "fh_log.h"
#include "fh_plugin_internal.h"
#include "fh_tcp.h"
#include "fh_prof.h"
#include "fh_alerts.h"
#include "fh_shr_cfg_table.h"
#include "fh_shr_lookup.h"
#include "fh_shr_lkp_order.h"
#include "fh_shr_lkp_symbol.h"
#include "fh_shr_cfg_lh.h"
#include "fh_shr_tcp_lh.h"

#define HB_ALARM_TIMEOUT   10          /** Indicates to monitor the HB message for 10Sec **/
/* global data for the Line Handler  */

uint32_t  nxt_seqNum = 0;   /* Next sequence number to expect  */
                                   /* Current session we have         */
char cur_session[10] = {0x20,0x20,0x20,0x20,
                        0x20,0x20,0x20,0x20,
                        0x20,0x20};

/* START ONLY ONE LINE HANDLER THREAD AT A TIME -- this code is not intended to be thread safe */
static pthread_t                     lh_thread;     /* line handler thread */
static const fh_info_build_t        *lh_info;       /* version, build, etc. information */
static fh_shr_lh_proc_t              lh_process;    /* feed handler process_data */
static fh_shr_tcp_cb_t              *lh_callbacks;  /* callbacks for packet parsing, etc */
static int                           lh_init  = 0;  /* indicates that lh init. is complete */
static int                           finished = 0;  /* flag that tells the line handler to exit */

/* Line handler Thread Id  */
static uint32_t   lh_threadid =  0;

/* Some local counters     */
int de_connection_count   = 0;

/* Heart Beat count associated with absence of HB period from Server */
int monitor_hb_count      = 0;

/* cached hook function(s) */
static fh_plugin_hook_t              hook_msg_flush = NULL;

uint64_t  dir_edge_seq_num    = 1; /* Initial sequence number to run with */
uint64_t  hb_count            = 0; /* Just a count of heartbeats received */

/* profiling declarations for latency measurements */
FH_PROF_DECL(lh_recv_latency, 1000000, 20, 2);
FH_PROF_DECL(lh_proc_latency, 1000000, 20, 2);


/*
 * Initialize line handler tables
 */
void fh_shr_tcp_lh_tbl_init()
{
    /* initialize the symbol table */
    fh_shr_lkp_sym_init(&lh_process.config->symbol_table, &lh_process.symbol_table);

    /* initialize the order table */
    fh_shr_lkp_ord_init(&lh_process.config->order_table, &lh_process.order_table);
}

void fh_shr_tcp_lh_exit()
{
    finished = 1;
}

uint32_t  fh_shr_tcp_lh_get_tid()
{
    return lh_threadid;
}

/*
 *  Do what needs to be done for end of trade session.
 *  For sure we need to inform the parties down stream
 */
inline void process_end_of_session(int * socketp)
{
    char  rx[2];
    int count;
    if((count = recv(*socketp,rx,2,0)) == 2) {
        FH_LOG_PGEN(LH,("Session Closed Received"));
    } else {
        FH_LOG(LH, ERR, ("Session close detected, but failed to RX the message, errno = %d",errno));
    }
    // send out a alert as we assume it is the end of a session
    lh_callbacks->alarm(&cur_session[0],sizeof(cur_session),FH_ALERT_SESSION_TERMINATED);
    // Close the socket and wait for a new session and reset the session info
    close(*socketp);
    memset(&cur_session[0],0x20,sizeof(cur_session));
    FH_LOG_PGEN(LH,("last sequence number sent out = %lld",dir_edge_seq_num));
    FH_LOG_PGEN(LH,("resetting sequnce number for next session to 1"));
    dir_edge_seq_num = 1;

}

/*
 *  Send a HB message and process the state of the line on the return
 *  This function is called to also determine the health of the
 *  connection.
 */
static FH_STATUS  send_hb_msg(int * socketp, fh_shr_cfg_lh_line_t *line,fh_info_stats_t *stats,int * new_connection)
{
    static char hb[2] = {'R',0x0A};
    ssize_t scount = 0;
    FH_STATUS rc;

    if (stats) {}
    /* send the HB message here since we haven't received any traffic */
    /* for a minute                                                   */
    if((scount = send(*socketp,&hb, 2, MSG_DONTWAIT)) != 2){
        FH_LOG(LH,ERR, ("Unable to send HB message errno = %d",errno));
        if((errno == ECONNRESET) || (errno == EPIPE)){
            if ((rc = close(*socketp)) == -1 ){
                FH_LOG(LH,ERR, ("Unable to close socket after connection broken errno = %d",errno));
            }
            *socketp = 0;
            FH_LOG(LH, WARN, (" Connection Broken, reestablishing the connection"));
            while( (rc = lh_callbacks->connect(&lh_process,line, &dir_edge_seq_num)) != FH_OK){
                FH_LOG(LH,ERR, ("error trying to create a socket and logging into the server, try again"));
                sleep(3);
            }
            *new_connection = 1;
        }

    }
    return FH_OK;
}

/*
 * Get the debug message from the buffer and publish the message
 *
 */
static inline int get_and_send_debug_msg(int *socketp,char *rx_buf)
{
    int     len,i;
    int     to_get = 51;
    while(1){
        len = recv(*socketp, rx_buf, to_get,MSG_PEEK|MSG_DONTWAIT) ;
        if ( len == -1) {
            continue;
        } else if ( (len == 0)) {
            return 0;
        } else { // could be upto to_get count
            for ( i = 0; i < len ; i++) {
                if ( rx_buf[i] == 0x0A ){
                /* we have found the end of this message and are able
                 * to get to the start of the next message
                 * Throw away the current message as incomplete.
                 */
                    len = recv (*socketp, rx_buf, i+1,0);
                    if ( len == i+1) {
                        return i+1;
                    } else if ( len == 0){
                        return 0;
                    }
                }
            }
            to_get += 51; // try and read more than last time
        }
    }
}


/*
 *  Get the remaining characters of the attributed quote message data
 *  When this function is called, we have already received 47 bytes
 *  what is missing is the last 4 bytes of the message, the 3
 *  remaining bytes of the MMID field and the LF end of message indicator.
 */

static inline int get_rest_of_attributed_data(int *socketp, char * rx_buf)
{
    int rx_len = 4;
    int more   = 0;
    int retval = 0;
    while(1){
        rx_len = recv(*socketp,rx_buf+more,rx_len - more , 0);
        if ( rx_len == -1){
            usleep(100);              /* Just try again and hope for the best */
        }else if (rx_len == 0){
            /* Connection Broken */
            retval = 0;
            break;
        }else if ( rx_len == (rx_len - more) ){
            retval = 4;
            break;
        }else{// > 0 but < 4
            more = rx_len;
        }
    }
    return retval;
}

/*
 * Find the start of the next message such that
 * when the next read is done, it starts with
 * the start of a valid message
 */

inline static void find_next_message(int *socketp, int* status)
{
    int reclen, i;
    char rx_buff[100];

    /* make sure we have sometime before we try to find the next message */
    usleep(10000);

    reclen = recv(*socketp, rx_buff, 51,MSG_PEEK|MSG_DONTWAIT) ;
    if ( reclen == -1) {
        FH_LOG(LH,INFO, (" Error on finding next message start = %d",errno));
        *status = FH_ERROR;
    } else if ( (reclen == 0)) {
        FH_LOG(LH, INFO, (" received 0 on next message start errno is %d",errno));
        *status = FH_ERROR;
    } else if ( (reclen > 0) && (reclen <= 51) ) {
        for ( i = 0; i < reclen ; i++) {
            if ( rx_buff[i] == 0x0A )
                break;
        }
        if ( i <= 51 ){
            /* we have found the end of this message and are able
             * to get to the start of the next message
             * Throw away the current message as incomplete.
             */
            reclen = recv (*socketp, rx_buff, i+1,0);
            if ( reclen == i+1) {
                *status = FH_OK;
            } else if ( reclen == -1 || reclen == 0){
                *status = FH_ERROR;
            }
        } else {
            *status = FH_ERROR;
        }

    }
}

/*
 * Attempt reconnect again. This function retries until successful
 * It will not return until a connect has happened Or the exit condition
 * is satisfied.
 */
static inline void dir_edge_reconnect(int *socketp, fh_shr_cfg_lh_line_t *  line)
{
    FH_STATUS rc;
    int count = 0;
    static int first_time = 0;

    /* first thing we send out the connection Broken alarm except the first time after
     * starting the feed handler
     */
    if( first_time == 1){
        lh_callbacks->alarm(&cur_session[0], sizeof(cur_session), FH_ALERT_TCP_CONNECTION_BROKEN);
    }
    first_time = 1;
    if ((rc = close(*socketp)) == -1 ){
        FH_LOG(LH, WARN, ("Unable to close socket after connection broken errno = %d",errno));
    }
    *socketp = 0;
    FH_LOG(LH, WARN, (" Connection Broken, reestablishing the connection"));
    while( ((rc = lh_callbacks->connect(&lh_process,line, &dir_edge_seq_num) != FH_OK) &&
            (!finished))){
        if ( count == 0) {
            FH_LOG_PGEN(LH, ("error trying to create a socket and logging into the server, trying until successful"));
            count = 1;
        }
        sleep(3);
    }
    if (!finished){
        FH_LOG_PGEN(LH, ("Successfully logged into server"));
        lh_callbacks->alarm(&cur_session[0],sizeof(cur_session),FH_ALERT_TCP_CONNECTION_ESTABLISHED);
    }
    de_connection_count++;
    /* reset the HB monitoring timeout value  */
    monitor_hb_count = HB_ALARM_TIMEOUT;
}
/*
 * Actual Body of the TCP line handler
 */

static void *fh_shr_tcp_lh_run(void *arg)
{
    int      reclen, len_to_rec, rx_count;
    int      rc, retval;
    fd_set         rdfds;    /* file descriptor set */
    fd_set           fds;
    int              len = 0;
    int         add_order = 0;
    struct timeval   tv;

    static int part_rx = 0;  /* Used in the partial receive of the PEEK of a message */

    fh_shr_cfg_lh_proc_t *config = (fh_shr_cfg_lh_proc_t *) arg;
    /* this connection only deals with one line  */
    char            *thread_name = NULL;
    int                 *socketp = &lh_process.lines[0].primary.socket;
    fh_info_stats_t       *stats = &lh_process.lines[0].primary.stats;

    fh_shr_lh_conn_t *     conn  = &lh_process.lines[0].primary;

    char            rx_buf[2048];

    fh_shr_cfg_lh_line_t *  line = &config->lines[0];

    uint32_t  primary_addr       = line->primary.address;
    uint16_t  primary_port       = line->primary.port;

    FD_ZERO(&rdfds);

     /* initialize latency measurement structures */
    if (FH_LL_OK(LH, STATS)) {
        FH_PROF_INIT(lh_proc_latency);
        FH_PROF_INIT(lh_recv_latency);
    }

    lh_threadid   = gettid();
    /* set thread affinity */
    if (config->cpu >= 0 && fh_cpu_setaffinity(CPU(config->cpu)) != FH_OK) {
        FH_LOG(LH, WARN, ("failed to assign CPU affinity %d to LH thread", config->cpu));
    }

    /*
     * allocate space for, generate, and log a "thread started" message for
     * this thread's name
     */
    thread_name = fh_util_thread_name("LH", config->name);
    fh_log_thread_start(thread_name);

    /* give the message parser a chance to initialize itself */
    lh_callbacks->init(&lh_process);


    /* start to connect to the server for the TCP connection  */
    if ( (primary_addr == 0) || (primary_port == 0)) {
        FH_LOG(LH, ERR, ("failed to get address and port to connect to %s %s",primary_addr, primary_port));
        exit(1);
    }

    /* connect and login if possible. It will wait till it can succeed*/
    dir_edge_reconnect(socketp, line);

    /* everthing ok so we can receive traffic from the exchange    */
    //s = *socketp;
    FD_SET(*socketp, &rdfds);
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while (!finished) {


        memcpy(&fds, &rdfds, sizeof(fd_set));

        retval = select((*socketp)+1, &fds, NULL, NULL,&tv );
        if (( retval == -1 ) && ( errno != EINTR)) {
            /* an error occured  */
            FH_LOG(LH, INFO, (" select returned an error %d",errno));
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            // Assume the connection is gone if EDABF errno
            if ( errno == EBADF){
                dir_edge_reconnect(socketp, line);
                FD_ZERO(&rdfds);
                FD_SET(*socketp, &rdfds);
            }

        } else if( retval == 0 ) {
            /* we have a timeout condition */
            static int new_conn = 0;
            //static char hb[2] = {'R',0x0A};
            //static ssize_t scount = 0;
            tv.tv_sec = 1;
            tv.tv_usec = 0;
            new_conn  = 0;
            monitor_hb_count--;
            if(monitor_hb_count == 0){
                /* generate an alarm, HB missing  */
                lh_callbacks->alarm(&cur_session[0],sizeof(cur_session),FH_ALERT_SERVER_HB_MISSING);
                monitor_hb_count = HB_ALARM_TIMEOUT;
            }
            /* send the HB message here since we haven't received any traffic */
            /* for a minute                                                   */
            send_hb_msg(socketp,line, stats, &new_conn);
            if ( new_conn ) {
                FD_ZERO(&rdfds);
                FD_SET(*socketp, &rdfds);
            }
            continue;

        } else {
            /* now we can go and get the data                     */
            /* First peek inside the message and then determine   */
            /* what we should really get from the details learned */
            /* during the PEEK operation                          */
            static int still_more = 0;
            static int offset     = 0;
            char rx_char          = '\0';
            /* Assume we dont want to check for HB's since data is*/
            /* being received from the server                     */
            monitor_hb_count = HB_ALARM_TIMEOUT;

            if( still_more > 0){
                /* this is the point where we are expecting a part*/
                /* of a message, lets assume all is here          */

                if(( reclen = recv(*socketp,&rx_buf[offset],still_more, 0)) == still_more){
                    lh_callbacks->parse(rx_buf, offset+still_more, rx_char,conn,line, &dir_edge_seq_num);
                    if (FH_LL_OK(LH, STATS)) {
                        FH_PROF_END(lh_recv_latency);
                    }


                    if (hook_msg_flush) {
                        hook_msg_flush(&rc);
                    }
                    stats->messages++;
                    stats->bytes += offset+still_more;
                    offset = 0;
                    still_more = 0;
                }else if ( reclen == -1) {
                    /* We have more data probably */
                    FH_LOG(LH, VSTATE, (" Errno on receive error = %d",errno));
                    if ( errno == EAGAIN ) continue;
                    if ( errno == ECONNRESET){
                        dir_edge_reconnect(socketp, line);
                        FD_ZERO(&rdfds);
                        FD_SET(*socketp, &rdfds);
                        still_more = 0;
                        offset = 0;
                        stats->message_errors++;
                    }
                }else if ((reclen > 0) && (reclen < still_more)) {
                    offset = offset + reclen;
                    still_more = still_more - reclen;
                }else if ((reclen == 0)) {
                    dir_edge_reconnect(socketp, line);
                    FD_ZERO(&rdfds);
                    FD_SET(*socketp, &rdfds);
                    still_more = 0;
                    offset = 0;
                    stats->message_errors++;
                    continue;
                }
            }
            while (1) {
                //reclen = recv(*socketp, rx_buf, 10,MSG_PEEK|MSG_DONTWAIT) ;
                reclen = recv(*socketp, &rx_buf[part_rx], (10 - part_rx),MSG_PEEK|MSG_DONTWAIT) ;
                if ( reclen == -1) {
                    FH_LOG(LH,VSTATE, (" Errno on receive = %d",errno));
                    if( errno != EAGAIN) break;
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                    break;
                } else if ( (reclen == 0)) {
                    FH_LOG(LH, INFO, (" received 0 and the errno is %d",errno));
                    /* connection broken, reconnect */
                    /* always try and reconnect unless we have been told to exit */
                    /* reconnect  again */
                    dir_edge_reconnect(socketp, line);
                    FD_ZERO(&rdfds);
                    FD_SET(*socketp, &rdfds);
                    still_more = 0;
                    offset = 0;
                    stats->message_errors++;
                    break;

                    //} else if ( reclen == 10 ) {
                } else if ( (reclen + part_rx) == 10 ) {
                    if(rx_buf[0] == 'S') {
                        //rx_char = rx_buf[reclen-1];
                        rx_char = rx_buf[9];
                        /* we can start looking for the sequenced message type */
                        if ( rx_char == 'S') {
                            len_to_rec = SYSTEM_EVENT_MSG_SIZE;
                        } else if(rx_char == 'A') {
                            len_to_rec = ADD_ORDER_MSG_SIZE;
                            add_order  = 1;
                        } else if(rx_char == 'E') {
                            len_to_rec = ORDER_EXECUTED_MSG_SIZE;
                        } else if(rx_char == 'X') {
                            len_to_rec = ORDER_CANCELED_MSG_SIZE;
                        } else if(rx_char == 'P') {
                            len_to_rec = TRADE_MSG_SIZE;
                        } else if(rx_char == 'B') {
                            len_to_rec = BROKEN_TRADE_MSG_SIZE;
                        } else if(rx_char == 'H') {
                            len_to_rec = SECURITY_STATUS_MSG_SIZE;
                        } else {
                            FH_LOG(LH, VSTATE, ("Unknown sequenced message type '%c' received",
                                                rx_char));
                            break;
                        }
                        /* now go and get the data so we can proceed */

                        if((reclen = recv(*socketp, &rx_buf[part_rx],len_to_rec - part_rx, 0)) == len_to_rec - part_rx) {
                            if (FH_LL_OK(LH, STATS)) {
                                FH_PROF_BEG(lh_proc_latency);
                            }
                            if(add_order == 1 ){
                                if(rx_buf[len_to_rec - 2] == 'A') {// attributed quote received
                                    len = get_rest_of_attributed_data(socketp,&rx_buf[len_to_rec]);
                                    if(len == 0)
                                        break;
                                }
                                add_order = 0;
                            }
                            lh_callbacks->parse(rx_buf, len_to_rec+len, rx_char,conn,line, &dir_edge_seq_num);

                            if (FH_LL_OK(LH, STATS)) {
                                FH_PROF_END(lh_proc_latency);
                            }
                            len = 0;
                            if (hook_msg_flush) {
                                hook_msg_flush(&rc);
                            }
                            stats->messages++;
                            //stats->bytes += reclen;
                            stats->bytes += len_to_rec;
                            part_rx = 0;
                        } else if( reclen == 0) {
                            if(( errno == ECONNRESET) || (errno == EPIPE)){
                                /* reconnect  again */
                                dir_edge_reconnect(socketp, line);
                                FD_ZERO(&rdfds);
                                FD_SET(*socketp, &rdfds);
                                still_more = 0;
                                offset = 0;
                                stats->message_errors++;
                                part_rx = 0;
                            }
                            break;
                            //} else if ( (reclen > 0) && (reclen < len_to_rec)) {
                        } else if ( (reclen > 0) && (reclen < (len_to_rec - part_rx))) {
                            /* We get here if there is no more data(TIMEOUT) or we lost  */
                            /* connection                                                */
                            offset     = reclen + part_rx;
                            still_more = len_to_rec - reclen - part_rx;
                            break;
                        } else { //== -1
                            tv.tv_sec = 1;
                            tv.tv_usec = 0;
                            FH_LOG(LH, ERR, (" on recv got error errno = %d",errno));
                            part_rx = 0;
                            break;
                        }
                    } else {
                        if(rx_buf[0] == '+'){
                            /** Received a Debug message **/
                            /** Needs special processing **/
                            static int len = 0;
                            if((len = get_and_send_debug_msg(socketp,rx_buf)) == 0){
                                break; // since we got a 0, it is a disconnect condition
                            }
                        }else{
                            FH_LOG(LH,ERR, ("did not get a 'S' character  at start rx = %c",rx_buf[0]));
                            /* find the start of the next message                */
                            find_next_message(socketp, &rc);
                            if ( rc == FH_ERROR){
                                FH_LOG(LH, ERR, (" Could not get start of next message after detecting lost message"));
                            }
                            break;
                        }
                    }
                } else { /* length is < 10 */
                    /* check for end of session condition */
                    if (reclen >= 2){
                        if ((rx_buf[0] == 'S')  && (rx_buf[1] == 0x0A)) {
                            FH_LOG(LH,INFO, ("End of session message received"));
                            process_end_of_session(socketp);
                        }else if ( (rx_buf[0] == 'H') && (rx_buf[1] == 0x0A)){
                            // we have received a heart beat message
                            if((rx_count = recv(*socketp,rx_buf, 2,0)) != 2) {
                                FH_LOG(LH, ERR, (" Could not get the HB message from TCP conn"));
                            }else if( rx_count == 2){
                                FH_LOG(LH, INFO, (" Rx a HB message"));
                                hb_count++;
                                monitor_hb_count = HB_ALARM_TIMEOUT;
                            }else if ( rx_count == 0) {
                                // Lost Connection :::::::
                                dir_edge_reconnect(socketp, line);
                                FD_ZERO(&rdfds);
                                FD_SET(*socketp, &rdfds);
                                still_more = 0;
                                offset = 0;
                                stats->message_errors++;
                            }
                        }else{
                            /* Received part of the 10, so we need the rest before proceeding */
                            /* need to do this to avoid getting stuck in a loop, where we     */
                            /* PEEK but never read, so when a connection drops, the select    */
                            /* always returns available status                                */
                            /* Normally not a problem unless the other side suddenly breaks   */
                            /* a connection ::) Should never happen, but will happen if I dont*/
                            /* fix it.                                                        */

                            rx_count = recv(*socketp,rx_buf,reclen,0);
                            if ( rx_count == reclen ) {
                                part_rx = rx_count;
                            }else if( rx_count == -1) {
                                tv.tv_sec = 1;
                                tv.tv_usec = 0;
                            }else if ( rx_count == 0) {
                                // connection is broken
                                dir_edge_reconnect(socketp, line);
                                FD_ZERO(&rdfds);
                                FD_SET(*socketp, &rdfds);
                            }else{
                                part_rx += rx_count;
                            }
                            break;
                        }
                    }
                    usleep(5000);
                    tv.tv_sec = 1;
                    tv.tv_usec = 0;
                    break;
                }
            }/* END WHILE */
        }



    } /* end while loop */

    /* if we get here, success so return a NULL pointer */
    return NULL;
}

/**
 *  @brief (private) Add the logging in configuration to a line configuration
 *
 *  @param config the configuration structure that contains the config. information
 *  @param name the name of the line being added
 *  @param lh_config the line handler config that will receive the added line
 *  @return status code indicating success or failure
 */
static FH_STATUS add_login_info(const fh_cfg_node_t *config, const char *name,
                                fh_shr_cfg_lh_conn_t *conn)
{
    const fh_cfg_node_t  *node      = NULL;
    const char *      property      = NULL;

    /* fetch the login node for this connection */
    node = fh_cfg_get_node(config, name);

    /* if the node doesn't exist, disable the connection (no configuraton == disabled) */
    if (node == NULL) {
        conn->enabled = 0;
        return FH_OK;
    }

    property = fh_cfg_get_string(node, "name");
    if (property == NULL ) {
        FH_LOG(CSI, ERR, ("Login Name not specified '%s' for %s connection", property, name));
        return FH_ERROR;
    }
    strncpy(&conn->login_name[0] ,property,sizeof(conn->login_name));

    property = fh_cfg_get_string(node, "password");
    if (property == NULL ) {
        FH_LOG(CSI, ERR, ("Login Password not specified '%s' for %s connection", property, name));
        return FH_ERROR;
    }
    strncpy(&conn->login_passwd[0],property, sizeof(conn->login_passwd));
    return FH_OK;

}



/**
 *  @brief (private) Add a connection configuration to a line configuration
 *
 *  @param config the configuration structure that contains the config. information
 *  @param name the name of the line being added
 *  @param lh_config the line handler config that will receive the added line
 *  @return status code indicating success or failure
 */
static FH_STATUS add_connection(const fh_cfg_node_t *config, const char *name,
                                fh_shr_cfg_lh_conn_t *conn)
{
    const fh_cfg_node_t  *node      = NULL;
    const char           *property  = NULL;
    struct in_addr        address;

    /* fetch the connection node for this connection */
    node = fh_cfg_get_node(config, name);

    /* if the node doesn't exist, disable the connection (no configuraton == disabled) */
    if (node == NULL) {
        conn->enabled = 0;
        return FH_OK;
    }

    /*
     * if the "enabled" property is missing the connection is enabled...if it is there it must
     * be yes or no, otherwise log an error and disable it
     */
    switch(fh_cfg_set_yesno(node, "enabled", &conn->enabled)) {

    /* if there is a valid enabled property, return if it is set to no */
    case FH_OK:
        if (!conn->enabled) return FH_OK;
        break;

    /* if the enabled property was not found log a warning (default = disabled) */
    case FH_ERR_NOTFOUND:
        FH_LOG(CSI, WARN, ("connection %s: missing property 'enabled'...disabling", name));
        return FH_ERR_NOTFOUND;

    /* any other error, warn about invalid value for property */
    default:
        FH_LOG(CSI, WARN, ("connection %s: property 'enabled' must be 'yes' or 'no'", name));
        return FH_ERROR;
    }

    /*
     * if we have gotten here either the connection is explicitly enabled or the enabled
     * property was not present...set the address
     */
    property = fh_cfg_get_string(node, "address");
    if (property == NULL || !inet_aton(property, &address)) {
        FH_LOG(CSI, ERR, ("invalid ip address '%s' for %s connection", property, name));
        return FH_ERROR;
    }
    conn->address = address.s_addr;

    /* set the connection's port address */
    switch(fh_cfg_set_uint16(node, "port", &conn->port)) {

    case FH_OK:
        break;

    case FH_ERR_NOTFOUND:
        FH_LOG(CSI, WARN, ("connection %s: missing port address property", name));
        return FH_ERR_NOTFOUND;

    default:
        FH_LOG(CSI, ERR, ("connection %s: invalid port address", name));
        return FH_ERROR;
    }

    /* set the interface */
    property = fh_cfg_get_string(node, "interface");
    if (property == NULL) {
        FH_LOG(CSI, ERR, ("missing interface specification for %s connection", name));
        return FH_ERR_NOTFOUND;
    }
    strcpy(conn->interface, property);

    /* if we manage to get here, connection was enabled and configured */
    conn->enabled = 1;
    return FH_OK;
}


/**
 *  @brief (private) Add a line configuration to a process configuration
 *
 *  @param config the configuration structure that contains the config. information
 *  @param name the name of the line being added
 *  @param lh_config the line handler config that will receive the added line
 *  @return status code indicating success or failure
 */
static FH_STATUS add_line(const fh_cfg_node_t *config, const char *name,
                          fh_shr_cfg_lh_proc_t *lh_config)
{
    const fh_cfg_node_t  *lines_node    = NULL;
    const fh_cfg_node_t  *line_node     = NULL;
    fh_shr_cfg_lh_line_t *line;

    /* fetch the lines node */
    lines_node = fh_cfg_get_node(config, "direct_edge.lines");
    if (lines_node == NULL) {
        FH_LOG(CSI, ERR, ("missing configuration option 'lines'"));
        return FH_ERROR;
    }

    /* fetch the node for the specific line being added */
    line_node = fh_cfg_get_node(lines_node, name);
    if (line_node == NULL) {
        FH_LOG(CSI, ERR, ("missing configuration for line '%d'", name));
        return FH_ERROR;
    }

    /* allocate space for the new line */
    lh_config->lines = (fh_shr_cfg_lh_line_t *)realloc(lh_config->lines,
                        sizeof(fh_shr_cfg_lh_line_t) * (lh_config->num_lines + 1));
    if (lh_config->lines == NULL) {
        FH_LOG(CSI, ERR, ("unable to allocate memory for line '%s'", name));
        return FH_ERROR;
    }

    /* initialize the data for the new line */
    line = &lh_config->lines[lh_config->num_lines++];
    memset(line, 0, sizeof(fh_shr_cfg_lh_line_t));
    line->process = lh_config;
    strcpy(line->name, name);

    /* set up the connections for the new line */
    if (add_connection(line_node, "primary", &line->primary) != FH_OK) {
        line->primary.line = line;
        return FH_ERROR;
    }

     /* set up the logging in info for that line */
    if ( add_login_info(line_node, "login", &line->primary) != FH_OK) {
        return FH_ERROR;

    }

    /* if we get here, success! */
    return FH_OK;
}



/**
 *  @brief Load a line handler configuration structure from a general config structure
 *
 *  @param process the process that is being run (and whose configuration is being loaded)
 *  @param config a configuration node from which to load the line handler configure
 *  @param lh_config the line handler structure in which to load the configuration
 *  @return status code indicating success or failure
 */
FH_STATUS fh_shr_cfg_tcp_lh_load(const char *process, const fh_cfg_node_t *config,
                             fh_shr_cfg_lh_proc_t *lh_config)
{
    char                  process_node_name[MAX_PROPERTY_LENGTH];
    const fh_cfg_node_t  *process_node  = NULL;
    const fh_cfg_node_t  *lines_node    = NULL;
    int                   i;
    FH_STATUS             rc;

    /* initialize the process configuration structure we have been passed */
    memset(lh_config, 0, sizeof(fh_shr_cfg_lh_proc_t));

    /* build the full, expected node name of the process configuration and fetch the node */
    sprintf(process_node_name, "direct_edge.processes.%s", process);
    process_node = fh_cfg_get_node(config, process_node_name);

    /* if the returned node is NULL, the process config doesn't exist */
    if (process_node == NULL) {
        FH_LOG(CSI, ERR, ("no process configuration for '%s'", process));
        return FH_ERROR;
    }

    /* copy the process name into the process configuration structure */
    strcpy(lh_config->name, process_node->name);

    /* if a proper CPU specification has been made, set it, otherwise default to -1 */
    switch (fh_cfg_set_int(process_node, "cpu", &lh_config->cpu)) {

    case FH_OK:
        break;

    case FH_ERR_NOTFOUND:
        FH_LOG(CSI, WARN, ("process %s: missing CPU specification", process));
        lh_config->cpu = -1;
        break;

    default:
        FH_LOG(CSI, WARN, ("process %s: invalid CPU specification", process));
        lh_config->cpu = -1;
        break;
    }

    /* load table configurations */
    fh_shr_cfg_tbl_load(config, "direct_edge.symbol_table", &lh_config->symbol_table);
    fh_shr_cfg_tbl_load(config, "direct_edge.order_table", &lh_config->order_table);

    /* load lines node for this process */
    lines_node = fh_cfg_get_node(process_node, "lines");
    if (lines_node == NULL) {
        FH_LOG(CSI, ERR, ("process configuration '%s' contains no lines", process));
        return FH_ERROR;
    }

    /* go through every line entry, loading the line that corresponds */
    for (i = 0; i < lines_node->num_values; i++) {
        if (add_line(config, lines_node->values[i], lh_config) != FH_OK) {
            return FH_ERROR;
        }
    }

    /* allow a plugin the chance to modify the loaded configuration */
    if (fh_plugin_is_hook_registered(FH_PLUGIN_CFG_LOAD)) {
        fh_plugin_get_hook(FH_PLUGIN_CFG_LOAD)(&rc, lh_config);
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("error occured during plugin configuration (%d)", rc));
            return rc;
        }
    }

    /* if we get here, success! */
    return FH_OK;
}

/*
 * Initialize all sockets etc.
 */
static FH_STATUS fh_shr_tcp_lh_init(fh_shr_cfg_lh_proc_t *config)
{
    FH_STATUS            rc;
    fh_shr_lh_line_t    *line;
    fh_shr_lh_conn_t    *primary;
    int                  i;

    /* point the global process data structure at the process configuration */
    lh_process.config = config;

    /* allocate line structures for each of the configured lines **/
    lh_process.lines = (fh_shr_lh_line_t *)malloc(sizeof(fh_shr_lh_line_t) * config->num_lines);
    if (lh_process.lines == NULL) {
        FH_LOG(LH, ERR, ("unable to allocate memory for line data (%s)", config->name));
        return FH_ERROR;
    }
    lh_process.num_lines = config->num_lines;

    /* loop through all of the lines in our process configuration */
    for (i = 0; i < config->num_lines; i++) {
        /* link each line data structure to its config */
        lh_process.lines[i].config = &config->lines[i];

        /* set a pointer to the connections for the line we are currently configuring */
        line        = &lh_process.lines[i];
        primary     = &line->primary;


        /* initialize the line's next expected sequence number */
        line->next_seq_no = 1;

        /* link this line's process pointer back to this process */
        line->process = &lh_process;

        /* set up the primary connection */
        primary->config = &line->config->primary;

        primary->line = line;
        strcpy(primary->tag, "primary");

    }

    /* zero all statistics */
    fh_shr_tcp_lh_clear_stats();

    /* initialize any tables that the feed handler is going to keep */
    fh_shr_tcp_lh_tbl_init();

    /* allow a plugin the change to modify the loaded configuration */
    if (fh_plugin_is_hook_registered(FH_PLUGIN_LH_INIT)) {
        fh_plugin_get_hook(FH_PLUGIN_LH_INIT)(&rc, &lh_process);
        if (rc != FH_OK) {
            FH_LOG(MGMT, ERR, ("error occured during plugin line handler init (%d)", rc));
            return rc;
        }
    }

    /* cache any hooks that have been registered and will later be called */
    if (fh_plugin_is_hook_registered(FH_PLUGIN_MSG_FLUSH)) {
        hook_msg_flush = fh_plugin_get_hook(FH_PLUGIN_MSG_FLUSH);
    }

    /* indicate that initialization is complete */
    lh_init = 1;

    /* as long as we get all the way here, success */
    return FH_OK;
}



FH_STATUS fh_shr_tcp_lh_start(const fh_info_build_t *info, fh_shr_cfg_lh_proc_t *config,
                          fh_shr_tcp_cb_t *callbacks)
{
    FH_STATUS rc;

    /* store references to feed handler info and configuration */
    lh_info         = info;
    lh_callbacks    = callbacks;

    /* initialize all socket                                   */
    if ((rc = fh_shr_tcp_lh_init(config)) != FH_OK) {
        FH_LOG(LH, ERR, ("failed to initialize sockets for line handler (%s)", config->name));
        return rc;
    }

    if (pthread_create(&lh_thread, NULL, fh_shr_tcp_lh_run, config) < 0) {
        FH_LOG(LH, ERR, ("failed to start line handler thread (%s): %s",
                         config->name, strerror(errno)));
        return FH_ERROR;
    }

    /* by the time execution gets here, success! */
    return FH_OK;
}


/*
 * Wait on the completion of the line handler thread (convenience method for the caller of
 * fh_shr_lh_start in case the caller wants to block until the thread has exited)
 */
void fh_shr_tcp_lh_wait()
{
    if (lh_thread) {
        pthread_join(lh_thread, NULL);
        FH_LOG(MGMT, VSTATE, ("%s line handler thread (%s) exited", lh_info->name,
                              lh_process.config->name));
    }
}


/*
 * Clear the statistics for lines belonging to this process
 */
void fh_shr_tcp_lh_clear_stats()
{
    int i;

    /* zero process stats */
    memset(&lh_process.stats, 0, sizeof(fh_info_stats_t));

    /* loop through each of the lines zeroing the line stats and each connection's stats */
    for (i = 0; i < lh_process.num_lines; i++) {
        memset(&lh_process.lines[i].stats, 0, sizeof(fh_info_stats_t));
        memset(&lh_process.lines[i].primary.stats, 0, sizeof(fh_info_stats_t));

    }
}

/*
 * Converts stats from internal line handler representation to the proper structure for
 * return to an FH manager
 */
void fh_shr_lh_get_stats(fh_adm_stats_resp_t *stats_resp)
{
    int                      i;
    fh_shr_lh_line_t        *line;
    fh_adm_line_stats_t     *stat_line;

    /* zero the stats response (avoids the potential for bad numbers if we don't happen */
    /* to populate every statistic) */
    memset(stats_resp, 0, sizeof(fh_adm_stats_resp_t));

    /* set the stats for each line in the structure */
    for (i = 0; i < lh_process.num_lines; i++) {
        /* set up a pointer to the current line (for ease of access) */
        line = &lh_process.lines[i];

        /* populate stats for the primary line (if enabled) */
        if (line->primary.config->enabled) {
            /* set up a pointer to the stats line being populated and give it a name*/
            stat_line = &stats_resp->stats_lines[stats_resp->stats_line_cnt];
            sprintf(stat_line->line_name, "%s_", line->config->name);
            fh_util_ucstring(stat_line->line_name + strlen(stat_line->line_name),
                             line->primary.tag);

            /* populate statistics */
            stat_line->line_msg_rx            = line->primary.stats.messages;
            stat_line->line_bytes             = line->primary.stats.bytes;
            stat_line->line_msg_loss          = line->primary.stats.lost_messages;


            /* increment the stat line count */
            stats_resp->stats_line_cnt++;
        }
    }
}
/*
 * Log a snapshot of basic stats since the last time this function was called
 */
void fh_shr_tcp_lh_snap_stats()
{
   /* if line handler initialization is not complete, just return */
    if (!lh_init || !FH_LL_OK(LH, XSTATS)) return;

    /* persistent data across calls */
    static uint64_t messages      = 0;
    static uint64_t errors        = 0;

    /* temporary data (just this call) */
    uint64_t        temp_messages = 0;
    uint64_t        temp_errors   = 0;
    int             i             = 0;

    /* loop through each line, counting stats for each connection */
    for (i = 0; i < lh_process.num_lines; i++) {
        temp_messages += lh_process.lines[i].primary.stats.messages;

    }

    /* log the gathered statistics (minus stats from the last call) */
    FH_LOG(LH, XSTATS, ("LH Aggregated Stats: %5lu PPS - %6lu MPS - (dups: %lu errs: %lu)",
                        temp_messages - messages,
                        temp_errors   - errors
                       ));

    /* save stats from this call for next time through */
    messages = temp_messages;
    errors   = temp_errors;
}


/*
 * Display collected latency statistics
 */
/*
 * fh_opra_lh_latency
 *
 * Dumps some statistics about latency when enabled.
 */
void fh_shr_tcp_lh_latency()
{
    if (FH_LL_OK(LH, STATS)) {
        FH_PROF_PRINT(lh_recv_latency);
        FH_PROF_PRINT(lh_proc_latency);
    }
}

/*
 * Converts stats from internal line handler representation to the proper structure for
 * return to an FH manager
 */
void fh_shr_tcp_lh_get_stats(fh_adm_stats_resp_t *stats_resp)
{
    int                      i;
    fh_shr_lh_line_t        *line;
    fh_adm_line_stats_t     *stat_line;

    /* zero the stats response (avoids the potential for bad numbers if we don't happen */
    /* to populate every statistic) */
    memset(stats_resp, 0, sizeof(fh_adm_stats_resp_t));

    /* set the stats for each line in the structure */
    for (i = 0; i < lh_process.num_lines; i++) {
        /* set up a pointer to the current line (for ease of access) */

        line = &lh_process.lines[i];

        /* populate stats for the primary line (if enabled) */
        if (line->primary.config->enabled) {
            /* set up a pointer to the stats line being populated and give it a name*/
            stat_line = &stats_resp->stats_lines[stats_resp->stats_line_cnt];
            sprintf(stat_line->line_name, "%s_", line->config->name);
            fh_util_ucstring(stat_line->line_name + strlen(stat_line->line_name),

                             line->primary.tag);

            /* populate statistics */
            stat_line->line_pkt_rx            = line->primary.stats.packets;
            stat_line->line_msg_rx            = line->primary.stats.messages;
            stat_line->line_bytes             = line->primary.stats.bytes;
            stat_line->line_pkt_errs          = line->primary.stats.packet_errors;
            stat_line->line_pkt_dups          = line->primary.stats.duplicate_packets;
            stat_line->line_pkt_seq_jump      = line->primary.stats.gaps;
            stat_line->line_msg_loss          = line->primary.stats.lost_messages;
            stat_line->line_msg_recovered     = line->primary.stats.recovered_messages;

            /* increment the stat line count */
            stats_resp->stats_line_cnt++;
        }

    }
}


