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

// -=- FTDialog.cxx

#include <vncviewer/FTDialog.h>

using namespace rfb;
using namespace rfb::win32;

FTDialog::FTDialog(HINSTANCE hInst, FileTransfer *pFT)
{
  m_pFileTransfer = pFT;
  m_hInstance = hInst;
  m_bDlgShown = false;

  m_pLocalLV = NULL;
  m_pRemoteLV = NULL;
  m_pProgress = NULL;

  m_hwndFTDialog = NULL;
  m_hwndLocalPath = NULL;
  m_hwndRemotePath = NULL;

  m_szLocalPath[0] = '\0';
  m_szRemotePath[0] = '\0';
  m_szLocalPathTmp[0] = '\0';
  m_szRemotePathTmp[0] = '\0';
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
    return true;
  }

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

  m_pProgress->initialize(0,0);

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

  if (DestroyWindow(m_hwndFTDialog)) m_hwndFTDialog = NULL;
}

BOOL CALLBACK 
FTDialog::FTDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  FTDialog *_this = (FTDialog *) GetWindowLong(hwnd, GWL_USERDATA);
  switch (uMsg)
  {
  case WM_INITDIALOG:
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
          _this->refreshState();
          return FALSE;
        }
        break;
        case IDC_FTREMOTEPATH:
          switch (HIWORD (wParam))
          {
          case CBN_SETFOCUS:
            _this->refreshState();
            return FALSE;
          }
          break;
      case IDC_FTCLOSE:
        _this->closeFTDialog();
        return FALSE;
      case IDC_FTLOCALUP:
        _this->refreshState();
        _this->onLocalOneUpFolder();
        return FALSE;
      case IDC_FTREMOTEUP:
        _this->refreshState();
        _this->onRemoteOneUpFolder();
        return FALSE;
      case IDC_FTLOCALRELOAD:
        _this->refreshState();
        _this->onLocalReload();
        return FALSE;
      case IDC_FTREMOTERELOAD:
       _this->refreshState();
       _this->onRemoteReload();
        return FALSE;
      case IDC_FTUPLOAD:
        _this->onUpload();
        return FALSE;
      case IDC_FTDOWNLOAD:
        _this->onDownload();
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
        _this->refreshState();
        return FALSE;
      case LVN_GETDISPINFO:
        _this->m_pLocalLV->onGetDispInfo((NMLVDISPINFO *) lParam);
        return FALSE;
      case LVN_ITEMACTIVATE:
        _this->onLocalItemActivate((LPNMITEMACTIVATE) lParam);
        return FALSE;
      case NM_RCLICK:
        return FALSE;
      }
      break;
    case IDC_FTREMOTELIST:
      switch (((LPNMHDR) lParam)->code)
      {
      case NM_CLICK:
      case NM_SETFOCUS:
      case LVN_ITEMCHANGED:
        _this->refreshState();
      case LVN_GETDISPINFO:
        _this->m_pRemoteLV->onGetDispInfo((NMLVDISPINFO *) lParam);
        return FALSE;
      case LVN_ITEMACTIVATE:
        _this->onRemoteItemActivate((LPNMITEMACTIVATE) lParam);
        return FALSE;
      case NM_RCLICK:
        return FALSE;
      }
      break;
    }
    break;
    case WM_CLOSE:
    case WM_DESTROY:
      _this->closeFTDialog();
      return FALSE;
  }
  return FALSE;
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
  m_pFileTransfer->requestFileList(m_szRemotePathTmp, FT_FLR_DEST_MAIN, 0);
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

}

void 
FTDialog::onDownload()
{

}

void
FTDialog::setIcon(int dest, int idIcon)
{
  HANDLE hIcon = LoadImage(m_hInstance, MAKEINTRESOURCE(idIcon), IMAGE_ICON, 16, 16, LR_SHARED);
  SendMessage(GetDlgItem(m_hwndFTDialog, dest), BM_SETIMAGE, (WPARAM) IMAGE_ICON, (LPARAM) hIcon);
  DestroyIcon((HICON) hIcon);
}

void
FTDialog::refreshState()
{
  if (!m_bDlgShown) return;
  
  int locPathLen = strlen(m_szLocalPath);
  int remPathLen = strlen(m_szRemotePath);
  
  if (GetFocus() == m_pLocalLV->getWndHandle()) {
    if (strlen(m_szLocalPath) != 0) {
      int nCount = ListView_GetSelectedCount(m_pLocalLV->getWndHandle());
      if (nCount <= 0) {
        setButtonsState(0, 0, -1);
      } else {
        if (remPathLen == 0) {
          setButtonsState(0, 0, -1);
        } else {
          setButtonsState(1, 0, -1);
        }
      }
    } else {
      setButtonsState(0, 0, -1);
    }
  } else {
    if (GetFocus() == m_pRemoteLV->getWndHandle()) {
      if (strlen(m_szRemotePath) != 0) {
        int nCount = ListView_GetSelectedCount(m_pRemoteLV->getWndHandle());
        if (nCount <= 0) {
          setButtonsState(0, 0, -1);
        } else {
          if (locPathLen == 0) {
            setButtonsState(0, 0, -1);
          } else {
            setButtonsState(0, 1, -1);
          }
        }
      } else {
        setButtonsState(0, 0, -1);
      }
    } else {
      setButtonsState(0, 0, -1);
    }
  }
/*
  if (m_pFileTransfer->isTransferEnable()) {
    setAllButtonsState(-1, -1, -1, -1, 1);
  } else {
    setAllButtonsState(-1, -1, -1, -1, 0);
  }
*/
}

void
FTDialog::setButtonsState(int uploadBtnState, int downloadBtnState, int cancelBtnState)
{
  switch (uploadBtnState)
  {
  case 0: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTUPLOAD), FALSE); break;
  case 1: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTUPLOAD), TRUE); break;
  default: break;
  }

  switch (downloadBtnState)
  {
  case 0: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTDOWNLOAD), FALSE); break;
  case 1: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTDOWNLOAD), TRUE); break;
  default: break;
  }

  switch (cancelBtnState)
  {
  case 0: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTCANCEL), FALSE); break;
  case 1: EnableWindow(GetDlgItem(m_hwndFTDialog, IDC_FTCANCEL), TRUE); break;
  default: break;
  }
}