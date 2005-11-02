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

FTDialog::FTDialog(HINSTANCE hInst, FileTransfer *pFT) : Dialog(hInst)
{
  m_pFileTransfer = pFT;
  m_hInstance = hInst;
  m_bDlgShown = false;
  m_szLocalPath[0] = '\0';
  m_szRemotePath[0] = '\0';
  m_szLocalPathTmp[0] = '\0';
  m_szRemotePathTmp[0] = '\0';
}

FTDialog::~FTDialog()
{
  closeFTDialog();
}

bool
FTDialog::createFTDialog()
{
	return false;
}

bool
FTDialog::initFTDialog()
{
  return false;
}

void
FTDialog::closeFTDialog()
{
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
