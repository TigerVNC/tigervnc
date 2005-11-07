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
  m_szLocalPath[0] = '\0';
  m_szRemotePath[0] = '\0';
  m_szLocalPathTmp[0] = '\0';
  m_szRemotePathTmp[0] = '\0';

  m_pLocalLV = NULL;
  m_pRemoteLV = NULL;
  m_pProgress = NULL;

  m_hwndFTDialog = NULL;
}

FTDialog::~FTDialog()
{
  destroyFTDialog();
}

bool
FTDialog::createFTDialog()
{
  if (m_hwndFTDialog != NULL) return true;

  m_hwndFTDialog = CreateDialogParam(m_hInstance, 
                                     MAKEINTRESOURCE(IDD_FILETRANSFER_DLG),
                                     NULL, 
                                     (DLGPROC) FTDialogProc,
                                     (LONG) this);
  
  if (m_hwndFTDialog == NULL) return false;
  
  m_pLocalLV = new FTListView(GetDlgItem(m_hwndFTDialog, IDC_FTLOCALLIST));
  m_pRemoteLV = new FTListView(GetDlgItem(m_hwndFTDialog, IDC_FTREMOTELIST));
  
  m_pProgress = new FTProgress(m_hwndFTDialog);
  
  if ((m_pLocalLV == NULL) || (m_pRemoteLV == NULL) || (m_pProgress == NULL)) {
    destroyFTDialog();
    return false;
  }

  initFTDialog();
  
  if (ShowWindow(m_hwndFTDialog, SW_SHOW) == 0) {
    UpdateWindow(m_hwndFTDialog);
    m_bDlgShown = true;
    return true;
  } else {
    destroyFTDialog();
    m_bDlgShown = false;
    return false;
  } 
}

bool
FTDialog::initFTDialog()
{
  m_pLocalLV->initialize(m_hInstance);
  m_pRemoteLV->initialize(m_hInstance);

  m_pProgress->initialize(0,0);

  return true;
}

bool
FTDialog::closeFTDialog()
{
  return false;
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
}

BOOL CALLBACK 
FTDialog::FTDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	FTDialog *_this = (FTDialog *) GetWindowLong(hwnd, GWL_USERDATA);
	return FALSE;
}

void
FTDialog::reloadLocalFileList()
{
}

void
FTDialog::reloadRemoteFileList()
{
}

void 
FTDialog::onLocalItemActivate(LPNMITEMACTIVATE lpnmia)
{
}

void 
FTDialog::onRemoteItemActivate(LPNMITEMACTIVATE lpnmia)
{
}

void
FTDialog::addLocalLVItems(FileInfo *pFI)
{
}

void 
FTDialog::addRemoteLVItems(FileInfo *pFI)
{
}

void 
FTDialog::onLocalOneUpFolder(char *pPath)
{
}

void 
FTDialog::onRemoteOneUpFolder(char *pPath)
{
}
