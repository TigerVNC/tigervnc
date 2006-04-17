/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
 *
 * Developed by Dennis Syrovatsky.
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

// -=- FTListView.h

#ifndef __RFB_WIN32_FTLISTVIEW_H__
#define __RFB_WIN32_FTLISTVIEW_H__

#include <windows.h>
#include <commctrl.h>

#include <rfb/FileInfo.h>
#include <rfb_win32/FolderManager.h>
#include <vncviewer/resource.h>

namespace rfb {
  namespace win32{
    class FTListView
    {
    public:
      FTListView(HWND hListView);
      ~FTListView();

      bool initialize(HINSTANCE hInst);
      
      void onGetDispInfo(NMLVDISPINFO *di);
      void addItems(FileInfo *pFI);
      void deleteAllItems();
      
      char *getActivateItemName(LPNMITEMACTIVATE lpnmia);
      int getSelectedItems(FileInfo *pFI);
      
      HWND getWndHandle() { return m_hListView; };
      
    private:
      HWND m_hListView;
      FileInfo m_fileInfo;
      HIMAGELIST m_hImageList;

      bool m_bInitialized;

      void addColumn(char *iText, int iOrder, int xWidth, int alignFmt);
      void initImageList(HINSTANCE hInst);
      
    };
  }
}

#endif // __RFB_WIN32_FTLISTVIEW_H__
