/********************************************************************************************

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
that you make are the intellectual property of and are owned by NYSE Euronext.  The Software is protected by
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

***************************************************************************************************************/


/*
*	Program : ArcaL2Msg.h
*	By      : Fred Jones/David Le/Anil Nagasamudra 
*	Date    : 12/1/06
*	Purpose : ArcaBook Binary Multicast Messages
*
*	Note: All prices are in readable form. All fields are 
*	left justify and NULL padded. The first char of these 
*	structures will always be the message type.
*
*	Edit history:
*
*	Date		By		Reason
*	========	====	================================================
*	12-01-06	FLJ		Define New Messges
*
*
*/

#ifndef __ARCABOOK_L2_MSG_H__
#define __ARCABOOK_L2_MSG_H__

#include <sys/types.h>

#ifdef OS_LINUX
#include <stdint.h>
#endif

#define ARCA_L2_SYMBOL_LEN					16
#define ARCA_L2_SOURCEID_LEN				20
#define ARCA_L2_FIRM_ID_LEN					5

typedef enum  {
    ARCAM_RETRANS_ORIGINAL                  = 1,
    ARCAM_RETRANS_RETRANSMITTED             = 2,
    ARCAM_RETRANS_REFRESH_RETRANSMISSION    = 5,
    ARCAM_RETRANS_FAILOVER_RETRANSMISSION   = 6,
    ARCAM_RETRANS_START_OF_UPDATE           = 7,
    ARCAM_RETRANS_ONLY_PACKET_IN_UPDATE     = 8,
    ARCAM_RETRANS_END_OF_UPDATE             = 9,
}eRETRANS_FLAG ;

/* MsgType definitions from message header */
typedef enum {
    ARCA_L2_SEQ_RESET_MSG_TYPE                  = 1,
    ARCA_L2_HB_MSG_TYPE                         = 2,
    ARCA_L2_UNAVAILABLE_MSG_TYPE                = 5,
    ARCA_L2_RETRANS_RESPONSE_MSG_TYPE           = 10,
    ARCA_L2_RETRANS_REQUEST_MSG_TYPE            = 20,
    ARCA_L2_HB_RESPONSE_MSG_TYPE                = 24,
    ARCA_L2_BOOK_REFRESH_REQUEST_MSG_TYPE       = 30,
    ARCA_L2_IMBALANCE_REFRESH_REQUEST_MSG_TYPE  = 31,
    ARCA_L2_BOOK_REFRESH_MSG_TYPE               = 32,
    ARCA_L2_IMBALANCE_REFRESH_MSG_TYPE          = 33,
    ARCA_L2_SYMBOL_IDX_MAP_REQUEST_MSG_TYPE     = 34,
    ARCA_L2_SYMBOL_IDX_MAP_MSG_TYPE             = 35,
    ARCA_L2_SYMBOL_CLEAR_MSG_TYPE               = 36,
    ARCA_L2_FIRM_IDX_MAP_MSG_TYPE               = 37,
    ARCA_L2_FIRM_IDX_MAP_REQUEST_MSG_TYPE       = 38,
    ARCA_L2_BOOK_MSG_TYPE                       = 99,
    /* Different field, but conveniently values don't overlap */
    ARCA_L2_ADD_MSG_TYPE                        = 100,
    ARCA_L2_MODIFY_MSG_TYPE                     = 101,
    ARCA_L2_DELETE_MSG_TYPE                     = 102,
    ARCA_L2_IMBALANCE_MSG_TYPE                  = 103,
    ARCAM_MAX_TYPE
} eMSG_TYPE ;


/****************** EXTERNAL MESSAGES ************************/


/******************************************
 * External Message Header
 *
 * Size = 16 
 ******************************************/
struct ArcaL2Header_t
{
	uint16_t	iMsgSize;				// Refers to overall Packet Length minus 2 bytes
	uint16_t	iMsgType;				// 1   - Sequence Number Reset
								// 2   - HB Message
								// 5   - Message Unavailable
								// 10  - Retrans Request Message
								// 20  - Retrans Response Message
								// 23  - Refresh Request Message
								// 24  - HB Refresh Response Message
								// 30  - Book Refresh Request Message
								// 31  - Imbalance Refresh Request Message
								// 33  - Imbalance Refresh Message
								// 34  - Symbol Index Mapping Request Message
								// 35  - Symbol Index Mapping Message
								// 36  - Symbol Clear
								// 37  - firm Index Mapping Messasge
								// 38  - firm Index Mapping Messasge Request
								// 99  - Book Message (Add, Modify, Delete)

	uint32_t	iMsgSeqNum;				// Line Sequence Number
	uint32_t	iSendTime;				// Milliseconds since Midnight
	uint8_t		iProductID;				// 115 - Book Feed
	uint8_t		iRetransFlag;				// 1    - Original Message
								// 2    - Retrans Message
								// 3    - Message Replay
								// 4    - Retrans of replayed Message
								// 5    - Refresh Retrans
                                // 6    - Failover retransmission
                                // 7    - Start of Update
                                // 8    - Only one packed in update
                                // 9    - End of Update
	uint8_t		iNumBodyEntries;
	uint8_t		iChannelID;				// Filler for Now. May specifiy id in future.
};
typedef struct ArcaL2Header_t ArcaL2Header_t;


/******************************************
 * External Refresh Message Header
 *
 * Message Type = 32
 *
 * Size = 48
 ******************************************/
struct ArcaL2FreshHeader_t
{
	ArcaL2Header_t Header;					// Packet Header

	uint8_t		iFiller;
	uint8_t		iSessionID;				// Source Session ID
	uint16_t	iSec;					// Stock Index
	uint16_t	iCurrentPktNum;				// Current Refresh Packet
	uint16_t	iTotalPktNum;				// Total Packets for this Refresh

	uint32_t	iLastSymbolSeqNum;			// Last seq number for symbol
	uint32_t	iLastPktSeqNum;				// Last Pkt seq number for symbol	

	char 		sSymbol[ARCA_L2_SYMBOL_LEN];		// Symbol
};
typedef struct ArcaL2FreshHeader_t ArcaL2FreshHeader_t;


/******************************************
 * External Book Order Message
 *
 * Message Type = 32
 *
 * Size = 28+
 ******************************************/
struct ArcaL2BookOrder_t
{
//	ArcaL2FreshHeader_t Header;			// Message Header

	uint32_t	iSymbolSequence;		// Symbol Sequence
	uint32_t	iSourceTime;			// Milliseconds since midnight
	uint32_t	iOrderID;			// Order Reference #
	uint32_t	iVolume;			// Volume		
	uint32_t	iPrice;				// Price		

	uint8_t		iPriceScale;			// Price Scale Code

	char		cSide;				// B - Buy
							// S - Sell

	char		cExchangeID;			// N - NYSE
							// P - NYSE ARCA

	char		cSecurityType;			// E - Equity
							// B - Bond

	uint16_t	iFirm;				// Firm Index

	char		sFiller[2];			// Filler
};
typedef struct ArcaL2BookOrder_t ArcaL2BookOrder_t;


/******************************************
 * External Add Message
 *
 * Message Type  = 99/100
 *
 * Size = 32+
 ******************************************/
struct ArcaL2Add_t
{
//	ArcaL2Header_t Header;				// Message Header

	uint16_t	iSec;					// Security/Stock Index
	uint16_t	iMsgType;				// 100 - Add Message
	
	uint32_t	iSymbolSequence;		// Symbol Sequence
	uint32_t	iSourceTime;			// Milliseconds since midnight
	uint32_t	iOrderID;				// Order Reference #
	uint32_t	iVolume;				// Volume		
	uint32_t	iPrice;					// Price

	uint8_t		iPriceScale;			// Price Scale Code

	char		cSide;					// B - Buy
										// S - Sell

	char		cExchangeID;			// N - NYSE
										// P - NYSE ARCA

	char		cSecurityType;			// E - Equity
										// B - Bond

	uint16_t	iFirm;					// Firm Index

	uint8_t		iSessionID;				// Source Session ID

	char		cFiller;				// Filler
};
typedef struct ArcaL2Add_t ArcaL2Add_t;


/******************************************
 * External Modify Message
 *
 * Message Type  = 99/101
 *
 * Size = 32+
 ******************************************/
struct ArcaL2Modify_t
{
//	ArcaL2Header_t Header;				// Message Header

	uint16_t	iSec;				// Stock Index
	uint16_t	iMsgType;			// 101 - Modify Message
	
	uint32_t	iSymbolSequence;		// Symbol Sequence
	uint32_t	iSourceTime;			// Milliseconds since midnight
	uint32_t	iOrderID;			// Order Reference #
	uint32_t	iVolume;			// Volume		
	uint32_t	iPrice;				// Price

	uint8_t		iPriceScale;			// Price Scale Code

	char		cSide;				// B - Buy
							// S - Sell

	char		cExchangeID;			// N - NYSE
							// P - NYSE ARCA

	char		cSecurityType;			// E - Equity
							// B - Bond

	uint16_t	iFirm;				// Firm Index

	uint8_t		iSessionID;			// Source Session ID

	char		cFiller;			// Filler

};
typedef struct ArcaL2Modify_t ArcaL2Modify_t;


/******************************************
 * External Delete Message
 *
 * Message Type = 99/102
 *
 * Size = 24+
 ******************************************/
struct ArcaL2Delete_t
{
//	ArcaL2Header_t Header;				// Message Header

	uint16_t	iSec;				// Stock Index
	uint16_t	iMsgType;			// 102 - Delete Message
	
	uint32_t	iSymbolSequence;		// Symbol Sequence
	uint32_t	iSourceTime;			// Milliseconds since midnight
	uint32_t	iOrderID;			// Order Reference #

	char		cSide;				// B - Buy
							// S - Sell

	char		cExchangeID;			// N - NYSE
							// P - NYSE ARCA

	char		cSecurityType;			// E - Equity
							// B - Bond

	uint8_t		iSessionID;			// Source Session ID

	uint16_t	iFirm;				// Firm Index

	char		sFiller[2];			// Filler

};
typedef struct ArcaL2Delete_t ArcaL2Delete_t;


/******************************************
 * External Imbalance Message
 *
 * Message Type = 99/103
 *
 * Size = 36+
 ******************************************/
struct ArcaL2Imbalance_t
{
//	ArcaL2Header_t Header;				// Message Header

	uint16_t	iSec;				// Stock Index
	uint16_t	iMsgType;			// 103 - Imbalance Message
	
	uint32_t	iSymbolSequence;		// Symbol Sequence
	uint32_t	iSourceTime;			// Milliseconds since midnight
	uint32_t	iVolume;			// Indicative Match Volume		
	int32_t		iTotalImbalance;		// Total Imbalance Volume		
	int32_t		iMarketImbalance;		// Market Imbalance Volume		
	uint32_t	iPrice;				// Indicative Match Price

	uint8_t		iPriceScale;			// Price Scale Code

	char		cAuctionType;			// O - Open
							// M - Market
							// H - Halt
							// C - Closing

	char		cExchangeID;			// N - NYSE
							// P - NYSE ARCA

	char		cSecurityType;			// E - Equity
							// B - Bond

	uint8_t		iSessionID;			// Source Session ID

	char		cFiller;			// Filler

	uint16_t	iAuctionTime;			// Projected Auction Time (hhmm)
};
typedef struct ArcaL2Imbalance_t ArcaL2Imbalance_t;


/************************************
 * External Imbalance Request (TCP/IP)
 *
 * Message Type = 31
 *
 * Size = 40
 ************************************/
struct ArcaL2ImbalanceRequest_t
{
	struct ArcaL2Header_t	Header;

	uint16_t	iSec;				// Symbol
	uint8_t		iSessionID;			// ME Session Id
  	char		cFiller;			// Filler

	char		sSourceID[ARCA_L2_SOURCEID_LEN];// Source ID
};
typedef struct ArcaL2ImbalanceRequest_t ArcaL2ImbalanceRequest_t;


/*********************************
 * External Refresh Request (TCP/IP)
 *
 * Message Type = 30
 *
 * Size = 40
 *********************************/
struct ArcaL2RefreshRequest_t
{
	struct ArcaL2Header_t	Header;

	uint16_t	iSec;				// Symbol
        uint8_t         iSessionID;			// ME Session Id
        char            cFiller;                        // Filler
	char		sSourceID[ARCA_L2_SOURCEID_LEN];// Source ID
};
typedef struct ArcaL2RefreshRequest_t ArcaL2RefreshRequest_t;


/************************************
 * External Retrans Request (TCP/IP)
 *
 * Message Type = 20
 *
 * Size = 44
 ************************************/
struct ArcaL2RetransRequest_t
{
	struct ArcaL2Header_t	Header;

	uint32_t	iBeginSeqNum;			// Begin Sequence Number
	uint32_t	iEndSeqNum;			// End Sequence Number
	char		sSourceID[ARCA_L2_SOURCEID_LEN];// Source ID
};
typedef struct ArcaL2RetransRequest_t ArcaL2RetransRequest_t;


/**************************************
 * Retrans Response Message (TCP/IP)
 *
 * Message Type = 10
 *
 * Size = 44
 **************************************/
struct ArcaL2RetransResponse_t
{
	struct ArcaL2Header_t	Header;

	uint32_t	iSourceSeqNum;			// Source Sequence Number
	char		sSourceID[ARCA_L2_SOURCEID_LEN];// Source ID

	char		cStatus;			// A - Accepted
							// R - Rejected

	uint8_t		iRejectReason;			// 0 - Accepted
							// 1 - Reject due to Permissions
							// 2 - Reject due to invalid sequence range
							// 3 - Reject due to maximum packet sequence range (> 1000)
							// 4 - Reject due to maximum num of requests in a day (> 500)
							// 5 - Reject due to maximum num of refresh requests in a day (> 500)
	char		sFiller[2];
};
typedef struct ArcaL2RetransResponse_t ArcaL2RetransResponse_t;


/**************************
 * Unavailable Message
 *
 * Message Type = 5
 *
 * Size = 24
 **************************/
struct ArcaL2Unavailable_t
{
	struct		ArcaL2Header_t		Header;

	uint32_t	iBeginSeqNum;		// Begin Sequence Number
	uint32_t	iEndSeqNum;		// End Sequence Number
};
typedef struct ArcaL2Unavailable_t ArcaL2Unavailable_t;


/***********************************************
 * External Symbol Mapping Request (TCP/IP)
 *
 * Message Type = 34
 *
 * Size = 40
 ************************************************/
struct ArcaL2SymbolUpdateRequest_t
{
	ArcaL2Header_t Header;			// Message Header

	uint16_t	iSec;			// Stock Index - < 1 send all 
						// 		 > 0 send only individual 

	uint8_t		iSessionID;		// Source Session ID
	uint8_t		iRetransmitMethod;	//0 . retransmit via UDP (this is the default)
						//1 . retransmit via TCP/IP connection

    char		sSourceID[ARCA_L2_SOURCEID_LEN];// Source ID
};
typedef struct ArcaL2SymbolUpdateRequest_t ArcaL2SymbolUpdateRequest_t;


/***********************************************
 * External Symbol Mapping 
 *
 * Message Type = 35
 *
 * Size = 20+
 ************************************************/
struct ArcaL2SymbolUpdate_t
{
//	ArcaL2Header_t Header;			// Message Header

	uint16_t	iSec;			// Stock Index

	uint8_t		iSessionID;		// Source Session ID
	char		cSysCode;		// System Code - filler to customer 
						// L - Listed
						// O - OTC
						// E - ETF

	char sSymbol[ARCA_L2_SYMBOL_LEN];	// Symbol
};
typedef struct ArcaL2SymbolUpdate_t ArcaL2SymbolUpdate_t;


/***********************************************
 * External Firm Mapping Request (TCP/IP)
 *
 * Message Type = 38
 *
 * Size = 40
 ************************************************/
struct ArcaL2FirmUpdateRequest_t
{
	ArcaL2Header_t Header;			// Message Header

	uint16_t	iFirm;			// Stock Index - < 1 send all 
						// 		 > 0 send only individual

	uint8_t		iRetransmitMethod;	//0 . retransmit via UDP (this is the default)
						//1 . retransmit via TCP/IP connection


	uint8_t		iFiller;		// Filler

	char		sSourceID[ARCA_L2_SOURCEID_LEN];// Source ID
};
typedef struct ArcaL2FirmUpdateRequest_t ArcaL2FirmUpdateRequest_t;


/***********************************************
 * External Firm Mapping 
 *
 * Message Type = 37
 *
 * Size = 12+
 ************************************************/
struct ArcaL2FirmUpdate_t
{
//	ArcaL2Header_t Header;			// Message Header

	uint16_t	iFirm;			// Firm Index
	char		sFiller[5];		// Filler

	char		sFirmID[ARCA_L2_FIRM_ID_LEN];	// Firm ID

};
typedef struct ArcaL2FirmUpdate_t ArcaL2FirmUpdate_t;


/************************************************
 * External Message-Packet Sequence Number Reset
 *
 * Message Type = 1
 *
 * Total Size = 16+
 ************************************************/
struct ArcaL2SequenceReset_t
{
//	ArcaL2Header_t	Header;
	uint32_t	iNextSeqNum;		// Next Sequence Number
};
typedef struct ArcaL2SequenceReset_t ArcaL2SequenceReset_t;


/***************************************
 * External Symbol Clear Message
 *
 * Message Type = 36
 *
 * Size = 8+
 **************************************/
struct ArcaL2SymbolClear_t
{
	//ArcaL2Header_t	Header;

	uint32_t	iNextSeqNum;            // Next Sequence Number
	uint16_t	iSec;			// Stock Index

	uint8_t		iSessionID;		// Source Session ID
	char		cFiller;		// Filler
};
typedef struct ArcaL2SymbolClear_t ArcaL2SymbolClear_t;


/**************************************
 * External Heartbeat Response Message (TCP/IP)
 *
 * Message Type = 24
 *
 * Size = 36
 *************************************/
struct ArcaL2HeartbeatResponse_t
{
	ArcaL2Header_t	Header;

	char	sSourceID[ARCA_L2_SOURCEID_LEN];// Source ID
};
typedef struct ArcaL2HeartbeatResponse_t ArcaL2HeartbeatResponse_t;


/*************************
 * External Message Union
 *************************/
typedef union ArcaL2MsgUnion
{
	ArcaL2Header_t        		Header;
//	ArcaL2Header_t        		Heartbeat;
	ArcaL2HeartbeatResponse_t	HeartbeatResponse;
	ArcaL2Unavailable_t			Unavailable;

	ArcaL2Add_t					Add;
	ArcaL2Modify_t				Modify;
	ArcaL2Delete_t				Delete;
	ArcaL2Imbalance_t			Imbalance;

	ArcaL2SequenceReset_t		SequenceReset;		// Packet sequence reset
	ArcaL2SymbolClear_t			SymbolClear;

	ArcaL2SymbolUpdateRequest_t	SymbolUpdateRequest;
	ArcaL2FirmUpdateRequest_t	FirmUpdateRequest;
	ArcaL2RefreshRequest_t		RefreshRequest;		// Symbol Refresh Request
	ArcaL2RetransRequest_t		RetransRequest;		// Packet Retrans Request
	ArcaL2ImbalanceRequest_t	ImbalanceRequest;	// Symbol Imbalance Request

	ArcaL2RetransResponse_t		RetransResponse;	// Refresh and Retrans Response

	ArcaL2SymbolUpdate_t		SymbolUpdate;		// Symbol Update
	ArcaL2FirmUpdate_t			FirmUpdate;			// Firm Update
	ArcaL2BookOrder_t			BookRefreshOrder;	// Book Refresh
}ArcaL2MsgUnion;

#endif

