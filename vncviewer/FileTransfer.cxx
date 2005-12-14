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
  m_bResized = false;
  m_bCancel = false;

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
  
  if (m_pReader != NULL) {
    delete m_pReader;
    m_pReader = NULL;
  }

  if (m_pWriter != NULL) {
    delete m_pWriter;
    m_pWriter = NULL;
  }

  freeQueues();
}

bool 
FileTransfer::initialize(rdr::InStream *pIS, rdr::OutStream *pOS)
{
  if (m_bInitialized) return false;

  m_pReader = new FTMsgReader(pIS);
  m_pWriter = new FTMsgWriter(pOS);

  freeQueues();

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
FileTransfer::addDeleteQueue(char *pPathPrefix, FileInfo *pFI, unsigned int flags)
{
  if ((m_bFTDlgShown) && (m_DeleteQueue.getNumEntries() > 0)) 
    m_pFTDialog->setStatusText("Starting Delete Operation");

  m_DeleteQueue.add(pPathPrefix, pPathPrefix, pFI, flags);

  checkDeleteQueue();
}

void
FileTransfer::checkDeleteQueue()
{
  if (m_DeleteQueue.getNumEntries() > 0) {
    if (m_bFTDlgShown) 
      m_pFTDialog->setStatusText("Delete Operation Executing: %s", m_DeleteQueue.getFullLocPathAt(0));
    if (m_DeleteQueue.getFlagsAt(0) & FT_ATTR_DELETE_LOCAL) {
      FolderManager fm;
      if (!fm.deleteIt(m_DeleteQueue.getFullLocPathAt(0))) {
        if (m_bFTDlgShown) m_pFTDialog->setStatusText("Delete Operation Failed");
      } else {
        if (m_bFTDlgShown) m_pFTDialog->setStatusText("Delete Operation Completed");
      }
      m_DeleteQueue.deleteAt(0);
      m_pFTDialog->postCheckDeleteQueueMsg();
    } else {
      if (m_DeleteQueue.getFlagsAt(0) & FT_ATTR_DELETE_REMOTE) {
        m_pWriter->writeFileDeleteRqst(strlen(m_DeleteQueue.getFullLocPathAt(0)), 
                                       m_DeleteQueue.getFullLocPathAt(0));

        char *pPath = m_DeleteQueue.getLocPathAt(0);
        m_queueFileListRqst.add(pPath, 0, 0, FT_FLR_DEST_DELETE);
        m_pWriter->writeFileListRqst(strlen(pPath), pPath, false);
      }
    }
  } else {
    if (m_bFTDlgShown) {
      m_pFTDialog->setStatusText("Delete Operation Completed Successfully");
      PostMessage(m_pFTDialog->getWndHandle(), WM_COMMAND, MAKEWPARAM(IDC_FTLOCALRELOAD, 0), 0);
      PostMessage(m_pFTDialog->getWndHandle(), WM_COMMAND, MAKEWPARAM(IDC_FTREMOTERELOAD, 0), 0);
    }
  }
}

void 
FileTransfer::addTransferQueue(char *pLocalPath, char *pRemotePath, 
                               FileInfo *pFI, unsigned int flags)
{
  if ((m_bFTDlgShown) && (!isTransferEnable())) m_pFTDialog->setStatusText("Starting Copy Operation");
  
  m_TransferQueue.add(pLocalPath, pRemotePath, pFI, (flags | FT_ATTR_RESIZE_NEEDED));

  checkTransferQueue();
}

bool
FileTransfer::resizeSending()
{
  for (unsigned int i = 0; i < m_TransferQueue.getNumEntries(); i++) {
    unsigned int flags = m_TransferQueue.getFlagsAt(i);
    if (flags & FT_ATTR_RESIZE_NEEDED) {
      if (flags & FT_ATTR_FILE) {
        m_bResized = true;
        m_dw64SizeSending += m_TransferQueue.getSizeAt(i);
        m_TransferQueue.clearFlagAt(i, FT_ATTR_RESIZE_NEEDED);
      } else {
        if (flags & FT_ATTR_DIR) {
          if (flags & FT_ATTR_COPY_DOWNLOAD) {
            m_bResized = true;
            char *pPath = m_TransferQueue.getFullRemPathAt(i);
            m_dirSizeRqstNum = i;
            m_pWriter->writeFileDirSizeRqst(strlen(pPath), pPath);
            return false;
          } else {
            if (flags & FT_ATTR_COPY_UPLOAD) {
              FolderManager fm;
              DWORD64 dw64Size;
              m_bResized = true;
              fm.getDirSize(m_TransferQueue.getFullLocPathAt(i), &dw64Size);
              m_dw64SizeSending += dw64Size;
              m_TransferQueue.clearFlagAt(i, FT_ATTR_RESIZE_NEEDED);
            }
          } // if (flags & FT_ATTR_COPY_DOWNLOAD)
        } // if (flags & FT_ATTR_FOLDER)
      } // if (flags & FT_ATTR_FILE)
    } // if (flags & FT_ATTR_NEEDED_RESIZE)
  } // for (unsigned int i = 0; i < m_TransferQueue.getNumEntries(); i++)

  if ((m_bFTDlgShown) && (m_bResized)) {
    m_pFTDialog->m_pProgress->clearAndInitGeneral(m_dw64SizeSending, 0);
    m_bResized = false;
  }

  return true;
}

void
FileTransfer::checkTransferQueue()
{
  if (!isTransferEnable()) {
    if (m_bFTDlgShown) {
      m_pFTDialog->m_pProgress->clearAll();
      m_dw64SizeSending = 0;
      m_bResized = false;
      m_pFTDialog->setStatusText("File Transfer Operation Completed Successfully");
      m_pFTDialog->afterCancelTransfer();
      PostMessage(m_pFTDialog->getWndHandle(), WM_COMMAND, MAKEWPARAM(IDC_FTLOCALRELOAD, 0), 0);
      PostMessage(m_pFTDialog->getWndHandle(), WM_COMMAND, MAKEWPARAM(IDC_FTREMOTERELOAD, 0), 0);
      return;
    }
  } else {
    if (!resizeSending()) return;

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
        m_pWriter->writeFileListRqst(strlen(pPath), pPath, false);
        return;
      }
    } else {
      if (flag0 & FT_ATTR_COPY_DOWNLOAD) {
        if (flag0 & FT_ATTR_FILE) {
          downloadFile();
          return;
        }
        if (flag0 & FT_ATTR_DIR) {
          FolderManager fm;
          char *pLocPath = m_TransferQueue.getFullLocPathAt(0);
          if (m_bFTDlgShown) m_pFTDialog->setStatusText("Creating Local Folder. %s", pLocPath);
          
          if (!fm.createDir(pLocPath)) {
            if (m_bFTDlgShown) m_pFTDialog->setStatusText("Creating Local Folder Failed.");
            m_TransferQueue.deleteAt(0);
            m_pFTDialog->postCheckTransferQueueMsg();
            return;
          } else {
            if ((m_bFTDlgShown) && (strcmp(m_TransferQueue.getLocPathAt(0), m_pFTDialog->getLocalPath()) == 0))
              PostMessage(m_pFTDialog->getWndHandle(), WM_COMMAND, MAKEWPARAM(IDC_FTLOCALRELOAD, 0), 0);

            m_TransferQueue.setFlagsAt(0, (m_TransferQueue.getFlagsAt(0) | FT_ATTR_FLR_DOWNLOAD_ADD));
            char *pRemPath = m_TransferQueue.getFullRemPathAt(0);
            m_queueFileListRqst.add(pRemPath, 0, 0, FT_FLR_DEST_DOWNLOAD);
            m_pWriter->writeFileListRqst(strlen(pRemPath), pRemPath, 0);
            return;
          }
        }
      }
    }
    if (m_bFTDlgShown) m_pFTDialog->setStatusText("File Transfer Operation Failed. Unknown data in the transfer queue");
  } // if (!isTransferEnable())
}

bool
FileTransfer::uploadFile()
{
  if (m_TransferQueue.getFlagsAt(0) & FT_ATTR_FILE) {
    if (m_fileReader.create(m_TransferQueue.getFullLocPathAt(0))) {
      
      if (m_bFTDlgShown) {
        m_pFTDialog->setStatusText("Upload Started: %s to %s", 
                                   m_TransferQueue.getFullLocPathAt(0), 
                                   m_TransferQueue.getFullRemPathAt(0));
        m_pFTDialog->m_pProgress->clearAndInitSingle(m_TransferQueue.getSizeAt(0), 0);
      }

      m_pWriter->writeFileUploadRqst(strlen(m_TransferQueue.getFullRemPathAt(0)),
                                     m_TransferQueue.getFullRemPathAt(0), 0);
      uploadFilePortion();
    }
  }
  return false;
}

bool
FileTransfer::downloadFile()
{
  if (m_TransferQueue.getFlagsAt(0) & FT_ATTR_FILE) {
    if (m_fileWriter.create(m_TransferQueue.getFullLocPathAt(0))) {
      if (m_bFTDlgShown) {
        m_pFTDialog->setStatusText("Download Started: %s to %s", 
                                   m_TransferQueue.getFullRemPathAt(0), 
                                   m_TransferQueue.getFullLocPathAt(0));
        m_pFTDialog->m_pProgress->clearAndInitSingle(m_TransferQueue.getSizeAt(0), 0);
      }
      m_pWriter->writeFileDownloadRqst(strlen(m_TransferQueue.getFullRemPathAt(0)),
                                       m_TransferQueue.getFullRemPathAt(0), 0);
      return true;
    } else return false;
  }
  return false;
}

void
FileTransfer::uploadFilePortion()
{
  if (checkCancelOperations()) {
    char reason[] = "The user cancel transfer";
    m_pWriter->writeFileUploadFailed(strlen(reason), reason);
  }

  if (m_fileReader.isCreated()) {
    char buf[FT_MAX_SENDING_SIZE];
    unsigned int bytesRead = 0;
    if (m_fileReader.read((void *)buf, FT_MAX_SENDING_SIZE, &bytesRead)) {
      if (bytesRead == 0) {
        m_pWriter->writeFileUploadData(m_TransferQueue.getDataAt(0));
        m_fileReader.close();
        if (m_bFTDlgShown) {
          m_pFTDialog->m_pProgress->clearAndInitSingle(0, 0);
          m_pFTDialog->setStatusText("Upload Completed");
        }
        m_TransferQueue.deleteAt(0);
        m_pFTDialog->postCheckTransferQueueMsg();
      } else {
        if (m_bFTDlgShown) m_pFTDialog->m_pProgress->increase(bytesRead);
        m_pWriter->writeFileUploadData(bytesRead, (char *)buf);
        m_pFTDialog->postUploadFilePortionMsg();
      }
    } else {
      m_fileReader.close();
      char reason[] = "Error While Reading File";
      m_pWriter->writeFileUploadFailed(strlen(reason), reason);
      if (m_bFTDlgShown) {
          m_pFTDialog->m_pProgress->clearAndInitSingle(0, 0);
          m_pFTDialog->setStatusText("Upload Failed");
      }
      m_TransferQueue.deleteAt(0);
      m_pFTDialog->postCheckTransferQueueMsg();
    }
  }
}

void 
FileTransfer::createRemoteFolder(char *pPath, char *pName)
{
  char fullPath[FT_FILENAME_SIZE];
  sprintf(fullPath, "%s\\%s", pPath, pName);
  m_pFTDialog->setStatusText("Creating Remote Folder: %s", fullPath);
  m_pWriter->writeFileCreateDirRqst(strlen(fullPath), fullPath);
  m_queueFileListRqst.add(pPath, 0, 0, FT_FLR_DEST_MAIN);
  m_pWriter->writeFileListRqst(strlen(pPath), pPath, false);
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
  unsigned int bufSize = 0;
  unsigned int modTime = 0;

  void *pFile = m_pReader->readFileDownloadData(&bufSize, &modTime);

  if (checkCancelOperations()) {
    char reason[] = "The user cancel transfer";
    m_pWriter->writeFileDownloadCancel(strlen(reason), reason);
  }

  if ((!m_fileWriter.isCreated()) || (!isTransferEnable())) {
    if (pFile != NULL) delete pFile;
    return false;
  }

  if (bufSize > 0) {
    unsigned int bytesWritten = 0;
    m_fileWriter.write(pFile, bufSize, &bytesWritten);
    delete pFile;
    if (bytesWritten != bufSize) {
      char reason[] = "Error File Writting to File";
      m_pWriter->writeFileDownloadCancel(strlen(reason), reason);
      if (m_bFTDlgShown) {
        m_pFTDialog->setStatusText("Download Failed");
        m_pFTDialog->m_pProgress->clearAndInitSingle(0, 0);
      }
      m_TransferQueue.deleteAt(0);
      m_pFTDialog->postCheckTransferQueueMsg();
      return false;
    } else {
      if (m_bFTDlgShown) {
        m_pFTDialog->m_pProgress->increase(bufSize);
      }
    }
    return true;
  } else {
    if (modTime != 0) {
      m_fileWriter.setTime(modTime);
      m_fileWriter.close();
      if (m_bFTDlgShown) {
        m_pFTDialog->setStatusText("Download Completed");
        m_pFTDialog->m_pProgress->clearAndInitSingle(0, 0);
      }
      m_TransferQueue.deleteAt(0);
      m_pFTDialog->postCheckTransferQueueMsg();
      return true;
    } else {
      m_fileWriter.close();
      char reason[] = "Error File Writting";
      if (m_bFTDlgShown) {
        m_pFTDialog->setStatusText("Download Failed");
        m_pFTDialog->m_pProgress->clearAndInitSingle(0, 0);
      }
      m_pWriter->writeFileDownloadCancel(strlen(reason), reason);
      m_TransferQueue.deleteAt(0);
      m_pFTDialog->postCheckTransferQueueMsg();
    }
  }
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
  m_dw64SizeSending += dw64DirSize;
  m_TransferQueue.clearFlagAt(m_dirSizeRqstNum, FT_ATTR_RESIZE_NEEDED);
  checkTransferQueue();
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
  unsigned int flags = m_TransferQueue.getFlagsAt(0);
  if (flags & FT_ATTR_FLR_UPLOAD_CHECK) {
    int num = isExistName(pFI, m_TransferQueue.getRemNameAt(0));
    if (num >= 0) { 
      if ((m_bFTDlgShown) && (strcmp(m_TransferQueue.getRemPathAt(0), m_pFTDialog->getRemotePath()) == 0)) {
        m_pFTDialog->addRemoteLVItems(pFI);
      }
    } else {
      if (flags & FT_ATTR_DIR) {
        m_TransferQueue.deleteAt(0);
        if (m_bFTDlgShown) m_pFTDialog->setStatusText("Create Remote Folder Failed.");
      }
    }
  }
  FolderManager fm;
  FileInfo fi;
  flags = m_TransferQueue.getFlagsAt(0);
  if (flags & FT_ATTR_FILE) {
    uploadFile();
    return true;
  } else {
    if (fm.getDirInfo(m_TransferQueue.getFullLocPathAt(0), &fi, 0)) {
      m_TransferQueue.add(m_TransferQueue.getFullLocPathAt(0),
        m_TransferQueue.getFullRemPathAt(0),
        &fi, FT_ATTR_COPY_UPLOAD);
    }
  }
  m_TransferQueue.deleteAt(0);
  m_pFTDialog->postCheckTransferQueueMsg();
  return true;
}

bool 
FileTransfer::procFLRDownload(FileInfo *pFI)
{
  unsigned int flags = m_TransferQueue.getFlagsAt(0);
  
  if ((flags & FT_ATTR_DIR) && (flags & FT_ATTR_FLR_DOWNLOAD_ADD)) {
    m_TransferQueue.add(m_TransferQueue.getFullLocPathAt(0), 
                        m_TransferQueue.getFullRemPathAt(0), 
                        pFI, FT_ATTR_COPY_DOWNLOAD);
    m_TransferQueue.deleteAt(0);
    m_pFTDialog->postCheckTransferQueueMsg();
    return true;
  } else {
    if (m_bFTDlgShown) m_pFTDialog->setStatusText("File Transfer Operation Failed: Unknown data from server.");
  }
  return false;
}

bool 
FileTransfer::procFLRDelete(FileInfo *pFI)
{
  if (isExistName(pFI, m_DeleteQueue.getLocNameAt(0)) >= 0) {
    if (m_bFTDlgShown) m_pFTDialog->setStatusText("Delete Operation Failed.");
  } else {
    if (m_bFTDlgShown) m_pFTDialog->setStatusText("Delete Operation Completed.");
  }
  m_DeleteQueue.deleteAt(0);
  checkDeleteQueue();
  return true;
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

int
FileTransfer::isExistName(FileInfo *pFI, char *pName)
{
  for (unsigned int i = 0; i < pFI->getNumEntries(); i++) {
    if (strcmp(pFI->getNameAt(i), pName) == 0) {
      return i;
    }
  }
  return -1;
}

bool
FileTransfer::checkCancelOperations()
{
  if (m_bFTDlgShown) m_pFTDialog->processDlgMsgs();
  if (m_bCancel) {
    endUndoneOperation();
    if (m_bFTDlgShown) {
      m_pFTDialog->setStatusText("All Operations Canceled");
    }
    return true;
  } else {
    return false;
  }
}

void 
FileTransfer::endUndoneOperation()
{
  m_bCancel = false;
  m_fileReader.close();
  m_fileWriter.close();
  freeQueues();
  m_dw64SizeSending = 0;
  m_pFTDialog->m_pProgress->clearAll();
}

void
FileTransfer::freeQueues()
{
  m_TransferQueue.free();
  m_DeleteQueue.free();
  m_queueFileListRqst.free();
}
