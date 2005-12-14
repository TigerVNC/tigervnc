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

// -=- FileTransfer.h

#ifndef __RFB_WIN32_FILETRANSFER_H__
#define __RFB_WIN32_FILETRANSFER_H__

#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rfb/msgTypes.h>
#include <rfb/FileInfo.h>
#include <rfb/FileReader.h>
#include <rfb/FileWriter.h>
#include <rfb/TransferQueue.h>
#include <vncviewer/FTDialog.h>
#include <vncviewer/FTMsgReader.h>
#include <vncviewer/FTMsgWriter.h>

namespace rfb {
  namespace win32 {
    class FTDialog;

    class FileTransfer
    {
    public:
      FileTransfer();
      ~FileTransfer();

      bool initialize(rdr::InStream *pIS, rdr::OutStream *pOS);
      bool processFTMsg(int type);
      bool show(HWND hwndParent);

      void requestFileList(char *pPath, int dest, bool bDirOnly);

      void addTransferQueue(char *pLocalPath, char *pRemotePath, 
                            FileInfo *pFI, unsigned int flags);
      void addDeleteQueue(char *pPathPrefix, FileInfo *pFI, 
                          unsigned int flags);

      bool isTransferEnable();

      void checkTransferQueue();
      void checkDeleteQueue();
      bool checkCancelOperations();

      void uploadFilePortion();

      void createRemoteFolder(char *pPath, char *pName);

      bool m_bCancel;

    private:
      bool m_bFTDlgShown;
      bool m_bInitialized;
      bool m_bResized;

      FTDialog *m_pFTDialog;

      FTMsgReader *m_pReader;
      FTMsgWriter *m_pWriter;

      FileReader m_fileReader;
      FileWriter m_fileWriter;

      FileInfo m_queueFileListRqst;

      TransferQueue m_TransferQueue;
      TransferQueue m_DeleteQueue;

      bool resizeSending();
      bool uploadFile();
      bool downloadFile();

      int  isExistName(FileInfo *pFI, char *pName);
      void freeQueues();

      void endUndoneOperation();
      
      bool procFileListDataMsg();
      bool procFileDownloadDataMsg();
      bool procFileUploadCancelMsg();
      bool procFileDownloadFailedMsg();
      bool procFileDirSizeDataMsg();
      bool procFileLastRqstFailedMsg();
      
      bool procFLRMain(FileInfo *pFI);
      bool procFLRBrowse(FileInfo *pFI);
      bool procFLRUpload(FileInfo *pFI);
      bool procFLRDownload(FileInfo *pFI);
      bool procFLRDelete(FileInfo *pFI);
      bool procFLRRename(FileInfo *pFI);

      DWORD64 m_dw64SizeSending;
      unsigned int m_dirSizeRqstNum;
    };
  }
}

#endif // __RFB_WIN32_FILETRANSFER_H__
