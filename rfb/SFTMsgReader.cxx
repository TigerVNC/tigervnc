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

#include <rfb/SFTMsgReader.h>

using namespace rfb;

SFTMsgReader::SFTMsgReader(rdr::InStream *pIS)
{
  m_pIS = pIS;
}

SFTMsgReader::~SFTMsgReader()
{
}
    
bool 
SFTMsgReader::readFileListRqst(unsigned int *pDirNameSize, char *pDirName, 
                               unsigned int *pFlags, bool *bDirOnly)
{
  return false;
}
    
bool 
SFTMsgReader::readFileDownloadRqst(unsigned int *pFilenameSize, char *pFilename, 
                                   unsigned int *pPosition)
{
  return false;
}

bool 
SFTMsgReader::readFileUploadRqst(unsigned int *pFilenameSize, char *pFilename, 
                                 unsigned int *pPosition)
{
  return false;
}
    
void *
SFTMsgReader::readFileUploadData(unsigned int *pDataSize, unsigned int *pModTime)
{
  return NULL;
}
    
bool 
SFTMsgReader::readFileCreateDirRqst(unsigned int *pDirNameSize, char *pDirName)
{
  return false;
}

bool 
SFTMsgReader::readFileDirSizeRqst(unsigned int *pDirNameSize, char *pDirName)
{
  return false;
}

bool 
SFTMsgReader::readFileDeleteRqst(unsigned int *pNameSize, char *pName)
{
  return false;
}
    
bool readFileRenameRqst(unsigned int *pOldNameSize, unsigned int *pNewNameSize,
                        char *pOldName, char *pNewName)
{
  return false;
}

char *
SFTMsgReader::readFileDownloadCancel(unsigned int *pReasonSize)
{
  return NULL;
}

char *
SFTMsgReader::readFileUploadFailed(unsigned int *pReasonSize)
{
  return NULL;
}

bool 
SFTMsgReader::readU8U16StringMsg(unsigned int *pReasonSize, char *pReason)
{
  return false;
}
