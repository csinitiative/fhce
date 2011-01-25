/*================================================================================*/
/*
 *		$Header: //depot/projects/Exchange/L2_branch/release_1.1/ArcaBook/utilities/compaction/DevPakMCEquities/AB_Fast.h#1 $
 *
 *    	ArcaBook FAST Implementation header file for multicast feed
 *
 *		Program : AB_Fast.h
 *		By      : Fred Jones/David Le
 *		Date    : 01/25/07
 *		Purpose : ArcaBook FAST field descriptions and identifiers for multicast
 *
 */
/*================================================================================*/
#ifndef ARCABOOKFAST_INCLUDE
#define ARCABOOKFAST_INCLUDE

		typedef unsigned char  uint8_t;
		typedef signed char  int8_t;
		typedef short int16_t;
		typedef unsigned short uint16_t;
		typedef int int32_t;
	//	typedef long int32_t;
		typedef unsigned int uint32_t;
	//	typedef unsigned long uint32_t;


/* needed for ntohs() etc. for Big-Endian to Little-Endian conversions */
#if defined(_WIN32) || defined(WIN32)
#include <Winsock2.h>
#else
#	include <netinet/in.h>
#endif

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/* Make sure all our structures are 1 byte aligned */
#pragma pack(1)

/* include the ArcaBook Multicat message header files for multicast feed */
#include "ArcaL2Msg.h"

/*===========================================================================*/
/* We need to keep some state information about every possible field we will
   be encoding/decoding.  This will allow us to do Copy/Increment encoding
   across messages 
*/

enum AB_FIELDS
{
	AB_MSG_TYPE	= 0,	/* 100-Add, 101-Modify, 102-Delete, 103-Imbalance		*/
	AB_STOCK_IDX	= 1,	/* Stock Index							*/
	AB_SEQUENCE	= 2,	/* Sequence Number						*/
	AB_TIME		= 3,	/* Source Time  Scaled Seconds + Milliseconds			*/
	AB_ORDER_ID	= 4,	/* Order Reference ID						*/
	AB_VOLUME	= 5,	/* Volume							*/
	AB_PRICE	= 6,	/* Price							*/
	AB_PRICE_SCALE	= 7,	/* Price Scale							*/
	AB_BUY_SELL	= 8,	/* Buy/Sell (B-Buy, S-Sell)					*/
	AB_EXCH_ID	= 9,	/* Exchange Code (N-NYSE, P-NYSE Arca)				*/
	AB_SECURITY_TYPE= 10,	/* Security Type (E-Equity)					*/
	AB_FIRM_ID	= 11,	/* Atrributed Quote/MMID					*/
	AB_SESSION_ID	= 12,	/* Session ID/Engine ID						*/
	AB_BITMAP	= 13,	/* Generic binary data						*/
	AB_MAX_FIELD	= 14	/* Maximum fields, placeholder					*/
};

/* We can use these duplicate ID's since they do not appear together in any message type	*/
#define AB_IMBALANCE AB_ORDER_ID
#define AB_MKT_IMBALANCE AB_BITMAP
#define AB_AUCTION_TYPE AB_BUY_SELL
#define AB_AUCTION_TIME AB_FIRM_ID
#define AB_SYMBOL_STRING AB_BITMAP
#define AB_FIRM_STRING AB_BITMAP


/*===========================================================================*/
/* FAST Field Encoding Operators */
#define OP_NONE        0
#define OP_COPY        1
#define OP_INCR        2

#define AB_MAX_STRLEN   (64)   /* the max length of single ASCII string in a field  */
#define AB_MAX_FAST_MSG (128)  /* the max length of a ArcaBook Options FAST encoded message */
#define AB_MIN_FAST_MSG (2)    /* the min length, we always have at least a pmap and the type field */
#define AB_MAX_PMAP ((AB_MAX_FIELD-1)/7 + 1)  /* the max length of a pmap, based on current field definitions */
#define AB_MAX_PMAP_BITS ( 7 * AB_MAX_PMAP )

/* we need state information for each possible field */
typedef struct tagFAST_STATE
{
	uint32_t field;		/* AB_* field id, always matches the index into array	*/   
	int32_t valid;		/* Is this a valid entry ?				*/
	int32_t encodeType;	/* COPY, INCR, etc.					*/
	int32_t size;		/* size in bytes of value below				*/
	union
	{
		uint32_t	uint32Val;			/* This is a normal 32bit unsigned integer in host order */
		uint16_t	uint16Val;			/* This is a normal 16bit unsigned integer		*/
		uint8_t		uint8Val[AB_MAX_STRLEN];	/* This is a normal unsigned string			*/
		int8_t		int8Val[AB_MAX_STRLEN];		/* This is a non null terminated string, use size above	*/
	}value;		/* the actual value, either a 4 byte unsigned integer, or up to a 64 byte string*/

}FAST_STATE;

#if       defined(INSTANTIATE_FASTSTATEINIT)
/* Define a constant state array with encoding methods and storage for encoded values */
static const FAST_STATE fastStateInit[AB_MAX_FIELD] =
	{
		{ AB_MSG_TYPE,0,OP_NONE,0,{0}},
		{ AB_STOCK_IDX,0,OP_COPY,0,{0}},
		{ AB_SEQUENCE,0,OP_INCR,0,{0}},
		{ AB_TIME,0,OP_COPY,0,{0}},
		{ AB_ORDER_ID,0,OP_COPY,0,{0}},
		{ AB_VOLUME,0,OP_COPY,0,{0}},
		{ AB_PRICE,0,OP_COPY,0,{0}},
		{ AB_PRICE_SCALE,0,OP_COPY,0,{0}},
		{ AB_BUY_SELL,0,OP_COPY,0,{0}},
		{ AB_EXCH_ID,0,OP_COPY,0,{0}},
		{ AB_SECURITY_TYPE,0,OP_COPY,0,{0}},
		{ AB_FIRM_ID,0,OP_COPY,0,{0}},
		{ AB_SESSION_ID,0,OP_COPY,0,{0}},
		{ AB_BITMAP,0,OP_NONE,0,{0}}
	};
#endif /* defined(INSTANTIATE_FASTSTATEINIT) */

/*===========================================================================*/


/* Check some platform specifics to make sure our types and structures are the correct size
 * C-style compile-time assertion, will report zero size array if sizes are not what we expected
 */
/*
static void CompileTimeAssertions(void)
{
	{COMPILE_TIME_ASSERT(sizeof( uint8_t )	== 1);}
	{COMPILE_TIME_ASSERT(sizeof( int8_t )   == 1);}

	{COMPILE_TIME_ASSERT(sizeof( int16_t )  == 2);}
	{COMPILE_TIME_ASSERT(sizeof( int32_t )  == 4);}

	{COMPILE_TIME_ASSERT(sizeof( ArcaL2Header_t )	== 16);}
	{COMPILE_TIME_ASSERT(sizeof( ArcaL2Add_t )	== 32);}
	{COMPILE_TIME_ASSERT(sizeof( ArcaL2Modify_t )	== 32);}
	{COMPILE_TIME_ASSERT(sizeof( ArcaL2Delete_t )	== 24);}
	{COMPILE_TIME_ASSERT(sizeof( ArcaL2Imbalance_t ) == 36);}
	{COMPILE_TIME_ASSERT(sizeof( ArcaL2SequenceReset_t ) == 4);}
	{COMPILE_TIME_ASSERT(sizeof( ArcaL2SymbolClear_t ) == 8);}
	{COMPILE_TIME_ASSERT(sizeof( ArcaL2SymbolUpdate_t ) == 20);}
	{COMPILE_TIME_ASSERT(sizeof( ArcaL2FirmUpdate_t ) == 12);}
	** A safety net to catch someone who adds a new, very large ArcaBook message **
{COMPILE_TIME_ASSERT( AB_MAX_FAST_MSG > sizeof( ArcaL2MsgUnion ) / 4 * 5);}
}
************/

/* Function Prototypes */

/*
 * Encode a ARCA Book for Equities Multicast binary message to FAST 
 * 
 * dest			Pointer to a uint8_t buffer that will be filled with the FAST encoded message
 * destLen		Pointer to a int32_t that will be filled with the length of the encoded FAST message
 * srcMsg		Pointer to a ArcaQuoteMsgUnion structure containing the message to encode
 * msgType		Pointer to a message type for the srcMsg being passed in
 * state		Pointer to the FAST state information
 *
 * returns 0 if success, < 0 on error
 */
int32_t ABFastEncode(uint8_t *dest, int32_t *destLen, const union ArcaL2MsgUnion *srcMsg, const uint16_t iMsgType, FAST_STATE *state);

/*
 * Decode a FAST message to ARCA Book binary message
 * 
 * dstMsg	Pointer to a ArcaQuoteMsgUnion structure to be filled with the decoded message
 * src		Pointer to a uint8_t buffer containing the FAST encoded message
 * srcLen	uint32_t containing the length of the src buffer
 * msgType	Pointer to a message type of the ArcaL2MsgUnion structure to be filled with the decoded message
 * state	Pointer to the FAST state information
 *
 * returns 0 if success, < 0 if error
 */
int32_t ABFastDecode(union ArcaL2MsgUnion *dstMsg, const uint8_t *src, int32_t *srcLen, uint16_t *iMsgType, FAST_STATE *state);

/*
 * Return codes, always < 0 on error
 */
#define AB_OK   			(0)   /* No error, everything OK  */
#define AB_GENERAL_ERROR	(-1)  /* General error            */     
#define AB_INVALID_FIELD	(-2)  /* The field ID is out of range            */   
#define AB_INVALID_STATE	(-3)  /* The state structure is invalid          */     
#define AB_INCOMPLETE_ERROR	(-4)  /* There is not a complete stream available to decode */
#define AB_BUF_ERROR		(-5)  /* There is not enough room in the destination buffer */
#define AB_INVALID_HEADER	(-6)  /* FAST Header is invalid */
#define AB_INVALID_ASCII	(-7)  /* The ASCII data being encoded is not ASCII ( > 0x80) */
#define AB_INVALID_LENGTH	(-8)  /* The data is either too long or too short */
#define AB_INVALID_TYPE		(-9)  /* iMsgType is not a valid type */

/*
 * 7-bit alignment and bit manipulation macros
 */

/* Set a bit in buf, using only 7-bits per byte, always skipping the high bit in each byte */
#define	SETBIT(buf,bitnum) (buf [(bitnum) / 7] |= (0x40 >> (bitnum) % 7))

/* Check if a bit is set, again using only 7-bits per byte */
#define ISBITSET(buf,bitnum) (buf[(bitnum) / 7] & (0x40 >> (bitnum) % 7))

/* Count how many bytes it will take to encode the 8-bit buf using 7-bits 
   We first convert the bytes to bits, then add 6 extra bits since the division by 7
   will truncate to an integer*/
#define ALIGN7(bytes) (((bytes)*8 + 6) / 7)

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#pragma pack()

#endif /*  ARCABOOKFAST_INCLUDE */

