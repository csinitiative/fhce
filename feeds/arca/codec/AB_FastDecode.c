/**********************************************************************************************************

End user license agreement

NOTICE TO USER: BY USING ALL OR ANY PORTION OF THE SOFTWARE ("SOFTWARE") YOU ACCEPT ALL
THE TERMS AND CONDITIONS OF THIS AGREEMENT.  YOU AGREE THAT THIS AGREEMENT IS
ENFORCEABLE LIKE ANY WRITTEN NEGOTIATED AGREEMENT SIGNED BY YOU. THIS AGREEMENT IS
ENFORCEABLE AGAINST YOU AND ANY LEGAL ENTITY THAT OBTAINED THE SOFTWARE AND ON WHOSE
BEHALF IT IS USED. IF YOU DO NOT AGREE, DO NOT USE THIS SOFTWARE.

1. Software License. As long as you comply with the terms of this Software License Agreement (this
"Agreement"), NYSE Euronext, Inc. grants to you a non-exclusive license to Use the
Software for your business purposes.

2. Intellectual Property Ownership, Copyright Protection.  The Software and any authorized copies
that you make are the intellectual property of and are owned by NYSE Euronext, Inc.  The Software is protected by
law, including without limitation the copyright laws of the United States and other countries, and by
international treaty provisions.

3. NO WARRANTY. The Software is being delivered to you "AS IS" and NYSE Euronext makes no warranty as
to its use or performance. NYSE EURONEXT DOES NOT AND CANNOT WARRANT THE PERFORMANCE OR
RESULTS YOU MAY OBTAIN BY USING THE SOFTWARE. EXCEPT FOR ANY WARRANTY, CONDITION,
REPRESENTATION OR TERM TO THE EXTENT TO WHICH THE SAME CANNOT OR MAY NOT BE EXCLUDED OR
LIMITED BY LAW APPLICABLE TO YOU IN YOUR JURISDICTION, NYSE EURONEXT MAKES NO WARRANTIES
CONDITIONS, REPRESENTATIONS, OR TERMS (EXPRESS OR IMPLIED WHETHER BY STATUTE, COMMON
LAW, CUSTOM, USAGE OR OTHERWISE) AS TO ANY MATTER INCLUDING WITHOUT LIMITATION TITLE,
NONINFRINGEMENT OF THIRD PARTY RIGHTS, MERCHANTABILITY, INTEGRATION, SATISFACTORY
QUALITY, OR FITNESS FOR ANY PARTICULAR PURPOSE.

4.  Indemnity.  You agree to hold NYSE Euronext harmless from any and all liabilities, losses, actions,
damages, or claims (including all reasonable expenses, costs, and attorneys fees) arising out of or relating
to any use of, or reliance on, the Software.

5. LIMITATION OF LIABILITY. IN NO EVENT WILL NYSE EURONEXT BE LIABLE TO YOU FOR ANY DAMAGES,
CLAIMS OR COSTS WHATSOEVER OR ANY DIRECT, CONSEQUENTIAL, INDIRECT, INCIDENTAL DAMAGES,
OR ANY LOST PROFITS OR LOST SAVINGS, EVEN IF NYSE EURONEXT HAS BEEN ADVISED OF THE
POSSIBILITY OF SUCH LOSS, DAMAGES, CLAIMS OR COSTS OR FOR ANY CLAIM BY ANY THIRD PARTY.

6. Export Rules. You agree that the Software will not be shipped, transferred or exported into any country
or used in any manner prohibited by the United States Export Administration Act or any other export laws,
restrictions or regulations (collectively the "Export Laws"). In addition, if the Software is identified as export
controlled items under the Export Laws, you represent and warrant that you are not a citizen, or otherwise
located within, an embargoed nation (including without limitation Iran, Iraq, Syria, Sudan, Libya, Cuba,
North Korea, and Serbia) and that you are not otherwise prohibited under the Export Laws from receiving
the Software. All rights to Use the Software are granted on condition that such rights are forfeited if you fail
to comply with the terms of this Agreement.

7. Governing Law. This Agreement will be governed by and construed in accordance with the substantive
 laws in force in the State of Illinois.

**********************************************************************************************************/


/*================================================================================*/
/*
 *		$Header: //depot/projects/Exchange/L2_branch/release_1.1/ArcaBook/utilities/compaction/DevPakMCEquities/AB_FastDecode.c#1 $
 *
 *    	ARCA Book FAST Decoder Implementation for Mutlicast
 *
 *		Program : AB_FastDecode.c
 *		By      : Fred Jones/David Le
 *		Date    : 02/23/07
 *		Purpose : ArcaBook FAST decoding routines
 *
 */
/*================================================================================*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* defines for message and standard types */
#include "AB_Fast.h"

/* count the length of a pmap up to n bytes */
static int32_t PmapLen(const uint8_t *pmap, int32_t iLen)
{
	int32_t i=0;

	if(0==iLen || !pmap)
		return 0;

	/* stop counting when we reach a byte >= 0x80 or have counted iLen chars */
	while(pmap[i] < 0x80)
	{
        // printf("pmap %x ",pmap[i]);printf(" i=%x\n",i);
		i++;
		if(i >= iLen-1)
			break;
	}
// printf("pmap len return %d\n",i+1);
	/* return the length of the pmap */
	return i+1;
}


/* Decode a signed 32-bit integer and place the result in *data 
 * 
 * field	The field ID we are decoding, ( AB_MSG_TYPE, etc.)
 * buf		The buffer containing the FAST message
 * len		Pointer to the length of buf, on return this will hold the number of bytes processed
 * data		This will hold the result, in host byte order
 * state	The FAST state information
 * pmap		The FAST pmap
 * 
 */
static int32_t DecodeI32 (uint32_t field, const uint8_t *buf, int32_t *len, int32_t *data, FAST_STATE *state, const uint8_t *pmap)
{
	int32_t iSize;

	/* validate the state structure and that the tag is in range */
	if(field >= AB_MAX_FIELD)
		return AB_INVALID_FIELD;
	if(state[field].field != field)
		return AB_INVALID_STATE;

	/* Check the state structure to see if we have an encoded value */
	if(!ISBITSET(pmap,field))
	{
		// See if this field is valid in the state
		if(state[field].valid)
		{
			if(state[field].encodeType == OP_INCR)
				state[field].value.uint32Val++;
			else if(state[field].encodeType == OP_COPY)
				;
			else
				return AB_INVALID_STATE;

			*data = state[field].value.uint32Val;
		}
		else
			return AB_INVALID_STATE;

		*len = 0;
		return AB_OK;
	}

	/* convert the number, move over all but the last byte */
	*data = 0;
	iSize = 0;
	while(*len > iSize  && buf[iSize] < 0x80)
	   *data = (*data << 7) | buf [iSize++];

	/* clear the high bit and move over the last byte */
	if(iSize < *len)
		*data = (*data << 7) | (buf [iSize++] & 0x7f);
	else
		return AB_INCOMPLETE_ERROR;

	/* See if we need to save this value for encoding */
	if(state[field].encodeType == OP_COPY || state[field].encodeType == OP_INCR)
	{
		state[field].valid = 1; 
		state[field].value.uint32Val = *data;
		state[field].size = sizeof(state[field].value.uint32Val);
	}

	*len = iSize;
	return AB_OK;   
}

/* Decode a signed 16-bit integer and place the result in *data 
 * 
 * field	The field ID we are decoding, ( AB_MSG_TYPE, etc.)
 * buf		The buffer containing the FAST message
 * len		Pointer to the length of buf, on return this will hold the number of bytes processed
 * data		This will hold the result, in host byte order
 * state	The FAST state information
 * pmap		The FAST pmap
 * 
 */
static int32_t DecodeI16 (uint32_t field, const uint8_t *buf, int32_t *len, int16_t *value, FAST_STATE *state, const uint8_t *pmap)
{
	int iRet;
	int32_t data;

	iRet =  DecodeI32(field, buf, len, &data, state, pmap) ;

	*value = (int16_t)data;

	return(iRet);
}

/* Decode a signed 8-bit integer and place the result in *data 
 * 
 * field	The field ID we are decoding, ( AB_MSG_TYPE, etc.)
 * buf		The buffer containing the FAST message
 * len		Pointer to the length of buf, on return this will hold the number of bytes processed
 * data		This will hold the result, in host byte order
 * state	The FAST state information
 * pmap		The FAST pmap
 * 
 */
static int32_t DecodeI8 (uint32_t field, const uint8_t *buf, int32_t *len, int8_t *value, FAST_STATE *state, const uint8_t *pmap)
{
	int iRet;
	int32_t data;

	iRet =  DecodeI32(field, buf, len, &data, state, pmap);

	*value = (int8_t)data;

	return(iRet);
}

/* Decode an unsigned iSCII string and put the result in *data 
 *
 * field	The field ID we are decoding, ( AB_MSG_TYPE, etc.)
 * buf		The buffer containing the FAST message
 * len		Pointer to the length of buf, on return this will hold the number of bytes processed
 * data		This will hold the result
 * state	The FAST state information
 * pmap		The FAST pmap
 */
static int32_t DecodeUASCII (uint32_t field, const uint8_t *buf, int32_t *len, unsigned char *data, FAST_STATE *state, const uint8_t *pmap)
{
	int32_t iSize;

	/* validate the state structure and that the tag is in range */
	if(field >= AB_MAX_FIELD)
		return AB_INVALID_FIELD;
	if(state[field].field != field)
		return AB_INVALID_STATE;

	/* Check the state structure to see if we have an encoded value */
	if(!ISBITSET(pmap,field))
	{
		if(state[field].valid)
		{
			if(state[field].encodeType == OP_COPY)
				memcpy(data,state[field].value.uint8Val,state[field].size);
			else
				return AB_INVALID_STATE;
		}
		else
			return AB_INVALID_STATE;

		*len = 0;
		return AB_OK;
	}

	/* move over the string, all but the last byte */
	iSize = 0;
	while(*len > iSize && buf[iSize] < 0x80)
	{
		data[iSize] = buf [iSize];
		iSize++;
	}

	/* clear the high bit and move over the last byte  */
	if(*len > iSize)
	{
		data[iSize] =  (buf [iSize] & 0xff); /* 0x7F ????? */
		iSize++;
	}
	else
	   return AB_INCOMPLETE_ERROR;

	/* See if we need to save this value for encoding */
	if(state[field].encodeType == OP_COPY)
	{
		/* If the string is too long, we can not copy encode it */
		if(iSize <= AB_MAX_STRLEN)
		{
			state[field].valid = 1; 
			memcpy(state[field].value.uint8Val,data,iSize);
			state[field].size = iSize;
		}
		else
			state[field].valid = 0; 
	}

	*len = iSize;
	return AB_OK;   
}

/* Decode an ASCII string and put the result in *data 
 *
 * field	The field ID we are decoding, ( AB_MSG_TYPE, etc.)
 * buf		The buffer containing the FAST message
 * len		Pointer to the length of buf, on return this will hold the number of bytes processed
 * data		This will hold the result
 * state	The FAST state information
 * pmap		The FAST pmap
 */
static int32_t DecodeASCII (uint32_t field, const uint8_t *buf, int32_t *len, char *data, FAST_STATE *state, const uint8_t *pmap)
{
	int32_t iSize;

	/* validate the state structure and that the tag is in range */
	if(field >= AB_MAX_FIELD)
		return AB_INVALID_FIELD;
	if(state[field].field != field)
		return AB_INVALID_STATE;

	/* Check the state structure to see if we have an encoded value */
	if(!ISBITSET(pmap,field))
	{
		if(state[field].valid)
		{
			if(state[field].encodeType == OP_COPY)
				memcpy(data,state[field].value.int8Val,state[field].size);
			else
				return AB_INVALID_STATE;
		}
		else
			return AB_INVALID_STATE;

		*len = 0;
		return AB_OK;
	}

	/* move over the string, all but the last byte */
	iSize = 0;
	while(*len > iSize && buf[iSize] < 0x80)
	{
		data[iSize] = buf [iSize];
		iSize++;
	}

	/* clear the high bit and move over the last byte  */
	if(*len > iSize)
	{
		data[iSize] =  (buf [iSize] & 0x7f);
		iSize++;
	}
	else
	   return AB_INCOMPLETE_ERROR;

	/* See if we need to save this value for encoding */
	if(state[field].encodeType == OP_COPY)
	{
		/* If the string is too long, we can not copy encode it */
		if(iSize <= AB_MAX_STRLEN)
		{
			state[field].valid = 1; 
			memcpy(state[field].value.int8Val,data,iSize);
			state[field].size = iSize;
		}
		else
			state[field].valid = 0; 
	}

	*len = iSize;
	return AB_OK;   
}

/* Decode a BitMap and put the result in *data 
 *
 * field	The field ID we are decoding, ( AB_MSG_TYPE, etc.)
 * buf		The buffer containing the FAST message
 * len		Pointer to the length of buf, on return this will hold the number of bytes processed
 * data		This will hold the result
 * state	The FAST state information
 * pmap		The FAST pmap
 */
static int32_t DecodeBitmap (uint32_t field, const uint8_t *buf, int32_t *len, uint8_t *data, FAST_STATE *state, const uint8_t *pmap)
{
	int32_t iSize;
	int32_t j;
	int32_t bit=0; 

	/* validate the state structure and that the tag is in range */
	if(field >= AB_MAX_FIELD)
		return AB_INVALID_FIELD;
	if(state[field].field != field)
		return AB_INVALID_STATE;

	/* We never encode this type, so just return OK */
	if(!ISBITSET(pmap,field))
	{
		*len = 0;
		return AB_OK;   
	}

	/* move over the bytes, 7-bits at a time */
	iSize = 0;
	while(*len > iSize)
	{
		data[iSize] = 0;
		for(j=0;j<8;j++)
		{
			bit = (8*iSize)+j;

			/* is this bit set in the FAST field ? */
			if(ISBITSET(buf,bit))
				data[iSize] |= (0x80 >> bit % 8);
		}

		iSize++;

		/* we are done once we hit the stop bit */
		if(buf[bit / 7] >= 0x80)
			break;
	}

	/* Did we have enough room ?  */
	if(*len <= iSize)
	   return AB_INCOMPLETE_ERROR;  

	*len = bit / 7 + 1;
	return AB_OK;   
}

/* validate a ArcaBook FAST message for multicast */
int32_t ABFastValidate(const uint8_t *src, int32_t srcLen, uint8_t *pmap)
{
	int32_t iOffset = 0;          /* offset within src of current position   */
	int32_t iPmapLen;             /* length of pmap */

	/* count the number of fields the pmap says is present, and make sure we have that many in the message */
	/* message will always be at least 2 bytes, at least 1 byte for pmap, 1 byte for mandatory type */
	if(srcLen < AB_MIN_FAST_MSG)
		return AB_INCOMPLETE_ERROR;

	/* decode the pmap    */
	iPmapLen = PmapLen(src,AB_MAX_PMAP);
	memcpy(pmap,src,iPmapLen);
	iOffset+= iPmapLen;

	/* Make sure pmap is valid with a stop bit */
	if(iPmapLen < 1 || pmap[iPmapLen-1] < 0x80)
		return AB_INVALID_HEADER;

	/* pmap must always have field 0, TYPE   */
	if(!ISBITSET(pmap,AB_MSG_TYPE))
		return AB_INVALID_HEADER;

	return AB_OK;   
}

/*
* Decode a FAST message to ARCA Book message
* 
* dstMsg	Pointer to a ArcaL2MsgUnion structure to be filled with the decoded message
* src		Pointer to a uint8_t buffer containing the FAST encoded message
* srcLen	uint32_t containing the length of the src buffer, on return, this will contain
*		the number of bytes processed fron the src buffer
* msgType	Pointer to a message type of the ArcaL2MsgUnion structure to be filled with the decoded message
* state		Pointer to the FAST state information
*
* returns 0 if success, < 0 if error
*/
int32_t ABFastDecode(union ArcaL2MsgUnion *dstMsg, const uint8_t *src, int32_t *srcLen, uint16_t *msgType, FAST_STATE *state)
{
	uint8_t pmap[AB_MAX_PMAP]={0,0};/* You must init this to ZERO  !!!		*/
	int32_t iOffset = 0;		/* offset within buf of current position	*/
	int32_t iRet;			/* return code 					*/
	int32_t iLen;			/* length available in input buffer for processing */
	FAST_STATE localState[AB_MAX_FIELD];

	/* message will always be at least 2 bytes, at least 1 byte for pmap, 1 byte for mandatory type */
	if(*srcLen < AB_MIN_FAST_MSG)
		return AB_INCOMPLETE_ERROR;

	/* validate the message, and make sure we have a full encoded message */
	if(AB_OK != (iRet=ABFastValidate(src, *srcLen, pmap)))
		return iRet;

	/* we already decoded the pmap, so advance Offset  */
	iOffset+= PmapLen(pmap,AB_MAX_PMAP);

	/* pull dstMsg the message type  */
	iLen = *srcLen - iOffset; 
	
	/* store local copy of the state, in case we error out */
	memcpy(localState,state,sizeof(localState));

	/* The first field in every ArcaBook FAST message is the message type */
	if(AB_OK==(iRet=DecodeI16(AB_MSG_TYPE,&src[iOffset],&iLen,(int16_t *)msgType,localState,pmap)))
		iOffset+= iLen;
	else
		return iRet;

// printf("in decode %d\n",*msgType);
	/* switch on the message type */
	switch (*msgType)
	{
		/* decode the 'add' message */
		case 100: /* ARCA_L2_ADD_MSG_TYPE */
			/* For every Field, decode, making sure we do not run over the input buffer */
//dstMsg->Header.iMsgSize = htons ( sizeof ( ArcaL2Add_t ) );
			dstMsg->Add.iMsgType = htons(*msgType);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_STOCK_IDX,&src[iOffset],&iLen,(int16_t *)&dstMsg->Add.iSec,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Add.iSec = htons(dstMsg->Add.iSec);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_SEQUENCE,&src[iOffset],&iLen,(int32_t *)&dstMsg->Add.iSymbolSequence,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Add.iSymbolSequence = htonl(dstMsg->Add.iSymbolSequence);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_TIME,&src[iOffset],&iLen,(int32_t *)&dstMsg->Add.iSourceTime,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Add.iSourceTime = htonl(dstMsg->Add.iSourceTime);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_ORDER_ID,&src[iOffset],&iLen,(int32_t *)&dstMsg->Add.iOrderID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Add.iOrderID = htonl(dstMsg->Add.iOrderID);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_VOLUME,&src[iOffset],&iLen,(int32_t *)&dstMsg->Add.iVolume,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Add.iVolume = htonl(dstMsg->Add.iVolume);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_PRICE,&src[iOffset],&iLen,(int32_t *)&dstMsg->Add.iPrice,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Add.iPrice = htonl(dstMsg->Add.iPrice);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI8(AB_PRICE_SCALE,&src[iOffset],&iLen,(int8_t *)&dstMsg->Add.iPriceScale,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_BUY_SELL,&src[iOffset],&iLen,(char *)&dstMsg->Add.cSide,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_EXCH_ID,&src[iOffset],&iLen,(char *)&dstMsg->Add.cExchangeID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_SECURITY_TYPE,&src[iOffset],&iLen,(char *)&dstMsg->Add.cSecurityType,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_FIRM_ID,&src[iOffset],&iLen,(int16_t *)&dstMsg->Add.iFirm,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Add.iFirm = htons(dstMsg->Add.iFirm);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI8(AB_SESSION_ID,&src[iOffset],&iLen,(int8_t *)&dstMsg->Add.iSessionID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			break;

		/* decode the 'modify' message */
		case 101: /* ARCA_L2_MODIFY_MSG_TYPE */
			/* For every Field, decode, making sure we do not run over the input buffer */

			dstMsg->Modify.iMsgType = htons(*msgType);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_STOCK_IDX,&src[iOffset],&iLen,(int16_t *)&dstMsg->Modify.iSec,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Modify.iSec = htons(dstMsg->Modify.iSec);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_SEQUENCE,&src[iOffset],&iLen,(int32_t *)&dstMsg->Modify.iSymbolSequence,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Modify.iSymbolSequence = htonl(dstMsg->Modify.iSymbolSequence);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_TIME,&src[iOffset],&iLen,(int32_t *)&dstMsg->Modify.iSourceTime,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Modify.iSourceTime = htonl(dstMsg->Modify.iSourceTime);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_ORDER_ID,&src[iOffset],&iLen,(int32_t *)&dstMsg->Modify.iOrderID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Modify.iOrderID = htonl(dstMsg->Modify.iOrderID);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_VOLUME,&src[iOffset],&iLen,(int32_t *)&dstMsg->Modify.iVolume,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Modify.iVolume = htonl(dstMsg->Modify.iVolume);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_PRICE,&src[iOffset],&iLen,(int32_t *)&dstMsg->Modify.iPrice,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Modify.iPrice = htonl(dstMsg->Modify.iPrice);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI8(AB_PRICE_SCALE,&src[iOffset],&iLen,(int8_t *)&dstMsg->Modify.iPriceScale,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_BUY_SELL,&src[iOffset],&iLen,(char *)&dstMsg->Modify.cSide,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_EXCH_ID,&src[iOffset],&iLen,(char *)&dstMsg->Modify.cExchangeID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_SECURITY_TYPE,&src[iOffset],&iLen,(char *)&dstMsg->Modify.cSecurityType,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_FIRM_ID,&src[iOffset],&iLen,(int16_t *)&dstMsg->Modify.iFirm,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Modify.iFirm = htons(dstMsg->Modify.iFirm);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI8(AB_SESSION_ID,&src[iOffset],&iLen,(int8_t *)&dstMsg->Modify.iSessionID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			break;

		/* decode the 'delete' message */
		case 102: /* ARCA_L2_DELETE_MSG_TYPE */
			/* For every Field, decode, making sure we do not run over the input buffer */

			dstMsg->Delete.iMsgType = htons(*msgType);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_STOCK_IDX,&src[iOffset],&iLen,(int16_t *)&dstMsg->Delete.iSec,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Delete.iSec = htons(dstMsg->Delete.iSec);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_SEQUENCE,&src[iOffset],&iLen,(int32_t *)&dstMsg->Delete.iSymbolSequence,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Delete.iSymbolSequence = htonl(dstMsg->Delete.iSymbolSequence);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_TIME,&src[iOffset],&iLen,(int32_t *)&dstMsg->Delete.iSourceTime,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Delete.iSourceTime = htonl(dstMsg->Delete.iSourceTime);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_ORDER_ID,&src[iOffset],&iLen,(int32_t *)&dstMsg->Delete.iOrderID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Delete.iOrderID = htonl(dstMsg->Delete.iOrderID);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_BUY_SELL,&src[iOffset],&iLen,(char *)&dstMsg->Delete.cSide,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_EXCH_ID,&src[iOffset],&iLen,(char *)&dstMsg->Delete.cExchangeID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_SECURITY_TYPE,&src[iOffset],&iLen,(char *)&dstMsg->Delete.cSecurityType,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI8(AB_SESSION_ID,&src[iOffset],&iLen,(int8_t *)&dstMsg->Delete.iSessionID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_FIRM_ID,&src[iOffset],&iLen,(int16_t *)&dstMsg->Delete.iFirm,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Delete.iFirm = htons(dstMsg->Delete.iFirm);

			break;

		/* decode the 'imbalance' message */
		case 103: /* ARCA_L2_IMBALANCE_MSG_TYPE */
			/* For every Field, decode, making sure we do not run over the input buffer */

			dstMsg->Imbalance.iMsgType = htons(*msgType);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_STOCK_IDX,&src[iOffset],&iLen,(int16_t *)&dstMsg->Imbalance.iSec,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Imbalance.iSec = htons(dstMsg->Imbalance.iSec);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_SEQUENCE,&src[iOffset],&iLen,(int32_t *)&dstMsg->Imbalance.iSymbolSequence,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Imbalance.iSymbolSequence = htonl(dstMsg->Imbalance.iSymbolSequence);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_TIME,&src[iOffset],&iLen,(int32_t *)&dstMsg->Imbalance.iSourceTime,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Imbalance.iSourceTime = htonl(dstMsg->Imbalance.iSourceTime);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_VOLUME,&src[iOffset],&iLen,(int32_t *)&dstMsg->Imbalance.iVolume,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Imbalance.iVolume = htonl(dstMsg->Imbalance.iVolume);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_ORDER_ID,&src[iOffset],&iLen,(int32_t *)&dstMsg->Imbalance.iTotalImbalance,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Imbalance.iTotalImbalance = htonl(dstMsg->Imbalance.iTotalImbalance);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_BITMAP,&src[iOffset],&iLen,(int32_t *)&dstMsg->Imbalance.iMarketImbalance,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Imbalance.iMarketImbalance = htonl(dstMsg->Imbalance.iMarketImbalance);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_PRICE,&src[iOffset],&iLen,(int32_t *)&dstMsg->Imbalance.iPrice,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Imbalance.iPrice = htonl(dstMsg->Imbalance.iPrice);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI8(AB_PRICE_SCALE,&src[iOffset],&iLen,(int8_t *)&dstMsg->Imbalance.iPriceScale,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_BUY_SELL,&src[iOffset],&iLen,(char *)&dstMsg->Imbalance.cAuctionType,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_EXCH_ID,&src[iOffset],&iLen,(char *)&dstMsg->Imbalance.cExchangeID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_SECURITY_TYPE,&src[iOffset],&iLen,(char *)&dstMsg->Imbalance.cSecurityType,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI8(AB_SESSION_ID,&src[iOffset],&iLen,(int8_t *)&dstMsg->Imbalance.iSessionID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_FIRM_ID,&src[iOffset],&iLen,(int16_t *)&dstMsg->Imbalance.iAuctionTime,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->Imbalance.iAuctionTime = htons(dstMsg->Imbalance.iAuctionTime);

			break;

		case 35: /* ARCA_L2_SYMBOL_IDX_MAP_MSG_TYPE */
			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_STOCK_IDX,&src[iOffset],&iLen,(int16_t *)&dstMsg->SymbolUpdate.iSec,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
//printf("iFirm %d\n",dstMsg->FirmUpdate.iFirm);


			dstMsg->SymbolUpdate.iSec = htons(dstMsg->SymbolUpdate.iSec);




			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI8(AB_SESSION_ID,&src[iOffset],&iLen,(int8_t *)&dstMsg->SymbolUpdate.iSessionID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			/* null padd symbols */
			memset(dstMsg->SymbolUpdate.sSymbol,'\0',sizeof(dstMsg->SymbolUpdate.sSymbol)); 

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII (AB_SYMBOL_STRING,&src[iOffset],&iLen,(char *)&dstMsg->SymbolUpdate.sSymbol,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
//printf("sSymbol %s\n",dstMsg->SymbolUpdate.sSymbol);
			break;
		case 36: /* ARCA_L2_SYMBOL_CLEAR_MSG_TYPE */
			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_SEQUENCE,&src[iOffset],&iLen,(int32_t *)&dstMsg->SymbolClear.iNextSeqNum,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->SymbolClear.iNextSeqNum = htonl(dstMsg->SymbolClear.iNextSeqNum);


			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_STOCK_IDX,&src[iOffset],&iLen,(int16_t *)&dstMsg->SymbolClear.iSec,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->SymbolClear.iSec = htons(dstMsg->SymbolClear.iSec);
			
			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII (AB_SESSION_ID,&src[iOffset],&iLen,(char *)&dstMsg->SymbolClear.iSessionID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			break;

		case 37: /* ARCA_L2_FIRM_IDX_MAP_MSG_TYPE */
			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_FIRM_ID,&src[iOffset],&iLen,(int16_t *)&dstMsg->FirmUpdate.iFirm,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
//printf("iFirm %d\n",dstMsg->FirmUpdate.iFirm);


			dstMsg->FirmUpdate.iFirm = htons(dstMsg->FirmUpdate.iFirm);
		

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII (AB_FIRM_STRING,&src[iOffset],&iLen,(char *)&dstMsg->FirmUpdate.sFirmID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
//printf("sFirmID %4s\n",dstMsg->FirmUpdate.sFirmID);

			break;
				/* decode the 'Book refresh' message */
		case 32: /* ARCA_L2_BOOK_REFRESH_MSG_TYPE */
			/* For every Field, decode, making sure we do not run over the input buffer */

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_SEQUENCE,&src[iOffset],&iLen,(int32_t *)&dstMsg->BookRefreshOrder.iSymbolSequence,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->BookRefreshOrder.iSymbolSequence = htonl(dstMsg->BookRefreshOrder.iSymbolSequence);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_TIME,&src[iOffset],&iLen,(int32_t *)&dstMsg->BookRefreshOrder.iSourceTime,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->BookRefreshOrder.iSourceTime = htonl(dstMsg->BookRefreshOrder.iSourceTime);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_ORDER_ID,&src[iOffset],&iLen,(int32_t *)&dstMsg->BookRefreshOrder.iOrderID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->BookRefreshOrder.iOrderID = htonl(dstMsg->BookRefreshOrder.iOrderID);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_VOLUME,&src[iOffset],&iLen,(int32_t *)&dstMsg->BookRefreshOrder.iVolume,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->BookRefreshOrder.iVolume = htonl(dstMsg->BookRefreshOrder.iVolume);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_PRICE,&src[iOffset],&iLen,(int32_t *)&dstMsg->BookRefreshOrder.iPrice,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->BookRefreshOrder.iPrice = htonl(dstMsg->BookRefreshOrder.iPrice);

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI8(AB_PRICE_SCALE,&src[iOffset],&iLen,(int8_t *)&dstMsg->BookRefreshOrder.iPriceScale,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_BUY_SELL,&src[iOffset],&iLen,(char *)&dstMsg->BookRefreshOrder.cSide,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_EXCH_ID,&src[iOffset],&iLen,(char *)&dstMsg->BookRefreshOrder.cExchangeID,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeASCII(AB_SECURITY_TYPE,&src[iOffset],&iLen,(char *)&dstMsg->BookRefreshOrder.cSecurityType,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;

			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI16(AB_FIRM_ID,&src[iOffset],&iLen,(int16_t *)&dstMsg->BookRefreshOrder.iFirm,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->BookRefreshOrder.iFirm = htons(dstMsg->BookRefreshOrder.iFirm);

            /* sFiller[2]? */
			break;
		case 1 : /* ARCA_L2_SEQ_RESET_MSG_TYPE */			//seq number reset
			iLen = *srcLen - iOffset; 
			if(AB_OK==(iRet=DecodeI32(AB_SEQUENCE,&src[iOffset],&iLen,(int32_t *)&dstMsg->SequenceReset.iNextSeqNum,localState,pmap)))
				iOffset+= iLen;
			else
				return iRet;
			dstMsg->SequenceReset.iNextSeqNum = htonl(dstMsg->SequenceReset.iNextSeqNum);
			break;
		/* catch all other messages here */
		default:
			/* just take the length of the message starting at 1 byte past the header */
			iOffset+= iLen;
			iLen = *srcLen - iOffset;

			iRet=DecodeBitmap(AB_BITMAP,&src[iOffset],&iLen,(uint8_t *)(dstMsg),localState,pmap);

			if (AB_OK == iRet)
				iOffset+= iLen;
			else
				return iRet;

			break;

	}

	/* return the length of the input buffer used */
	*srcLen = iOffset;

	/* update the state */
	memcpy(state,localState,sizeof(localState));
	return AB_OK;
}

