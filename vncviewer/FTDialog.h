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

// -=- FTDialog.h

#ifndef __RFB_WIN32_FTDIALOG_H__
#define __RFB_WIN32_FTDIALOG_H__

#include <windows.h>
#include <commctrl.h>

#include <rfb/FileInfo.h>
#include <rfb_win32/Dialog.h>
#include <vncviewer/FileTransfer.h>
#include <vncviewer/resource.h>

namespace rfb {
  namespace win32 {
    class FTDialog : public Dialog
    {
    public:
      FTDialog(HINSTANCE hInst, FileTransfer *pFT);
      ~FTDialog();
      
      bool createFTDialog();
      void closeFTDialog();
      
      static BOOL CALLBACK FTDialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
      
      void addLocalLVItems(FileInfo *pFI);
      void addRemoteLVItems(FileInfo *pFI);
      
      void reloadLocalFileList();
      void reloadRemoteFileList();
      
      char *getLocalPath() { return m_szLocalPath; };
      char *getRemotePath() { return m_szRemotePath; };
      
    private:
      FileTransfer *m_pFileTransfer;
      
      HWND m_hwndFTDialog;
      HINSTANCE m_hInstance;
      
      void onLocalItemActivate(LPNMITEMACTIVATE lpnmia);
      void onRemoteItemActivate(LPNMITEMACTIVATE lpnmia);

      bool initFTDialog();
      
      void onLocalOneUpFolder(char *pPath);
      void onRemoteOneUpFolder(char *pPath);
      
      char m_szLocalPath[MAX_PATH];
      char m_szRemotePath[MAX_PATH];
      char m_szLocalPathTmp[MAX_PATH];
      char m_szRemotePathTmp[MAX_PATH];
      
      bool m_bDlgShown;
    };
  }
}

#endif // __RFB_WIN32_FTDIALOG_H__
