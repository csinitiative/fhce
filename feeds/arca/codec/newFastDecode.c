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

/* Copyright Tervela, Inc. 2008 */

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

#undef VERBOSE_FAST_DECODE

/* copy the pmap and count its length up to n bytes */
static int32_t copyPmap(uint8_t * dest, const uint8_t * pmap, int32_t iLen )
{
	int32_t i=0;

	if( 0==iLen || !pmap || !dest )
		return 0;

	/* stop counting when we reach a byte >= 0x80 or have counted iLen chars */
	while(pmap[i] < 0x80)
	{
        dest[i] = pmap[i];
#if       defined(VERBOSE_FAST_DECODE)
printf("pmap %x ",pmap[i]);printf(" i=%x\n",i);
#endif /* defined(VERBOSE_FAST_DECODE) */
		i++;
        if(i >= iLen-1) {
            dest[i] = pmap[i];
			break;
        }
	}
#if       defined(VERBOSE_FAST_DECODE)
printf("pmap len return %d\n",i+1);
#endif /* defined(VERBOSE_FAST_DECODE) */
	/* return the length of the pmap */
	return i+1;
}

/* Decode a signed 32-bit integer and place the result in *data 
 * 
 * buf		The buffer containing the FAST message
 * len		Pointer to the length of buf, on return this will hold
 *          the number of bytes processed
 * data		This will hold the result, in NETWORK byte order
 * pState	Pointer to the FAST state information for the applicable field
 * pmap		The FAST pmap
 * 
 */
static int32_t DecodeI32 ( const uint8_t *buf, int32_t *len, int32_t *data,
                           FAST_STATE *pState, const uint8_t *pmap)
{
	int32_t iSize;

	/* Check the state structure to see if we have an encoded value */
	if(!ISBITSET(pmap, pState ->field))
	{
		// See if this field is valid in the state
		if(pState ->valid)
		{
			if( pState ->encodeType == OP_INCR)
				pState ->value.uint32Val++;
			else if(pState ->encodeType == OP_COPY)
				;
			else
				return AB_INVALID_STATE;

			*data = htonl ( pState ->value.uint32Val );
		}
		else
        {
			return AB_INVALID_STATE;
        }
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
	if(pState ->encodeType == OP_COPY || pState ->encodeType == OP_INCR)
	{
		pState ->valid = 1; 
		pState ->value.uint32Val = *data;
		pState ->size = sizeof(pState ->value.uint32Val);
	}

    *data = htonl ( *data );
	*len = iSize;
	return AB_OK;   
}

/* Decode a signed 16-bit integer and place the result in *data 
 * 
 * buf		The buffer containing the FAST message
 * len		Pointer to the length of buf, on return this will hold the number of bytes processed
 * data		This will hold the result, in NETWORK byte order
 * pState	Pointer to the FAST state information for the applicable field
 * pmap		The FAST pmap
 * 
 */
static int32_t DecodeI16 ( const uint8_t *buf, int32_t *len, int16_t *value, FAST_STATE *pState, const uint8_t *pmap)
{
	int iRet;
    union {
        int32_t l;
        int16_t s[2];
    } data;

	iRet =  DecodeI32( buf, len, &data.l, pState, pmap) ;

	*value = data.s[1];

	return(iRet);
}

/* Decode a signed 8-bit integer and place the result in *data 
 * 
 * buf		The buffer containing the FAST message
 * len		Pointer to the length of buf, on return this will hold the number of bytes processed
 * data		This will hold the result
 * pState	Pointer to the FAST state information for the applicable field
 * pmap		The FAST pmap
 * 
 */
static int32_t DecodeI8 ( const uint8_t *buf, int32_t *len, int8_t *value, FAST_STATE *pState, const uint8_t *pmap)
{
	int iRet;
    union {
        int32_t l;
        int8_t  b[4];
    } data;

	iRet =  DecodeI32( buf, len, &data.l, pState, pmap);

	*value = data.b[3];

	return(iRet);
}


/* Decode an ASCII string and put the result in *data 
 *
 * field	The field ID we are decoding, ( AB_MSG_TYPE, etc.)
 * buf		The buffer containing the FAST message
 * len		Pointer to the length of buf, on return this will hold the number of bytes processed
 * data		This will hold the result
 * pState	Pointer to the FAST state information for the applicable field
 * pmap		The FAST pmap
 */
static int32_t DecodeASCII ( const uint8_t *buf, int32_t *len, char *data,
                             FAST_STATE *pState, const uint8_t *pmap)
{
	int32_t iSize;

	/* Check the state structure to see if we have an encoded value */
	if(!ISBITSET(pmap,pState ->field))
	{
		if(pState ->valid)
		{
			if(pState ->encodeType == OP_COPY)
				memcpy(data,pState ->value.int8Val,pState ->size);
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
	if(pState ->encodeType == OP_COPY)
	{
		/* If the string is too long, we can not copy encode it */
		if(iSize <= AB_MAX_STRLEN)
		{
			pState ->valid = 1; 
			memcpy(pState ->value.int8Val,data,iSize);
			pState ->size = iSize;
		}
		else
			pState ->valid = 0; 
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
static int32_t DecodeBitmap (uint32_t field, const uint8_t *buf, int32_t *len, uint8_t *data, const uint8_t *pmap)
{
	int32_t iSize;
	int32_t j;
	int32_t bit=0; 

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
static int32_t ABFastValidate(const uint8_t *src, int32_t srcLen,
                              uint8_t *pmap, int32_t * pPmapLen)
{
    int32_t iOffset = 0;          /* offset within src of current position   */
    /* int32_t pPmapLen;             length of pmap */

    /* Duplicate of a check made before we come here
    if(srcLen < AB_MIN_FAST_MSG)
        return AB_INCOMPLETE_ERROR; */

    /* decode the pmap
	pPmapLen = PmapLen(src,AB_MAX_PMAP);
	memcpy(pmap,src,pPmapLen);    */
	* pPmapLen = copyPmap( pmap, src, AB_MAX_PMAP );
	iOffset += * pPmapLen;

	/* Make sure pmap is valid with a stop bit */
	if( (* pPmapLen < 1) || (pmap[* pPmapLen - 1] < 0x80) )
		return AB_INVALID_HEADER;

	/* pmap must always have field 0, TYPE   */
	if(!ISBITSET(pmap,AB_MSG_TYPE))
		return AB_INVALID_HEADER;

	return AB_OK;   
}

/*
* Decode a FAST message to ARCA Book message
* 
* dstMsg    Pointer to a ArcaL2MsgUnion structure to be filled with the
*           decoded message
* src		Pointer to a uint8_t buffer containing the FAST encoded message
* srcLen	uint32_t containing the length of the src buffer,
*           on return, this will contain the number of bytes processed from
*           the src buffer
* msgType	Pointer to a location to be filled with the decoded message type
* state		Pointer to the FAST state information table (must be initialized
*           for each packet (not message)
*
* returns 0 if success, < 0 if error
*/
int32_t ABFastDecode(union ArcaL2MsgUnion * dstMsg, const uint8_t * src,
                     int32_t * srcLen, uint16_t * msgType,
                     FAST_STATE * state ) /* was state, see below */
{
    uint8_t     pmap[AB_MAX_PMAP]={0,0}; /* You must init this to ZERO  !!!	*/
    int32_t     iOffset = 0;        /* offset within buf of current position */
    int32_t     iRet;               /* return code */
    int32_t     iLen;      /* length available in input buffer for processing */
    int32_t     iPmapLen;           /* length of pmap */
    /* FAST_STATE  localState[AB_MAX_FIELD]; Removed because the caller's
       state table is initialized by the caller before each packet.
       Caller's table was copied to this local table and copied back before
       a successful return.  Using the caller's table directly produces the
       same result unless there is an error.  In that case, we do not preserve
       the caller's table.  The only use I can see for that is if the caller
       expects to obtain a correct copy of the packet from the other feed
       or from a retransmission, and resume processing this packet from the
       same point using the preserved table.  I think it is a better idea to
       process the replacement packet from scratch. */

	/* message will always be at least 2 bytes,
       at least 1 byte for pmap, 1 byte for mandatory type */
	if(*srcLen < AB_MIN_FAST_MSG)
		return AB_INCOMPLETE_ERROR;

	/* validate the message, and make sure we have a full encoded message */
	if(AB_OK != (iRet=ABFastValidate(src, *srcLen, pmap, & iPmapLen)))
		return iRet;

	/* we already decoded the pmap, so advance Offset  */
	iOffset += iPmapLen ;

	/* pull dstMsg the message type  */
	iLen = *srcLen - iOffset; 
	
	/* store local copy of the state, in case we error out
	memcpy(localState,state,sizeof(localState)); see above */

	/* The first field transmitted for every ArcaBook FAST message is the
       message type field, whether or not the field is part of the unencoded
       message format. */
	if(AB_OK==(iRet=DecodeI16(&src[iOffset],&iLen,(int16_t *)msgType,&state[AB_MSG_TYPE],pmap)))
		iOffset+= iLen;
	else
		return iRet;
    *msgType = ntohs( *msgType );

#if       defined(VERBOSE_FAST_DECODE)
printf("in decode %d\n",*msgType);
#endif /* defined(VERBOSE_FAST_DECODE) */

    if (( *msgType >= ARCA_L2_ADD_MSG_TYPE       ) && 
        ( *msgType <= ARCA_L2_IMBALANCE_MSG_TYPE ))
    {              /* Handle the four most frequently-occurring message types */
        /* The first four fields are the same for all four of these messages. */
		dstMsg->Add.iMsgType = htons(*msgType);

		iLen = *srcLen - iOffset; 
		if(AB_OK==(iRet=DecodeI16(&src[iOffset], &iLen,
                                  (int16_t *)&dstMsg->Add.iSec,
                                  &state[AB_STOCK_IDX], pmap)))
			iOffset+= iLen;
		else
			return iRet;

		iLen = *srcLen - iOffset; 
		if(AB_OK==(iRet=DecodeI32(&src[iOffset], &iLen,
                                  (int32_t *)&dstMsg->Add.iSymbolSequence,
                                  &state[AB_SEQUENCE], pmap)))
			iOffset+= iLen;
		else
			return iRet;

		iLen = *srcLen - iOffset; 
		if(AB_OK==(iRet=DecodeI32(&src[iOffset], &iLen,
                                  (int32_t *)&dstMsg->Add.iSourceTime,
                                  &state[AB_TIME], pmap)))
			iOffset+= iLen;
		else
			return iRet;

        switch (*msgType) {

            case ARCA_L2_ADD_MSG_TYPE: /* 100 */
                /* decode the remainder of the 'add' message */
            case ARCA_L2_MODIFY_MSG_TYPE: /* 101 */
                /* decode the 'modify' message - same format as add */
			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->Add.iOrderID,&state[AB_ORDER_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->Add.iVolume,&state[AB_VOLUME],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->Add.iPrice,&state[AB_PRICE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI8(&src[iOffset],&iLen,(int8_t *)&dstMsg->Add.iPriceScale,&state[AB_PRICE_SCALE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->Add.cSide,&state[AB_BUY_SELL],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->Add.cExchangeID,&state[AB_EXCH_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->Add.cSecurityType,&state[AB_SECURITY_TYPE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI16(&src[iOffset],&iLen,(int16_t *)&dstMsg->Add.iFirm,&state[AB_FIRM_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI8(&src[iOffset],&iLen,(int8_t *)&dstMsg->Add.iSessionID,&state[AB_SESSION_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    break;

            case ARCA_L2_DELETE_MSG_TYPE: /* 102 */
                /* decode the remainder of the 'delete' message */

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->Delete.iOrderID,&state[AB_ORDER_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->Delete.cSide,&state[AB_BUY_SELL],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->Delete.cExchangeID,&state[AB_EXCH_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->Delete.cSecurityType,&state[AB_SECURITY_TYPE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI8(&src[iOffset],&iLen,(int8_t *)&dstMsg->Delete.iSessionID,&state[AB_SESSION_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI16(&src[iOffset],&iLen,(int16_t *)&dstMsg->Delete.iFirm,&state[AB_FIRM_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;
			    break;

            case ARCA_L2_IMBALANCE_MSG_TYPE: /* 103 */
                /* decode the remainder of the 'imbalance' message */
			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->Imbalance.iVolume,&state[AB_VOLUME],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->Imbalance.iTotalImbalance,&state[AB_ORDER_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->Imbalance.iMarketImbalance,&state[AB_BITMAP],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->Imbalance.iPrice,&state[AB_PRICE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI8(&src[iOffset],&iLen,(int8_t *)&dstMsg->Imbalance.iPriceScale,&state[AB_PRICE_SCALE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->Imbalance.cAuctionType,&state[AB_BUY_SELL],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->Imbalance.cExchangeID,&state[AB_EXCH_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->Imbalance.cSecurityType,&state[AB_SECURITY_TYPE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI8(&src[iOffset],&iLen,(int8_t *)&dstMsg->Imbalance.iSessionID,&state[AB_SESSION_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI16(&src[iOffset],&iLen,(int16_t *)&dstMsg->Imbalance.iAuctionTime,&state[AB_FIRM_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;
			    break;
        } /* End    switch (*msgType) */
    }              /* Handle the four most frequently-occurring message types */
    else {                         /* Less frequently-occurring message types */

        switch (*msgType) {

            case ARCA_L2_SYMBOL_IDX_MAP_MSG_TYPE: /* 35 */
			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI16(&src[iOffset],&iLen,(int16_t *)&dstMsg->SymbolUpdate.iSec,&state[AB_STOCK_IDX],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;
#if       defined(VERBOSE_FAST_DECODE)
printf("iSec %d\n",dstMsg->SymbolUpdate.iSec);
#endif /* defined(VERBOSE_FAST_DECODE) */

                iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI8(&src[iOffset],&iLen,(int8_t *)&dstMsg->SymbolUpdate.iSessionID,&state[AB_SESSION_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;
			    /* null pad symbols */
			    memset(dstMsg->SymbolUpdate.sSymbol,'\0',sizeof(dstMsg->SymbolUpdate.sSymbol)); 

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII (&src[iOffset],&iLen,(char *)&dstMsg->SymbolUpdate.sSymbol,&state[AB_SYMBOL_STRING],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;
#if       defined(VERBOSE_FAST_DECODE)
printf("sSymbol %s\n",dstMsg->SymbolUpdate.sSymbol);
#endif /* defined(VERBOSE_FAST_DECODE) */
                break;

            case ARCA_L2_SYMBOL_CLEAR_MSG_TYPE: /* 36 */
			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->SymbolClear.iNextSeqNum,&state[AB_SEQUENCE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI16(&src[iOffset],&iLen,(int16_t *)&dstMsg->SymbolClear.iSec,&state[AB_STOCK_IDX],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;
    			
			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII (&src[iOffset],&iLen,(char *)&dstMsg->SymbolClear.iSessionID,&state[AB_SESSION_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;
			    break;

            case ARCA_L2_FIRM_IDX_MAP_MSG_TYPE: /* 37 */
			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI16(&src[iOffset],&iLen,(int16_t *)&dstMsg->FirmUpdate.iFirm,&state[AB_FIRM_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;
#if       defined(VERBOSE_FAST_DECODE)
printf("iFirm %d\n",dstMsg->FirmUpdate.iFirm);
#endif /* defined(VERBOSE_FAST_DECODE) */
			    /* null pad firm */
			    memset(dstMsg->FirmUpdate.sFirmID,'\0',sizeof(dstMsg->FirmUpdate.sFirmID));
			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII (&src[iOffset],&iLen,(char *)&dstMsg->FirmUpdate.sFirmID,&state[AB_FIRM_STRING],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;
#if       defined(VERBOSE_FAST_DECODE)
printf("sFirmID %4s\n",dstMsg->FirmUpdate.sFirmID);
#endif /* defined(VERBOSE_FAST_DECODE) */

			    break;

        case ARCA_L2_BOOK_REFRESH_MSG_TYPE: /* 32 */
            /* decode the 'Book refresh' message */

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->BookRefreshOrder.iSymbolSequence,&state[AB_SEQUENCE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->BookRefreshOrder.iSourceTime,&state[AB_TIME],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->BookRefreshOrder.iOrderID,&state[AB_ORDER_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->BookRefreshOrder.iVolume,&state[AB_VOLUME],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->BookRefreshOrder.iPrice,&state[AB_PRICE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI8(&src[iOffset],&iLen,(int8_t *)&dstMsg->BookRefreshOrder.iPriceScale,&state[AB_PRICE_SCALE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->BookRefreshOrder.cSide,&state[AB_BUY_SELL],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->BookRefreshOrder.cExchangeID,&state[AB_EXCH_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeASCII(&src[iOffset],&iLen,(char *)&dstMsg->BookRefreshOrder.cSecurityType,&state[AB_SECURITY_TYPE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI16(&src[iOffset],&iLen,(int16_t *)&dstMsg->BookRefreshOrder.iFirm,&state[AB_FIRM_ID],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;

			    break;

            case ARCA_L2_SEQ_RESET_MSG_TYPE: /* 1 */			//seq number reset
			    iLen = *srcLen - iOffset; 
			    if(AB_OK==(iRet=DecodeI32(&src[iOffset],&iLen,(int32_t *)&dstMsg->SequenceReset.iNextSeqNum,&state[AB_SEQUENCE],pmap)))
				    iOffset+= iLen;
			    else
				    return iRet;
			    break;

		    default:
		    /* catch all other messages here */
			    /* just take the length of the message starting at 1 byte past the header */
			    iOffset+= iLen;
			    iLen = *srcLen - iOffset;

			    iRet=DecodeBitmap(AB_BITMAP,&src[iOffset],&iLen,(uint8_t *)(dstMsg),pmap);

			    if (AB_OK == iRet)
				    iOffset+= iLen;
			    else
				    return iRet;

			    break;
        } /* end   switch() */
    }                              /* Less frequently-occurring message types */

	/* return the length of the input buffer used */
	*srcLen = iOffset;

	/* update the state
	memcpy(state,localState,sizeof(localState)); see above */
	return AB_OK;
}

