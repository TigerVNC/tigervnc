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

// -=- CFTMsgReader.cxx

#include <rfb/CFTMsgReader.h>

using namespace rfb;

CFTMsgReader::CFTMsgReader(rdr::InStream *pIS)
{
  m_pInStream = pIS;
}

CFTMsgReader::~CFTMsgReader()
{

}

int 
CFTMsgReader::readFileListData(FileInfo *pFileInfo)
{
  unsigned char flags = m_pInStream->readU8();
  int numFiles = m_pInStream->readU16();
  int dataSize = m_pInStream->readU16();
  int compressedSize = m_pInStream->readU16();
  
  if (flags & 0x80) {
    return -1;
  } else {
    if (numFiles > 0) {
      char *pFilenames = new char[compressedSize];
      SIZEDATAINFO *pSDI = new SIZEDATAINFO[numFiles];
      for (int i = 0; i < numFiles; i++) {
        pSDI[i].size = m_pInStream->readU32();
        pSDI[i].data = m_pInStream->readU32();
      }
      m_pInStream->readBytes((void *)pFilenames, compressedSize);
      createFileInfo(numFiles, pFileInfo, pSDI, pFilenames);
      delete [] pSDI;
      delete [] pFilenames;
    }
  }
  return numFiles;
}

void * 
CFTMsgReader::readFileDownloadData(unsigned int *pSize, unsigned int *pModTime)
{
  unsigned char compressLevel = m_pInStream->readU8();
  int realSize = m_pInStream->readU16();
  int compressedSize = m_pInStream->readU16();

  if ((realSize == 0) && (compressedSize == 0)) {
    *pSize = 0;
    *pModTime = m_pInStream->readU32();
    return NULL;
  } else {
    char *pFile = new char [compressedSize];
    if (pFile == NULL) {
      m_pInStream->skip(compressedSize);
      *pModTime = 0;
      return NULL;
    } else {
      m_pInStream->readBytes(pFile, compressedSize);
      *pSize = compressedSize;
      return pFile;
    }
  }
}

char * 
CFTMsgReader::readFileUploadCancel(unsigned int *pReasonSize)
{
  m_pInStream->skip(1);
  return readReasonMsg(pReasonSize);
}

char * 
CFTMsgReader::readFileDownloadFailed(unsigned int *pReasonSize)
{
  m_pInStream->skip(1);
  return readReasonMsg(pReasonSize);
}

int 
CFTMsgReader::readFileDirSizeData(unsigned short *pDirSizeLow16, 
                                 unsigned int *pDirSizeHigh32)
{
  m_pInStream->skip(1);
  *pDirSizeLow16 = m_pInStream->readU16();
  *pDirSizeHigh32 = m_pInStream->readU32();
  return 1;
}

char * 
CFTMsgReader::readFileLastRqstFailed(int *pTypeOfRequest, unsigned int *pReasonSize)
{
  *pTypeOfRequest = m_pInStream->readU8();
  return readReasonMsg(pReasonSize);
}

bool 
CFTMsgReader::createFileInfo(unsigned int numFiles, FileInfo *fi, 
                             SIZEDATAINFO *pSDInfo, char *pFilenames)
{
  int pos = 0;
  int size = 0;
  for (unsigned int i = 0; i < numFiles; i++) {
    size = pSDInfo[i].size;
    if (size == FT_NET_ATTR_DIR) {
      fi->add((pFilenames + pos), size, pSDInfo[i].data, FT_ATTR_DIR);
    } else {
      fi->add((pFilenames + pos), size, pSDInfo[i].data, FT_ATTR_FILE);
    }
    pos += strlen(pFilenames + pos) + 1;
  }
  return true;
}

char * 
CFTMsgReader::readReasonMsg(unsigned int *pReasonSize)
{
  int reasonLen = m_pInStream->readU16();
  int _reasonLen = reasonLen + 1;
  char *pReason;
  if (reasonLen == 0) {
    *pReasonSize = 0;
    return NULL;
  } else {
    pReason = new char [_reasonLen];
    if (pReason == NULL) {
      m_pInStream->skip(reasonLen);
      *pReasonSize = 0;
      return NULL;
    }
    m_pInStream->readBytes(pReason, reasonLen);
    memset(((char *)pReason+reasonLen), '\0', 1);
    return pReason;
  }
}

