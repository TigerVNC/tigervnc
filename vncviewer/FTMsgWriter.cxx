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

// -=- FTMsgWriter.cxx

#include <vncviewer/FTMsgWriter.h>

using namespace rfb;
using namespace rfb::win32;

FTMsgWriter::FTMsgWriter(rdr::OutStream *pOS)
{
  m_pOutStream = pOS;
}

FTMsgWriter::~FTMsgWriter()
{
}

bool 
FTMsgWriter::writeFileListRqst(char *pDirName, bool bDirOnly)
{
  char dirName[FT_FILENAME_SIZE];
  strcpy(dirName, pDirName);
  int len = convertToUnixPath(dirName);
  if (len <= 0) return false;

  unsigned char flags = 0;
  if (bDirOnly) flags = 0x10;

  m_pOutStream->writeU8(msgTypeFileListRequest);
  m_pOutStream->writeU8(flags);
  m_pOutStream->writeU16(len);
  m_pOutStream->writeBytes((void *)dirName, len);
  m_pOutStream->flush();

  return true;
}


bool 
FTMsgWriter::writeFileDownloadCancel(unsigned short reasonLen, char *pReason)
{
  m_pOutStream->writeU8(msgTypeFileDownloadCancel);
  return writeU8U16StringMsg(reasonLen, pReason);
}

bool 
FTMsgWriter::writeFileDownloadRqst(unsigned short filenameLen, char *pFilename, 
                                   unsigned int position)
{
  m_pOutStream->writeU8(msgTypeFileDownloadRequest);

  m_pOutStream->flush();

  return false;
}

bool 
FTMsgWriter::writeFileUploadData(unsigned short dataSize, char *pData)
{
  m_pOutStream->writeU8(msgTypeFileUploadData);
  
  m_pOutStream->flush();

  return false;
}

bool 
FTMsgWriter::writeFileUploadData(unsigned int modTime)
{
  m_pOutStream->writeU8(msgTypeFileUploadData);

  m_pOutStream->flush();

  return false;
}

bool 
FTMsgWriter::writeFileUploadFailed(unsigned short reasonLen, char *pReason)
{
  m_pOutStream->writeU8(msgTypeFileUploadFailed);
  return writeU8U16StringMsg(reasonLen, pReason);
}

bool 
FTMsgWriter::writeFileUploadRqst(unsigned short filenameLen, char *pFilename, 
                                 unsigned int position)
{
  m_pOutStream->writeU8(msgTypeFileUploadRequest);

  m_pOutStream->flush();

  return false;
}

bool 
FTMsgWriter::writeFileCreateDirRqst(unsigned short dirNameLen, char *pDirName)
{
  if (dirNameLen >= FT_FILENAME_SIZE) return false;

  char path[FT_FILENAME_SIZE];
  strcpy(path, pDirName);
  int nameLen = convertToUnixPath(path);

  m_pOutStream->writeU8(msgTypeFileCreateDirRequest);
  return writeU8U16StringMsg(nameLen, path);
}

bool 
FTMsgWriter::writeFileDirSizeRqst(unsigned short dirNameLen, char *pDirName)
{
  if (dirNameLen >= FT_FILENAME_SIZE) return false;

  char path[FT_FILENAME_SIZE];
  strcpy(path, pDirName);
  int nameLen = convertToUnixPath(path);

  m_pOutStream->writeU8(msgTypeFileDirSizeRequest);
  return writeU8U16StringMsg(nameLen, path);
}

bool 
FTMsgWriter::writeFileRenameRqst(unsigned short oldNameLen, unsigned short newNameLen,
                                 char *pOldName, char *pNewName)
{
  if ((oldNameLen >= FT_FILENAME_SIZE) || (newNameLen >= FT_FILENAME_SIZE)) return false;

  char oldName[FT_FILENAME_SIZE];
  char newName[FT_FILENAME_SIZE];

  strcpy(oldName, pOldName);
  strcpy(newName, pNewName);

  int _oldNameLen = convertToUnixPath(oldName);
  int _newNameLen = convertToUnixPath(newName);

  m_pOutStream->writeU8(msgTypeFileRenameRequest);
  m_pOutStream->writeU8(0);
  m_pOutStream->writeU16(_oldNameLen);
  m_pOutStream->writeU16(_newNameLen);
  m_pOutStream->writeBytes(oldName, _oldNameLen);
  m_pOutStream->writeBytes(newName, _newNameLen);
  m_pOutStream->flush();
  
  return true;
}

bool 
FTMsgWriter::writeFileDeleteRqst(unsigned short nameLen, char *pName)
{
  if (nameLen >= FT_FILENAME_SIZE) return false;

  char path[FT_FILENAME_SIZE];
  strcpy(path, pName);
  int _nameLen = convertToUnixPath(path);

  m_pOutStream->writeU8(msgTypeFileDeleteRequest);
  return writeU8U16StringMsg(_nameLen, path);
}

bool 
FTMsgWriter::writeU8U16StringMsg(unsigned short strLength, char *pString)
{
  m_pOutStream->writeU8(0);
  m_pOutStream->writeU16(strLength);
  m_pOutStream->writeBytes(pString, strLength);
  m_pOutStream->flush();
  return true;
}

int
FTMsgWriter::convertToUnixPath(char *path)
{
  int len = strlen(path);
  if (len >= FT_FILENAME_SIZE) return -1;
  if (len == 0) {strcpy(path, "/"); return 1;}
  for (int i = (len - 1); i >= 0; i--) {
    if (path[i] == '\\') path[i] = '/';
    path[i+1] = path[i];
  }
  path[len + 1] = '\0';
  path[0] = '/';
  return strlen(path);
}
