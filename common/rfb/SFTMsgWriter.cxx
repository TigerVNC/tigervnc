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
  unsigned int numFiles = pFileInfo->getNumEntries();

  m_pOS->writeU8(msgTypeFileListData);
  m_pOS->writeU8(flags);
  m_pOS->writeU16(numFiles);
  
  if (numFiles == 0) {
    m_pOS->writeU16(0);
    m_pOS->writeU16(0);
  } else {
    unsigned int filenamesSize = pFileInfo->getFilenamesSize() + numFiles;

    m_pOS->writeU16(filenamesSize);
    m_pOS->writeU16(filenamesSize);

    char *pFilenames = new char [filenamesSize];
    unsigned int pos = 0;

    for (unsigned int i = 0; i < numFiles; i++) {
      char *pName = pFileInfo->getNameAt(i);
      unsigned int len = strlen(pName);

      memcpy((void *)&pFilenames[pos], pName, len + 1);
      pos += (len + 1);

      if (pFileInfo->getFlagsAt(i) & FT_ATTR_DIR) {
        m_pOS->writeU32(FT_NET_ATTR_DIR);
      } else {
        m_pOS->writeU32(pFileInfo->getSizeAt(i));
      }
      m_pOS->writeU32(pFileInfo->getDataAt(i));
    }

    m_pOS->writeBytes(pFilenames, filenamesSize);

    delete [] pFilenames;
  }

  m_pOS->flush();

  return true;
}

bool 
SFTMsgWriter::writeFileDownloadData(unsigned int dataSize, void *pData)
{
  m_pOS->writeU8(msgTypeFileDownloadData);
  m_pOS->writeU8(0);
  m_pOS->writeU16(dataSize);
  m_pOS->writeU16(dataSize);
  m_pOS->writeBytes(pData, dataSize);
  m_pOS->flush();
  return true;
}

bool 
SFTMsgWriter::writeFileDownloadData(unsigned int modTime)
{
  m_pOS->writeU8(msgTypeFileDownloadData);
  m_pOS->writeU8(0);
  m_pOS->writeU16(0);
  m_pOS->writeU16(0);
  m_pOS->writeU32(modTime);
  m_pOS->flush();
  return true;
}

bool
SFTMsgWriter::writeFileUploadCancel(unsigned int reasonLen, char *pReason)
{
  m_pOS->writeU8(msgTypeFileUploadCancel);
  return writeU8U16StringMsg(0, reasonLen, pReason);
}

bool 
SFTMsgWriter::writeFileDownloadFailed(unsigned int reasonLen, char *pReason)
{
  m_pOS->writeU8(msgTypeFileDownloadFailed);
  return writeU8U16StringMsg(0, reasonLen, pReason);
}

bool 
SFTMsgWriter::writeFileDirSizeData(unsigned int dirSizeLow, 
                                   unsigned short dirSizeHigh)
{
  m_pOS->writeU8(msgTypeFileDirSizeData);
  m_pOS->writeU8(0);
  m_pOS->writeU16(dirSizeHigh);
  m_pOS->writeU32(dirSizeLow);
  m_pOS->flush();
  return true;
}

bool 
SFTMsgWriter::writeFileLastRqstFailed(unsigned char lastRequest, 
                                      unsigned short reasonLen,
                                      char *pReason)
{
  m_pOS->writeU8(msgTypeFileLastRequestFailed);
  return writeU8U16StringMsg(lastRequest, reasonLen, pReason);
}

bool
SFTMsgWriter::writeU8U16StringMsg(unsigned char p1, unsigned short p2, char *pP3)
{
  m_pOS->writeU8(p1);
  m_pOS->writeU16(p2);
  m_pOS->writeBytes(pP3, p2);
  m_pOS->flush();
  return true;
}
