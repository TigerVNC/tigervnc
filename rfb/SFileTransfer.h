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

// -=- SFileTransfer.h

#ifndef __RFB_SFILETRANSFER_H__
#define __RFB_SFILETRANSFER_H__

#include <network/Socket.h>
#include <rfb/SFTMsgReader.h>
#include <rfb/SFTMsgWriter.h>
#include <rfb/FileWriter.h>
#include <rfb/FileReader.h>
#include <rfb/FileInfo.h>
#include <rfb/fttypes.h>

namespace rfb {
  class SFileTransfer
  {
  public:
    SFileTransfer(network::Socket *sock);
    virtual ~SFileTransfer();

    bool processMessages(int type);
    bool sendFileDownloadPortion();

  protected:
    bool processFileListRequest();
    bool processFileDownloadRequest();
    bool processFileUploadRequest();
    bool processFileUploadData();
    bool processFileDownloadCancel();
    bool processFileUploadFailed();
    bool processFileCreateDirRequest();
    bool processFileDirSizeRequest();
    bool processFileRenameRequest();
    bool processFileDeleteRequest();

    virtual bool initDownloadCallback();
    virtual bool makeFileList(char *pszPath, FileInfo *pFI, bool bDirOnly);
    virtual bool convertPathFromNet(char *pszPath);

    virtual bool deleteIt(char *pszPath);
    virtual bool renameIt(char *pszOldPath, char *pszNewPath);
    virtual bool createDir(char *pszPath);

    virtual bool getDirSize(char *pszName, unsigned short *pHighSize16, unsigned int *pLowSize32);

    bool m_bUploadStarted;
    bool m_bDownloadStarted;
    
  private:
    SFTMsgReader m_reader;
    SFTMsgWriter m_writer;

    FileWriter m_fileWriter;
    FileReader m_fileReader;

    network::Socket *m_pSocket;
  };
}

#endif // __RFB_SFILETRANSFER_H__
