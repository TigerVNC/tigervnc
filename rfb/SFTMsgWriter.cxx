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

// -=- SFTMsgWriter.cxx

#include <rfb/SFTMsgWriter.h>

using namespace rfb;

SFTMsgWriter::SFTMsgWriter(rdr::OutStream *pOS)
{
  m_pOS = pOS;
}

SFTMsgWriter::~SFTMsgWriter()
{
}
    
bool 
SFTMsgWriter::writeFileListData(unsigned char flags, rfb::FileInfo *pFileInfo)
{
  return false;
}

bool 
SFTMsgWriter::writeFileDownloadData(unsigned int dataSize, void *pData)
{
  return false;
}

bool 
SFTMsgWriter::writeFileDownloadData(unsigned int modTime)
{
  return false;
}

bool
SFTMsgWriter::writeFileUploadCancel(unsigned int reasonLen, char *pReason)
{
  return false;
}

bool 
SFTMsgWriter::writeFileDownloadFailed(unsigned int reasonLen, char *pReason)
{
  return false;
}

bool 
SFTMsgWriter::writeFileDirSizeData(unsigned int dirSizeLow, 
                                   unsigned short dirSizeHigh)
{
  return false;
}

bool 
SFTMsgWriter::writeFileLastRqstFailed(unsigned char lastRequest, 
                                      unsigned short reasonLen,
                                      char *pReason)
{
  return false;
}

bool
SFTMsgWriter::writeU8U16StringMsg(unsigned char p1, unsigned short p2, char *pP3)
{
  return false;
}
