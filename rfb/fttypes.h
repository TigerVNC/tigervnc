/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
 *    
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

// -=- fttypes.h

#ifndef __RFB_FTTYPES_H__
#define __RFB_FTTYPES_H__

#include "stdio.h"
#include "stdlib.h"
#include "string.h"

#define FT_FILENAME_SIZE 256

#define FT_MAX_STATUS_STRINGS		 255
#define FT_MAX_LENGTH_STATUS_STRINGS 130

#define FT_ATTR_UNKNOWN			0x00000000
#define FT_ATTR_FILE			0x00000001
#define FT_ATTR_FOLDER			0x00000002

typedef struct tagSIZEDATAINFO
{
	unsigned int size;
	unsigned int data;
} SIZEDATAINFO;

typedef struct tagSIZEDATAFLAGSINFO
{
	unsigned int size;
	unsigned int data;
	unsigned int flags;
} SIZEDATAFLAGSINFO;

typedef struct tagFILEINFO
{
	char name[FT_FILENAME_SIZE];
	SIZEDATAFLAGSINFO info;
} FILEINFO;

typedef struct tagFILEINFOEX
{
	char locPath[FT_FILENAME_SIZE];
	char locName[FT_FILENAME_SIZE];
	char remPath[FT_FILENAME_SIZE];
	char remName[FT_FILENAME_SIZE];
	SIZEDATAFLAGSINFO info;
} FILEINFOEX;

#endif // __RFB_FTTYPES_H__