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

// -=- FTBrowseDlg.h

#ifndef __RFB_WIN32_FTBROWSEDLG_H__
#define __RFB_WIN32_FTBROWSEDLG_H__

#include <windows.h>
#include <commctrl.h>

#include <rfb/FileInfo.h>
#include <vncviewer/FTDialog.h>
#include <vncviewer/resource.h>

namespace rfb {
  namespace win32 {
    class FTDialog;

    class FTBrowseDlg
    {
    public:
      FTBrowseDlg(FTDialog *pFTDlg);
      ~FTBrowseDlg();

      static BOOL CALLBACK FTBrowseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

      bool create();
      void destroy();

      void addItems(FileInfo *pFI);
      char *getPath();

      void deleteChildItems();

    private:
      HWND m_hwndDlg;
      HWND m_hwndTree;
      FTDialog *m_pFTDlg;
      HTREEITEM m_hParentItem;

      char m_szPath[FT_FILENAME_SIZE];

      char *pathInvert(char *pPath);
      char *getTVPath(HTREEITEM hTItem);
    };
  }
}

#endif // __RFB_WIN32_FTBROWSEDLG_H__
