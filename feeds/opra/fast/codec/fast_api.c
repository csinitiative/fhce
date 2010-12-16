// $Id: fastapi.c,v 1.2 2006/02/09 19:14:24 Daniel.May Exp $
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

#include "fast_api.h"
#include "fast_common.h"

#if !FAST_OPTIMIZE
//////////////////////////////////////////////////////////////////////

static void reset_buffer (fast_buffer_t* buffer)
{
   buffer->head = buffer->data;
   buffer->tail = buffer->data;
   buffer->end  = buffer->data + sizeof (buffer->data);
}

static void init_buffer (fast_buffer_t* buffer, int fd)
{
   reset_buffer (buffer);

   buffer->fd = fd;
}
#endif

static  int buffer_empty_p (fast_buffer_t* buffer)
{
   return buffer->head >= buffer->tail;
}

#if !FAST_OPTIMIZE
static  int get_buffer_left (fast_buffer_t* buffer)
{
   return buffer->end - buffer->tail;
}

#if 0
static  int get_buffer_size (fast_buffer_t* buffer)
{
  return buffer->end - buffer->data;
}
#endif

static  int get_buffer_used (fast_buffer_t* buffer)
{
   return buffer->tail - buffer->head;
}
#endif

//////////////////////////////////////////////////////////////////////

static  u32 get_tag_op (u32 tag)
{
   return (tag >> TAG_SHIFT_OP) & TAG_MAX_OP;
}

static  u32 get_tag_slot (u32 tag)
{
   return (tag >> TAG_SHIFT_SLOT) & TAG_MAX_SLOT;
}

static  u32 get_tag_tid (u32 tag)
{
   return (tag >> TAG_SHIFT_TID) & TAG_MAX_TID;
}

static  u32 get_tag_type (fast_tag_t tag)
{
   return (tag >> TAG_SHIFT_TYPE) & TAG_MAX_TYPE;
}

static  fast_cv_t* cv_get_tag_state (fast_codec_t* codec, u32 tag)
{
   u32 slot = get_tag_tid (tag);

   return & codec->cv [slot];
}

static  i32* cv_get_i32_values (fast_codec_t* codec, u32 tag)
{
   return cv_get_tag_state (codec, tag)->i32_values;
}

static  u32* cv_get_u32_values (fast_codec_t* codec, u32 tag)
{
   return cv_get_tag_state (codec, tag)->u32_values;
}

static  u8** cv_get_str_values (fast_codec_t* codec, u32 tag)
{
   return cv_get_tag_state (codec, tag)->str_values;
}

static  u32* cv_get_valid_values (fast_codec_t* codec, u32 tag)
{
   return cv_get_tag_state (codec, tag)->valid;
}

//////////////////////////////////////////////////////////////////////

static  i32 cv_get_i32 (fast_codec_t* codec, u32 tag)
{
   return cv_get_i32_values (codec, tag) [get_tag_slot (tag)];
}

static  u32 cv_get_u32 (fast_codec_t* codec, u32 tag)
{
   return cv_get_u32_values (codec, tag) [get_tag_slot (tag)];
}

#if FAST_OPTIMIZE
static u8 fast_strings[TAG_MAX_SLOT][1024];
#endif

static  u8* cv_get_str (fast_codec_t* codec, u32 tag)
{
   u8** str_values = cv_get_str_values (codec, tag);
   u32 slot = get_tag_slot (tag);

   if (str_values [slot] == NULL)
   {
#if FAST_OPTIMIZE
      str_values [slot] = fast_strings[slot];
#else
      str_values [slot] = (u8 *)malloc (1024);
      assert (str_values [slot] != NULL);
#endif
      *(str_values [slot]) = 0;
   }

   return str_values [slot];
}

//////////////////////////////////////////////////////////////////////

static  void cv_set_valid (fast_codec_t* codec, u32 tag)
{
   cv_get_valid_values (codec, tag) [get_tag_slot (tag)] = 1;
}

static  int cv_is_valid (fast_codec_t* codec, u32 tag)
{
   return cv_get_valid_values (codec, tag) [get_tag_slot (tag)] != 0;
}

//////////////////////////////////////////////////////////////////////

#if !FAST_OPTIMIZE
static  int cv_eq_i32 (fast_codec_t* codec, u32 tag, i32 data)
{
   return cv_is_valid (codec, tag) && cv_get_i32 (codec, tag) == data;
}

static  int cv_eq_u32 (fast_codec_t* codec, u32 tag, u32 data)
{
   return cv_is_valid (codec, tag) && cv_get_u32 (codec, tag) == data;
}

static  int cv_eq_str (fast_codec_t* codec, u32 tag, u8* data, int size)
{
   u8* cp = cv_get_str (codec, tag);

   // Invalid or not the same length
   // Fixme: cp [size] != '\0' not a valid test?
   //if (cv_is_valid (codec, tag) == 0 || cp [size] != '\0')
   if (cv_is_valid (codec, tag) == 0 || !strncmp( (char *)cp , (char*) data, size))
      return 0;

   return memcmp (cp, data, size) == 0;
}
#endif

//////////////////////////////////////////////////////////////////////

//! @brief Set the current signed integer value for the specified tag

static  void cv_set_i32 (fast_codec_t* codec, u32 tag, i32 data)
{
   cv_get_i32_values (codec, tag) [get_tag_slot (tag)] = data;
   cv_set_valid (codec, tag);
}

//! @brief Set the current unsigned integer value for the specified tag

static  void cv_set_u32 (fast_codec_t* codec, u32 tag, u32 data)
{
   cv_get_u32_values (codec, tag) [get_tag_slot (tag)] = data;
   cv_set_valid (codec, tag);
}

//! @brief Set the current string value for the specified tag

static  void cv_set_str (fast_codec_t* codec, u32 tag,
                               u8* data, int size)
{
   u8* cp = cv_get_str (codec, tag);

   memcpy (cp, data, size);
   cp [size] = '\0';

   cv_set_valid (codec, tag);
}
//////////////////////////////////////////////////////////////////////

#if !FAST_OPTIMIZE
static int u32_to_size (u32 data)
{
   if (data < 0x00000080) return 1; // 128
   if (data < 0x00004000) return 2; // 16384
   if (data < 0x00200000) return 3; // 2097152
   if (data < 0x10000000) return 4; // 268435456

   return 5;
}

static int i32_to_size (i32 data)
{
   if (data < 0)
   {
      if (data >= (i32)0xffffffc0) return 1; // -64
      if (data >= (i32)0xffffe000) return 2; // -8192
      if (data >= (i32)0xfff00000) return 3; // -1048576
      if (data >= (i32)0xf8000000) return 4; // -124317728
   }
   else
   {
      if (data <  (i32)0x00000040) return 1; // 64
      if (data <  (i32)0x00002000) return 2; // 8192
      if (data <  (i32)0x00100000) return 3; // 1048576
      if (data <  (i32)0x08000000) return 4; // 134217728
   }
   return 5;
}
#endif

//////////////////////////////////////////////////////////////////////

static const char* format_error_code (fast_error_t code)
{
   switch (code)
   {
    case FAST_ERR_NONE:     return "none";
    case FAST_ERR_CODEC:    return "bad codec";
    case FAST_ERR_SIZE:     return "size error";
    case FAST_ERR_VALUE:    return "missing value";
    case FAST_ERR_TAG_OP:   return "tag op unsupported";
    case FAST_ERR_TAG_TYPE: return "bad tag type";
    case FAST_ERR_CALL_SEQ: return "bad call sequence";
    case FAST_ERR_IO:       return "I/O library error";
    default:                return "??";
   }
}

static const char* format_tag_op (fast_op_t op)
{
   static char buffer [32];

   switch (op)
   {
    case FAST_OP_NONE:  return "NONE";
    case FAST_OP_COPY:  return "COPY";
    case FAST_OP_INCR:  return "INCR";
    case FAST_OP_DELTA: return "DELTA";

    default:
      snprintf (buffer, sizeof (buffer), "%u", op);
      return buffer;
   }
}

static const char* format_tag_type (fast_type_t type)
{
   static char buffer [32];

   switch (type)
   {
    case FAST_TYPE_NULL: return "NULL";
    case FAST_TYPE_U32:  return "U32";
    case FAST_TYPE_I32:  return "I32";
    case FAST_TYPE_STR:  return "STR";

    default:
      snprintf (buffer, sizeof (buffer), "%u", type);
      return buffer;
   }
}

//////////////////////////////////////////////////////////////////////

static int set_err (const char* fn, fast_codec_t* codec,
                     int tag, fast_error_t code,
                     const char* format, ...)
{
   char buffer [1024];
   va_list ap;

   codec->error->fn   = fn;
   codec->error->tag  = tag;
   codec->error->code = code;

   va_start (ap, format);
   vsnprintf (buffer, sizeof (buffer), format, ap);
   va_end   (ap);

   if (codec->error->text != NULL)
      free (codec->error->text);

   codec->error->text = (char *)malloc (strlen (buffer) + 1);
   strcpy (codec->error->text, buffer);

   if (codec->verbose >= 1)
        fprintf (stderr, "error: %s\n", fast_error_string (codec));

   return -1;
}

#if FAST_OPTIMIZE
#define check_codec(fn, codec) (codec != NULL)
#define check_type(fn, codec, tag, type) (codec != NULL && tag && type)
#else
static int check_codec (const char* fn, fast_codec_t* codec)
{
   if (codec == NULL)
   {
        //fprintf (stderr, "error: [%s] NULL codec\n", fn);
      return -1;
   }
   if (codec->magic != FAST_CODEC_MAGIC)
      return set_err (fn, codec, -1, FAST_ERR_CODEC,
                      "bad codec magic number");

   return 0;
}

static int check_type (const char* fn, fast_codec_t* codec,
                       fast_tag_t tag, u32 type)
{
   if (get_tag_type (tag) != type)
      return set_err (fn, codec, tag, FAST_ERR_TAG_TYPE,
                      "type=%s (%s expected)",
                      format_tag_type ((fast_type_t)get_tag_type (tag)),
                      format_tag_type ( (fast_type_t)type));

   return 0;
}
#endif

static int bad_op_error (const char* fn, fast_codec_t* codec, fast_tag_t tag)
{
   return set_err (fn, codec, tag, FAST_ERR_TAG_OP, "");
}
                                
static int value_error (const char* fn, fast_codec_t* codec, fast_tag_t tag)
{
   return set_err (fn, codec, tag, FAST_ERR_VALUE, "");
}

//! @brief Generate an error string using the current error info

const char* fast_error_string (fast_codec_t* codec)
{
   fast_tag_t tag = codec->error->tag;

   static char buffer [1024];

   snprintf (buffer, sizeof (buffer), "[%s] %s|%s|%u|%u %s: %s",
             codec->error->fn,
             format_tag_type ( (fast_type_t)get_tag_type (tag)),
             format_tag_op ((fast_op_t)get_tag_op (tag)),
             get_tag_tid (tag),
             get_tag_slot (tag),
             format_error_code (codec->error->code),
             codec->error->text ? codec->error->text : "");

   return buffer;
}

int fast_print_error (fast_codec_t* codec, FILE* fp)
{
   const char* msg = fast_error_string (codec);

   fprintf (fp, "%s\n", msg);
   return 0;
}

//////////////////////////////////////////////////////////////////////

//! @brief Get a presence bit from the current presence map

static int get_pmap (fast_codec_t* codec, fast_tag_t tag)
{
   u32 pos = get_tag_slot (tag);

   // Optimization: Don't range check pos separately
   // We only need to check for the current size.
   if (pos >= codec->pmap->size)
      return 0;

   return codec->pmap->bits [pos];
}

#if !FAST_OPTIMIZE
//! @brief Reset the current presence map

static void reset_pmap (fast_codec_t* codec)
{
   // Fixme: Consider memset'ing to max_pos.
   memset (codec->pmap->bits, 0, MAX_PMAP_BITS);
   codec->pmap->max_pos = 0;
}

//! @brief Set a presence bit in the current presence map

static int set_pmap (fast_codec_t* codec, fast_tag_t tag)
{
   u32 pos = get_tag_slot (tag);

   if (pos > codec->pmap->max_pos)
   {
      // Optimization: Only range check pos if pos
      // is larger than the current max_pos
      if (pos >= MAX_PMAP_BITS)
         return set_err (FUNCTION, codec, tag, FAST_ERR_SIZE,
                         "pmap slot=%d (< %d expected)",
                         pos, MAX_PMAP_BITS);

      codec->pmap->max_pos = pos;
   }
   codec->pmap->bits [pos] = 1;
   return 0;
}
#endif

static u32 bit_mask [8] = { 64, 32, 16, 8, 4, 2, 1, 0 };

#if !FAST_OPTIMIZE
// emit_pmap copies the pmap into the data buffer
// and returns the size of the pmap

//! Serialize the current presence map

static int emit_pmap (fast_codec_t* codec, u8* data)
{
   int bytes = align7 (codec->pmap->max_pos + 1); // get map size in bytes
   int offset;
   int p1;

   // pack slots from map->bits (seven at a time) into
   // data to form a STOP bit coded field.

   for (p1 = 0, offset = 0 ; p1 < bytes ; p1 ++, offset += 7)
   {
      u8* bits = & codec->pmap->bits [offset];
      u32 temp = 0;
      int p2;

      for (p2 = 0 ; p2 < 7 ; p2 ++) // pack 7 slot into temp
      {
         if (bits [p2] > 0)
            temp |= bit_mask [p2];
      }

      data [p1] = temp;
   }

   data [bytes - 1] |= 0x80; // Set the STOP bit
   return bytes;
}

//////////////////////////////////////////////////////////////////////

//! @brief Append a byte to the message buffer

static  void emit_byte (fast_codec_t* codec, u8 data)
{
   *codec->msg->tail ++ = data;
}

//////////////////////////////////////////////////////////////////////

//! @brief Verify that there is enough room in the output buffer
//! to store size bytes of data into the buffer.

static   int check_msg_avail (const char* fn, fast_codec_t* codec,
                                    fast_tag_t tag, int size)
{
   if (get_buffer_left (codec->msg) < size)
      return set_err (fn, codec, tag, FAST_ERR_SIZE,
                      "message buffer overflow avail=%d needed=%d",
                      get_buffer_left (codec->msg), size);
   return 0;
}

//! @callgraph

//! @brief Serialize a 32-bit signed integer field

static int emit_i32 (fast_codec_t* codec, fast_tag_t tag, i32 data)
{
   int size = i32_to_size (data);

   // Check buffer space once, not for every byte
   if (check_msg_avail (FUNCTION, codec, tag, size) < 0)
      return -1;

   switch (size)
   {
      // Shifts are arithmetic (signed)
      // The sign bit of data will be copied on right shifts
    case 5: emit_byte (codec, (data >> 28) & 0x7f);
    case 4: emit_byte (codec, (data >> 21) & 0x7f);
    case 3: emit_byte (codec, (data >> 14) & 0x7f);
    case 2: emit_byte (codec, (data >>  7) & 0x7f);
    case 1: emit_byte (codec, (data & 0x7f) | 0x80);
      {
         int code = set_pmap (codec, tag);

         if (code < 0)
            return code;
      }
      return size;

    default:
      return set_err (FUNCTION, codec, tag, FAST_ERR_SIZE,
                      "size=%d data=%d (%08x)", size, data, data);
   }
}

//! @brief Serielize a 32-bit unsigned integer field

static int emit_u32 (fast_codec_t* codec, fast_tag_t tag, u32 data)
{
   int size = u32_to_size (data);

   // Check buffer space once, not for every byte
   if (check_msg_avail (FUNCTION, codec, tag, size) < 0)
      return -1;

   switch (size)
   {
      // Shifts are logical (unsigned)
    case 5: emit_byte (codec,  data >> 28);
    case 4: emit_byte (codec, (data >> 21) & 0x7f);
    case 3: emit_byte (codec, (data >> 14) & 0x7f);
    case 2: emit_byte (codec, (data >>  7) & 0x7f);
    case 1: emit_byte (codec, (data & 0x7f) | 0x80);
      {
         int code = set_pmap (codec, tag);

         if (code < 0)
            return code;
      }
      return size;

    default:
      return set_err (FUNCTION, codec, tag, FAST_ERR_SIZE,
                      "size=%d data=%d (%08x)", size, data, data);
   }
}

//! @brief Serialize an ASCII string field

static int emit_str (fast_codec_t* codec, fast_tag_t tag, u8* data, int size)
{
   // Zero length strings cannot be encoded
   if (size <= 0)
      return set_err (FUNCTION, codec, tag, FAST_ERR_SIZE,
                      "size=%d (>0 expected)", size);

   // Check buffer space once, not for every byte
   if (check_msg_avail (FUNCTION, codec, tag, size) < 0)
      return -1;

   {
      int p1;

      for (p1 = 0 ; p1 < size - 1 ; p1 ++)
         emit_byte (codec, data [p1]);

      emit_byte (codec, data [p1] | 0x80); // Last byte
   }

   {
      int code = set_pmap (codec, tag);

      if (code < 0)
         return code;
   }
   return size;
}
#endif

//////////////////////////////////////////////////////////////////////

//! @brief Deserialize an SBIT coded field

static int parse_bytes (fast_codec_t* codec, u8* data, int size)
{
   int p1;

   // Fixme: Filler may be other than space?
#if !FAST_OPTIMIZE
   memset (data, 0, size);
#endif

   for (p1 = 0 ; p1 < size ; p1 ++)
   {
      u8 temp;

      fast_buffer_t* input = codec->input;

      if (buffer_empty_p (input))
      {
         return -1;
      }

      temp = *input->head ++;

      data [p1] = temp;

      if (temp >= 0x80)
      {
         data [p1] &= 0x7f;
         return p1 + 1;
      }
   }

   // Fixme: calculate the real number of bytes in the field.

   return set_err (FUNCTION, codec, -1, FAST_ERR_SIZE,
                   "parse buffer overflow size=%d", size);
}

static int parse_pmap (fast_codec_t* codec)
{
   fast_pmap_t* pmap = codec->pmap;

   u8  data [MAX_PMAP_BYTES];

   int bytes = parse_bytes (codec, data, MAX_PMAP_BYTES);
   int offset;
   int p1;

   if (bytes < 0)
      return bytes;

   for (p1 = 0, offset = 0 ; p1 < bytes ; p1 ++, offset += 7)
   {
      u32 temp = data [p1];
      int p2;

      for (p2 = 0 ; p2 < 7 ; p2 ++)
      {
         pmap->bits [offset + p2] = (temp & bit_mask [p2]) != 0;
      }
   }

   codec->pmap->size = 7 * bytes;
   return bytes;
}

#define SIGN_MASK_I32 0x40
#define SIGN_MASK_U32 0x00

static int parse_32 (fast_codec_t* codec, fast_tag_t tag,
                     u32* data, int sign_mask)
{
   u8  buffer [5];
   int bytes;
   int p1;
   i32 temp;

   if (get_pmap (codec, tag) == 0)
      return 0;

   bytes = parse_bytes (codec, buffer, sizeof (buffer));

   if (bytes < 0)
      return bytes;

   temp = 0 - ((buffer [0] & sign_mask) != 0);

   for (p1 = 0 ; p1 < bytes ; p1 ++)
      temp = (temp << 7) | buffer [p1];

   *data = temp;
   return bytes;
}

//////////////////////////////////////////////////////////////////////

//! @brief Deserialize a 32-bit unsigned integer

static int parse_u32 (fast_codec_t* codec, fast_tag_t tag, u32* data)
{
   return parse_32 (codec, tag, data, SIGN_MASK_U32);
}

//! @brief Deserialize a 32-bit signed integer

static int parse_i32 (fast_codec_t* codec, fast_tag_t tag, i32* data)
{
   return parse_32 (codec, tag, (u32*) data, SIGN_MASK_I32);
}

//! @brief Deserialize an ASCII string

static int parse_str (fast_codec_t* codec, fast_tag_t tag, u8* data, int size)
{
   return get_pmap (codec, tag) == 0 ? 0 : parse_bytes (codec, data, size);
}

//////////////////////////////////////////////////////////////////////
// Decoder functions

//! @brief Start decoding a message
//! Parse the presence map

void fast_set_input_buffer(fast_codec_t* codec, u8* buf, int size)
{
    // copy encoded msg to input buffer
    fast_buffer_t* in = codec->input;
#if FAST_OPTIMIZE
    in->data = buf;
#else
    memcpy(in->data, buf, size);
#endif
    in->head = in->data;
    in->tail = in->head + size;
}

int fast_decode_new_msg (fast_codec_t* codec, fast_tag_t tag)
{
   tag = 0;
   if (check_codec (FUNCTION, codec) < 0)
      return -1;

   return parse_pmap (codec);
}

//! @brief Finish decoding a message

int fast_decode_end_msg (fast_codec_t* codec, fast_tag_t tag)
{
   tag = 0;
   //fprintf(stderr,"start of decode end msg \n");
   if (check_codec (FUNCTION, codec) < 0)
      return -1;

   return 0;
}

//////////////////////////////////////////////////////////////////////

//! @brief Decode an ASCII string

int fast_decode_str (fast_codec_t* codec, fast_tag_t tag,
                     u8* data, int size)
{
   fast_op_t op = (fast_op_t)get_tag_op ((fast_op_t)tag);

   int bytes;

   if (check_codec (FUNCTION, codec) < 0)
      return -1;

   if (check_type  (FUNCTION, codec, tag, FAST_TYPE_STR) < 0)
      return -1;

   switch (op)
   {
    case FAST_OP_NONE:
      bytes = parse_str (codec, tag, data, size);

      if (bytes < 0) // End of input
         return -1;

      if (bytes == 0)
         return value_error (FUNCTION, codec, tag);

      break;

    case FAST_OP_COPY:
      bytes = parse_str (codec, tag, data, size);

      if (bytes < 0)
         return -1;

      if (bytes > 0)
      {
         // Set current value
         cv_set_str (codec, tag, data, size);
      }
      else
      {
         // Copy current value
         memcpy (data, cv_get_str (codec, tag), size);
         bytes = strlen((const char*)data);
      }
      //return size;
      return bytes < size ? bytes : size;

    case FAST_OP_INCR:
      return bad_op_error (FUNCTION, codec, tag);

    case FAST_OP_DELTA:
      {
         u8  buffer [1024];
         u8* curr = cv_get_str (codec, tag);

         u32 bytes = parse_str (codec, tag, buffer, size);

         if (cv_is_valid (codec, tag))
         {
            int temp;

            if (bytes > (u32)size)
               return set_err (FUNCTION, codec, tag, FAST_ERR_SIZE,
                               "oversized value %d > size=%d", bytes, size);

            // This implementation requires delta string field values
            // to be exactly the size of the field.

            temp = strlen ((char*) curr);

            if (temp != size)
               return set_err (FUNCTION, codec, tag, FAST_ERR_SIZE,
                               "size mismatch %d != %d", temp, size);

            // Replace (tail of) current value
            memcpy (curr + size - bytes, buffer, bytes);
         }
         else
         {
            // Set current value
            cv_set_str (codec, tag, buffer, bytes);
         }

         memcpy (data, cv_get_str (codec, tag), size);
      }
      break;

    default:
      bad_op_error (FUNCTION, codec, tag);
   }
   return 0;
}

//////////////////////////////////////////////////////////////////////

//! @brief Decode a 32-bit signed integer

int fast_decode_i32 (fast_codec_t* codec, fast_tag_t tag, i32* value)
{
   fast_op_t op = (fast_op_t)get_tag_op ((fast_op_t)tag);

   i32 temp = 0;
   int bytes;

   if (check_codec (FUNCTION, codec) < 0)
      return -1;

   if (check_type  (FUNCTION, codec, tag, FAST_TYPE_I32) < 0)
      return -1;

   switch (op)
   {
    case FAST_OP_NONE: // Value must be present
      bytes = parse_i32 (codec, tag, & temp);

      if (bytes > 0)
      {
         cv_set_i32 (codec, tag, temp);
         break;
      }

      if (bytes < 0) // End of input
         return -1;

      return value_error (FUNCTION, codec, tag);

    case FAST_OP_COPY: // Use previous value if not present
      bytes = parse_i32 (codec, tag, & temp);

      if (bytes == 0) // No value - use previous [common case]
         break;

      if (bytes < 0) // End of input
         return -1;

      cv_set_i32 (codec, tag, temp);
      break;

    case FAST_OP_INCR: // Use previous value plus one if not present
      bytes = parse_i32 (codec, tag, & temp);

      if (bytes < 0) // End of input
         return -1;

      if (bytes == 0) // No value - use previous plus one
      {
         // Rule: The previous value must be valid
         if (cv_is_valid (codec, tag) == 0)
            return value_error (FUNCTION, codec, tag);

         temp = cv_get_i32 (codec, tag) + 1;
      }

      cv_set_i32 (codec, tag, (u32) temp);
      break;

    case FAST_OP_DELTA:
      if (cv_is_valid (codec, tag))
      {
         // Rule: delta defaults to ZERO if there is
         // no value present

         i32 delta;

         bytes = parse_i32 (codec, tag, & delta);

         if (bytes < 0) // error state already set
            return -1;

         if (bytes > 0)
         {
            temp = cv_get_i32 (codec, tag) + delta;
            cv_set_i32 (codec, tag, temp);
         }
      }
      else
      {
         // Rule: A field must be present it the
         // current value isn't valid

         bytes = parse_i32 (codec, tag, & temp);

         if (bytes < 0) // error state already set
            return -1;

         if (bytes == 0)
            return value_error (FUNCTION, codec, tag);

         cv_set_i32 (codec, tag, temp);
      }
      break;

    default:
      return bad_op_error (FUNCTION, codec, tag);
   }

   *value = cv_get_i32 (codec, tag);
   return 0;
}

//////////////////////////////////////////////////////////////////////

//! @brief Decode a 32-bit unsigned integer

int fast_decode_u32 (fast_codec_t* codec, fast_tag_t tag, u32* value)
{
   fast_op_t op = (fast_op_t)get_tag_op ((fast_op_t)tag);

   u32 next = 0;
   int bytes;

   if (check_codec (FUNCTION, codec) < 0)
      return -1;

   if (check_type  (FUNCTION, codec, tag, FAST_TYPE_U32) < 0)
      return -1;

   switch (op)
   {
    case FAST_OP_NONE: // Value must be present
      bytes = parse_u32 (codec, tag, & next);

      if (bytes < 0) // End of input
         return -1;

      if (bytes == 0) // No data
         return value_error (FUNCTION, codec, tag);

      break;

    case FAST_OP_COPY: // Use previous value if not present
      bytes = parse_u32 (codec, tag, & next);
//fprintf(stderr,"bytes=%d\n",bytes);
//if(next>100)fprintf(stderr,"s  %d\n",next);
//if(next<=100)fprintf(stderr,"s %c\n",next);

      if (bytes < 0) // End of input
         return -1;

      if (bytes == 0) // No value - use previous [common case]
      {
         if (cv_is_valid (codec, tag) == 0) // must be valid
            return value_error (FUNCTION, codec, tag);
         next = cv_get_u32 (codec, tag);
//if(next>100)fprintf(stderr,"use previous %d\n",next);
//if(next<=100)fprintf(stderr,"use previous %c\n",next);
         break;
      }
      break;

    case FAST_OP_INCR: // use previous value plus one if not present
      bytes = parse_u32 (codec, tag, & next);

      if (bytes < 0) // End of input
         return -1;

      if (bytes == 0) // No value - use previous plus one
      {
         if (cv_is_valid (codec, tag) == 0) // must be valid
            return value_error (FUNCTION, codec, tag);

         next = cv_get_u32 (codec, tag) + 1;
      }
      break;

    case FAST_OP_DELTA:
      if (cv_is_valid (codec, tag))
      {
         // Rule: delta defaults to ZERO if there is
         // no value present

         i32 delta;

         bytes = parse_i32 (codec, tag, & delta);
         
         if (bytes < 0)
            return -1;

         next = cv_get_u32 (codec, tag);

         if (bytes > 0)
            next += delta;
      }
      else
      {
         // Rule: A field must be present if the
         // current value isn't valid

         bytes = parse_u32 (codec, tag, & next);

         if (bytes < 0)
            return -1;

         if (bytes == 0)
            return value_error (FUNCTION, codec, tag);
      }
      break;

    default:
      return bad_op_error (FUNCTION, codec, tag);
   }

   cv_set_u32 (codec, tag, next);

   *value = cv_get_u32 (codec, tag);
   
   return 0;
}

//////////////////////////////////////////////////////////////////////

#if !FAST_OPTIMIZE
// flush_group moves the current pmap and body data to the output
// buffer.
// Currently, flush_group is only called from flush_msg, but will
// also be called from other sites when repeating groups become
// supported in this implementation. 


//! @brief Flush a group of fields
//! Emit the presence map for the group and the serialized field data
//
//! @param codec

static int flush_group (fast_codec_t* codec)
{
   fast_buffer_t* output = codec->output;
   fast_buffer_t* msg = codec->msg;

   int map_size;

   //! @remark Make sure there is room in the output buffer

   int size = get_buffer_used (msg);
   int need = MAX_PMAP_BYTES + size;
      
   if (get_buffer_left (output) <= need)
      return set_err (FUNCTION, codec, -1, FAST_ERR_SIZE,
                      "output buffer overflow left=%d need=%d",
                      get_buffer_left (output), need);

   //! Copy the pmap into the output buffer
   map_size = emit_pmap (codec, output->tail);

   output->tail += map_size;

   //! Copy the message body into the output buffer
   memcpy (output->tail, msg->head, size);

   output->tail += size;

   //! Reset the presence map
   reset_pmap (codec);

   reset_buffer (codec->msg);
   return 0;
}

//! @brief Flush a message
//! Flush the last group of the message and write serialized data

static int flush_msg (fast_codec_t* codec)
{
   //fast_buffer_t* output = codec->output;

   flush_group (codec);

   return 0;
}


int fast_get_output_buffer(fast_codec_t* codec, u8* buf, int size)
{

   int cbufsize = get_buffer_used(codec->output);

   if (cbufsize >  size)
      fprintf(stderr,
   "Aiee!! size of encoded data (%d) is more than the passed size (%d)!! \n", cbufsize, size);

   int tsize = cbufsize<size?cbufsize:size;

   memcpy(buf, codec->output->head, tsize);
   
   return tsize;
}
#endif

//////////////////////////////////////////////////////////////////////

// fast_encode_new_msg is called to setup encoding for a message.
// It must not be called again without calling fast_encode_end_msg
// in between.

//! @brief Start encoding a new message

int fast_encode_new_msg (fast_codec_t* codec, fast_tag_t tag)
{
#if !FAST_OPTIMIZE
   check_codec (FUNCTION, codec);
#endif

   if (codec->in_message != 0)
      return set_err (FUNCTION, codec, tag, FAST_ERR_CALL_SEQ,
                      "already in a message");

   codec->in_message = 1;
   codec->curr_tag = tag;
   return 0;
}

#if !FAST_OPTIMIZE
//! @brief Finish encoding a message

int fast_encode_end_msg (fast_codec_t* codec, fast_tag_t tag)
{
   check_codec (FUNCTION, codec);

   if (codec->in_message == 0)
      return set_err (FUNCTION, codec, tag, FAST_ERR_CALL_SEQ,
                      "not in a message");

   codec->in_message = 0;
   return flush_msg (codec);
}

//////////////////////////////////////////////////////////////////////

//! @brief Encode a 32-bit signed integer

int fast_encode_i32 (fast_codec_t* codec, fast_tag_t tag, i32 value)
{
   fast_op_t op = (fast_op_t)get_tag_op (tag);

   if (check_codec (FUNCTION, codec) < 0)
      return -1;

   if (check_type  (FUNCTION, codec, tag, FAST_TYPE_I32) < 0)
      return -1;

   switch (op)
   {
    case FAST_OP_NONE: // Always emit value
      break;

    case FAST_OP_COPY: // Emit value if not equal to previous value
      if (cv_eq_i32 (codec, tag, value))
         return 0;
      break;

    case FAST_OP_INCR: // Fixme: Implement
      return bad_op_error (FUNCTION, codec, tag);

    case FAST_OP_DELTA: // Fixme: Implement
      return bad_op_error (FUNCTION, codec, tag);

    default:
      return bad_op_error (FUNCTION, codec, tag);
   }

   cv_set_i32 (codec, tag, value);
   return emit_i32 (codec, tag, value);
}

static int find_char_delta_offset (u8* a, u8* b, int size)
{
   int p1;

   for (p1 = 0 ; p1 < size ; p1 ++)
      if (a [p1] != b [p1])
         break;

   return p1;
}

//! @brief Encode an ASCII string

int fast_encode_str (fast_codec_t* codec, fast_tag_t tag,
                     u8* data, int size)
{
   fast_op_t op = (fast_op_t)get_tag_op (tag);

   if (check_codec (FUNCTION, codec) < 0)
      return -1;

   if (check_type  (FUNCTION, codec, tag, FAST_TYPE_STR) < 0)
      return -1;

   // Note: We don't prune the string in the codec layer
   // Pruning should be done by the app codec

   switch (op)
   {
    case FAST_OP_NONE:
      break; // Always emit value

    case FAST_OP_COPY: // Emit value if not equal to previous value
      if (cv_eq_str (codec, tag, data, size))
         return 0;

      break;

    case FAST_OP_INCR: // Not defined in the spec
      return bad_op_error (FUNCTION, codec, tag);

    case FAST_OP_DELTA:
      {
         u8* curr = cv_get_str (codec, tag);
         u32 offset = find_char_delta_offset (curr, data, size);

         if (offset == (u32)size)
            return 0;

         memcpy (curr + offset, data + offset, size - offset);

         data += offset;
         size -= offset;

         return emit_str (codec, tag, data, size);
      }

    default:
      return bad_op_error (FUNCTION, codec, tag);
   }

   cv_set_str (codec, tag, data, size);
   return emit_str (codec, tag, data, size);
}

//! @brief Encode a 32-bit unsigned integer

int fast_encode_u32 (fast_codec_t* codec, fast_tag_t tag, u32 value)
{
   fast_op_t op = (fast_op_t)get_tag_op (tag);
   u32 next = value;

   if (check_codec (FUNCTION, codec) < 0)
      return -1;

   if (check_type  (FUNCTION, codec, tag, FAST_TYPE_U32) < 0)
      return -1;

   switch (op)
   {
    case FAST_OP_NONE: // Always emit value
      break;

    case FAST_OP_COPY: // Emit value if not equal to previous value
      if (cv_eq_u32 (codec, tag, value))
         return 0;

      break;

    case FAST_OP_INCR: // Emit value if not previous value plus one
      next = value + 1;

      if (cv_eq_u32 (codec, tag, value))
      {
         cv_set_u32 (codec, tag, next);
         return 0;
      }
      break;

    case FAST_OP_DELTA:
      if (cv_is_valid (codec, tag)) // Emit value if not equal to previous
      {
         i32 delta = (i32) value - cv_get_u32 (codec, tag);

         // Fixme: use parameter to control zero deltas
         if (delta == 0)
            return 0;

         cv_set_u32 (codec, tag, value);
         return emit_i32 (codec, tag, delta);
      }
      break;

    default:
      return bad_op_error (FUNCTION, codec, tag);
   }

   cv_set_u32 (codec, tag, next); // update previous value state
   return emit_u32 (codec, tag, value); // emit the value
}
#endif

//////////////////////////////////////////////////////////////////////

//! @brief Reset a CODEC (encoder/decoder)

#if !FAST_OPTIMIZE
fast_codec_t* fast_reset_codec (fast_codec_t *codec)
{
   // reset all the allocated str_values.
   // str_values are allocated slot wise, hence, a run of all indices
   // is necessary.
   int i, j;
   for(i = 0; i < TAG_MAX_TID; i++)
   {
      for( j = 0; j < MAX_TAG; j++)
      {
         if(codec->cv[i].str_values[j] != NULL)
            free(codec->cv[i].str_values[j]);

            //memset ((char*) codec->cv[i].str_values[j], 0, 1024);
      }
   }

   if (codec->error->text != NULL)
      free (codec->error->text);

   memset (codec, 0, sizeof (*codec));

   codec->magic  = FAST_CODEC_MAGIC;

   init_buffer (codec->msg,   -1);
   init_buffer (codec->input,  0);
   init_buffer (codec->output, 1);

   return codec;
}
#endif

//! @brief Create a CODEC (encoder/decoder)

fast_codec_t* fast_create_codec (void)
{
   fast_codec_t* codec = (fast_codec_t*)malloc (sizeof (*codec));

   if (codec == NULL)
   {
      fprintf (stderr, "error: [%s] malloc failed\n", FUNCTION);
      return NULL;
   }

   memset (codec, 0, sizeof (*codec));

   codec->magic  = FAST_CODEC_MAGIC;

#if !FAST_OPTIMIZE
   init_buffer (codec->msg,   -1);
   init_buffer (codec->output, 1);
   init_buffer (codec->input,  0);
#endif

   return codec;
}

int fast_destroy_codec (fast_codec_t* codec)
{

#if !FAST_OPTIMIZE
   // free all the allocated str_values.
   // str_values are allocated slot wise, hence, a run of all indices
   // is necessary.
   int i,j;
   for(i = 0; i < TAG_MAX_TID; i++)
   {
      for(j = 0; j < MAX_TAG; j++)
      {
         if(codec->cv[i].str_values[j] != NULL)
             free(codec->cv[i].str_values[j]);
      }
   }
#endif

   if (codec->error->text != NULL)
      free (codec->error->text);

   if (codec == NULL)
   {
      fprintf (stderr, "error: [%s] null codec\n", FUNCTION);
      return -1;
   }

   if (codec->magic != FAST_CODEC_MAGIC)
   {
      fprintf (stderr, "error: [%s] bad codec magic number\n", FUNCTION);
      return -1;
   }

   codec->magic = 0; // To prevent dangling references
   free (codec);
   return 0;
}

u32 fast_ascii_to_u32 (u8* data, int size)
{
   u32 temp = 0;
   int p1;

   for (p1 = 0 ; p1 < size ; p1 ++)
   {
      int chr = data [p1];
      temp = temp * 10 + chr - '0';
   }
   return temp;
}
