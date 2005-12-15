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

// -=- FTBrpwseDlg.cxx

#include <vncviewer/FTBrowseDlg.h>

using namespace rfb;
using namespace rfb::win32;

FTBrowseDlg::FTBrowseDlg(FTDialog *pFTDlg)
{

}

FTBrowseDlg::~FTBrowseDlg()
{

}

BOOL CALLBACK 
FTBrowseDlg::FTBrowseDlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  FTBrowseDlg *_this = (FTBrowseDlg *) GetWindowLong(hwnd, GWL_USERDATA);
  switch (uMsg)
  {
  case WM_INITDIALOG:
    {
      SetWindowLong(hwnd, GWL_USERDATA, lParam);
      return FALSE;
    }
    break;
  case WM_COMMAND:
    {
      switch (LOWORD(wParam))
      {
      case IDOK:
        return FALSE;
      case IDCANCEL:
        return FALSE;
      }
    }
    break;
  case WM_NOTIFY:
    switch (LOWORD(wParam))
    {
    case IDC_FTBROWSETREE:
      switch (((LPNMHDR) lParam)->code)
      {
      case TVN_SELCHANGED:
        return FALSE;
      case TVN_ITEMEXPANDING:
        return FALSE;
      }
      break;
    }
    break;
    case WM_CLOSE:
    case WM_DESTROY:
      return FALSE;
    }
    return 0;
}
