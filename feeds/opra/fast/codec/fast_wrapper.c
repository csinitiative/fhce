/**************************************************************************
*  System        :      OPRA
*
*  Module Name   : fast_wrapper.c
*  Copyright (c) : SIAC 2007
*  Date          : 3/2007
*
*  Description   : Function definitions to interact with the Fast codec.
***************************************************************************/

#include <stdio.h>
#include "fast_wrapper.h"
#include "fast_types.h"
#include "fast_api.h"

#if !FAST_OPTIMIZE
///////////////////////////////////////////////////////////////////////
//
//  void reset(Fast *fast)
//
//  wrapper for codec reset function
//
///////////////////////////////////////////////////////////////////////

void reset(Fast *fast)
{
	fast_reset_codec(fast->codec);
}

///////////////////////////////////////////////////////////////////////
//
// int getBuffer(Fast* fast, unsigned char* buf, int size) 
//
// wrapper function for fast_get_output_buffer 
//
///////////////////////////////////////////////////////////////////////

int getBuffer(Fast* fast, unsigned char* buf, int size)
{
    return fast_get_output_buffer(fast->codec, buf, size); 
}
#endif

///////////////////////////////////////////////////////////////////////
//
// void setBuffer(Fast* fast, unsigned char* buf, int size)
//
// wrapper function for fast_set_input_buffer
//
///////////////////////////////////////////////////////////////////////

void setBuffer(Fast* fast, unsigned char* buf, int size)
{
    fast_set_input_buffer(fast->codec, buf, size); 
}

///////////////////////////////////////////////////////////////////////
//
// int decode_new_msg(Fast* fast, unsigned int tag)
//
// wrapper function for fast_decode_new_msg
//
///////////////////////////////////////////////////////////////////////

int decode_new_msg(Fast* fast, unsigned int tag)
{
    return fast_decode_new_msg(fast->codec, tag); 
}

///////////////////////////////////////////////////////////////////////
//
// int decode_end_msg(Fast* fast, unsigned int tag)
//
// wrapper function for fast_decode_end_msg
//
///////////////////////////////////////////////////////////////////////

int decode_end_msg(Fast* fast, unsigned int tag)
{ 
    return fast_decode_end_msg(fast->codec, tag); 
}

///////////////////////////////////////////////////////////////////////
//
// unsigned int decode_u32(Fast* fast, unsigned int tag)
//
// wrapper function for fast_decode_u32 
//
///////////////////////////////////////////////////////////////////////

unsigned int decode_u32(Fast* fast, unsigned int tag)
{
    unsigned int temp;
    fast_decode_u32(fast->codec, tag, &temp);
	return temp;
}

///////////////////////////////////////////////////////////////////////
//
// int decode_str(Fast* fast, unsigned int tag, unsigned char* data, int size)
//
// wrapper function for fast_decode_str 
//
///////////////////////////////////////////////////////////////////////

int decode_str(Fast* fast, unsigned int tag, unsigned char* data, int size)
{
    return fast_decode_str(fast->codec, tag, data, size); 
}

///////////////////////////////////////////////////////////////////////
//
// void init_fast(Fast* fast)
//
// Initialized Fast Structure 
//
///////////////////////////////////////////////////////////////////////

void init_fast(Fast* fast)
{
   fast->codec = fast_create_codec();
   if (fast->codec)
   {
#if !FAST_OPTIMIZE
       fast->reset = &reset;
       fast->getBuffer   = &getBuffer;
#endif
       fast->setBuffer   = &setBuffer;
       fast->decode_new_msg = &decode_new_msg;
       fast->decode_end_msg = &decode_end_msg;
       fast->decode_u32  = &decode_u32;
       fast->decode_str  = &decode_str;
   } 
   else
       printf ("Unable to get memory for codec\n");
}

///////////////////////////////////////////////////////////////////////
//
// void destroy_fast(Fast* fast)
//
// Cleanup function for Fast structure 
//
///////////////////////////////////////////////////////////////////////

void destroy_fast(Fast* fast)
{
	fast_destroy_codec(fast->codec);
}
