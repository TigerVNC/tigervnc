/* Copyright (C) 2004 TightVNC Team.  All Rights Reserved.
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
 */

// -=- ToolBar control class.

#include "ToolBar.h"

ToolBar::ToolBar() : hwndToolBar(0), tbID(-1) {
  INITCOMMONCONTROLSEX icex;

  // Ensure that the common control DLL is loaded
  icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
  icex.dwICC  = ICC_BAR_CLASSES;
  InitCommonControlsEx(&icex);
}

ToolBar::~ToolBar() {
  DestroyWindow(getHandle());
}

bool ToolBar::create(int _tbID, HWND parentHwnd, DWORD dwStyle) {
  dwStyle |= WS_CHILD;

  // Create the ToolBar window
  hwndToolBar = CreateWindowEx(0, TOOLBARCLASSNAME, 0, dwStyle, 
    0, 0, 25, 25, parentHwnd, (HMENU)_tbID, GetModuleHandle(0), 0);

  if (hwndToolBar) {
    tbID = _tbID;

    // It's required for backward compatibility
    SendMessage(hwndToolBar, TB_BUTTONSTRUCTSIZE, (WPARAM)sizeof(TBBUTTON), 0);
  }
  return (hwndToolBar ? true : false);
};

int ToolBar::addBitmap(int nButtons, UINT bitmapID) {
  assert(nButtons > 0);
  TBADDBITMAP resBitmap;
  resBitmap.hInst = GetModuleHandle(0);
  resBitmap.nID = bitmapID;
  return SendMessage(getHandle(), TB_ADDBITMAP, nButtons, (LPARAM)&resBitmap);
}

int ToolBar::addSystemBitmap(UINT stdBitmapID) {
  TBADDBITMAP resBitmap;
  resBitmap.hInst = HINST_COMMCTRL;
  resBitmap.nID = stdBitmapID;
  return SendMessage(getHandle(), TB_ADDBITMAP, 0, (LPARAM)&resBitmap);
}

bool ToolBar::setBitmapSize(int width, int height) {
  int result = SendMessage(getHandle(), TB_SETBITMAPSIZE, 
    0, MAKELONG(width, height));
  return (result ? true : false);
}
