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

using namespace rfb::win32;

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

bool ToolBar::create(int _tbID, HWND _parentHwnd, DWORD dwStyle) {
  parentHwnd = _parentHwnd;
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

bool ToolBar::addButton(int iBitmap, int idCommand, BYTE state, BYTE style, UINT dwData, int iString) {
  TBBUTTON tbb;
  tbb.iBitmap = iBitmap;
  tbb.idCommand = idCommand;
  tbb.fsState = state;
  tbb.fsStyle = style;
  tbb.dwData = dwData;
  tbb.iString = iString;

  int result = SendMessage(getHandle(), TB_ADDBUTTONS, 1, (LPARAM)&tbb);
  if (result) {
    SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
  }
  return (result ? true : false);
}

bool ToolBar::addNButton(int nButtons, LPTBBUTTON tbb) {
  assert(nButtons > 0);
  assert(tbb > 0);
  int result = SendMessage(getHandle(), TB_ADDBUTTONS, nButtons, (LPARAM)tbb);
  if (result) {
    SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
  }
  return (result ? true : false);
}

bool ToolBar::deleteButton(int indexButton) {
  assert(indexButton >= 0);
  int result = SendMessage(getHandle(), TB_DELETEBUTTON, indexButton, 0);
  
  if (result) {
    SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
  }
  return (result ? true : false);
}

bool ToolBar::insertButton(int indexButton, LPTBBUTTON tbb) {
  assert(indexButton >= 0);
  assert(tbb > 0);
  int result = SendMessage(getHandle(), TB_INSERTBUTTON, 
    indexButton, (LPARAM)tbb);

  if (result) {
    SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
  }
  return (result ? true : false);
}

int ToolBar::getButtonInfo(int idButton, TBBUTTONINFO *btnInfo) {
  assert(idButton >= 0);
  assert(btnInfo > 0);
  return SendMessage(getHandle(), TB_GETBUTTONINFO, idButton, (LPARAM)btnInfo);
}

int ToolBar::getButtonsHeight() {
  return HIWORD(SendMessage(getHandle(), TB_GETBUTTONSIZE, 0, 0));
}

int ToolBar::getButtonsWidth() {
  return LOWORD(SendMessage(getHandle(), TB_GETBUTTONSIZE, 0, 0));
}

bool ToolBar::setButtonInfo(int idButton, TBBUTTONINFO* btnInfo) {
  assert(idButton >= 0);
  assert(btnInfo > 0);
  int result = SendMessage(getHandle(), TB_SETBUTTONINFO, 
    idButton, (LPARAM)(LPTBBUTTONINFO)btnInfo);
  return (result ? true : false);
}

bool ToolBar::checkButton(int idButton, bool check) {
  assert(idButton >= 0);
  int result = SendMessage(getHandle(), TB_CHECKBUTTON, 
    idButton, MAKELONG(check, 0));
  return (result ? true : false);
}

bool ToolBar::enableButton(int idButton, bool enable) {
  assert(idButton >= 0);
  int result = SendMessage(getHandle(), TB_ENABLEBUTTON, 
    idButton, MAKELONG(enable, 0));
  return (result ? true : false);
}

bool ToolBar::pressButton(int idButton, bool press) {
  assert(idButton >= 0);
  int result = SendMessage(getHandle(), TB_PRESSBUTTON, 
    idButton, MAKELONG(press, 0));
  return (result ? true : false);
}

bool ToolBar::getButtonRect(int nIndex, LPRECT buttonRect) {
  int result = SendMessage(getHandle(), TB_GETITEMRECT, 
    nIndex, (LPARAM)buttonRect);
  return (result ? true : false);
}

bool ToolBar::setButtonSize(int width, int height) {
  assert(width > 0);
  assert(height > 0);
  int result = SendMessage(getHandle(), TB_SETBUTTONSIZE, 
    0, MAKELONG(width, height));
  if (result) {
    SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
    return true;
  }
  return false; 
}

void ToolBar::autoSize() {
  DWORD style = SendMessage(getHandle(), TB_GETSTYLE,  0, 0);
  if (style & CCS_NORESIZE) {
    RECT r, btnRect;
    GetClientRect(parentHwnd, &r);
    getButtonRect(0, &btnRect);
    int height = getButtonsHeight() + btnRect.top * 2 + 2;
    SetWindowPos(getHandle(), HWND_TOP, 0, 0, r.right - r.left, height, 
      SWP_NOMOVE);
  } else {
    SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
  }
}

int ToolBar::getHeight() {
  RECT r;
  GetWindowRect(getHandle(), &r);
  return r.bottom - r.top;
}

int ToolBar::getTotalWidth() {
  SIZE size;
  SendMessage(getHandle(), TB_GETMAXSIZE, 0, (LPARAM)(LPSIZE)&size);
  return size.cx;
}

void ToolBar::show() {
  ShowWindow(getHandle(), SW_SHOW);
}

void ToolBar::hide() {
  ShowWindow(getHandle(), SW_HIDE);
}

bool ToolBar::isVisible() {
  DWORD style = GetWindowLong(getHandle(), GWL_STYLE);
  return (bool)(style & WS_VISIBLE);
}
