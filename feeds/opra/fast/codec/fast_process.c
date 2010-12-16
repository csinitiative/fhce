/*****************************************************************************
*  System        : OPRA
*
*  Module Name   : fast_process.c
*  Copyright (c) : SIAC 2007
*  Date          : 3/2007
*
*  Description	 : This file contains the definition of the main decode 
*                  function. The function reads the category of the message
*                  and calls appropriate decoding functions.     
******************************************************************************/

#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include "fast_process.h"
#include "fast_decode.h"
#include "fast_opra.h"
#include "fh_log.h"

/*
 * Decoder index-table based on category for performance reasons.
 */
typedef int (FastOpra_decode_t)(Fast* fast, char * decoded_msg, OpraMsg_v2 *opra_msg);

#define NUM_OF_DECODE_FUNCTIONS  ('k' - 'A' + 1)

static FastOpra_decode_t*  FastOpra_msg_ops[NUM_OF_DECODE_FUNCTIONS];

/*
 * FastOpraDecoder
 *
 * This calls the correct decoder function based on the message category.
 */
static inline int FastOpraDecoder(Fast * fast, char * decoded_msg, char category, OpraMsg_v2 * msg)
{
    FastOpra_decode_t* decode_msg = NULL;

    if ((category >= 'C') && (category <= 'k')) {
        decode_msg = FastOpra_msg_ops[category - 'A'];
    }

    if (decode_msg == NULL) {
        return decode_OpraFastDefMsg_v2(fast, decoded_msg, msg);
    }
    else {
        return decode_msg(fast, decoded_msg, msg);
    }
}

/*
 * fast_opra_init
 *
 * Initialize the decoder callback functions, and also initialize
 * the fast decoder context.
 */
void fast_opra_init(Fast *fast)
{
    /*
     * Initialize the decoder function pointers for all message
     * categories.
     */
    memset(FastOpra_msg_ops, 0 , sizeof(FastOpra_msg_ops));

    FastOpra_msg_ops['k' - 'A'] = decode_OpraFastQuoteSizeMsg_v2;
    FastOpra_msg_ops['d' - 'A'] = decode_OpraFastOpenIntMsg_v2;
    FastOpra_msg_ops['a' - 'A'] = decode_OpraFastLastSaleMsg_v2;
    FastOpra_msg_ops['f' - 'A'] = decode_OpraFastEodMsg_v2;
    FastOpra_msg_ops['O' - 'A'] = decode_OpraFastFcoLastSaleMsg_v2;
    FastOpra_msg_ops['U' - 'A'] = decode_OpraFastFcoQuoteMsg_v2;
    FastOpra_msg_ops['F' - 'A'] = decode_OpraFastFcoEodMsg_v2;
    FastOpra_msg_ops['Y' - 'A'] = decode_OpraFastUlValueMsg_v2;
    FastOpra_msg_ops['C' - 'A'] = decode_OpraFastAdminMsg_v2;
    FastOpra_msg_ops['H' - 'A'] = decode_OpraFastControlMsg_v2;

    /*
     * Initialize the Fast context
     */
    init_fast(fast);
}


/*
 * fast_opra_hdr_info
 *
 * Decode the message header, and returns the category and the message sequence
 * number.
 */
int fast_opra_hdr_info(Fast *fast, uint8_t *buffer, uint32_t len, char *msg_cat, char *msg_type, uint32_t *msg_sn, uint32_t *time)
{
    int acq_size;

    /*
     * If the size of the message is 0xff, it means that it is the last
     * message of the packet, and therefore, we can read to the end of the
     * packet.
     */
    acq_size = buffer[15];
    if (acq_size == 0xff) {
        fast->setBuffer(fast, (uint8_t *) (buffer+16), len);
    }
    else if ((acq_size > 0) && (acq_size < 0xff)) {
        fast->setBuffer(fast, (uint8_t *) (buffer+16), acq_size);
    }
    else {
        return -1;
    }

    if (fast->decode_new_msg(fast, OPRA_BASE_TID) < 0) {
        return -1;
    }

    // We need to read even the fields we are not insterested in to access the
    // message sequence number
    *msg_cat  = (char) fast->decode_u32(fast, MESSAGE_CATEGORY_V2);
    *msg_type = (char) fast->decode_u32(fast, MESSAGE_TYPE_V2);

    if (msg_sn) {
        (void) fast->decode_u32(fast, PARTICIPANT_ID_V2); 
        (void) fast->decode_u32(fast, RETRANSMISSION_REQUESTER_V2);
        *msg_sn   = (uint32_t) fast->decode_u32(fast, MESSAGE_SEQUENCE_NUMBER_V2); 
    }
    if (time) {
        (void) fast->decode_u32(fast, PARTICIPANT_ID_V2); 
        (void) fast->decode_u32(fast, RETRANSMISSION_REQUESTER_V2);
        (void) fast->decode_u32(fast, MESSAGE_SEQUENCE_NUMBER_V2); 
        *time  = (uint32_t) fast->decode_u32(fast, TIME_V2);
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//
//  Reads the category for each encoded message and calls the appropriate
//  decoding function for that message. The decoded message is then appended 
//  to the decoded packet. 
//
///////////////////////////////////////////////////////////////////////////

int fast_opra_decode(Fast* fast, uint8_t* encoded_packet, uint32_t rec_len)
{
    int            esize     = 0;
    char           category  = 0;
    //char           soh       = 0;
    int            read_len  = rec_len;
    unsigned char  msize     = 0;
    OpraMsg_v2     msg;
    int            num_msgs  = 0;
    ushort         version   = VERSION_1; // default version
    unsigned char  offset    = 0;
    char           decoded_msg[PACKET_SIZE+1]  = {'\0'};
    //unsigned int   packet_len = 0;

    // Strip off the SOH from Encoded packet.
    encoded_packet++;

    // Add SOH as the first byte of the decoded packet and
    // then decode each message from encoded packet and append
    // to the decoded packet to get the final ascii OPRA packet
	read_len--;	// read SOH

	//Note: As in version 1, the 1st byte is msg length which is
	// 		never less than 10
	//get the version number from 2nd byte of the packet
    if(encoded_packet[offset] == VERSION_2)
    {
        //get the version
        version = encoded_packet[offset];

        //change the offset if version is equal to versionSION_2
		offset = SEQ_OFFSET + VERSION_OFFSET + NUM_MSG_OFFSET;
    }

    FH_ASSERT(version == VERSION_2);

    // Be in loop till all messages are read
    while(read_len > 1) //last byte is ETX
    {
        // Each message has 1 bytes for size at the begining 
        // of the message
	    msize = *(encoded_packet + offset);
    	encoded_packet += offset + LEN_OFFSET;

        // read msg len byte
	    read_len = read_len - offset - LEN_OFFSET;  

        //reset the offset to ZERO.
        offset=0;

        // if the size field contains 0xFF then it is the last 
        // message in the packet so decode till the end of packet
        // i.e. ETX else decode for the message size 
		if (msize == 0xFF)
    		fast->setBuffer(fast, encoded_packet, rec_len);
		else
    		// initialize the fast buffer with encoded packet 
    		fast->setBuffer(fast, encoded_packet, msize);

        // Test if more messages are pending in the encoded packet
        if(fast->decode_new_msg(fast, OPRA_BASE_TID) < 0)
            break;

        // Read the category and call appropriate decoding function 
        category = fast->decode_u32(fast, MESSAGE_CATEGORY_V2);

        esize = FastOpraDecoder(fast, &decoded_msg[0], category, &msg);

        num_msgs++;

        if (fast->decode_end_msg(fast, OPRA_BASE_TID) < 0)
            break;
 
        //fprintf(stderr,"end ..\n");
        encoded_packet += msize;
        
        // length of encoded packet left to be decoded
        read_len -= msize;
    }
    // reset the fast buffer 
#if !FAST_OPTIMIZE
    fast->reset(fast);
#endif

    return(num_msgs);
}


