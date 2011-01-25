/*
 * Copyright (C) 2008, 2009, 2010 The Collaborative Software Foundation.
 *
 * This file is part of FeedHandlers (FH).
 *
 * FH is free software: you can redistribute it and/or modify it under the terms of the
 * GNU Lesser General Public License as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * FH is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without
 * even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with FH.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __NOTIFY_CONSTANTS_H
#define __NOTIFY_CONSTANTS_H
/*********************************************************************/
/* file: fh_notify_constants.h                                       */
/* Usage: symbolic constants for book feed line handlers             */
/*               used to build summary status for alerts and notifys */
/* Author: Wally Matthews of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/* Inherited from Tervela Arca Multicast feed handler                */
/*   with mods to remove Tervela specifics                           */
/*********************************************************************/
#define LINE_AC 0
#define LINE_DJ 0x40000000
#define LINE_KQ 0x80000000
#define LINE_RZ 0xc0000000
#define LINE_AM 0
#define LINE_NZ 0x40000000
#define AS_RECEIVED_ORDERING 0
#define PACKET_LOSS 2
#define LOST_FEED 8
#define RECOVERED_FEED 16
#define IN_SEQUENCE 32
#define OUT_OF_SEQUENCE 64
#define PURGE_ACCELERATION 3
#define LINE_ID_MASK 0xC0000000
#define AS_RECEIVED_ORDER_SUMMARY 0
#define INSEQUENCE_SUMMARY 0x20000000
#define OUTSEQUENCE_SUMMARY 0
#define NO_PACKET_LOSS_SUMMARY 0x10000000
#define PACKET_LOSS_SUMMARY 0
#define PRIMARY_FEED_UP 0x8000000
#define SECONDARY_FEED_UP 0x4000000
#define REREQUEST_FEED_UP 0x2000000
#define EXTREME_PACKET_LOSS 0x1000000
#define PACKET_LOSS_MASK 0xffffffLL
#endif //__NOTIFY_CONSTANTS_H
