/**************************************************************************
*  System        : OPRA
*
*  Module Name   : fast_process.h
*  Copyright (c) : SIAC 2007
*  Date          : 1/2008
*
*  Description   : Header for decode API for Ver 1 and Ver 2
***************************************************************************/

#ifndef _fast_decode_h_
#define _fast_decode_h_

#include "fast_opra.h"

int decode_OpraFastDefMsg_v2        (Fast*, char *, OpraMsg_v2 *);
int decode_OpraFastQuoteSizeMsg_v2  (Fast*, char *, OpraMsg_v2 *);
int decode_OpraFastOpenIntMsg_v2    (Fast*, char *, OpraMsg_v2 *);
int decode_OpraFastLastSaleMsg_v2   (Fast*, char *, OpraMsg_v2 *);
int decode_OpraFastEodMsg_v2        (Fast*, char *, OpraMsg_v2 *);
int decode_OpraFastFcoLastSaleMsg_v2(Fast*, char *, OpraMsg_v2 *);
int decode_OpraFastFcoQuoteMsg_v2   (Fast*, char *, OpraMsg_v2 *);
int decode_OpraFastFcoEodMsg_v2     (Fast*, char *, OpraMsg_v2 *);
int decode_OpraFastUlValueMsg_v2    (Fast*, char *, OpraMsg_v2 *);
int decode_OpraFastAdminMsg_v2      (Fast*, char *, OpraMsg_v2 *);
int decode_OpraFastControlMsg_v2    (Fast*, char *, OpraMsg_v2 *);
int decode_OpraFastDefMsg_v2        (Fast*, char *, OpraMsg_v2 *);


#endif
