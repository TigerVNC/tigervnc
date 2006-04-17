/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
 *
 * Developed by Dennis Syrovatsky.
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

#define FT_MAX_SENDING_SIZE 8192

#define FT_NET_ATTR_DIR ((unsigned int)-1)

#define FT_ATTR_UNKNOWN			    0x00000000
#define FT_ATTR_FILE			    0x00000001
#define FT_ATTR_DIR 			    0x00000002

#define FT_ATTR_RESIZE_NEEDED	    0x00040000
#define FT_ATTR_FOLDER_EXISTS	    0x00080000
#define FT_ATTR_COPY_OVERWRITE	    0x00100000
#define FT_ATTR_FLR_UPLOAD_CHECK	0x00200000
#define FT_ATTR_FLR_UPLOAD_ADD	    0x00400000
#define FT_ATTR_COPY_UPLOAD	        0x00800000
#define FT_ATTR_FLR_DOWNLOAD_CHECK  0x01000000
#define FT_ATTR_FLR_DOWNLOAD_ADD	0x02000000
#define FT_ATTR_COPY_DOWNLOAD	    0x04000000
#define FT_ATTR_DELETE_LOCAL	    0x08000000
#define FT_ATTR_DELETE_REMOTE	    0x10000000
#define FT_ATTR_RENAME_LOCAL	    0x20000000
#define FT_ATTR_RENAME_REMOTE	    0x40000000

#define FT_FLR_DEST_MAIN     101
#define FT_FLR_DEST_BROWSE   102
#define FT_FLR_DEST_DOWNLOAD 103
#define FT_FLR_DEST_UPLOAD   104
#define FT_FLR_DEST_DELETE   105
#define FT_FLR_DEST_RENAME   106

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
