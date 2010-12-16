// $Id: types.h,v 1.2 2006/02/09 19:14:25 Daniel.May Exp $
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

#ifndef _fast_types_h_
#define _fast_types_h_ 1

#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>

#include <unistd.h>
#include <fcntl.h>

#include <netinet/in.h>

#if ! defined (CYGWIN) && ! defined (WIN32)
#include <dlfcn.h>
#endif

typedef int i32;

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

typedef long long i64;
typedef unsigned long long u64;

typedef double f64;

#define FUNCTION __FUNCTION__

#if defined (WIN32)
#define inline __inline
#endif

#define swap32(x) (x) = ntohl (x)
#define swap16(x) (x) = ntohs (x)

#endif // _fast_types_h_
