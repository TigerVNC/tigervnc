/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
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

// -=- FTDialog.cxx

#include <vncviewer/FTDialog.h>

using namespace rfb;
using namespace rfb::win32;

const char FTDialog::szCheckDeleteQueueText[]    = "TightVNC.Viewer.CheckDeleteQueue.Msg";
const char FTDialog::szCheckTransferQueueText[]  = "TightVNC.Viewer.CheckTransferQueue.Msg";
const char FTDialog::szUploadFilePortionText[]   = "TightVNC.Viewer.UploadFilePortion.Msg";

FTDialog::FTDialog(HINSTANCE hInst, FileTransfer *pFT)
{
  m_pFileTransfer = pFT;
  m_hInstance = hInst;

  m_bDlgShown = false;
  m_bLocalBrowsing = true;
  m_bCloseDlgAfterCancel = false;

  m_pLocalLV = NULL;
  m_pRemoteLV = NULL;
  m_pProgress = NULL;
  m_pCancelingDlg = NULL;
  m_pCreateFolderDlg = NULL;
  m_pRenameDlg = NULL;
  m_pBrowseDlg = NULL;

  m_hwndFTDialog = NULL;
  m_hwndLocalPath = NULL;
  m_hwndRemotePath = NULL;

  m_FTMenuSource = 0;
  m_dwNumStatusStrings = 0;

  m_szLocalPath[0] = '\0';
  m_szRemotePath[0] = '\0';
  m_szLocalPathTmp[0] = '\0';
  m_szRemotePathTmp[0] = '\0';
  m_szCreateFolderName[0] = '\0';
}

FTDialog::~FTDialog()
{
  destroyFTDialog();
}

bool
FTDialog::createFTDialog(HWND hwndParent)
{
  if (m_hwndFTDialog != NULL) {
    ShowWindow(m_hwndFTDialog, SW_SHOW);
    m_bDlgShown = true;
    showLocalLVItems();
    showRemoteLVItems();
    return true;
  }

  if (!initFTWndMsgs()) return false;

  m_hwndFTDialog = CreateDialogParam(m_hInstance, 
                                     MAKEINTRESOURCE(IDD_FILETRANSFER_DLG),
                                     hwndParent, 
                                     (DLGPROC) FTDialogProc,
                                     (LONG) this);
  
  if (m_hwndFTDialog == NULL) return false;

  HWND hwndLocalList = GetDlgItem(m_hwndFTDialog, IDC_FTLOCALLIST);
  HWND hwndRemoteList = GetDlgItem(m_hwndFTDialog, IDC_FTREMOTELIST);

  if ((hwndLocalList == NULL) || (hwndRemoteList == NULL)) {
    destroyFTDialog();
    return false;
  }

  m_pLocalLV = new FTListView(hwndLocalList);
  m_pRemoteLV = new FTListView(hwndRemoteList);
  
  m_pProgress = new FTProgress(m_hwndFTDialog);
  
  if ((m_pLocalLV == NULL) || (m_pRemoteLV == NULL) || (m_pProgress == NULL)) {
    destroyFTDialog();
    return false;
  }

  initFTDialog();
  
  ShowWindow(m_hwndFTDialog, SW_SHOW);
  UpdateWindow(m_hwndFTDialog);
  m_bDlgShown = true;
  return true;
}

bool
FTDialog::initFTDialog()
{
  m_pLocalLV->initialize(m_hInstance);
  m_pRemoteLV->initialize(m_hInstance);

  m_hwndLocalPath = GetDlgItem(m_hwndFTDialog, IDC_FTLOCALPATH);
  m_hwndRemotePath = GetDlgItem(m_hwndFTDialog, IDC_FTREMOTEPATH);

  setIcon(IDC_FTLOCALUP, IDI_FTUP);
  setIcon(IDC_FTREMOTEUP, IDI_FTUP);
  setIcon(IDC_FTLOCALRELOAD, IDI_FTRELOAD);
  setIcon(IDC_FTREMOTERELOAD, IDI_FTRELOAD);

  showLocalLVItems();
  showRemoteLVItems();

  return true;
}

bool
FTDialog::initFTWndMsgs()
{
  m_msgCheckDeleteQueue    = RegisterWindowMessage(szCheckDeleteQueueText);
  m_msgCheckTransferQueue  = RegisterWindowMessage(szCheckTransferQueueText);
  m_msgUploadFilePortion   = RegisterWindowMessage(szUploadFilePortionText);

  if ((m_msgCheckDeleteQueue) && 
      (m_msgCheckTransferQueue) && 
      (m_msgUploadFilePortion)) return true;

  return false;
}

bool
FTDialog::closeFTDialog()
{
  ShowWindow(m_hwndFTDialog, SW_HIDE);
  m_bDlgShown = false;
  return true;
}

void
FTDialog::destroyFTDialog()
{
  if (m_pLocalLV != NULL) {
    delete m_pLocalLV;
    m_pLocalLV = NULL;
  }

  if (m_pRemoteLV != NULL) {
    delete m_pRemoteLV;
    m_pRemoteLV = NULL;
  }

  if (m_pProgress != NULL) {
    delete m_pProgress;
    m_pProgress = NULL;
  }

  if(m_pCancelingDlg != NULL) {
    delete m_pCancelingDlg;
    m_pCancelingDlg = NULL;
  }

  if (m_pCreateFolderDlg != NULL) {
    delete m_pCreateFolderDlg;
    m_pCreateFolderDlg = NULL;
  }

  if(m_pRenameDlg != NULL) {
    delete m_pRenameDlg;
    m_pRenameDlg = NULL;
  }

  if (DestroyWindow(m_hwndFTDialog)) m_hwndFTDialog = NULL;
}

BOOL CALLBACK 
FTDialog::FTDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  FTDialog *_this = (FTDialog *) GetWindowLong(hwnd, GWL_USERDATA);

  switch (uMsg)
  {
  case WM_INITDIALOG:
    _this = (FTDialog*)lParam;
    SetWindowLong(hwnd, GWL_USERDATA, (LONG) lParam);
    SetForegroundWindow(hwnd);
    return TRUE;
  case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
      case IDC_FTLOCALPATH:
        switch (HIWORD (wParam))
        {
        case CBN_SETFOCUS:
          _this->setButtonsState();
          return FALSE;
        }
        break;
      case IDC_FTREMOTEPATH:
        switch (HIWORD (wParam))
        {
        case CBN_SETFOCUS:
          _this->setButtonsState();
          return FALSE;
        }
        break;
      case IDC_FTCLOSE:
        _this->onClose();
        return FALSE;
      case IDC_FTLOCALBROWSE:
        _this->onLocalBrowse();
        return FALSE;
      case IDC_FTREMOTEBROWSE:
        _this->onRemoteBrowse();
        return FALSE;
      case IDC_FTLOCALUP:
        _this->setButtonsState();
        _this->onLocalOneUpFolder();
        return FALSE;
      case IDC_FTREMOTEUP:
        _this->setButtonsState();
        _this->onRemoteOneUpFolder();
        return FALSE;
      case IDC_FTLOCALRELOAD:
        _this->setButtonsState();
        _this->onLocalReload();
        return FALSE;
      case IDC_FTREMOTERELOAD:
       _this->setButtonsState();
       _this->onRemoteReload();
        return FALSE;
      case IDC_FTUPLOAD:
        _this->onUpload();
        return FALSE;
      case IDC_FTDOWNLOAD:
        _this->onDownload();
        return FALSE;
      case IDC_FTCANCEL:
        _this->onFTCancel();
        return FALSE;
      case IDM_FTCOPY:
      case IDM_FTRENAME:
      case IDM_FTDELETE:
      case IDM_FTCANCEL:
      case IDM_FTCREATEFOLDER:
        _this->onFTMenuCommand(LOWORD(wParam));
        return FALSE;
      }
    }
    break;
    
  case WM_NOTIFY:
    switch (LOWORD(wParam))
    {
    case IDC_FTLOCALLIST:
      switch (((LPNMHDR) lParam)->code)
      {
      case NM_CLICK:
      case NM_SETFOCUS:
      case LVN_ITEMCHANGED:
        _this->setButtonsState();
        return FALSE;
      case LVN_GETDISPINFO:
        _this->m_pLocalLV->onGetDispInfo((NMLVDISPINFO *) lParam);
        return FALSE;
      case LVN_ITEMACTIVATE:
        _this->onLocalItemActivate((LPNMITEMACTIVATE) lParam);
        return FALSE;
      case NM_RCLICK:
        _this->onLocalRButton();
        return FALSE;
      }
      break;
    case IDC_FTREMOTELIST:
      switch (((LPNMHDR) lParam)->code)
      {
      case NM_CLICK:
      case NM_SETFOCUS:
      case LVN_ITEMCHANGED:
        _this->setButtonsState();
      case LVN_GETDISPINFO:
        _this->m_pRemoteLV->onGetDispInfo((NMLVDISPINFO *) lParam);
        return FALSE;
      case LVN_ITEMACTIVATE:
        _this->onRemoteItemActivate((LPNMITEMACTIVATE) lParam);
        return FALSE;
      case NM_RCLICK:
        _this->onRemoteRButton();
        return FALSE;
      }
      break;
    }
    break;
    case WM_CLOSE:
      _this->onClose();
      return FALSE;
  }

  if (_this != NULL) {
    if (uMsg == _this->m_msgCheckTransferQueue)
      _this->m_pFileTransfer->checkTransferQueue();

    if (uMsg == _this->m_msgUploadFilePortion)
      _this->m_pFileTransfer->uploadFilePortion();
    
    if (uMsg == _this->m_msgCheckDeleteQueue)
      _this->m_pFileTransfer->checkDeleteQueue();
  }

  return FALSE;
}

void
FTDialog::onClose()
{
  if (m_pFileTransfer->isTransferEnable()) {
    m_bCloseDlgAfterCancel = true;
    onFTCancel();
  } else {
    closeFTDialog();
  }  
}

void 
FTDialog::onLocalItemActivate(LPNMITEMACTIVATE lpnmia)
{
  if (strlen(m_szLocalPath) == 0) {
   strcpy(m_szLocalPathTmp, m_pLocalLV->getActivateItemName(lpnmia)); 
  } else {
    sprintf(m_szLocalPathTmp, "%s\\%s", m_szLocalPath, m_pLocalLV->getActivateItemName(lpnmia));
  }
  showLocalLVItems();
}

void 
FTDialog::onRemoteItemActivate(LPNMITEMACTIVATE lpnmia)
{
  if (strlen(m_szRemotePath) == 0) {
   strcpy(m_szRemotePathTmp, m_pRemoteLV->getActivateItemName(lpnmia)); 
  } else {
    sprintf(m_szRemotePathTmp, "%s\\%s", m_szRemotePath, m_pRemoteLV->getActivateItemName(lpnmia));
  }
  showRemoteLVItems();
}

void 
FTDialog::onEndBrowseDlg(bool bResult)
{
  if (m_pBrowseDlg == NULL) return;

  if (bResult) {
    if (m_bLocalBrowsing) {
      strcpy(m_szLocalPathTmp, m_pBrowseDlg->getPath());
      showLocalLVItems();
    } else {
      strcpy(m_szRemotePathTmp, m_pBrowseDlg->getPath());
      showRemoteLVItems();
    }
  }
  delete m_pBrowseDlg;
  m_pBrowseDlg = NULL;
}

void 
FTDialog::onLocalBrowse()
{
  if (m_pBrowseDlg != NULL) return;
  
  m_bLocalBrowsing = true;

  m_pBrowseDlg = new FTBrowseDlg(this);
  m_pBrowseDlg->create();

  getBrowseItems("");
}

void 
FTDialog::onRemoteBrowse()
{
  if (m_pBrowseDlg != NULL) return;
  
  m_bLocalBrowsing = false;

  m_pBrowseDlg = new FTBrowseDlg(this);
  if (m_pBrowseDlg->create()) {
    m_pFileTransfer->requestFileList("", FT_FLR_DEST_BROWSE, true);
  } else {
    delete m_pBrowseDlg;
    m_pBrowseDlg = NULL;
  }
}

void 
FTDialog::onLocalReload()
{
  strcpy(m_szLocalPathTmp, m_szLocalPath);
  showLocalLVItems();
}

void 
FTDialog::onRemoteReload()
{
  strcpy(m_szRemotePathTmp, m_szRemotePath);
  showRemoteLVItems();
}

void
FTDialog::showLocalLVItems()
{
  FolderManager fm;
  FileInfo fileInfo;
  if (fm.getDirInfo(m_szLocalPathTmp, &fileInfo, 0)) {
    fileInfo.sort();
    m_pLocalLV->deleteAllItems();
    m_pLocalLV->addItems(&fileInfo);
    strcpy(m_szLocalPath, m_szLocalPathTmp);
    SetWindowText(m_hwndLocalPath, m_szLocalPath);
  }
}

void
FTDialog::showRemoteLVItems()
{
  m_pFileTransfer->requestFileList(m_szRemotePathTmp, FT_FLR_DEST_MAIN, false);
}

void 
FTDialog::addRemoteLVItems(FileInfo *pFI)
{
  pFI->sort();
  m_pRemoteLV->deleteAllItems();
  m_pRemoteLV->addItems(pFI);
  strcpy(m_szRemotePath, m_szRemotePathTmp);
  SetWindowText(m_hwndRemotePath, m_szRemotePath);
  UpdateWindow(m_hwndFTDialog);
}

void 
FTDialog::onLocalOneUpFolder()
{
  strcpy(m_szLocalPathTmp, m_szLocalPath);
  makeOneUpFolder(m_szLocalPathTmp);
  showLocalLVItems();
}

void 
FTDialog::onRemoteOneUpFolder()
{
  strcpy(m_szRemotePathTmp, m_szRemotePath);
  makeOneUpFolder(m_szRemotePathTmp);
  showRemoteLVItems();
}

void 
FTDialog::reqFolderUnavailable()
{
  strcpy(m_szRemotePathTmp, m_szRemotePath);
  SetWindowText(m_hwndRemotePath, m_szRemotePath);
  UpdateWindow(m_hwndFTDialog);
}

int
FTDialog::makeOneUpFolder(char *pPath)
{
  if (strcmp(pPath, "") == 0) return strlen(pPath);
  for (int i=(strlen(pPath)-2); i>=0; i--) {
    if (pPath[i] == '\\') {
      pPath[i] = '\0';
      break;
    }
    if (i == 0) pPath[0] = '\0';
  }
  return strlen(pPath);
}

void 
FTDialog::onUpload()
{
  FileInfo fi;
  char prefix[FT_FILENAME_SIZE];
  prefix[0] = '\0';

  if (m_pFileTransfer->isTransferEnable())
    strcpy(prefix, "File Transfer is active.\nDo you want to add selected file(s)/folder(s) to transfer queue?\n\n");
  
  int numSel = m_pLocalLV->getSelectedItems(&fi);
  if (numSel > 0) {
    char *pBuf = new char [(numSel + 4) * (MAX_PATH + 3) + strlen(prefix) + 1];
    sprintf(pBuf, "%sFrom: Local Computer %s\\\n\n", prefix, m_szLocalPath);
    
    for (unsigned int i = 0; i < fi.getNumEntries(); i++)
      sprintf(pBuf, "%s%s, ", pBuf, fi.getNameAt(i));
    
    sprintf(pBuf, "%s\n\nTo: Remote Computer %s\\", pBuf, m_szRemotePath);
    
    if (strlen(pBuf) > 2048) 
      sprintf(pBuf, "%sFrom: Local Computer %s\\\n\nTo: Remote Computer %s\\\n\nTotal %d file(s)/folder(s)",
        prefix, m_szLocalPath, m_szRemotePath, numSel);
    
    if (MessageBox(m_hwndFTDialog, pBuf, "Copy Selected Files and Folders", MB_OKCANCEL) == IDOK)
      m_pFileTransfer->addTransferQueue(m_szLocalPath, m_szRemotePath, &fi, FT_ATTR_COPY_UPLOAD);

    setButtonsState();
    
    delete [] pBuf;
    return;
  }
  
  setButtonsState();
  setStatusText("File Transfer Impossible. No file(s) selected for transfer");
}

void 
FTDialog::onDownload()
{
  FileInfo fi;
  char prefix[FT_FILENAME_SIZE];
  prefix[0] = '\0';

  if (m_pFileTransfer->isTransferEnable())
    strcpy(prefix, "File Transfer is active.\nDo you want to add selected file(s)/folder(s) to transfer queue?\n\n");
  
  int numSel = m_pRemoteLV->getSelectedItems(&fi);
  if (numSel > 0) {
    char *pBuf = new char [(numSel + 4) * (MAX_PATH + 3) + strlen(prefix) + 1];
    sprintf(pBuf, "%sFrom: Remote Computer %s\\\n\n", prefix, m_szRemotePath);
    
    for (unsigned int i = 0; i < fi.getNumEntries(); i++)
      sprintf(pBuf, "%s%s, ", pBuf, fi.getNameAt(i));
    
    sprintf(pBuf, "%s\n\nTo: Local Computer %s\\", pBuf, m_szLocalPath);
    
    if (strlen(pBuf) > 2048) 
      sprintf(pBuf, "%sFrom: Remote Computer %s\\\n\nTo: Local Computer %s\\\n\nTotal %d file(s)/folder(s)",
        prefix, m_szRemotePath, m_szLocalPath, numSel);
    
    if (MessageBox(m_hwndFTDialog, pBuf, "Copy Selected Files and Folders", MB_OKCANCEL) == IDOK)
      m_pFileTransfer->addTransferQueue(m_szLocalPath, m_szRemotePath, &fi, FT_ATTR_COPY_DOWNLOAD);

    setButtonsState();
    
    delete [] pBuf;
    return;
  }
  
  setButtonsState();
  setStatusText("File Transfer Impossible. No file(s) selected for transfer");
}

void 
FTDialog::onLocalRename()
{
  if (m_pRenameDlg != NULL) return;

  FileInfo fi;
  if (m_pLocalLV->getSelectedItems(&fi) == 1) {
    unsigned int flags = fi.getFlagsAt(0);
    m_pRenameDlg = new FTDialog::RenameDlg(this);
    if (m_pRenameDlg != NULL) {
      if (flags & FT_ATTR_DIR) {
        m_pRenameDlg->create(fi.getNameAt(0), true);
      } else {
        m_pRenameDlg->create(fi.getNameAt(0), false);
      }
      if (strlen(m_pRenameDlg->getFolderName()) > 0) {
        FolderManager fm;
        fm.renameIt(m_szLocalPath, fi.getNameAt(0), m_pRenameDlg->getFolderName());
        showLocalLVItems();
      }
      delete m_pRenameDlg;
      m_pRenameDlg = NULL;
    }
  }
  setButtonsState();
}

void 
FTDialog::onRemoteRename()
{
  if (m_pRenameDlg != NULL) return;

  FileInfo fi;
  if (m_pRemoteLV->getSelectedItems(&fi) == 1) {
    unsigned int flags = fi.getFlagsAt(0);
    m_pRenameDlg = new FTDialog::RenameDlg(this);
    if (m_pRenameDlg != NULL) {
      if (flags & FT_ATTR_DIR) {
        m_pRenameDlg->create(fi.getNameAt(0), true);
      } else {
        m_pRenameDlg->create(fi.getNameAt(0), false);
      }
      if (strlen(m_pRenameDlg->getFolderName()) > 0) {
        m_pFileTransfer->renameRemote(m_szRemotePath, fi.getNameAt(0), m_pRenameDlg->getFolderName());
      }
      delete m_pRenameDlg;
      m_pRenameDlg = NULL;
    }
  }
  setButtonsState();
}

void 
FTDialog::onLocalDelete()
{
  FileInfo fi;
  if (m_pLocalLV->getSelectedItems(&fi) > 0) {
    m_pFileTransfer->addDeleteQueue(m_szLocalPath, &fi, FT_ATTR_DELETE_LOCAL);
  }
  setButtonsState();
}

void 
FTDialog::onRemoteDelete()
{
  FileInfo fi;
  if (m_pRemoteLV->getSelectedItems(&fi) > 0) {
    m_pFileTransfer->addDeleteQueue(m_szRemotePath, &fi, FT_ATTR_DELETE_REMOTE);
  }
  setButtonsState();
}

bool
FTDialog::getCreateFolderName()
{
  if (m_pCreateFolderDlg != NULL) return false;

  bool bResult = false;

  m_pCreateFolderDlg = new FTDialog::CreateFolderDlg(this);
  m_pCreateFolderDlg->create();
  if (strlen(m_pCreateFolderDlg->getFolderName()) != 0) {
    strcpy(m_szCreateFolderName, m_pCreateFolderDlg->getFolderName());
    bResult = true;
  } else {
    m_szCreateFolderName[0] = '\0';
    bResult = false;
  }
  delete m_pCreateFolderDlg;
  m_pCreateFolderDlg = NULL;

  return bResult;
}

void 
FTDialog::onLocalCreateFolder()
{
  if (getCreateFolderName()) {
    char path[FT_FILENAME_SIZE];
    sprintf(path, "%s\\%s", m_szLocalPath, m_szCreateFolderName);
    setStatusText("Creating Local Folder: %s", path);
    FolderManager fm;
    if (fm.createDir(path)) showLocalLVItems();
  }
}

void 
FTDialog::onRemoteCreateFolder()
{
  if (getCreateFolderName()) {
    m_pFileTransfer->createRemoteFolder(m_szRemotePath, m_szCreateFolderName);
  }
}

void 
FTDialog::onFTCancel()
{
  if (m_pCancelingDlg != NULL) return;

  m_pCancelingDlg = new CancelingDlg(this);

  m_pCancelingDlg->create();
}

void 
FTDialog::cancelTransfer(bool bResult)
{
  if (m_pCancelingDlg != NULL) {
    delete m_pCancelingDlg;
    m_pCancelingDlg = NULL;
  }

  setButtonsState();

  if ((m_bCloseDlgAfterCancel) && (bResult)) {
    m_bCloseDlgAfterCancel = false;
    closeFTDialog();
  }

  m_pFileTransfer->m_bCancel = bResult;
}

void 
FTDialog::afterCancelTransfer()
{
  if (m_pCancelingDlg != NULL) {
    delete m_pCancelingDlg;
    m_pCancelingDlg = NULL;
  }
}

void
FTDialog::onFTMenuCommand(int command)
{
  switch (command) 
  {
  case IDM_FTCOPY:
    {
      switch (m_FTMenuSource)
      {
        case IDC_FTLOCALLIST:  onUpload();   break;
        case IDC_FTREMOTELIST: onDownload(); break;
        default: break;
      }
    }
    break;
  case IDM_FTRENAME:
    {
      switch (m_FTMenuSource)
      {
        case IDC_FTLOCALLIST:  onLocalRename();  break;
        case IDC_FTREMOTELIST: onRemoteRename(); break;
        default: break;
      }
    }
    break;
  case IDM_FTDELETE:
    {
      switch (m_FTMenuSource)
      {
        case IDC_FTLOCALLIST:  onLocalDelete();  break;
        case IDC_FTREMOTELIST: onRemoteDelete(); break;
        default: break;
      }
    }
    break;
  case IDM_FTCANCEL: onFTCancel(); break;
  case IDM_FTCREATEFOLDER:
    {
      switch (m_FTMenuSource)
      {
        case IDC_FTLOCALLIST:  onLocalCreateFolder();  break;
        case IDC_FTREMOTELIST: onRemoteCreateFolder(); break;
        default: break;
      }
    }
    break;
  default:
    break;
  }
}

void 
FTDialog::onLocalRButton()
{
  refreshBtnState();
  m_FTMenuSource = IDC_FTLOCALLIST;
  showFTMenu(m_BtnState.uploadBtn, m_BtnState.createLocalFldBtn, 
             m_BtnState.renameLocalBtn, m_BtnState.deleteLocalBtn, 
             m_BtnState.cancelBtn);
}

void 
FTDialog::onRemoteRButton()
{
  refreshBtnState();
  m_FTMenuSource = IDC_FTREMOTELIST;
  showFTMenu(m_BtnState.downloadBtn, m_BtnState.createRemoteFldBtn, 
             m_BtnState.renameRemoteBtn, m_BtnState.deleteRemoteBtn, 
             m_BtnState.cancelBtn);
}

void
FTDialog::showFTMenu(bool copyBtnState, bool createFldBtnState, bool renameBtnState, 
                     bool deleteBtnState, bool cancelBtnState)
{
  HMENU hMenu = LoadMenu(m_hInstance, MAKEINTRESOURCE(IDR_FTMENU));
  HMENU hFTMenu = GetSubMenu(hMenu, 0);

  SetMenuDefaultItem(hFTMenu, IDM_FTCOPY, FALSE);

  SetForegroundWindow(m_hwndFTDialog);

  if (copyBtnState) {
    EnableMenuItem(hMenu, IDM_FTCOPY, MF_ENABLED);
  } else {
    EnableMenuItem(hMenu, IDM_FTCOPY, MF_DISABLED | MF_GRAYED);
  }
  if (createFldBtnState) {
    EnableMenuItem(hMenu, IDM_FTCREATEFOLDER, MF_ENABLED);
  } else {
    EnableMenuItem(hMenu, IDM_FTCREATEFOLDER, MF_DISABLED | MF_GRAYED);
  }
  if (renameBtnState) {
    EnableMenuItem(hMenu, IDM_FTRENAME, MF_ENABLED);
  } else {
    EnableMenuItem(hMenu, IDM_FTRENAME, MF_DISABLED | MF_GRAYED);
  }
  if (deleteBtnState) {
    EnableMenuItem(hMenu, IDM_FTDELETE, MF_ENABLED);
  } else {
    EnableMenuItem(hMenu, IDM_FTDELETE, MF_DISABLED | MF_GRAYED);
  }
  if (cancelBtnState) {
    EnableMenuItem(hMenu, IDM_FTCANCEL, MF_ENABLED);
  } else {
    EnableMenuItem(hMenu, IDM_FTCANCEL, MF_DISABLED | MF_GRAYED);
  }
  DrawMenuBar(m_hwndFTDialog);

  POINT cursorPosition;
  GetCursorPos(&cursorPosition);
  TrackPopupMenu(hFTMenu, 0, cursorPosition.x, cursorPosition.y, 0, m_hwndFTDialog, 0);
  DestroyMenu(hFTMenu);
}

void
FTDialog::setIcon(int dest, int idIcon)
{
  HANDLE hIcon = LoadImage(m_hInstance, MAKEINTRESOURCE(idIcon), IMAGE_ICON, 16, 16, LR_SHARED);
  SendMessage(GetDlgItem(m_hwndFTDialog, dest), BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon);
  DestroyIcon((HICON) hIcon);
}

void
FTDialog::refreshBtnState()
{
  if (!m_bDlgShown) return;
  
  int locPathLen = strlen(m_szLocalPath);
  int remPathLen = strlen(m_szRemotePath);
  
  if (GetFocus() == m_pLocalLV->getWndHandle()) {
    m_BtnState.downloadBtn = false;
    if (strlen(m_szLocalPath) != 0) {
      m_BtnState.createLocalFldBtn = true;
      int nCount = ListView_GetSelectedCount(m_pLocalLV->getWndHandle());
      if (nCount <= 0) {
        m_BtnState.uploadBtn = false;
        m_BtnState.deleteLocalBtn = false;
        m_BtnState.renameLocalBtn = false;
      } else {
        m_BtnState.deleteLocalBtn = true;
        if (remPathLen == 0) {
          m_BtnState.uploadBtn = false;
        } else {
          m_BtnState.uploadBtn = true;
        }
        if (nCount == 1) {
          m_BtnState.renameLocalBtn = true;
        } else {
          m_BtnState.renameLocalBtn = false;
        }
      }
    } else {
      m_BtnState.uploadBtn = false;
      m_BtnState.createLocalFldBtn = false;
      m_BtnState.deleteLocalBtn = false;
      m_BtnState.renameLocalBtn = false;
      m_BtnState.downloadBtn = false;
    }
  } else {
    if (GetFocus() == m_pRemoteLV->getWndHandle()) {
      m_BtnState.uploadBtn = false;
      if (strlen(m_szRemotePath) != 0) {
        m_BtnState.createRemoteFldBtn = true;
        int nCount = ListView_GetSelectedCount(m_pRemoteLV->getWndHandle());
        if (nCount <= 0) {
          m_BtnState.downloadBtn = false;
          m_BtnState.deleteRemoteBtn = false;
          m_BtnState.renameRemoteBtn = false;
        } else {
          m_BtnState.deleteRemoteBtn = true;
          if (locPathLen == 0) {
            m_BtnState.downloadBtn = false;
          } else {
            m_BtnState.downloadBtn = true;
          }
          if (nCount == 1) {
            m_BtnState.renameRemoteBtn = true;
          } else {
            m_BtnState.renameRemoteBtn = false;
          }
        }
      } else {
        m_BtnState.downloadBtn = false;
        m_BtnState.createRemoteFldBtn = false;
        m_BtnState.deleteRemoteBtn = false;
        m_BtnState.renameRemoteBtn = false;
        m_BtnState.uploadBtn = false;
      }
    } else {
    }
  }
  if (m_pFileTransfer->isTransferEnable()) 
    m_BtnState.cancelBtn = true;
  else
    m_BtnState.cancelBtn = false;
}

void
FTDialog::setButtonsState()
{
  refreshBtnState();

  switch (m_BtnState.uploadBtn)
  {
  case false: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTUPLOAD), FALSE); break;
  case true: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTUPLOAD), TRUE); break;
  }

  switch (m_BtnState.downloadBtn)
  {
  case false: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTDOWNLOAD), FALSE); break;
  case true: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTDOWNLOAD), TRUE); break;
  }

  switch (m_BtnState.cancelBtn)
  {
  case false: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTCANCEL), FALSE); break;
  case true: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTCANCEL), TRUE); break;
  }

  UpdateWindow(m_hwndFTDialog);
}

void 
FTDialog::setStatusText(LPCSTR format,...)
{
  char text[1024];
  va_list args;
  va_start(args, format);
  int nSize = _vsnprintf(text, sizeof(text), format, args);

  HWND hStatusBox = GetDlgItem(m_hwndFTDialog, IDC_FTSTATUS);

  LRESULT lRes = SendMessage(hStatusBox, (UINT) CB_INSERTSTRING, (WPARAM) 0, (LPARAM) text);
  if ((lRes != CB_ERR) && (lRes != CB_ERRSPACE)) {
    lRes = SendMessage(hStatusBox, (UINT) CB_SETCURSEL, (WPARAM) lRes, (LPARAM) 0);
  }
  
  m_dwNumStatusStrings++;
  if (m_dwNumStatusStrings > FT_MAX_STATUS_STRINGS) {
    int numItems = SendMessage(hStatusBox, (UINT) CB_GETCOUNT, (WPARAM) 0, (LPARAM) 0); 
    if (numItems != CB_ERR) {
      m_dwNumStatusStrings--;
      SendMessage(hStatusBox, (UINT) CB_DELETESTRING, (WPARAM) numItems - 1, (LPARAM) 0); 
    }
  }
}

void 
FTDialog::getBrowseItems(char *pPath)
{
  if (m_bLocalBrowsing) {
    FileInfo fi;
    FolderManager fm;
    fm.getDirInfo(pPath, &fi, 1);
    if (m_pBrowseDlg != NULL) m_pBrowseDlg->addItems(&fi);
  } else {
    m_pFileTransfer->requestFileList(pPath, FT_FLR_DEST_BROWSE, true);
  }
}

void 
FTDialog::addBrowseItems(FileInfo *pFI)
{
  if (m_pBrowseDlg == NULL) return;

  m_pBrowseDlg->addItems(pFI);
}

void 
FTDialog::processDlgMsgs()
{
  MSG msg;
  while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
}

void 
FTDialog::postCheckTransferQueueMsg()
{
  PostMessage(m_hwndFTDialog, m_msgCheckTransferQueue, 0, 0);
}

void 
FTDialog::postUploadFilePortionMsg()
{
  PostMessage(m_hwndFTDialog, m_msgUploadFilePortion, 0, 0);
}

void 
FTDialog::postCheckDeleteQueueMsg()
{
  PostMessage(m_hwndFTDialog, m_msgCheckDeleteQueue, 0, 0);
}

FTDialog::CancelingDlg::CancelingDlg(FTDialog *pFTDlg)
{
  m_pFTDlg = pFTDlg;
  m_hwndDlg = NULL;
}

FTDialog::CancelingDlg::~CancelingDlg()
{
  destroy();
}

bool 
FTDialog::CancelingDlg::create()
{
  if (m_hwndDlg != NULL) return false;

  m_hwndDlg = CreateDialogParam(GetModuleHandle(0), 
                                MAKEINTRESOURCE(IDD_FTCANCELING),
                                NULL, 
                                (DLGPROC) cancelingDlgProc,
                                (LONG) this);

  if (m_hwndDlg == NULL) return false;

  ShowWindow(m_hwndDlg, SW_SHOW);
  DrawIcon(GetDC(m_hwndDlg), 15, 22, LoadIcon(NULL, IDI_QUESTION));
  UpdateWindow(m_hwndDlg);

  return true;
}

bool 
FTDialog::CancelingDlg::destroy()
{
  if (m_hwndDlg == NULL) return true;

  if (DestroyWindow(m_hwndDlg)) {
    m_hwndDlg = NULL;
    return true;
  } else {
    return false;
  }
}

bool
FTDialog::CancelingDlg::close(bool bResult)
{
  if (m_hwndDlg == NULL) return true;

  destroy();

  m_pFTDlg->cancelTransfer(bResult);

  return false;
}

BOOL CALLBACK 
FTDialog::CancelingDlg::cancelingDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  FTDialog::CancelingDlg *_this = (FTDialog::CancelingDlg *) GetWindowLong(hwnd, GWL_USERDATA);
  switch (uMsg)
  {
  case WM_INITDIALOG:
      SetWindowLong(hwnd, GWL_USERDATA, (LONG) lParam);
      SetForegroundWindow(hwnd);
    return TRUE;

  case WM_COMMAND:
    switch (LOWORD(wParam))
    {
    case IDOK:
      _this->close(true);
      return TRUE;
    case IDCANCEL:
      _this->close(false);
      return TRUE;
    }
    break;
    case WM_CLOSE:
      _this->close(false);
      return TRUE;
  }
  return FALSE;
}

FTDialog::CreateFolderDlg::CreateFolderDlg(FTDialog *pFTDlg) : Dialog(GetModuleHandle(0))
{
  m_pFTDlg = pFTDlg;
  m_szFolderName[0] = '\0';
}

FTDialog::CreateFolderDlg::~CreateFolderDlg()
{

}

bool
FTDialog::CreateFolderDlg::create()
{
  return showDialog(MAKEINTRESOURCE(IDD_FTCREATEFOLDER), m_pFTDlg->getWndHandle());
}

bool 
FTDialog::CreateFolderDlg::onOk()
{
  strcpy(m_szFolderName, getItemString(IDC_FTFOLDERNAME));
  return true;
}

FTDialog::RenameDlg::RenameDlg(FTDialog *pFTDlg) : CreateFolderDlg(pFTDlg)
{
  m_bFolder = false;
}

FTDialog::RenameDlg::~RenameDlg()
{
}

bool 
FTDialog::RenameDlg::create(char *pFilename, bool bFolder)
{
  m_bFolder = bFolder;
  strcpy(m_szFilename, pFilename);
  return showDialog(MAKEINTRESOURCE(IDD_FTCREATEFOLDER), m_pFTDlg->getWndHandle());
}

void
FTDialog::RenameDlg::initDialog()
{
  char buf[2*FT_FILENAME_SIZE];
  if (m_bFolder) {
    SetWindowText(handle, "Rename Folder");
    sprintf(buf, "Rename Folder '%s'", m_szFilename);
  } else {
    SetWindowText(handle, "Rename File");
    sprintf(buf, "Rename File '%s'", m_szFilename);
  }

  setItemString(IDC_FTTEXT, buf);
  setItemString(IDC_FTFOLDERNAME, m_szFilename);
  SendDlgItemMessage(handle, IDC_FTFOLDERNAME, EM_SETSEL, (WPARAM) 0, (LPARAM) -1);
}
