/**************************************************************************
*  System        : OPRA
*
*  Module Name   : fast_process.h
*  Copyright (c) : SIAC 2007
*  Date          : 3/2007
*
*  Description   : Header for decode API
***************************************************************************/

#ifndef _fast_process_h_
#define _fast_process_h_

#include "fast_wrapper.h"

#define PACKET_SIZE  10000      // Opra Packet Size
#define US           0x1F       // Unit Separator
#define SOH          0x01       // Start of Header
#define ETX          0x03       // End of Text

// offsets for V2 messages
#define SEQ_OFFSET 		10
#define VERSION_OFFSET 	1
#define NUM_MSG_OFFSET 	3

// offset for V1 messages
#define LEN_OFFSET 		1

#define VERSION_2 		2
#define VERSION_1 		1

// Initialize OPRA Fast context and message decoders
void fast_opra_init(Fast *fast);

// Prepare the decoding process by initializing the Fast context
int  fast_opra_prepare(Fast *fast, uint8_t *buffer, int len);

// Decode fast opra messages
int  fast_opra_decode(Fast* fast, uint8_t* encoded_packet, uint32_t rec_len);

// Retrieve the message category and sequence number
int  fast_opra_hdr_info(Fast *fast, uint8_t *buffer, uint32_t len, char *msg_cat,
                        char *msg_type, uint32_t *msg_sn, uint32_t *msg_time);

#endif // _fast_process_h_
