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

// -=- CFTMsgWriter.cxx

#include <rfb/CFTMsgWriter.h>

using namespace rfb;

CFTMsgWriter::CFTMsgWriter(rdr::OutStream *pOS)
{
  m_pOutStream = pOS;
}

CFTMsgWriter::~CFTMsgWriter()
{
}

bool 
CFTMsgWriter::writeFileListRqst(unsigned short dirnameLen, char *pDirName, 
                                bool bDirOnly)
{
  if (dirnameLen >= FT_FILENAME_SIZE) return false;

  unsigned char flags = 0;
  if (bDirOnly) flags = 0x10;

  m_pOutStream->writeU8(msgTypeFileListRequest);
  m_pOutStream->writeU8(flags);
  m_pOutStream->writeU16(dirnameLen);
  m_pOutStream->writeBytes((void *)pDirName, dirnameLen);
  m_pOutStream->flush();

  return true;
}


bool 
CFTMsgWriter::writeFileDownloadCancel(unsigned short reasonLen, char *pReason)
{
  m_pOutStream->writeU8(msgTypeFileDownloadCancel);
  return writeU8U16StringMsg(reasonLen, pReason);
}

bool 
CFTMsgWriter::writeFileDownloadRqst(unsigned short filenameLen, char *pFilename, 
                                    unsigned int position)
{
  if (filenameLen >= FT_FILENAME_SIZE) return false;

  m_pOutStream->writeU8(msgTypeFileDownloadRequest);
  m_pOutStream->writeU8(0);
  m_pOutStream->writeU16(filenameLen);
  m_pOutStream->writeU32(position);
  m_pOutStream->writeBytes(pFilename, filenameLen);
  m_pOutStream->flush();

  return true;
}

bool 
CFTMsgWriter::writeFileUploadData(unsigned short dataSize, char *pData)
{
  m_pOutStream->writeU8(msgTypeFileUploadData);
  m_pOutStream->writeU8(0);
  m_pOutStream->writeU16(dataSize);
  m_pOutStream->writeU16(dataSize);
  m_pOutStream->writeBytes(pData, dataSize);
  m_pOutStream->flush();

  return true;
}

bool 
CFTMsgWriter::writeFileUploadData(unsigned int modTime)
{
  m_pOutStream->writeU8(msgTypeFileUploadData);
  m_pOutStream->writeU8(0);
  m_pOutStream->writeU16(0);
  m_pOutStream->writeU16(0);
  m_pOutStream->writeU32(modTime);
  m_pOutStream->flush();

  return true;
}

bool 
CFTMsgWriter::writeFileUploadFailed(unsigned short reasonLen, char *pReason)
{
  m_pOutStream->writeU8(msgTypeFileUploadFailed);
  return writeU8U16StringMsg(reasonLen, pReason);
}

bool 
CFTMsgWriter::writeFileUploadRqst(unsigned short filenameLen, char *pFilename, 
                                  unsigned int position)
{
  if (filenameLen >= FT_FILENAME_SIZE) return false;

  m_pOutStream->writeU8(msgTypeFileUploadRequest);
  m_pOutStream->writeU8(0);
  m_pOutStream->writeU16(filenameLen);
  m_pOutStream->writeU32(position);
  m_pOutStream->writeBytes((void *)pFilename, filenameLen);
  m_pOutStream->flush();

  return true;
}

bool 
CFTMsgWriter::writeFileCreateDirRqst(unsigned short dirNameLen, char *pDirName)
{
  if (dirNameLen >= FT_FILENAME_SIZE) return false;

  m_pOutStream->writeU8(msgTypeFileCreateDirRequest);
  return writeU8U16StringMsg(dirNameLen, pDirName);
}

bool 
CFTMsgWriter::writeFileDirSizeRqst(unsigned short dirNameLen, char *pDirName)
{
  if (dirNameLen >= FT_FILENAME_SIZE) return false;

  m_pOutStream->writeU8(msgTypeFileDirSizeRequest);
  return writeU8U16StringMsg(dirNameLen, pDirName);
}

bool 
CFTMsgWriter::writeFileRenameRqst(unsigned short oldNameLen, unsigned short newNameLen,
                                  char *pOldName, char *pNewName)
{
  if ((oldNameLen >= FT_FILENAME_SIZE) || (newNameLen >= FT_FILENAME_SIZE)) return false;

  m_pOutStream->writeU8(msgTypeFileRenameRequest);
  m_pOutStream->writeU8(0);
  m_pOutStream->writeU16(oldNameLen);
  m_pOutStream->writeU16(newNameLen);
  m_pOutStream->writeBytes(pOldName, oldNameLen);
  m_pOutStream->writeBytes(pNewName, newNameLen);
  m_pOutStream->flush();
  
  return true;
}

bool 
CFTMsgWriter::writeFileDeleteRqst(unsigned short nameLen, char *pName)
{
  if (nameLen >= FT_FILENAME_SIZE) return false;

  m_pOutStream->writeU8(msgTypeFileDeleteRequest);
  return writeU8U16StringMsg(nameLen, pName);
}

bool 
CFTMsgWriter::writeU8U16StringMsg(unsigned short strLength, char *pString)
{
  m_pOutStream->writeU8(0);
  m_pOutStream->writeU16(strLength);
  m_pOutStream->writeBytes(pString, strLength);
  m_pOutStream->flush();
  return true;
}
