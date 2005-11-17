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
 *
 * TightVNC distribution homepage on the Web: http://www.tightvnc.com/
 *
 */

// -=- SFTMsgReader.cxx

#include <winvnc/SFTMsgReader.h>

using namespace winvnc;

SFTMsgReader::SFTMsgReader()
{
}

SFTMsgReader::~SFTMsgReader()
{
}

int 
SFTMsgReader::readFileListRqst(unsigned char *pFlags, char *pDirName, bool bDirOnly)
{
  return 0;
}

int 
SFTMsgReader::readFileDownloadCancel(char *pReason)
{
  return 0;
}

int 
SFTMsgReader::readFileDownloadRqst(char *pFilename, unsigned int *pPosition)
{
  return 0;
}

int 
SFTMsgReader::readFileUploadData(char *pData)
{
  return 0;
}

int 
SFTMsgReader::readFileUploadData(unsigned int *pModTime)
{
  return 0;
}

int 
SFTMsgReader::readFileUploadFailed(char *pReason)
{
  return 0;
}

int 
SFTMsgReader::readFileUploadRqst(char *pFilename, unsigned int *pPosition)
{
  return 0;
}

int 
SFTMsgReader::readFileCreateDirRqst(char *pDirName)
{
  return 0;
}

int 
SFTMsgReader::readFileDirSizeRqst(char *pDirName)
{
  return 0;
}

int 
SFTMsgReader::readFileRenameRqst(unsigned short *pOldNameLen, 
                                 unsigned short *pNewNameLen,
                                 char *pOldName, char *pNewName)
{
  return 0;
}

int 
SFTMsgReader::readFileDeleteRqst(char *pName)
{
  return 0;
}
