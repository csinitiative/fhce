// $Id: fastapi.h,v 1.2 2006/02/09 19:14:24 Daniel.May Exp $
//
// FIX Adapted for STreaming (sm) (FAST Protocol (sm)) 
//
// Copyright (c) 2005-2006, Pantor Engineering AB (http://www.pantor.com)
// Copyright (c) 2005-2006, SpryWare LLC (http://www.spryware.com)
// Copyright (c) 2005-2006, FIX Protocol Ltd (http://www.fixprotocol.org)
// All Rights Reserved.
//
// This work is distributed under the W3C® Software License [1] in the
// hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS 
// FOR A PARTICULAR PURPOSE.
//
// [1] http://www.w3.org/Consortium/Legal/2002/copyright-software-20021231
// [FPL's Intellectual Property details] http://www.fixprotocol.org/ip
// [FAST Protocol details] http://www.fixprotocol.org/fast
// [FAST Protocol credits] http://fixprotocol.org/fastcredits

#ifndef _fast_api_h_
#define _fast_api_h_ 1

#include "fast_types.h"

#define FAST_OPTIMIZE (1)

#define STR_ARGS(_x) _x, sizeof (_x)

//////////////////////////////////////////////////////////////////////

typedef enum fast_op_t
{
   FAST_OP_NONE = 0,
   FAST_OP_COPY,
   FAST_OP_INCR,
   FAST_OP_DELTA,
}
fast_op_t;

typedef enum fast_type_t
{
   FAST_TYPE_NULL = 0,
   FAST_TYPE_U32,
   FAST_TYPE_I32,
   FAST_TYPE_STR,
}
fast_type_t;

typedef enum fast_error_t
{
   FAST_ERR_NONE       =  0,
   FAST_ERR_CODEC      = -1,
   FAST_ERR_SIZE       = -2,
   FAST_ERR_VALUE      = -3,
   FAST_ERR_TAG_OP     = -4,
   FAST_ERR_TAG_TYPE   = -5,
   FAST_ERR_CALL_SEQ   = -6,
   FAST_ERR_IO         = -7,
}
fast_error_t;

//////////////////////////////////////////////////////////////////////

#define TAG_MAX_SLOT     0xff // extendable to 0xfff
#define TAG_MAX_TID      0xf  // extendable to 0xfff
#define TAG_MAX_OP       0xf
#define TAG_MAX_TYPE     0xf

#define TAG_SHIFT_SLOT    0
#define TAG_SHIFT_TID    12
#define TAG_SHIFT_OP     24
#define TAG_SHIFT_TYPE   28

#define MAKE_TAG(type,op,tid,slot) \
   (((type) << TAG_SHIFT_TYPE) | ((op)   << TAG_SHIFT_OP) | \
    ((tid)  << TAG_SHIFT_TID)  | ((slot) << TAG_SHIFT_SLOT))


#define MAX_TAG 64
#define MAX_TID  4

#define MAX_PMAP_BYTES 8
#define MAX_PMAP_BITS  (7 * MAX_PMAP_BYTES)
#define MAX_MSG_SIZE  2048

typedef unsigned int fast_tag_t;


typedef struct fast_cv_t
{
   i32 i32_values [MAX_TAG];
   u32 u32_values [MAX_TAG];
   u8* str_values [MAX_TAG];
   u32 valid      [MAX_TAG];
}
fast_cv_t;


typedef struct fast_pmap_t
{
   u8  bits [MAX_PMAP_BITS];
   u32 size;
   u32 max_pos;
}
fast_pmap_t;

typedef struct fast_buffer_t
{
   int fd;

   u8* head;
   u8* tail;
   u8* end;

#if FAST_OPTIMIZE
   u8 *data;
#else
   u8  data [MAX_MSG_SIZE];
#endif
}
fast_buffer_t;

typedef struct fast_codec_error_t
{
   const char*  fn;
   char*        text;

   fast_tag_t   tag;
   fast_error_t code;
}
fast_codec_error_t;


#define FAST_CODEC_MAGIC 0xC0DEC

typedef struct fast_codec_t
{
   u32 magic;

   const char* name;

   fast_pmap_t pmap [1];

#if !FAST_OPTIMIZE
   fast_buffer_t msg    [1];
   fast_buffer_t output [1];
#endif
   fast_buffer_t input  [1];

   fast_cv_t cv [TAG_MAX_TID];

   // Config variables
   int count;
   int skip_io;
   int verbose;

   // State variables
   int curr_tag;
   int in_message;

   fast_codec_error_t error [1];
}
fast_codec_t;


#ifdef __cplusplus
extern "C" {
#endif 

/**
 * @defgroup fastapi fastapi functions
 * @{
 */
/**
*	reset codec buffers
**/
fast_codec_t* fast_reset_codec (fast_codec_t *codec);

/**
*	@brief Create and initialize the codec
*
*	@return	A pointer to opaque type fast_codec_t
*   @remark This function will allocate a segment of memory on your behalf to manage the
*          codec state information.  The pointer returned should be treated as an opaque 
*          handle, it is a required parameter to most other fastapi functions.
*/
fast_codec_t* fast_create_codec (void);

/**
*	@brief Destroy the codec and release associated memory
*
*  @param	codec Pointer to a fast_codec_t
*  @remark  This function will free the segment of memory on your behalf originally allocated by the
*          fast_create_codec() function. The codec is invalid upon return.
*/
int  fast_destroy_codec    (fast_codec_t* codec);

/**
*	@brief Reset the codec state for a specific tag
*
*	@param	codec Pointer to a fast_codec_t
*	@param	tag The fast_codec_tag we are reseting
*   @remark This function will reset the state of the codec for the specific tag.
*/
void fast_reset_state      (fast_codec_t* codec, fast_tag_t tag);

/**
*	@brief Set the input FILE stream for a codec
*
*	@param	codec Pointer to a fast_codec_t
*	@param	fptr FILE Pointer to a FILE stream
*   @remark The default input stream in stdin
*/
int  fast_set_codec_input  (fast_codec_t* codec, FILE* fptr);

/**
*	@brief Set the output FILE stream for a codec
*
*	@param	codec Pointer to a fast_codec_t
*	@param	fptr FILE Pointer to a FILE stream
*   @remark The default output stream in stdout
*/
int  fast_set_codec_output (fast_codec_t* codec, FILE* fptr);

/**
*	@brief Make a FAST tag
*/
fast_tag_t fast_make_tag   (fast_op_t, fast_type_t, u32 tid, u32 slot);

/**
*	assign input buffer for decoding
**/
void fast_set_input_buffer(fast_codec_t* codec, u8* buf, int size);

/**
*	@brief Decode the first tag of a new message
*
*	@param	codec Pointer to a fast_codec_t
*	@param	tag The fast_codec_tag we are decoding
*  @remark This function must be called to decode the first tag of a new message.  
*          The fast_codec_tag should be the template ID
*/
int fast_decode_new_msg    (fast_codec_t* codec, fast_tag_t tag);

/**
*	@brief Complete the decoding of a message
*
*	@param	codec Pointer to a fast_codec_t
*	@param	tag The fast_codec_tag we are decoding
*  @remark This function must be called after the last tag of a message has been decoded  
*/
int fast_decode_end_msg    (fast_codec_t* codec, fast_tag_t tag);

/**
*	@brief Decode a 32-bit signed integer
*
*	@param	codec Pointer to a fast_codec_t
*	@param	tag The fast_codec_tag we are decoding
*	@param	data Pointer to a i32 that will contain the result
*/
int fast_decode_i32        (fast_codec_t* codec, fast_tag_t tag, i32* data);

/**
*	@brief Decode a 32-bit unsigned integer
*
*	@param	codec Pointer to a fast_codec_t
*	@param	tag The fast_codec_tag we are decoding
*	@param	data Pointer to a u32 that will contain the result
*/
int fast_decode_u32        (fast_codec_t* codec, fast_tag_t tag, u32* data);

/**
*	@brief Decode an ASCII String
*
*	@param	codec Pointer to a fast_codec_t
*	@param	tag The fast_codec_tag we are decoding
*	@param	data Pointer to a char * that will contain the result
*	@param	size The maximum number of chars data can hold
*  @remark The resulting sting will NOT be NULL terminated, this function will only
*          fill the string with char's decoded from the input. 
*/
int fast_decode_str        (fast_codec_t* codec, fast_tag_t tag, u8* data, int size);

/**
*	get encoded output buffer
**/
int fast_get_output_buffer(fast_codec_t* codec, u8* buf, int size);


/**
*	@brief Encode the first tag of a new message
*
*	@param	codec Pointer to a fast_codec_t
*	@param	tag The fast_codec_tag we are decoding
*  @remark This function must be called to encode the first tag of a new message.  
*          The fast_codec_tag should be the template ID
*/
int fast_encode_new_msg    (fast_codec_t* codec, fast_tag_t tag);

/**
*	@brief Complete the encoding of a message
*
*	@param	codec Pointer to a fast_codec_t
*	@param	tag The fast_codec_tag we are encoding
*  @remark This function must be called after the last tag of a message has been encoded  
*/
int fast_encode_end_msg    (fast_codec_t* codec, fast_tag_t tag);

/**
*	@brief Encode a 32-bit signed integer
*
*	@param	codec Pointer to a fast_codec_t
*	@param	tag The fast_codec_tag we are decoding
*	@param	data i32 integer containing the value to encode
*/
int fast_encode_i32        (fast_codec_t* codec, fast_tag_t tag, i32 data);

/**
*	@brief Encode a 32-bit unsigned integer
*
*	@param	codec Pointer to a fast_codec_t
*	@param	tag The fast_codec_tag we are decoding
*	@param	data u32 integer containing the value to encode
*/
int fast_encode_u32        (fast_codec_t* codec, fast_tag_t tag, u32 data);

/**
*	@brief Encode an ASCII String
*
*	@param	codec Pointer to a fast_codec_t
*	@param	tag The fast_codec_tag we are decoding
*	@param	data char * containing the value to encode
*	@param	size The number of characters to encode
*  @remark This function will encode exactly size number of char's, it will not 
*          not assume a NULL terminated string.
*/
int fast_encode_str        (fast_codec_t* codec, fast_tag_t tag, u8* data, int size);

/**
*	@brief Format and print the last reported codec error to a human readable string to a FILE stream
*
*   @param	codec Pointer to a fast_codec_t
*   @param	fptr  FILE stream
*   @returns The error string
*/
int fast_print_error       (fast_codec_t* codec, FILE* fptr);

/**
*	@brief Convert the last reported codec error to a human readable string
*
*   @param	codec Pointer to a fast_codec_t
*   @returns The error string
*/
const char* fast_error_string (fast_codec_t* codec);

/**
*	@brief Convert a u8 byte arrray of characters to a u32
*
*	@param	 data Pointer to a u8 containing ASCII number
*	@param	 size Lenght of data
*  @returns The converted u32 number
*  @remark This function will convert an ASCII string of numbers to a binary value
*          All characters must be in range of '0' - '9'
*/
u32 fast_ascii_to_u32 (u8* data, int size);

/** @} */
#ifdef __cplusplus
}
#endif

#endif // _fast_api_h_
