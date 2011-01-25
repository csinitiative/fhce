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

#ifndef __DATA_CONVERSIONS_H
#define __DATA_CONVERSIONS_H
/*********************************************************************/
/* file: fh_data_conversions.h                                       */
/* Usage: conversion routines  for itch and arca feed handlers       */
/*         itch uses explicit little Endian; arca uses explicit big  */
/* Author: Wally Matthews of Collaborative Software Initiative       */
/* Conception: Nov. 9, 2008                                          */
/* Inherited from Tervela Arca Multicast feed handler                */
/*********************************************************************/
#include <stdint.h>

#define little_endian_16(buffer)                          \
    (((uint16_t)(((char*)buffer)[1])&0xff)<<8) + ((uint16_t)(((char*)buffer)[0]&0xff))
/*
inline uint16_t little_endian_16(const char* buffer)
{
    return ((((uint16_t)buffer[1])&0xff)<<8) + (((uint16_t)buffer[0]&0xff));
};
*/
#define big_endian_16(buffer)                            \
    (((uint16_t)(((char*)buffer)[0])&0xff)<<8) + ((uint16_t)(((char*)buffer)[1]&0xff))
/*
inline uint16_t big_endian_16(const char* buffer)
{
    return ((((uint16_t)buffer[0])&0xff)<<8) + (((uint16_t)buffer[1]&0xff));
};
*/
#define little_endian_16Out(buffer, val)                 \
    ((char*)buffer)[0] = (char)((uint16_t)val)&0xff;                                \
    ((char*)buffer)[1] = (char)(((uint16_t)val)>>8)&0xff;
/*
inline void little_endian_16Out(unsigned char* buffer, const uint16_t val)
{
    buffer[0] = val&0xff;
    buffer[1] = (val>>8)&0xff;
};
*/
#define little_endian_32Out(buffer,val)                  \
    ((char*)buffer)[0] = (char)(val&0xff);                                \
    ((char*)buffer)[1] = (char)((val>>8)&0xff);                           \
    ((char*)buffer)[2] = (char)((val>>16)&0xff);                          \
    ((char*)buffer)[3] = (char)((val>>24)&0xff);
/*
inline void little_endian_32Out(unsigned char* buffer, const uint32_t val)
{
    buffer[0] = val&0xff;
    buffer[1] = (val>>8)&0xff;
    buffer[2] = (val>>16)&0xff;
    buffer[3] = (val>>24)&0xff;
};
*/
#define little_endian_32(buffer)                         \
    (uint32_t) ( (((uint32_t) ((char*)buffer)[3] & 0xff) << 24)+   \
                 (((uint32_t) ((char*)buffer)[2] & 0xff) << 16)+  \
                 (((uint32_t) ((char*)buffer)[1] & 0xff) << 8)+   \
                 (((uint32_t) ((char*)buffer)[0] & 0xff)) )
/*
inline uint32_t little_endian_32(const char* buffer)
{
    uint32_t val = (buffer[3] & 0xff) << 24;
    val += ((uint32_t) buffer[2] & 0xff) << 16;
    val += ((uint32_t) buffer[1] & 0xff) << 8;
    val += ((uint32_t) buffer[0] & 0xff);
    return val;
};
*/
#define big_endian_32(buffer)                            \
    (uint32_t) ( ((uint32_t)((((char*)buffer)[0] & 0xff) << 24))+ \
               ((uint32_t)((((char*)buffer)[1] & 0xff)<<16))+     \
               ((uint32_t)((((char*)buffer)[2] & 0xff)<<8))+      \
               ((uint32_t)(((char*)buffer)[3] & 0xff)) ) 
/*
inline uint32_t big_endian_32(const char* buffer)
{
    uint32_t val = (buffer[0] & 0xff) << 24;
    val += ((uint32_t) buffer[1] & 0xff) << 16;
    val += ((uint32_t) buffer[2] & 0xff) << 8;
    val += ((uint32_t) buffer[3] & 0xff);
    return val;
};
*/
#endif //__DATA_CONVERSIONS_H
