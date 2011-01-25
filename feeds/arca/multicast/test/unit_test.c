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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
//#include <netinet/in.h>
#include "fh_arca_constants.h"
#include "fh_arca_headers.h"
#include "fh_feed_group.h"
#include "unit_test.h"

struct feed_group *group;      //used by all tests
struct msg_hdr hdr;            //used by message parsing tests
struct msg_body body;          //         ditto

struct seq_number_reset    startofday;         //sequence number reset message
struct message_unavailable missing_msg;        //message unavailable message
struct symbol_clear        blow_away_symbol;   //symbol clear message
struct symbol_mapping      map_symbols;        //symbol mapping message
struct firm_mapping        map_firms;          //firm mapping message
struct imbalance_refresh   imb_refresh;        //imbalance refresh message
struct mixed_orders        orders_msg;         //message with orders
struct symbol_clear        invalid_msg;        //badly formed message
/*-------------------------------------------------------------------------*/
/* test parsing of sequence number reset message (including message header)*/
/*-------------------------------------------------------------------------*/
int unit_test_sequence_reset() 
{
    int rc             = 0;
    int pkt_size       = 0;
    int pkt_hdr_size   = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(startofday);
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&startofday,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr sequence reset expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size -= pkt_hdr_size;
    rc = parse_mesg(group,((char*)&startofday)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=pkt_size) 
    {
        fprintf(stderr,
            " Parse sequence number reset Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of message unavailable message                             */
/*-------------------------------------------------------------------------*/
int unit_test_missing_msgs() 
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(missing_msg);
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&missing_msg,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr msg unavailable expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size -= pkt_hdr_size;
    rc = parse_mesg(group,((char*)&missing_msg)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=pkt_size) 
    {
        fprintf(stderr,
            " Parse message unavailable Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of symbol clear message                                    */
/*-------------------------------------------------------------------------*/
int unit_test_symbol_clear() 
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(blow_away_symbol);
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&blow_away_symbol,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr symbol clear expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size -= pkt_hdr_size;
    rc = parse_mesg(group,((char*)&blow_away_symbol)+pkt_hdr_size,&hdr,&body,
         &pkt_size,0,0);
    if (rc!=pkt_size) 
    {
        fprintf(stderr,
            " Parse symbol clear Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of symbol mapping message (3 message bodies)               */
/*-------------------------------------------------------------------------*/
int unit_test_symbol_mapping()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(map_symbols);
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&map_symbols,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr map symbols expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size -= pkt_hdr_size;
    rc = parse_mesg(group,((char*)&map_symbols)+pkt_hdr_size,&hdr,&body,
         &pkt_size,0,0);
    if (rc!=20) 
    {
        fprintf(stderr,
            " Parse symbol mapping Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of firm mapping message (3 message bodies)                 */
/*-------------------------------------------------------------------------*/
int unit_test_firm_mapping() 
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(map_firms);
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&map_firms,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr map firms expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size -= pkt_hdr_size;
    rc = parse_mesg(group,((char*)&map_firms)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=12) 
    {
        fprintf(stderr,
            " Parse firm mapping Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of imbalance refresh message                               */
/*-------------------------------------------------------------------------*/
int unit_test_imbalance_refresh()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(imb_refresh);
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&imb_refresh,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr imb refresh expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size -= pkt_hdr_size;
    rc = parse_mesg(group,((char*)&imb_refresh)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=pkt_size) 
    {
        fprintf(stderr,
            " Parse imbalance refresh Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of order messages (imbalance, add, mod, add, del)          */
/*-------------------------------------------------------------------------*/
int unit_test_orders()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;
    char* msg_ptr;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(orders_msg);
    // check the packet header
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&orders_msg,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr Orders expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size -= pkt_hdr_size;
    msg_ptr = ((char*) &orders_msg)+pkt_hdr_size;
    // check the imbalance first body in message
    rc = parse_mesg(group,msg_ptr,&hdr,&body,&pkt_size,0,0);
    if (rc!=36) 
    {
        fprintf(stderr,
            " Parse Orders Imbalance Failed returned %d expected 36\n",
            rc);
        return 1;
    }
    pkt_size-=rc;
    msg_ptr+=rc;
    // check the first add order second body in message
    rc = parse_mesg(group,msg_ptr,&hdr,&body,&pkt_size,0,0);
    if (rc!=32) 
    {
        fprintf(stderr,
            " Parse Orders first add Failed returned %d expected 32\n",
            rc);
        return 1;
    }
    pkt_size-=rc;
    msg_ptr+=rc;
    // check the mod order third body in message
    rc = parse_mesg(group,msg_ptr,&hdr,&body,&pkt_size,0,0);
    if (rc!=32) 
    {
        fprintf(stderr,
            " Parse Orders mod Failed returned %d expected 32\n",
            rc);
        return 1;
    }
    pkt_size-=rc;
    msg_ptr+=rc;
    // check the second add order fourth body in message
    rc = parse_mesg(group,msg_ptr,&hdr,&body,&pkt_size,0,0);
    if (rc!=32) 
    {
        fprintf(stderr,
            " Parse Orders second add Failed returned %d expected 32\n",
            rc);
        return 1;
    }
    pkt_size-=rc;
    msg_ptr+=rc;
    // check the delete order fifth and last body in message
    rc = parse_mesg(group,msg_ptr,&hdr,&body,&pkt_size,0,0);
    if (rc!=24) 
    {
        fprintf(stderr,
            " Parse Orders delete Failed returned %d expected 24\n",
            rc);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of a message with an invalid message type in header        */
/*-------------------------------------------------------------------------*/
int unit_test_invalid_msg()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(invalid_msg);
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&invalid_msg,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr invalid msg expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size-=pkt_hdr_size;
    rc = parse_mesg(group,((char*)&invalid_msg)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc==pkt_size) 
    {
        fprintf(stderr,
            " Parse invalid msg type Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of orders message with an invalid message type in body     */
/*-------------------------------------------------------------------------*/
int unit_test_invalid_order_type()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    uint16_t save_msg_type = 0;
    pkt_size = sizeof(orders_msg);
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&orders_msg,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr Orders msg expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    save_msg_type = orders_msg.first_imbalance.msg_type;
    orders_msg.first_imbalance.msg_type = 104;
    pkt_size-=pkt_hdr_size;
    rc = parse_mesg(group,((char*)&orders_msg)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=1) 
    {
        fprintf(stderr,
            " Parse invalid order msg type Failed returned %d expected 1\n",
            rc);
        rc = 1;
    } 
    else 
    {
        rc = 0;
    }
    // restore the msg_type 
    orders_msg.first_imbalance.msg_type  = save_msg_type;
    return rc;
};

/*! \brief Test parsing of a message whose length is less than the header size
 *
 *  \return 0 on success, non-zero on failure
 */
int unit_test_runt_header()
{
    int rc          = 0;
    int pkt_size    = 15;
    
    // initialize header and body structures
    memset(&hdr,  0, sizeof(hdr));
    memset(&body, 0, sizeof(body));
    
    // parse a packet header that is too small and test that the failure was detected
    rc = parse_packet_hdr(&hdr, (char *)&startofday, pkt_size, (uint64_t)16758765);   
    if (rc != -1) 
    {
        fprintf(stderr, 
            "Parse of runt packet FAILED - returned %d, expected -1\n", rc);
        return 1;
    }
    // if we get here, test was a success
    return 0;
}

/*-------------------------------------------------------------------------*/
/* test parsing of a sequence number reset message whose length is short   */
/*-------------------------------------------------------------------------*/
int unit_test_runt_sequence_reset()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(startofday)-1;
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&startofday,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr sequence reset runt expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size-=pkt_hdr_size;
    rc = parse_mesg(group,((char*)&startofday)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=0) 
    {
        fprintf(stderr,
            " Parse runt sequence reset Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of message unavailable message whose length is too short   */
/*-------------------------------------------------------------------------*/
int unit_test_runt_missing_msgs()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(missing_msg)-1;
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&missing_msg,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr msg unavail runt expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size-=pkt_hdr_size;
    rc = parse_mesg(group,((char*)&missing_msg)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=0) 
    {
        fprintf(stderr,
            " Parse runt message unavailable Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of symbol clear message whose length is too short          */
/*-------------------------------------------------------------------------*/
int unit_test_runt_symbol_clear()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(blow_away_symbol)-1;
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&blow_away_symbol,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr symbol clear runt expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size-=pkt_hdr_size;
    rc = parse_mesg(group,((char*)&blow_away_symbol)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=0) 
    {
        fprintf(stderr,
            " Parse runt symbol clear Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of symbol mapping message whose length is too short        */
/*-------------------------------------------------------------------------*/
int unit_test_runt_symbol_mapping()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = 16+19;   //packet header size + first body size - 1
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&map_symbols,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr symbol map runt expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size-=pkt_hdr_size;
    rc = parse_mesg(group,((char*)&map_symbols)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=0) 
    {
        fprintf(stderr,
            " Parse runt symbol mapping Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of firm mapping message whose length is too short          */
/*-------------------------------------------------------------------------*/
int unit_test_runt_firm_mapping()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = 16 +11;  //packet header size + first body size - 1
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&map_firms,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr firm map runt expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size-=pkt_hdr_size;
    rc = parse_mesg(group,((char*)&map_firms)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=0) 
    {
        fprintf(stderr,
            " Parse runt firm mapping Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of imbalance refresh message whose length is too short     */
/*-------------------------------------------------------------------------*/
int unit_test_runt_imbalance_refresh()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(imb_refresh)-1;
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&imb_refresh,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr imb refresh runt expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size-=pkt_hdr_size;
    rc = parse_mesg(group,((char*)&imb_refresh)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=0) 
    {
        fprintf(stderr,
            " Parse runt imbalance refresh Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of add order message whose length is too short             */
/*   should see the problem parsing second message body                    */
/*-------------------------------------------------------------------------*/
int unit_test_runt_add_order() 
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;
    char* msg_ptr       = NULL;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = 16+sizeof(struct imbalance_body)+sizeof(struct order_body)-1;
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&orders_msg,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr add order runt expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size=31; // body size -1
    msg_ptr = ((char*)&orders_msg)+pkt_hdr_size+36; //skip over imbalance
    rc = parse_mesg(group,msg_ptr,&hdr,&body,&pkt_size,0,0);
    if (rc!=0) 
    {
        fprintf(stderr,
            " Parse runt add order Failed returned %d expected 0\n",rc);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of mod order message whose length is too short             */
/*   should see the problem parsing third message body                     */
/*-------------------------------------------------------------------------*/
int unit_test_runt_mod_order()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;
    char* msg_ptr       = NULL;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = 16+sizeof(struct imbalance_body)+(2*sizeof(struct order_body))
        -1;
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&orders_msg,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr mod order runt expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    //skip over imbalance and first add order
    pkt_size=31;
    msg_ptr = ((char*) &orders_msg)+pkt_hdr_size+36+32; 
    rc = parse_mesg(group,msg_ptr,&hdr,&body,&pkt_size,0,0);
    if (rc!=0) 
    {
        fprintf(stderr,
            " Parse runt mod order Failed returned %d expected %d",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of imbalance message whose length is too short             */
/*   should see the problem parsing first message body                     */
/*-------------------------------------------------------------------------*/
int unit_test_runt_imbalance()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = 16+sizeof(struct imbalance_body)-1;
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&orders_msg,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr imb order runt expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    pkt_size-=pkt_hdr_size;
    rc = parse_mesg(group,((char*)&orders_msg)+pkt_hdr_size,&hdr,&body,
        &pkt_size,0,0);
    if (rc!=0) 
    {
        fprintf(stderr,
            " Parse runt imbalance order Failed returned %d expected %d\n",
            rc,pkt_size);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test parsing of delete order message whose length is too short          */
/*   should see the problem after parsing 4 previous message bodies        */
/*-------------------------------------------------------------------------*/
int unit_test_runt_del_order()
{
    int rc              = 0;
    int pkt_size        = 0;
    int pkt_hdr_size    = 0;
    char* msg_ptr=NULL;

    memset(&hdr,0,sizeof(hdr));
    memset(&body,0,sizeof(body));
    pkt_size = sizeof(orders_msg)-1;
    pkt_hdr_size = parse_packet_hdr(&hdr,(char*)&orders_msg,pkt_size,
        (uint64_t)16758765);
    if (pkt_hdr_size!=16) 
    {
        fprintf(stderr,
            " Parse fail for msg hdr del order runt expected 16 was %d\n",
            pkt_hdr_size);
        return 1;
    }
    // skip over all but last message
    pkt_size=23;
    msg_ptr = ((char*)&orders_msg)+pkt_hdr_size+36+32+32+32;
    rc = parse_mesg(group,msg_ptr,&hdr,&body,&pkt_size,0,0);
    if (rc!=0) 
    {
        fprintf(stderr,
            " Parse runt del order Failed returned %d expected 0\n",rc);
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* build message headers for messages used in unit testing                 */
/*-------------------------------------------------------------------------*/
void build_message_headers() 
{
    // header - message type
    startofday.msg_type   = htons((uint16_t) 1);
    missing_msg.msg_type  = htons((uint16_t) 5);
    blow_away_symbol.msg_type = htons((uint16_t) 36);
    map_symbols.msg_type  = htons((uint16_t) 35);
    map_firms.msg_type    = htons((uint16_t) 37);
    imb_refresh.msg_type   = htons((uint16_t) 33);
    orders_msg.msg_type   = htons((uint16_t) 99);
    invalid_msg.msg_type  = htons((uint16_t) 3);
    // header - message size
    startofday.msg_size   = htons(sizeof(startofday));
    missing_msg.msg_size  = htons(sizeof(struct message_unavailable));
    blow_away_symbol.msg_size= htons(sizeof(struct symbol_clear));
    map_symbols.msg_size  = htons(sizeof(struct symbol_mapping));
    map_firms.msg_size    = htons(sizeof(struct firm_mapping));
    imb_refresh.msg_size  = htons(sizeof(struct imbalance_refresh));
    orders_msg.msg_size   = htons(sizeof(struct mixed_orders));
    invalid_msg.msg_size  = htons(sizeof(struct symbol_clear));
    // header - message sequence numbers
    startofday.msg_seq_num= htonl((uint32_t) 999);
    missing_msg.msg_seq_num=htonl((uint32_t) 1000000);
    blow_away_symbol.msg_seq_num=htonl((uint32_t) 100000000);
    map_symbols.msg_seq_num=htonl((uint32_t) 13);
    map_firms.msg_seq_num = htonl((uint32_t) 14);
    imb_refresh.msg_seq_num=htonl((uint32_t) 15);
    orders_msg.msg_seq_num= htonl((uint32_t) 16);
    invalid_msg.msg_seq_num=htonl((uint32_t) 17);
    // header - sent time
    startofday.send_time  = htonl((uint32_t) 9000);
    missing_msg.send_time = htonl((uint32_t) 9010);
    blow_away_symbol.send_time= htonl((uint32_t) 9030);
    map_symbols.send_time = htonl((uint32_t) 9050);
    map_firms.send_time   = htonl((uint32_t) 9060);
    imb_refresh.send_time  = htonl((uint32_t) 9070);
    orders_msg.send_time  = htonl((uint32_t) 9090);
    invalid_msg.msg_seq_num=htonl((uint32_t) 9101);
    // header - product id
    startofday.product_id = (uint8_t) 115;
    missing_msg.product_id= (uint8_t) 115;
    blow_away_symbol.product_id=(uint8_t) 115;
    map_symbols.product_id= (uint8_t) 115;
    map_firms.product_id  = (uint8_t) 115;
    imb_refresh.product_id = (uint8_t) 115;
    orders_msg.product_id = (uint8_t) 115;
    invalid_msg.product_id= (uint8_t) 115;
    // header - retrans flag
    startofday.retrans_flag = (uint8_t) 1;
    missing_msg.retrans_flag= (uint8_t) 1;
    blow_away_symbol.retrans_flag=(uint8_t) 1;
    map_symbols.retrans_flag= (uint8_t) 1;
    map_firms.retrans_flag  = (uint8_t) 1;
    imb_refresh.retrans_flag = (uint8_t) 1;
    orders_msg.retrans_flag = (uint8_t) 1;
    invalid_msg.retrans_flag= (uint8_t) 1;
    // header - number of message bodies
    startofday.num_body_entries = (uint8_t) 1;
    missing_msg.num_body_entries= (uint8_t) 1;
    blow_away_symbol.num_body_entries=(uint8_t) 1;
    map_symbols.num_body_entries= (uint8_t) SYMBOL_MAPPING_BODIES;
    map_firms.num_body_entries  = (uint8_t) FIRM_MAPPING_BODIES;
    imb_refresh.num_body_entries = (uint8_t) 1;
    orders_msg.num_body_entries = (uint8_t) 5;
    invalid_msg.num_body_entries= (uint8_t) 1;
};
/*-------------------------------------------------------------------------*/
/* build the body for sequence number reset message                        */
/*-------------------------------------------------------------------------*/
void build_startofday()
{
    startofday.next_seq_number  = htonl((uint32_t) 1);
};
/*-------------------------------------------------------------------------*/
/* build the body for message unavailable message                          */
/*-------------------------------------------------------------------------*/
void build_missing_sequences() 
{
    missing_msg.begin_seq_num   = htonl((uint32_t) 111);
    missing_msg.end_seq_num     = htonl((uint32_t) 115);
};
/*-------------------------------------------------------------------------*/
/* build the body for symbol clear message                                 */
/*-------------------------------------------------------------------------*/
void build_symbol_clear() 
{
    blow_away_symbol.next_seq_num   = htonl((uint32_t) 260);
    blow_away_symbol.symbol_index   = htons((uint16_t) 75);
    blow_away_symbol.session_id     = (uint8_t) 7;
};
/*-------------------------------------------------------------------------*/
/* build the body for symbol mapping message (3 bodies)                    */
/*-------------------------------------------------------------------------*/
void build_mapping_symbols() 
{
    map_symbols.bodies[0].index = htons((uint16_t) 75);
    map_symbols.bodies[0].session_id=(uint8_t) 7;
    memcpy(&(map_symbols.bodies[0].symbol),"msft",5);
    map_symbols.bodies[1].index = htons((uint16_t) 76);
    map_symbols.bodies[1].session_id=(uint8_t) 7;
    memcpy(&(map_symbols.bodies[1].symbol),"glw",4);
    map_symbols.bodies[2].index = htons((uint16_t) 77);
    map_symbols.bodies[2].session_id=(uint8_t) 7;
    memcpy(&(map_symbols.bodies[2].symbol[0]),"ibm.q",6);
};
/*-------------------------------------------------------------------------*/
/* build the body for firm mapping message (3 bodies)                      */
/*-------------------------------------------------------------------------*/
void build_mapping_firms() 
{
    map_firms.bodies[0].index   = htons((uint16_t) 5);
    memcpy(&(map_firms.bodies[0].firm[0]),"gs",3);
    map_firms.bodies[1].index   = htons((uint16_t) 6);
    memcpy(&(map_firms.bodies[1].firm[0]),"bt",3);
    map_firms.bodies[2].index   = htons((uint16_t) 7);
    memcpy(&(map_firms.bodies[2].firm[0]),"fid",4);
};
/*-------------------------------------------------------------------------*/
/* build the body for imbalance refresh message                            */
/*-------------------------------------------------------------------------*/
void build_imbalance_refresh()
{
    imb_refresh.body.symbol_index = htons((uint16_t) 75);
    imb_refresh.body.msg_type     = htons((uint16_t) 33);
    imb_refresh.body.source_seq_num=htonl((uint32_t) 9037);
    imb_refresh.body.source_time  = htonl((uint32_t) 66734);
    imb_refresh.body.volume       = htonl((uint32_t) 60000);
    imb_refresh.body.total_imbalance=htonl((uint32_t) 50000);
    imb_refresh.body.mkt_imbalance = htonl((uint32_t) 40000);
    imb_refresh.body.price_numerator=htonl((uint32_t) 99763);
    imb_refresh.body.price_scale  = (uint8_t)  5;
    imb_refresh.body.auction_type = 'O';
    imb_refresh.body.exchange     = 'P';
    imb_refresh.body.security_type= 'E';
    imb_refresh.body.session      = (uint8_t) 7;
    imb_refresh.body.auction_time = htons((uint16_t) 8055);
};
/*-------------------------------------------------------------------------*/
/* build the bodies for the mixed orders message (5 bodies)                */
/*-------------------------------------------------------------------------*/
void build_orders() 
{
    //build the first_imbalance message
    orders_msg.first_imbalance.symbol_index   = htons((uint16_t) 76);
    orders_msg.first_imbalance.msg_type       = htons((uint16_t) 103);
    orders_msg.first_imbalance.source_seq_num = htonl((uint32_t) 33);
    orders_msg.first_imbalance.source_time    = htonl((uint32_t) 9099);
    orders_msg.first_imbalance.volume         = htonl((uint32_t) 900);
    orders_msg.first_imbalance.total_imbalance= htonl((uint32_t) 0);
    orders_msg.first_imbalance.mkt_imbalance  = htonl((uint32_t) 0xffffffd3);
    orders_msg.first_imbalance.price_numerator= htonl((uint32_t) 7);
    orders_msg.first_imbalance.price_scale    = (uint8_t)  4;
    orders_msg.first_imbalance.auction_type   = 'M';
    orders_msg.first_imbalance.exchange      = 'N';
    orders_msg.first_imbalance.security_type  = 'E';
    orders_msg.first_imbalance.session        = (uint8_t)  7;
    orders_msg.first_imbalance.auction_time   = (uint16_t) 6;
    //build the first add order message
    orders_msg.first_add.symbol_index         = htons((uint16_t) 75);
    orders_msg.first_add.msg_type             = htons((uint16_t) 100);
    orders_msg.first_add.source_seq_num       = htonl((uint32_t) 1);
    orders_msg.first_add.source_time          = htonl((uint32_t) 9099);
    orders_msg.first_add.volume               = htonl((uint32_t) 200);
    orders_msg.first_add.order_id             = htonl((uint32_t) 99);
    orders_msg.first_add.price_numerator      = htonl((uint32_t) 8960);
    orders_msg.first_add.price_scale          = (uint8_t)  2;
    orders_msg.first_add.side                 = 'B';
    orders_msg.first_add.exchange             = 'P';
    orders_msg.first_add.security_type        = 'E';
    orders_msg.first_add.firm_index           = htons((uint16_t) 5);
    orders_msg.first_add.session_id           = (uint8_t)  7;
    //build the first mod order message
    orders_msg.first_mod.symbol_index         = htons((uint16_t) 75);
    orders_msg.first_mod.msg_type             = htons((uint16_t) 101);
    orders_msg.first_mod.source_seq_num       = htonl((uint32_t) 2);
    orders_msg.first_mod.source_time          = htonl((uint32_t) 9099);
    orders_msg.first_mod.volume               = htonl((uint32_t) 100);
    orders_msg.first_mod.order_id             = htonl((uint32_t) 99);
    orders_msg.first_mod.price_numerator      = htonl((uint32_t) 8960);
    orders_msg.first_mod.price_scale          = (uint8_t)  2;
    orders_msg.first_mod.side                 = 'B';
    orders_msg.first_mod.exchange             = 'P';
    orders_msg.first_mod.security_type        = 'E';
    orders_msg.first_mod.firm_index           = htons((uint16_t) 5);
    orders_msg.first_mod.session_id           = (uint8_t)  7;
    //build the second add order message
    orders_msg.second_add.symbol_index        = htons((uint16_t) 76);
    orders_msg.second_add.msg_type            = htons((uint16_t) 100);
    orders_msg.second_add.source_seq_num      = htonl((uint32_t) 1);
    orders_msg.second_add.source_time         = htonl((uint32_t) 9099);
    orders_msg.second_add.volume              = htonl((uint32_t) 500);
    orders_msg.second_add.order_id            = htonl((uint32_t) 100);
    orders_msg.second_add.price_numerator     = htonl((uint32_t) 785);
    orders_msg.second_add.price_scale         = (uint8_t)  2;
    orders_msg.second_add.side                = 'S';
    orders_msg.second_add.exchange            = 'P';
    orders_msg.second_add.security_type       = 'E';
    orders_msg.second_add.firm_index          = htons((uint16_t) 6);
    orders_msg.second_add.session_id          = (uint8_t)  7;
    //build the first delete order message
    orders_msg.first_del.symbol_index         = htons((uint16_t) 75);
    orders_msg.first_del.msg_type             = htons((uint16_t) 102);
    orders_msg.first_del.source_seq_num       = htonl((uint32_t) 3);
    orders_msg.first_del.source_time          = htonl((uint32_t) 9099);
    orders_msg.first_del.order_id             = htonl((uint32_t) 99);
    orders_msg.first_del.side                 = 'B';
    orders_msg.first_del.exchange             = 'P';
    orders_msg.first_del.security_type        = 'E';
    orders_msg.first_del.session_id           = (uint8_t)  7;
    orders_msg.first_del.firm_index           = htons((uint16_t) 5);
};
/*-------------------------------------------------------------------------*/
/* build the messages for unit testing                                     */
/*-------------------------------------------------------------------------*/
void build_messages() 
{
    //  clear the space
    memset(&startofday,0,sizeof(struct seq_number_reset));
    memset(&missing_msg,0,sizeof(struct message_unavailable));
    memset(&blow_away_symbol,0,sizeof(struct symbol_clear));
    memset(&map_symbols,0,sizeof(struct symbol_mapping));
    memset(&map_firms,0,sizeof(struct firm_mapping));
    memset(&imb_refresh,0,sizeof(struct imbalance_refresh));
    memset(&orders_msg,0,sizeof(struct mixed_orders));
    memset(&invalid_msg,0,sizeof(struct symbol_clear));
    //build the message headers
    build_message_headers();
    //build the message bodies
    build_startofday();     
    build_missing_sequences();
    build_symbol_clear();
    build_mapping_symbols();
    build_mapping_firms();
    build_imbalance_refresh();
    build_orders();
    //invalid message doesnt need a body
};
/*-------------------------------------------------------------------------*/
/* test sequences in order                                                 */
/*-------------------------------------------------------------------------*/
int in_order() 
{
    int rc = 0;

    //establish starting conditions
    group->primary_expected_sequence=5;
    group->secondary_expected_sequence=5;
    set_in_sequence(group);
    init_missing(group);
    //next sequence from primary
    rc = need_2_publish(group,0,5);
    if (rc==0) 
    {
        fprintf(stderr," gap detection wrong publication action\n");
        return 1;
    }
    if (group->in_sequence==0) 
    {
        fprintf(stderr," gap changed in_sequence state\n");
        return 1;
    }
    rc = is_missing_empty(group);
    if (rc==0) 
    {
        fprintf(stderr," gap window exists when it shouldn't\n");
        return 1;
    }
    //next sequence from secondary
    rc = need_2_publish(group,1,6);
    if (rc==0) 
    {
        fprintf(stderr," gap detection wrong publication action\n");
        return 1;
    }
    if (group->in_sequence==0) 
    {
        fprintf(stderr," gap changed in_sequence state\n");
        return 1;
    }
    rc = is_missing_empty(group);
    if (rc==0) 
    {
        fprintf(stderr," gap window exists when it shouldn't\n");
        return 1;
    }
    //duplicate second sequence from primary
    rc = need_2_publish(group,0,6);
    if (rc==1) 
    {
        fprintf(stderr," gap detection wrong publication action\n");
        return 1;
    }
    if (group->in_sequence==0) 
    {
        fprintf(stderr," gap changed in_sequence state\n");
        return 1;
    }
    rc = is_missing_empty(group);
    if (rc==0) 
    {
        fprintf(stderr," gap window exists when it shouldn't\n");
        return 1;
    }
    //duplicate first sequence from secondary
    rc = need_2_publish(group,1,5);
    if (rc==1) 
    {
        fprintf(stderr," gap detection wrong publication action\n");
        return 1;
    }
    if (group->in_sequence==0) 
    {
        fprintf(stderr," gap changed in_sequence state\n");
        return 1;
    }
    rc = is_missing_empty(group);
    if (rc==0) 
    {
        fprintf(stderr," gap window exists when it shouldn't\n");
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test gap fill from primary rcv messages out of order                    */
/*-------------------------------------------------------------------------*/
int order_recovery_primary() 
{
    int rc = 0;
   
    //establish starting conditions
    group->primary_expected_sequence=5;
    group->secondary_expected_sequence=5; 
    set_in_sequence(group);
    init_missing(group);
    //establish gap of 1 sequence
    rc = need_2_publish(group,0,6);
    if (rc==0) 
    {
        fprintf(stderr," gap detection wrong publication action\n");
        return 1;
    }
    if (group->in_sequence==1) 
    {
        fprintf(stderr," gap failed to change in_sequence state\n");
        return 1;
    }
    rc = is_sequence_in_missing(group,5);
    if (rc==0)  
    {
        fprintf(stderr," gap failed to init window properly\n");
        return 1;
    }
    //fill one sequence gap from primary
    rc = need_2_publish(group,0,5);
    if (rc==0) 
    {
        fprintf(stderr," gap detection wrong publication action\n");
        return 1;
    }
    if (group->in_sequence==0) 
    {
        fprintf(stderr," gap failed to change in_sequence state\n");
        return 1;
    }
    rc = is_missing_empty(group);
    if (rc==0) 
    {
        fprintf(stderr," gap window not cleared when gap filled\n");
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test gap fill from secondary; gap primary fill from secondary           */
/*-------------------------------------------------------------------------*/
int order_recovery_secondary() 
{
    int rc=0; 

    //establish starting conditions
    group->primary_expected_sequence=5;
    group->secondary_expected_sequence=5;
    set_in_sequence(group);
    init_missing(group);
    //establish gap of 1 sequence from primary
    rc = need_2_publish(group,0,6);
    if (rc==0) 
    {
        fprintf(stderr," gap detection wrong publication action\n");
        return 1;
    }
    if (group->in_sequence==1) 
    {
        fprintf(stderr," gap failed to change in_sequence state\n");
        return 1;
    }
    rc = is_sequence_in_missing(group,5);
    if (rc==0)  
    {
        fprintf(stderr," gap failed to init window properly\n");
        return 1;
    }
    //fill gap from secondary
    rc = need_2_publish(group,1,5);
    if (rc==0) 
    {
        fprintf(stderr," gap detection wrong publication action\n");
        return 1;
    }
    if (group->in_sequence==0) 
    {
        fprintf(stderr," gap failed to change in_sequence state\n");
        return 1;
    }
    rc = is_missing_empty(group);
    if (rc==0) 
    {
        fprintf(stderr," gap window not cleared when gap filled\n");
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test 2 sequence gap fill in reverse order                               */
/*-------------------------------------------------------------------------*/
int gap_fill()
{
    int rc = 0;

    //establish starting conditions
    group->primary_expected_sequence=5;
    group->secondary_expected_sequence=5;
    set_in_sequence(group);
    init_missing(group);
    //establish gap of 2 sequence numbers
    rc = need_2_publish(group,0,7);
    if (rc==0) 
    {
        fprintf(stderr," gap detection wrong publication action\n");
        return 1;
    }
    if (group->in_sequence==1) 
    {
        fprintf(stderr," gap failed to change in_sequence state\n");
        return 1;
    }
    rc = is_sequence_in_missing(group,5);
    if (rc==0) 
    {
        fprintf(stderr," gap failed to init window properly\n");
        return 1;
    }
    rc = is_sequence_in_missing(group,6);
    if (rc==0) 
    {
        fprintf(stderr," gap failed to init window properly\n");
        return 1;
    }
    //fill tail of gap
    rc = need_2_publish(group,0,6);
    if (rc==0) 
    {
        fprintf(stderr," gap detection wrong publication action\n");
        return 1;
    }
    if (group->in_sequence==1) 
    {
        fprintf(stderr," gap fill failed in_sequence restored wrong\n");
        return 1;
    }
    rc = is_missing_empty(group);
    if (rc==1)
    {
        fprintf(stderr," gap fill failed window empty wrong\n");
        return 1;
    }
    rc = is_low_sequence_in_missing(group,5);
    if (rc==0) 
    {
        fprintf(stderr," gap fill window wrong \n");
        return 1;
    }
    //fill head of gap
    rc = need_2_publish(group,0,5);
    if (rc==0) 
    {
        fprintf(stderr," gap failed to init window properly\n");
        return 1;
    }
    if (group->in_sequence==0) 
    {
        fprintf(stderr," gap fill failed to restore in_sequence\n");
        return 1;
    }
    rc = is_missing_empty(group);
    if (rc==0)
    {
        fprintf(stderr," gap fill failed\n");
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test first gap is too large                                             */
/*-------------------------------------------------------------------------*/
int big_first_gap() 
{
    int rc = 0;

    //establish starting conditions
    int big_sequence = 0;
    group->primary_expected_sequence=5;
    group->secondary_expected_sequence=5;
    set_in_sequence(group);
    init_missing(group);
    //calculate sequence that will be too large
    big_sequence = group->primary_expected_sequence + MISSING_RANGE;
    //create gap 
    rc = need_2_publish(group,0,big_sequence);
    rc = is_missing_empty(group);
    if (rc==1) 
    {
        fprintf(stderr," big gap failed. No window established\n");
        return 1;
    }
    if (group->in_sequence==1) 
    {
        fprintf(stderr," big gap fill failed should not be in sequence\n");
        return 1;
    }
    return 0;
};
/*-------------------------------------------------------------------------*/
/* test second gap is too large                                            */
/*-------------------------------------------------------------------------*/
int big_second_gap() 
{
    int rc = 0;

    //establish starting conditions
    int big_sequence = 0;
    group->primary_expected_sequence=5;
    group->secondary_expected_sequence=5;
    set_in_sequence(group);
    init_missing(group);
    //calculate sequence that is too large
    big_sequence = group->primary_expected_sequence + MISSING_RANGE;
    //create first gap; side effect is create a window
    rc = need_2_publish(group,0,15);
    //create second gap that is too large
    rc = need_2_publish(group,1,big_sequence);
    rc = is_missing_empty(group);
    if (rc==1) 
    {
        fprintf(stderr," big gap failed. window should be established\n");
        return 1;
    }
    if (group->in_sequence==1) 
    {
        fprintf(stderr," big gap fill failed should not be in_sequence\n");
        return 1;
    }
    return 0;
};

/*! \brief Entry point for unit testing
 *
 *  \return 0 on success, non-zero on failure
 */
int unit_test_main()
{
    int rc = 0;
    
    // initialize the feed group used for unit testing
    group = new_feed_group();
    if (group==NULL) {
        fprintf(stderr,"UNIT TEST FAILED: Could not allocate a feed group\n");
        return 1;
    }
    init_mutexes(group);
    // run unit tests
    build_messages();
    rc = in_order();                             //sequences in order
    if (rc==0)
        rc = order_recovery_primary();           //out of order primary
    if (rc==0)
        rc = order_recovery_secondary();         //loss primary; fill secondary
    if (rc==0)
        rc = gap_fill();                         //test gap fill
    if (rc==0)
        rc = big_first_gap();                    //first gap window too large
    if (rc==0)
        rc = big_second_gap();                   //second gap window too large
    if (rc==0)
        rc = unit_test_sequence_reset();         // sequence number reset message
    if (rc==0) 
        rc = unit_test_missing_msgs();           // message unavailable message
    if (rc==0) 
        rc = unit_test_symbol_clear();           // symbol clear message
    if (rc==0)
        rc = unit_test_symbol_mapping();         // symbol mapping message
    if (rc==0)
        rc = unit_test_firm_mapping();           // firm mapping message
    if (rc==0)
        rc = unit_test_imbalance_refresh();      // imbalance refresh message
    if (rc==0)
        rc = unit_test_orders();                 // mix of order messages; each type
    if (rc==0)
        rc = unit_test_invalid_msg();            // invalid message type in header
    if (rc==0)
        rc = unit_test_invalid_order_type();     // invalid message type in body
    if (rc==0)
        rc = unit_test_runt_header();            // runt message header
    if (rc==0)
        rc = unit_test_runt_sequence_reset();    // runt sequence reset message
    if (rc==0)
        rc = unit_test_runt_missing_msgs();      // runt message unavailable message
    if (rc==0)
        rc = unit_test_runt_symbol_clear();      // runt symbol clear message
    if (rc==0)
        rc = unit_test_runt_symbol_mapping();    // runt symbol mapping message
    if (rc==0)
        rc = unit_test_runt_firm_mapping();      // runt firm mapping message
    if (rc==0)
        rc = unit_test_runt_imbalance_refresh(); // runt imbalance refresh message
    if (rc==0)
        rc = unit_test_runt_add_order();         // runt add order message
    if (rc==0)
        rc = unit_test_runt_mod_order();         // runt mod order message
    if (rc==0)
        rc = unit_test_runt_imbalance();         // runt imbalance message
    if (rc==0)
        rc = unit_test_runt_del_order();         // runt delete order message
    destroy_mutexes(group);
    destroy_group(group);
    
    // return last status code
    return rc;
}

/* stand alone entry point*/
int main(int argc, char* argv[]) {
    /* suppress unused parameter warnings */
    if (argc || argv){}

    unit_test_main();
    return 0;
}
