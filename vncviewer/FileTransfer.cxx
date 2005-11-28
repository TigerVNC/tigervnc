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

// -=- FileTransfer.cxx

#include <vncviewer/FileTransfer.h>

using namespace rfb;
using namespace rfb::win32;

FileTransfer::FileTransfer()
{
  m_bFTDlgShown = false;
  m_bInitialized = false;

  m_pFTDialog = new FTDialog(GetModuleHandle(0), this);

  m_pReader = NULL;
  m_pWriter = NULL;

  m_dw64SizeSending = 0;
  m_dirSizeRqstNum = 0;
}

FileTransfer::~FileTransfer()
{
  if (m_pFTDialog != NULL) {
    delete m_pFTDialog;
    m_pFTDialog = NULL;
  }
}

bool 
FileTransfer::initialize(rdr::InStream *pIS, rdr::OutStream *pOS)
{
  if (m_bInitialized) return false;

  m_pReader = new FTMsgReader(pIS);
  m_pWriter = new FTMsgWriter(pOS);

  m_TransferQueue.free();

  m_bInitialized = true;
  return true;
}

bool 
FileTransfer::show(HWND hwndParent)
{
  if (!m_bInitialized) return false;

  m_bFTDlgShown = m_pFTDialog->createFTDialog(hwndParent);
  return m_bFTDlgShown;
}

bool
FileTransfer::processFTMsg(int type)
{
  if (!m_bInitialized) return false;

  switch (type)
  {
  case msgTypeFileListData:
    return procFileListDataMsg();
  case msgTypeFileDownloadData:
    return procFileDownloadDataMsg();
  case msgTypeFileUploadCancel:
    return procFileUploadCancelMsg();
  case msgTypeFileDownloadFailed:
    return procFileDownloadFailedMsg();
  case msgTypeFileDirSizeData:
    return procFileDirSizeDataMsg();
  case msgTypeFileLastRequestFailed:
    return procFileLastRqstFailedMsg();
  default:
    return false;
  }
}

bool 
FileTransfer::isTransferEnable()
{
  if (m_TransferQueue.getNumEntries() > 0) return true; else return false;
}

void 
FileTransfer::upload(TransferQueue *pTransQueue)
{
  if ((m_bFTDlgShown) && (!isTransferEnable())) m_pFTDialog->setStatusText("Starting Copy Operation");
  
  pTransQueue->setFlagToAll(FT_ATTR_RESIZE_NEEDED);
  
  m_TransferQueue.add(pTransQueue);

  resizeSending();

  checkTransferQueue();
}

void 
FileTransfer::download(TransferQueue *pTransQueue)
{

}

void
FileTransfer::resizeSending()
{

}

void
FileTransfer::checkTransferQueue()
{
  if (!isTransferEnable()) {
    if (m_bFTDlgShown) {
      m_pFTDialog->setStatusText("File Transfer Operation Completed Successfully");
      PostMessage(m_pFTDialog->getWndHandle(), WM_COMMAND, MAKEWPARAM(IDC_FTLOCALRELOAD, 0), 0);
      PostMessage(m_pFTDialog->getWndHandle(), WM_COMMAND, MAKEWPARAM(IDC_FTREMOTERELOAD, 0), 0);
      return;
    }
  } else {
    for (unsigned int i = 0; i < m_TransferQueue.getNumEntries(); i++) {
      unsigned int flags = m_TransferQueue.getFlagsAt(i);
      if (flags & FT_ATTR_RESIZE_NEEDED) {
        if (flags & FT_ATTR_FILE) {
          m_dw64SizeSending += m_TransferQueue.getSizeAt(i);
          m_TransferQueue.clearFlagAt(i, FT_ATTR_RESIZE_NEEDED);
        } else {
          if (flags & FT_ATTR_DIR) {
            if (flags & FT_ATTR_COPY_DOWNLOAD) {
              char *pPath = m_TransferQueue.getFullRemPathAt(i);
              m_dirSizeRqstNum = i;
              m_pWriter->writeFileDirSizeRqst(strlen(pPath), pPath);
              return;
            } else {
              if (flags & FT_ATTR_COPY_UPLOAD) {
                FolderManager fm;
                DWORD64 dw64Size;
                fm.getDirSize(m_TransferQueue.getFullLocPathAt(i), &dw64Size);
                m_dw64SizeSending += dw64Size;
                m_TransferQueue.clearFlagAt(i, FT_ATTR_RESIZE_NEEDED);
              }
            } // if (flags & FT_ATTR_COPY_DOWNLOAD)
          } // if (flags & FT_ATTR_FOLDER)
        } // if (flags & FT_ATTR_FILE)
      } // if (flags & FT_ATTR_NEEDED_RESIZE)
    } // for (unsigned int i = 0; i < m_TransferQueue.getNumEntries(); i++)

    unsigned int flag0 = m_TransferQueue.getFlagsAt(0);
    
    if (flag0 & FT_ATTR_COPY_UPLOAD) {
      if (flag0 & FT_ATTR_FILE) {
          uploadFile();
          return;
      }
      if (flag0 & FT_ATTR_DIR) {
        char *pFullPath = m_TransferQueue.getFullRemPathAt(0);
        if (m_bFTDlgShown) m_pFTDialog->setStatusText("Creating Remote Folder. %s", pFullPath);
        m_pWriter->writeFileCreateDirRqst(strlen(pFullPath), pFullPath);

        char *pPath = m_TransferQueue.getRemPathAt(0);
        m_TransferQueue.setFlagsAt(0, (flag0 | FT_ATTR_FLR_UPLOAD_CHECK));
        m_queueFileListRqst.add(pPath, 0, 0, FT_FLR_DEST_UPLOAD);
        m_pWriter->writeFileListRqst(strlen(pPath), pPath, 0);
        return;
      }
    } else {
      if (flag0 & FT_ATTR_COPY_DOWNLOAD) {
        if (flag0 & FT_ATTR_FILE) {
        }
        if (flag0 & FT_ATTR_DIR) {
        }
      }
    }
    if (m_bFTDlgShown) m_pFTDialog->setStatusText("File Transfer Operation Failed. Unknown data in the transfer queue");

  } // if (!isTransferEnable())
}

bool
FileTransfer::uploadFile()
{
  return false;
}

bool
FileTransfer::downloadFile()
{
  return false;
}

bool 
FileTransfer::procFileListDataMsg()
{
  FileInfo fileInfo;
  int res = m_pReader->readFileListData(&fileInfo);

  bool bResult;
  switch (m_queueFileListRqst.getFlagsAt(0))
  {
  case FT_FLR_DEST_MAIN:
    if (!m_bFTDlgShown) break;
    
    if (res < 0) {
      m_pFTDialog->reqFolderUnavailable();
      bResult = true;
    } else {
      bResult = procFLRMain(&fileInfo);
    }
    break;
  case FT_FLR_DEST_BROWSE:
    bResult = procFLRBrowse(&fileInfo);
    break;
  case FT_FLR_DEST_UPLOAD:
    bResult = procFLRUpload(&fileInfo);
    break;
  case FT_FLR_DEST_DOWNLOAD:
    bResult = procFLRDownload(&fileInfo);
    break;
  case FT_FLR_DEST_DELETE:
    bResult = procFLRDelete(&fileInfo);
    break;
  case FT_FLR_DEST_RENAME:
    bResult = procFLRRename(&fileInfo);
    break;
  }
  m_queueFileListRqst.deleteAt(0);
  return bResult;
}

bool 
FileTransfer::procFileDownloadDataMsg()
{
  return false;
}

bool 
FileTransfer::procFileUploadCancelMsg()
{
  return false;
}

bool 
FileTransfer::procFileDownloadFailedMsg()
{
  return false;
}

bool 
FileTransfer::procFileDirSizeDataMsg()
{
  DWORD64 dw64DirSize = 0;
  m_pReader->readFileDirSizeData(&dw64DirSize);
  m_TransferQueue.clearFlagAt(m_dirSizeRqstNum, FT_ATTR_RESIZE_NEEDED);
  return true;
}

bool 
FileTransfer::procFileLastRqstFailedMsg()
{
  return false;
}

bool 
FileTransfer::procFLRMain(FileInfo *pFI)
{
  if (m_bFTDlgShown) m_pFTDialog->addRemoteLVItems(pFI);
  return true;
}

bool 
FileTransfer::procFLRBrowse(FileInfo *pFI)
{
  return false;
}

bool 
FileTransfer::procFLRUpload(FileInfo *pFI)
{
  return false;
}

bool 
FileTransfer::procFLRDownload(FileInfo *pFI)
{
  return false;
}

bool 
FileTransfer::procFLRDelete(FileInfo *pFI)
{
  return false;
}

bool 
FileTransfer::procFLRRename(FileInfo *pFI)
{
  return false;
}

void 
FileTransfer::requestFileList(char *pPath, int dest, bool bDirOnly)
{
  m_queueFileListRqst.add(pPath, 0, 0, dest);

  m_pWriter->writeFileListRqst(strlen(pPath), pPath, bDirOnly);
}