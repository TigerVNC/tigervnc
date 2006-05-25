/* Copyright (C) 2006 TightVNC Team.  All Rights Reserved.
 *    
 * Developed by Dennis Syrovatsky
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

// -=- SFileTransfer.cxx

#include <rfb/msgTypes.h>
#include <rfb/SFileTransfer.h>

using namespace rfb;

SFileTransfer::SFileTransfer(network::Socket *sock) : 
  m_bUploadStarted(false), m_bDownloadStarted(false),
  m_reader(&sock->inStream()), m_writer(&sock->outStream()), m_pSocket(sock)
{
}

SFileTransfer::~SFileTransfer()
{
}

bool
SFileTransfer::processMessages(int type)
{
  switch(type)
  {
    case msgTypeFileListRequest:
      return processFileListRequest();
    case msgTypeFileDownloadRequest:
      return processFileDownloadRequest();
    case msgTypeFileUploadRequest:
      return processFileUploadRequest();
    case msgTypeFileUploadData:
      return processFileUploadData();
    case msgTypeFileDownloadCancel:
      return processFileDownloadCancel();
    case msgTypeFileUploadFailed:
      return processFileUploadFailed();
    case msgTypeFileCreateDirRequest:
      return processFileCreateDirRequest();
    case msgTypeFileDirSizeRequest:
      return processFileDirSizeRequest();
    case msgTypeFileRenameRequest:
      return processFileRenameRequest();
    case msgTypeFileDeleteRequest:
      return processFileDeleteRequest();
    default:
      return false;
  }
}

bool 
SFileTransfer::processFileListRequest()
{
  char szDirName[FT_FILENAME_SIZE] = {0};
  unsigned int dirNameSize = FT_FILENAME_SIZE;
  unsigned int flags = 0;

  if (!m_reader.readFileListRqst(&dirNameSize, szDirName, &flags)) return false;

  if (!convertPathFromNet(szDirName)) return false;

  bool bDirOnly = false;
  if (flags & 0x10) bDirOnly = true;

  FileInfo fi;
  if (!makeFileList(szDirName, &fi, bDirOnly)) {
    flags = (flags | 0x80);  
  }
  return m_writer.writeFileListData((unsigned char)flags, &fi);
}

bool 
SFileTransfer::processFileDownloadRequest()
{
  char szName[FT_FILENAME_SIZE] = {0};
  unsigned int nameSize = FT_FILENAME_SIZE;
  unsigned int position = 0;

  if (!m_reader.readFileDownloadRqst(&nameSize, szName, &position)) return false;

  if (!convertPathFromNet(szName)) return false;

  if (m_bDownloadStarted) {
    char reason[] = "The download is already started";
    m_writer.writeFileLastRqstFailed(msgTypeFileDownloadRequest, strlen(reason), reason);
    return false;
  }

  if (!m_fileReader.create(szName)) return false;

  m_bDownloadStarted = true;

  sendFileDownloadPortion();

  return true;
}

bool
SFileTransfer::sendFileDownloadPortion()
{
  char buffer[FT_MAX_SENDING_SIZE];
  unsigned int bytesRead = 0;

  if (m_fileReader.read((void *)buffer, FT_MAX_SENDING_SIZE, &bytesRead)) {
    if (bytesRead == 0) {
      m_writer.writeFileDownloadData(m_fileReader.getTime());
      m_fileReader.close();
      m_bDownloadStarted = false;
      return true;
    } else {
      m_writer.writeFileDownloadData(bytesRead, buffer);
      return initDownloadCallback();
    }
  } else {
    char reason[] = "Error while reading from file";
    m_writer.writeFileDownloadFailed(strlen(reason), reason);
    m_fileReader.close();
    m_bDownloadStarted = false;
    return true;
  }
}

bool 
SFileTransfer::processFileUploadRequest()
{
  char szName[FT_FILENAME_SIZE] = {0};
  unsigned int nameSize = FT_FILENAME_SIZE;
  unsigned int position = 0;

  if (!m_reader.readFileUploadRqst(&nameSize, szName, &position)) return false;

  if (!convertPathFromNet(szName)) return false;

  if (m_bUploadStarted) {
    char reason[] = "The upload is already started";
    m_writer.writeFileLastRqstFailed(msgTypeFileUploadRequest, strlen(reason), reason);
    return false;
  }

  if (!m_fileWriter.create(szName)) {
    char reason[] = "Can't create local file";
    m_writer.writeFileLastRqstFailed(msgTypeFileUploadRequest, strlen(reason), reason);
    return true;
  }

  m_bUploadStarted = true;

  return true;
}

bool 
SFileTransfer::processFileUploadData()
{
  unsigned int dataSize = 0;
  unsigned int modTime = 0;

  char *pUploadData = m_reader.readFileUploadData(&dataSize, &modTime);

  if (!m_bUploadStarted) {
      char reason[] = "Upload is impossible";
      m_writer.writeFileUploadCancel(strlen(reason), reason);
  } else {
    if (pUploadData == NULL) {
      if (modTime == 0) {
        char reason[] = "Upload failed";
        m_writer.writeFileUploadCancel(strlen(reason), reason);
      } else {
        m_fileWriter.setTime(modTime);
      }
      m_fileWriter.close();
      m_bUploadStarted = false;
    } else {
      unsigned int dataWritten = 0;
      m_fileWriter.write(pUploadData, dataSize, &dataWritten);
      if (dataWritten != dataSize) {
        char reason[] = "Upload failed";
        m_writer.writeFileUploadCancel(strlen(reason), reason);
        m_fileWriter.close();
        m_bUploadStarted = false;
      }
    }
  }
  delete [] pUploadData;
  return true;
}

bool 
SFileTransfer::processFileDownloadCancel()
{
  char szReason[FT_FILENAME_SIZE] = {0};
  unsigned int reasonSize = FT_FILENAME_SIZE;

  if (!m_reader.readFileDownloadCancel(&reasonSize, szReason)) return false;

  m_fileReader.close();
  m_bDownloadStarted = false;
  return true;
}

bool 
SFileTransfer::processFileUploadFailed()
{
  char szReason[FT_FILENAME_SIZE] = {0};
  unsigned int reasonSize = FT_FILENAME_SIZE;

  if (!m_reader.readFileUploadFailed(&reasonSize, szReason)) return false;

  deleteIt(m_fileWriter.getFilename());
  m_fileWriter.close();
  m_bUploadStarted = false;
  return true;
}

bool 
SFileTransfer::processFileCreateDirRequest()
{
  char szName[FT_FILENAME_SIZE] = {0};
  unsigned int nameSize = FT_FILENAME_SIZE;

  if (!m_reader.readFileCreateDirRqst(&nameSize, szName)) return false;

  if (!convertPathFromNet(szName)) return false;

  return createDir(szName);
}

bool 
SFileTransfer::processFileDirSizeRequest()
{
  char szName[FT_FILENAME_SIZE] = {0};
  unsigned int nameSize = FT_FILENAME_SIZE;

  if (!m_reader.readFileDirSizeRqst(&nameSize, szName)) return false;

  if (!convertPathFromNet(szName)) return false;

  unsigned short highSize16 = 0;
  unsigned int lowSize32 = 0;

  if (!getDirSize(szName, &highSize16, &lowSize32)) return false;

  return m_writer.writeFileDirSizeData(lowSize32, highSize16);
}

bool 
SFileTransfer::processFileRenameRequest()
{
  char szOldName[FT_FILENAME_SIZE] = {0};
  char szNewName[FT_FILENAME_SIZE] = {0};

  unsigned int oldNameSize = FT_FILENAME_SIZE;
  unsigned int newNameSize = FT_FILENAME_SIZE;

  if (!m_reader.readFileRenameRqst(&oldNameSize, &newNameSize, szOldName, szNewName)) return false;

  if ((!convertPathFromNet(szOldName)) || (!convertPathFromNet(szNewName))) return false;

  return renameIt(szOldName, szNewName);
}

bool 
SFileTransfer::processFileDeleteRequest()
{
  char szName[FT_FILENAME_SIZE] = {0};
  unsigned int nameSize = FT_FILENAME_SIZE;

  if (!m_reader.readFileDeleteRqst(&nameSize, szName)) return false;

  if (!convertPathFromNet(szName)) return false;

  return deleteIt(szName);
}

bool 
SFileTransfer::convertPathFromNet(char *pszPath)
{
  return true;
}

bool 
SFileTransfer::makeFileList(char *pszPath, FileInfo *pFI, bool bDirOnly)
{
  return false;
}

bool 
SFileTransfer::deleteIt(char *pszPath)
{
  return false;
}

bool 
SFileTransfer::renameIt(char *pszOldPath, char *pszNewPath)
{
  return false;
}

bool 
SFileTransfer::createDir(char *pszPath)
{
  return false;
}

bool 
SFileTransfer::getDirSize(char *pszName, unsigned short *pHighSize16, 
                          unsigned int *pLowSize32)
{
  return false;
}

bool
SFileTransfer::initDownloadCallback()
{
  return false;
}
