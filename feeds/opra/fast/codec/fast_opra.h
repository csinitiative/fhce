/**************************************************************************
*  System        : OPRA
*
*  Module Name   : opra_fast.h
*  Copyright (c) : SIAC 2007
*  Date          : 3/2007
*
*  Description   : Definition of opra messages
***************************************************************************/

#ifndef _fast_opra_h_
#define _fast_opra_h_

#include "fast_api.h"

////////////////////////////////////////////////////////////////////////////////
//
//  enumerators for opra message field tags
//
////////////////////////////////////////////////////////////////////////////////

enum opra_tid
{
    OPRA_BASE_TID=0
};

enum opra_msg_field_tags_v2
{
        // opra header

        MESSAGE_CATEGORY_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 0),
        MESSAGE_TYPE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 1),
        PARTICIPANT_ID_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 2),
        RETRANSMISSION_REQUESTER_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 3),
        MESSAGE_SEQUENCE_NUMBER_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_INCR, OPRA_BASE_TID, 4),
        TIME_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 5),

    	//sequential order 

        SECURITY_SYMBOL_V2 = 
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 6),
        EXPIRATION_MONTH_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 7),
        EXPIRATION_DATE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 8),
        YEAR_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 9),
        STRIKE_PRICE_DENOMINATOR_CODE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 10),
        EXPLICIT_STRIKE_PRICE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 11),
        STRIKE_PRICE_CODE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 12),
        VOLUME_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 13),
        OPEN_INT_VOLUME_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 14),
        PREMIUM_PRICE_DENOMINATOR_CODE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 15),
        PREMIUM_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 16),
        OPEN_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 17),
        HIGH_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 18),
        LOW_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 19),
        LAST_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 20),
        NET_CHANGE_INDICATOR_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 21),
        NET_CHANGE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 22),
        UNDERLYING_PRICE_DENOM_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 23),
        UNDERLYING_STOCK_PRICE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 24),

        BID_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 25),
        BID_SIZE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 26),
        OFFER_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 27),
        OFFER_SIZE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 28),
        SESSION_INDICATOR_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 29),
        BBO_INDICATOR_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 30),

        BEST_BID_PARTICIPANT_ID_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 31),
        BEST_BID_PRICE_DENOMINATOR_CODE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 32),
        BEST_BID_PRICE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 33),
        BEST_BID_SIZE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 34),

        BEST_OFFER_PARTICIPANT_ID_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 35),

        BEST_OFFER_PRICE_DENOMINATOR_CODE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 36),
        BEST_OFFER_PRICE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 37),
        BEST_OFFER_SIZE_V2 = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 38),
    	NUMBER_OF_INDICES_IN_GROUP_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 39),
    	NUMBER_OF_FOREIGN_CURRENCY_SPOT_VALUES_IN_GROUP_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 40),
    	INDEX_SYMBOL_V2 = 
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 41),
    	INDEX_VALUE_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 42),
    	BID_INDEX_VALUE_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 43),
    	OFFER_INDEX_VALUE_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 44),
    	FCO_SYMBOL_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 45),
    	DECIMAL_PLACEMENT_INDICATOR_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 46),
    	FOREIGN_CURRENCY_SPOT_VALUE_V2 =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 47),
    	TEXT_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 48),
    	DEF_MSG_V2 =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 49),
};

enum opra_msg_field_tags
{
        // opra header
        MESSAGE_CATEGORY = 
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 1),
        MESSAGE_TYPE =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 2),
        PARTICIPANT_ID =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 3),
        RETRANSMISSION_REQUESTER =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 4),
        MESSAGE_SEQUENCE_NUMBER =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_INCR, OPRA_BASE_TID, 5),
        TIME =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 6),

        // category 'k', 'a', 'u'
        SECURITY_SYMBOL =
            MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 7),
        EXPIRATION_DATE =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 8),
        YEAR =
            MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 9),
        STRIKE_PRICE_CODE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 10),
        STRIKE_PRICE_DENOMINATOR_CODE = 
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 11),
        EXPLICIT_STRIKE_PRICE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 12),
        PREMIUM_PRICE_DENOMINATOR_CODE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 13),
        BID_PRICE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 14),
        BID_SIZE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 15),
        OFFER_PRICE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 16),
        OFFER_SIZE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 17),
        SESSION_INDICATOR =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 18),
        BBO_INDICATOR =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 19),

        // best bid appendage
        BEST_BID_PARTICIPANT_ID =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 20),
        BEST_BID_PRICE_DENOMINATOR_CODE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 21),
        BEST_BID_PRICE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 22),
        BEST_BID_SIZE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 23),

        // best offer appendage
        BEST_OFFER_PARTICIPANT_ID =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 24),
        BEST_OFFER_PRICE_DENOMINATOR_CODE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 25),
        BEST_OFFER_PRICE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 26),
        BEST_OFFER_SIZE = 
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 27),

        // reserved fields
        RESERVED_FIELD_1 =
           MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 28),
        RESERVED_FIELD_2 =
           MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 29),
        RESERVED_FIELD_3 =
           MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 30),

        // category 'd'
        OPEN_INT_VOLUME =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 31),

        // category 'O'
        VOLUME = 
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 32),
        PREMIUM_PRICE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 33),

        // category 'f', 'F'
        OPEN_PRICE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 34),
        HIGH_PRICE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 35),
        LOW_PRICE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 36),
        LAST_PRICE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 37),
        NET_CHANGE_INDICATOR =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 38),
        NET_CHANGE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 39),
        UNDERLYING_PRICE_DENOM =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 40),
        UNDERLYING_STOCK_PRICE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 41),

       // category 'Y'
    	NUMBER_OF_INDICES_IN_GROUP =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 42),
    	INDEX_SYMBOL =
           MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 43),
    	INDEX_VALUE =
           MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 44),
    	BID_INDEX_VALUE =
           MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 45),
    	OFFER_INDEX_VALUE =
           MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 46),
    	NUMBER_OF_FOREIGN_CURRENCY_SPOT_VALUES_IN_GROUP =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 47),
    	FCO_SYMBOL =
           MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 48),
    	DECIMAL_PLACEMENT_INDICATOR =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 49),
    	FOREIGN_CURRENCY_SPOT_VALUE =
           MAKE_TAG (FAST_TYPE_U32, FAST_OP_COPY, OPRA_BASE_TID, 50),

        // category 'C' & 'H'
    	TEXT =
           MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 51),

        // all messages with category not defined
    	DEF_MSG =
           MAKE_TAG (FAST_TYPE_STR, FAST_OP_COPY, OPRA_BASE_TID, 52),
};

////////////////////////////////////////////////////////////////////////////////
//
//  structure for opra header 
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct opraHdr_v2 
{
   unsigned char participantId;
   unsigned char retran; 
   unsigned char category;
   unsigned char type;
   unsigned int  seqNumber;
   unsigned int  time;
}OpraMsgHdr_v2;


////////////////////////////////////////////////////////////////////////////////
//
//  structure for Default message
//
////////////////////////////////////////////////////////////////////////////////

typedef struct defMsg
{
  unsigned char category;
  unsigned char body[450];
} CatDefMsg;

////////////////////////////////////////////////////////////////////////////////
//
//  common structure for best bid , best offer
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct append_v2
{
   unsigned char partId;
   unsigned char denominator;
   unsigned int  price;
   unsigned int  size;
}Appendage_v2;


///////////////////////////////////////////////////////////////////////////////
//  structure for best-bid offer
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct best_bid_offer_v2
{
   Appendage_v2 bestBid;
   Appendage_v2 bestOffer;
} Best_Bid_Offer_v2;


////////////////////////////////////////////////////////////////////////////////
//
//  structure for best-bid offer
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef union bbo_appendage_v2
{
   Appendage_v2      bestBid;
   Appendage_v2      bestOffer;
   Best_Bid_Offer_v2 bestBidOffer;
} BBO_Appendage_v2;


////////////////////////////////////////////////////////////////////////////////
//
//  structure for catgoty 'k' message
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct QuoteSizeBody_v2
{
   OpraMsgHdr_v2 hdr;
   unsigned   char symbol[5];
   unsigned   char expirationMonth;
   unsigned   char expirationDate[2];
   unsigned   char year[2];
   unsigned   char strikePriceDenomCode;
   unsigned   int  explicitStrike;
#if !defined PH2
   unsigned   char strikePriceCode;
#endif
   unsigned   char premiumPriceDenomCode;
   unsigned   int  bidQuote;
   unsigned   int  bidSize;
   unsigned   int  askQuote;
   unsigned   int  askSize;
   unsigned   char sessionIndicator;
   unsigned   char bboIndicator;
   BBO_Appendage_v2   bbo;          
} CatkMsg_v2; 


////////////////////////////////////////////////////////////////////////////////
//
//  structure for catgoty 'd' message
//
////////////////////////////////////////////////////////////////////////////////

//	V2


typedef struct OpenIntBody_v2
{
   OpraMsgHdr_v2 hdr;
   unsigned   char symbol[5];
   unsigned   char expirationMonth;
   unsigned   char expirationDate[2];
   unsigned   char year[2];
   unsigned   char strikePriceDenomCode;
   unsigned   int  explicitStrike;
#if !defined PH2
   unsigned   char strikePriceCode;
#endif
   unsigned   int  openIntVolume;
} CatdMsg_v2;

////////////////////////////////////////////////////////////////////////////////
//
//  structure for catgoty 'a' message
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct LastSaleBody_v2
{
   OpraMsgHdr_v2 hdr;
   unsigned   char symbol[5];
   unsigned   char expirationMonth;
   unsigned   char expirationDate[2];
   unsigned   char year[2];
   unsigned   char strikePriceDenomCode;
   unsigned   int  explicitStrike;
#if !defined PH2
   unsigned   char strikePriceCode;
#endif
   unsigned   int  volume;
   unsigned   char premiumPriceDenomCode;
   unsigned   int  premium;
   unsigned   char sessionIndicator;
} CataMsg_v2;

////////////////////////////////////////////////////////////////////////////////
//
//  structure for catgoty 'f' message
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct EodBody_v2
{
   OpraMsgHdr_v2 hdr;
   unsigned   char symbol[5];
   unsigned   char expirationMonth;
   unsigned   char expirationDate[2];
   unsigned   char year[2];
   unsigned   char strikePriceDenomCode;
   unsigned   int  explicitStrike;
#if !defined PH2
   unsigned   char strikePriceCode;
#endif
   unsigned   int  volume;
   unsigned   int  openIntVolume;
   unsigned   char premiumPriceDenomCode;
   unsigned   int  open;
   unsigned   int  high;
   unsigned   int  low;
   unsigned   int  last;
   unsigned   char netChangeIndicator;
   unsigned   int  netChange;
   unsigned   char underlyingPriceDenomCode;
   unsigned   int  underlyingStockPrice;
   unsigned   int  bidQuote;
   unsigned   int  askQuote;
} CatfMsg_v2;


////////////////////////////////////////////////////////////////////////////////
//
//  structure for catgoty 'O' message
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct FcoLastSale_v2
{
   OpraMsgHdr_v2 hdr;
   unsigned   char symbol[3];
   unsigned   char expirationDate[2];
#if !defined PH2
   unsigned   char strikePriceCode;
#endif
   unsigned   int  volume;
   unsigned   int  premiumPrice;
} CatOMsg_v2;


////////////////////////////////////////////////////////////////////////////////
//
//  structure for catgoty 'U' message
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct FcoQuote_v2
{
   OpraMsgHdr_v2 hdr;
   unsigned   char symbol[3];
   unsigned   char expirationDate[2];
#if !defined PH2
   unsigned   char strikePriceCode;
#endif
   unsigned   int  bidPrice;
   unsigned   int  offerPrice;
} CatUMsg_v2;

////////////////////////////////////////////////////////////////////////////////
//
//  structure for catgoty 'F' message
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct FcoEod_v2
{
   OpraMsgHdr_v2 hdr;
   unsigned   char symbol[3];
   unsigned   char expirationDate[2];
#if !defined PH2
   unsigned   char strikePriceCode;
#endif
   unsigned   int  volume;
   unsigned   int  openIntVolume;
   unsigned   int  open;
   unsigned   int  high;
   unsigned   int  low ;
   unsigned   int  last;
   unsigned   char netChangeIndicator;
   unsigned   int  netChange;
   unsigned   int  underlyingStockPrice;
} CatFMsg_v2;

////////////////////////////////////////////////////////////////////////////////
//
//  structure for catgoty 'Y' message body
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct ulValueBody_v2
{
    unsigned int  numOfIndices;
    struct
    {
        unsigned char symbol[3];
        union
        {
            unsigned char indexValue[8];
            struct
            {
                unsigned char bidValueIndex[8];
                unsigned char offerValueIndex[8];
            } bidOffer;
            struct
            {
                unsigned char decimalPlacementIndicator[2];
                unsigned int  foreignCurSpotValue;
            }fcSpotVal;

        } group;
    } indexGroup[5];
} CatYBody_v2;

////////////////////////////////////////////////////////////////////////////////
//
//  structure for catgoty 'Y' message
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct ulValueMsg_v2
{
   OpraMsgHdr_v2 hdr;
   CatYBody_v2 body;
} CatYMsg_v2;

////////////////////////////////////////////////////////////////////////////////
//
//  structure for catgoty 'C' message
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct adminMsg_v2
{
    OpraMsgHdr_v2 hdr;
    unsigned char text[450];    
} CatCMsg_v2;

////////////////////////////////////////////////////////////////////////////////
//
//  structure for catgoty 'H' message
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef struct ctrlMsg_v2
{
    OpraMsgHdr_v2 hdr;
    unsigned char text[450];    
} CatHMsg_v2;

////////////////////////////////////////////////////////////////////////////////
//
//  union of opra messages
//
////////////////////////////////////////////////////////////////////////////////

//	V2

typedef union OpraMessage_v2
{
    CatDefMsg  defMsg;
    CatkMsg_v2 quoteSizeBody;
    CatdMsg_v2 openIntBody;
    CataMsg_v2 lastSaleBody;
    CatfMsg_v2 eodBody;
    CatOMsg_v2 fcoLastSale;
    CatUMsg_v2 fcoQuote;
    CatFMsg_v2 fcoEod;
    CatYMsg_v2 ulValueMsg;
    CatCMsg_v2 adminMsg;
    CatHMsg_v2 ctrlMsg;
}OpraMsg_v2;

#endif
