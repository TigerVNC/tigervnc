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
  return false;
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

  m_pWriter->writeFileListRqst(pPath, bDirOnly);
}