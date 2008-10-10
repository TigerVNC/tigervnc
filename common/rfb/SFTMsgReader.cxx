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
                               unsigned int *pFlags)
{
  *pFlags = m_pIS->readU8();
  unsigned int dirNameSize = m_pIS->readU16();

  if (dirNameSize >= FT_FILENAME_SIZE) {
    m_pIS->skip(dirNameSize);
    return false;
  } else {
    m_pIS->readBytes(pDirName, dirNameSize);
    *pDirNameSize = dirNameSize;
    pDirName[dirNameSize] = '\0';
    return true;
  }
}
    
bool 
SFTMsgReader::readFileDownloadRqst(unsigned int *pFilenameSize, char *pFilename, 
                                   unsigned int *pPosition)
{
  unsigned char compressedLevel = 0;
  return readU8U16U32StringMsg(&compressedLevel, pFilenameSize, pPosition, pFilename);
}

bool 
SFTMsgReader::readFileUploadRqst(unsigned int *pFilenameSize, char *pFilename, 
                                 unsigned int *pPosition)
{
  unsigned char compressedLevel = 0;
  return readU8U16U32StringMsg(&compressedLevel, pFilenameSize, pPosition, pFilename);
}
    
char *
SFTMsgReader::readFileUploadData(unsigned int *pDataSize, unsigned int *pModTime)
{
  /*
   * Compressed level is not used now so we have to skip one byte
   *
   * unsigned char compressedLevel = m_pIS->readU8();
   */
  (void) m_pIS->readU8();
  unsigned int realSize = m_pIS->readU16();
  unsigned int compressedSize = m_pIS->readU16();

  if ((realSize == 0) && (compressedSize == 0)) {
    *pDataSize = 0;
    *pModTime = m_pIS->readU32();
    return NULL;
  } else {
    char *pData = new char [compressedSize];
    m_pIS->readBytes(pData, compressedSize);
    *pDataSize = compressedSize;
    *pModTime = 0;
    return pData;
  }
}
    
bool 
SFTMsgReader::readFileCreateDirRqst(unsigned int *pDirNameSize, char *pDirName)
{
  return readU8U16StringMsg(pDirNameSize, pDirName);
}

bool 
SFTMsgReader::readFileDirSizeRqst(unsigned int *pDirNameSize, char *pDirName)
{
  return readU8U16StringMsg(pDirNameSize, pDirName);
}

bool 
SFTMsgReader::readFileDeleteRqst(unsigned int *pNameSize, char *pName)
{
  return readU8U16StringMsg(pNameSize, pName);
}
    
bool 
SFTMsgReader::readFileRenameRqst(unsigned int *pOldNameSize, 
                                 unsigned int *pNewNameSize,
                                 char *pOldName, char *pNewName)
{
  m_pIS->skip(1);

  unsigned int oldNameSize = m_pIS->readU16();
  unsigned int newNameSize = m_pIS->readU16();

  if ((oldNameSize >= *pOldNameSize) || (newNameSize >= *pNewNameSize)) {
    m_pIS->skip(oldNameSize);
    m_pIS->skip(newNameSize);
    return false;
  }

  if (oldNameSize != 0) {
    m_pIS->readBytes(pOldName, oldNameSize);
    pOldName[oldNameSize] = '\0';
    *pOldNameSize = oldNameSize;
  } else {
    *pOldNameSize = 0;
    pOldName[0] = '\0';
  }

  if (newNameSize != 0) {
    m_pIS->readBytes(pNewName, newNameSize);
    pNewName[newNameSize] = '\0';
  } else {
    *pNewNameSize = 0;
    pNewName[0] = '\0';
  }

  return true;
}

bool
SFTMsgReader::readFileDownloadCancel(unsigned int *pReasonSize, char *pReason)
{
  return readU8U16StringMsg(pReasonSize, pReason);
}

bool
SFTMsgReader::readFileUploadFailed(unsigned int *pReasonSize, char *pReason)
{
  return readU8U16StringMsg(pReasonSize, pReason);
}

bool 
SFTMsgReader::readU8U16StringMsg(unsigned int *pReasonSize, char *pReason)
{
  m_pIS->skip(1);
  unsigned int reasonSize = m_pIS->readU16();

  if (reasonSize >= FT_FILENAME_SIZE) {
    m_pIS->skip(reasonSize);
    return false;
  } else {
    if (reasonSize == 0) {
      pReason[0] = '\0';
    } else {
      m_pIS->readBytes(pReason, reasonSize);
      pReason[reasonSize] = '\0';
    }
      *pReasonSize = reasonSize;
    return true;
  }
}

bool 
SFTMsgReader::readU8U16U32StringMsg(unsigned char *pU8, unsigned int *pU16, 
                                    unsigned int *pU32, char *pString)
{
  *pU8 = m_pIS->readU8();
  unsigned int strSize = m_pIS->readU16();
  *pU32 = m_pIS->readU32();

  if (strSize >= FT_FILENAME_SIZE) {
    m_pIS->skip(strSize);
    return false;
  } else {
    *pU16 = strSize;
    if (strSize == 0) {
      pString[0] = '\0';
    } else {
      m_pIS->readBytes(pString, strSize);
      pString[strSize] = '\0';
    }
    return true;
  }
}
