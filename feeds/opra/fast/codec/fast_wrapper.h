/**************************************************************************
*  System        : OPRA
*
*  Module Name   : fast_wrapper.h
*  Copyright (c) : SIAC 2007
*  Date          : 3/2007
*
*  Description   : Declaration of fast_wrapper structure
***************************************************************************/

#ifndef _fastwrapper_h_
#define _fastwrapper_h_

#include "fast_api.h"

// Fast structures. Pointers to functions that interact with Fast Codec
typedef struct Fast
{
    unsigned char (*getCategory)(struct Fast* fast, unsigned int tag);
#if !FAST_OPTIMIZE
    void (*reset)(struct Fast* fast);
    int (*getBuffer)(struct Fast* fast, unsigned char* buf, int size);
#endif
    void (*setBuffer)(struct Fast* fast, unsigned char* buf, int size);
    int (*decode_new_msg)(struct Fast* fast, unsigned int tag);
    int (*decode_end_msg)(struct Fast* fast, unsigned int tag);
    unsigned char (*decode_ch)(struct Fast* fast, unsigned int tag);
    unsigned int (*decode_u32)(struct Fast* fast, unsigned int tag);
    int (*decode_str)(struct Fast* fast, unsigned int tag, unsigned char* data, int size);
    void (*verbose)(struct Fast* fast); 

    fast_codec_t* codec;
} Fast;

// initialize Fast Codec
void init_fast(Fast* fast);

// resets Fast Codec
void reset(Fast *fast);

// destroy Fast Codec
void destroy_fast(Fast* fast);


#endif
