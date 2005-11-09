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

  m_hwndLocalPath = GetDlgItem(m_hwndLocalPath, IDC_FTLOCALPATH);
  m_hwndRemotePath = GetDlgItem(m_hwndRemotePath, IDC_FTREMOTEPATH);

  return true;
}

bool
FTDialog::closeFTDialog()
{
  ShowWindow(m_hwndFTDialog, SW_HIDE);
  m_bDlgShown = false;
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
			case IDC_FTCLOSE:
				_this->closeFTDialog();
				return FALSE;
			case IDC_FTLOCALUP:
				_this->onLocalOneUpFolder();
				return FALSE;
			case IDC_FTREMOTEUP:
				_this->onRemoteOneUpFolder();
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
				case LVN_GETDISPINFO:
					_this->m_pLocalLV->onGetDispInfo((NMLVDISPINFO *) lParam);
					return FALSE;
			}
		break;
		case IDC_FTREMOTELIST:
			switch (((LPNMHDR) lParam)->code)
			{
				case LVN_GETDISPINFO:
					_this->m_pRemoteLV->onGetDispInfo((NMLVDISPINFO *) lParam);
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
}

void 
FTDialog::onRemoteItemActivate(LPNMITEMACTIVATE lpnmia)
{
}

void
FTDialog::addLocalLVItems(char *pPath, FileInfo *pFI)
{
  pFI->sort();
  SetWindowText(m_hwndLocalPath, pPath);
  m_pLocalLV->deleteAllItems();
  m_pLocalLV->addItems(pFI);
}

void 
FTDialog::addRemoteLVItems(char *pPath, FileInfo *pFI)
{
  pFI->sort();
  SetWindowText(m_hwndRemotePath, pPath);
  m_pRemoteLV->deleteAllItems();
  m_pRemoteLV->addItems(pFI);
}

void 
FTDialog::onLocalOneUpFolder()
{
}

void 
FTDialog::onRemoteOneUpFolder()
{
}
