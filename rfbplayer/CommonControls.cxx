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

#include "CommonControls.h"

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

bool ToolBar::create(int tbID, HWND parentHwnd, DWORD dwStyle) {
  return createEx(tbID, parentHwnd, 0, dwStyle);
};

bool ToolBar::createEx(int _tbID, HWND parentHwnd, DWORD dwExStyle, DWORD dwStyle) {
  dwStyle |= WS_CHILD;

  // Create ToolBar window
  hwndToolBar = CreateWindowEx(dwExStyle, TOOLBARCLASSNAME, 0, dwStyle, 
    0, 0, 60, 30, parentHwnd, (HMENU)_tbID, GetModuleHandle(0), 0);

  if (hwndToolBar) {
    tbID = _tbID;

    // It's required for backward compatibility
    setButtonStructSize(sizeof(TBBUTTON));
  }
  return (hwndToolBar ? true : false);
};

DWORD ToolBar::getStyle() {
  return SendMessage(getHandle(), TB_GETSTYLE, 0, 0);
}

void ToolBar::setStyle(DWORD dwStyle) {
  SendMessage(getHandle(), TB_SETSTYLE, 0, dwStyle);
}

void ToolBar::setButtonStructSize(int nBytes) {
  SendMessage(getHandle(), TB_BUTTONSTRUCTSIZE, (WPARAM)nBytes, 0);
}

DWORD ToolBar::setExtendedStyle(DWORD tbExStyle) {
  return SendMessage(getHandle(), TB_SETEXTENDEDSTYLE, 0, tbExStyle);
}

void ToolBar::autoSize() {
  SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
}

int ToolBar::addBitmap(int nButtons, UINT resID) {
  TBADDBITMAP resBitmap;
  if (nButtons > 0) {
    // Load bitmap from resources
    resBitmap.hInst = GetModuleHandle(0);
  } else if (nButtons == -1) {
    // Load system-defined button bitmaps
    resBitmap.hInst = HINST_COMMCTRL;
  } else {
    return -1;
  }
  resBitmap.nID = resID;
  return SendMessage(getHandle(), TB_ADDBITMAP, nButtons, (LPARAM)&resBitmap);
}

int ToolBar::addString(UINT strID) {
  return SendMessage(getHandle(), TB_ADDSTRING, 
    (WPARAM)GetModuleHandle(0), MAKELONG(strID, 0));
}

int ToolBar::addStrings(LPCTSTR lpStrings) {
  return SendMessage(getHandle(), TB_ADDSTRING, 0, MAKELONG(lpStrings, 0));
}

bool ToolBar::addNButton(int nButtons, LPTBBUTTON tbb) {
  int result = SendMessage(getHandle(), TB_ADDBUTTONS, nButtons, (LPARAM)tbb);
  if (result) {
    for (int i = 0; i < nButtons; i++) tbButtons.push_back(tbb[i]);
    SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
  }
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
    tbButtons.push_back(tbb);
    SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
  }
  return (result ? true : false);
}

UINT ToolBar::commandToIndex(int idButton) {
  return SendMessage(getHandle(), TB_COMMANDTOINDEX, idButton, 0);
}

void ToolBar::customize() {
  SendMessage(getHandle(), TB_CUSTOMIZE, 0, 0);
}

int ToolBar::getButtonCount() {
  return (byte)SendMessage(getHandle(), TB_BUTTONCOUNT, 0, 0);
}

int ToolBar::getHeight() {
  RECT tbRect;
	GetWindowRect(getHandle(), &tbRect);
  return (tbRect.bottom - tbRect.top);
}

int ToolBar::getWidth() {
  RECT tbRect;
	GetWindowRect(getHandle(), &tbRect);
  return (tbRect.right - tbRect.left);
}

void ToolBar::setSize(int width, int height) {
  RECT tbRect;
  GetClientRect(getHandle(), &tbRect);

	SetWindowPos(getHandle(), HWND_TOP, tbRect.left, tbRect.top,
		tbRect.left + width, tbRect.top + height, SWP_NOMOVE);
}

int ToolBar::getButtonWidth() {
  return LOWORD(SendMessage(getHandle(), TB_GETBUTTONSIZE, 0, 0));
}

int ToolBar::getButtonHeight() {
  return HIWORD(SendMessage(getHandle(), TB_GETBUTTONSIZE, 0, 0));
}

bool ToolBar::setButtonSize(int width, int height) {
  int result = SendMessage(getHandle(), TB_SETBUTTONSIZE, 0, MAKELONG(width, height));
  if (result) {
    SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
    return true;
  }
  return false; 
}

bool ToolBar::insertButton(int indexButton, TBBUTTON tbb) {
  int result = SendMessage(getHandle(), TB_INSERTBUTTON, 
    indexButton, (LPARAM)&tbb);

  if (result) {
    list <TBBUTTON>::iterator iter = tbButtons.begin();
    for (int i = 0; i < indexButton; i++) iter++;
    tbButtons.insert(iter, tbb);
    SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
  }
  return (result ? true : false);
}

bool ToolBar::deleteButton(int indexButton) {
  int result = SendMessage(getHandle(), TB_DELETEBUTTON, indexButton, 0);
  if (result) {
    list <TBBUTTON>::iterator iter = tbButtons.begin();
    for (int i = 0; i < indexButton; i++) iter++;
    tbButtons.erase(iter);
    SendMessage(getHandle(), TB_AUTOSIZE, 0, 0);
  }
  return (result ? true : false);
}

int ToolBar::getButtonState(int idButton) {
  return SendMessage(getHandle(), TB_GETSTATE, idButton, 0);
}

bool ToolBar::setButtonState(int idButton, int btnState) {
  int result = SendMessage(getHandle(), TB_SETSTATE, 
    idButton, MAKELONG(btnState, 0));
  return (result ? true : false);
}

bool ToolBar::getButton(int nIndex, LPTBBUTTON lpButton) {
  int result = SendMessage(getHandle(), TB_GETBUTTON, 
    nIndex, (LPARAM)lpButton);
  return (result ? true : false);
}

int ToolBar::getButtonInfo(int idButton, TBBUTTONINFO *btnInfo) {
  return SendMessage(getHandle(), TB_GETBUTTONINFO, idButton, (LPARAM)btnInfo);
}

bool ToolBar::enableButton(int idButton, bool enable) {
  int result = SendMessage(getHandle(), TB_ENABLEBUTTON, 
    idButton, MAKELONG(enable, 0));
  return (result ? true : false);
}

bool ToolBar::checkButton(int idButton, bool check) {
  int result = SendMessage(getHandle(), TB_CHECKBUTTON, 
    idButton, MAKELONG(check, 0));
  return (result ? true : false);
}

bool ToolBar::hideButton(int idButton, bool hide) {
  int result = SendMessage(getHandle(), TB_HIDEBUTTON, idButton, MAKELONG(hide, 0));
  return (result ? true : false);
}

bool ToolBar::indeterminateButton(int idButton, bool indeterminate) {
  int result = SendMessage(getHandle(), TB_INDETERMINATE, idButton, 
    MAKELONG(indeterminate, 0));
  return (result ? true : false);
}

void ToolBar::loadImages(int bitmapID, HINSTANCE hinst) {
  if (hinst == 0) hinst = HINST_COMMCTRL;
  SendMessage(getHandle(), TB_LOADIMAGES, bitmapID, (LPARAM)hinst);
}

bool ToolBar::markButton(int idButton, bool fHighlight) {
  int result = SendMessage(getHandle(), TB_MARKBUTTON, 
    idButton, MAKELONG(fHighlight, 0));
  return (result ? true : false);
}

bool ToolBar::pressButton(int idButton, bool press) {
  int result = SendMessage(getHandle(), TB_PRESSBUTTON, 
    idButton, MAKELONG(press, 0));
  return (result ? true : false);
}

void ToolBar::restoreState(HKEY hKeyRoot, LPCTSTR lpszSubKey, 
                           LPCTSTR lpszValueName) {
  TBSAVEPARAMS tbSaveParams;
  tbSaveParams.hkr = hKeyRoot;
  tbSaveParams.pszSubKey = lpszSubKey;
  tbSaveParams.pszValueName = lpszValueName;
  SendMessage(getHandle(), TB_SAVERESTORE, 0, (LPARAM)&tbSaveParams);
}

void ToolBar::saveState(HKEY hKeyRoot, LPCTSTR lpszSubKey, 
                           LPCTSTR lpszValueName) {
  TBSAVEPARAMS tbSaveParams;
  tbSaveParams.hkr = hKeyRoot;
  tbSaveParams.pszSubKey = lpszSubKey;
  tbSaveParams.pszValueName = lpszValueName;
  SendMessage(getHandle(), TB_SAVERESTORE, 1, (LPARAM)&tbSaveParams);
}

DWORD ToolBar::setDrawTextFlags(DWORD dwMask, DWORD dwDTFlags) {
  return SendMessage(getHandle(), TB_SETDRAWTEXTFLAGS, dwMask, dwDTFlags);
};

bool ToolBar::setButtonInfo(int idButton, TBBUTTONINFO* ptbbi) {
  int result = SendMessage(getHandle(), TB_SETBUTTONINFO, 
    idButton, (LPARAM)(LPTBBUTTONINFO)ptbbi);
  return (result ? true : false);
}

bool ToolBar::getAnchorHighlight() {
  int result = SendMessage(getHandle(), TB_GETANCHORHIGHLIGHT, 0, 0);
  return (result ? true : false);
}

int ToolBar::getBitmap(int idButton) {
  return SendMessage(getHandle(), TB_GETBITMAP, idButton, 0);
}

DWORD ToolBar::getBitmapFlags() {
  return SendMessage(getHandle(), TB_GETBITMAPFLAGS, 0, 0);
}

HIMAGELIST ToolBar::getDisabledImageList() {
  return (HIMAGELIST)SendMessage(getHandle(), TB_GETDISABLEDIMAGELIST, 0,  0);
}

HRESULT ToolBar::getDropTarget(IDropTarget** ppDropTarget) {
  return SendMessage(getHandle(), TB_GETOBJECT, 
    (WPARAM)&IID_IDropTarget, (LPARAM)ppDropTarget);
}

DWORD ToolBar::getExtendedStyle() {
  return SendMessage(getHandle(), TB_GETEXTENDEDSTYLE, 0, 0);
}

HIMAGELIST ToolBar::getHotImageList() {
  return (HIMAGELIST)SendMessage(getHandle(), TB_GETHOTIMAGELIST, 0, 0);
}

int ToolBar::getHotItem() {
  return SendMessage(getHandle(), TB_GETHOTITEM, 0, 0);
}

HIMAGELIST ToolBar::getImageList() {
  return (HIMAGELIST)SendMessage(getHandle(), TB_GETIMAGELIST, 0, 0);
}

void ToolBar::getInsertMark(TBINSERTMARK* ptbim) {
  SendMessage(getHandle(), TB_GETINSERTMARK, 0, (LPARAM)ptbim);
}

COLORREF ToolBar::getInsertMarkColor() {
  return (COLORREF)SendMessage(getHandle(), TB_GETINSERTMARKCOLOR, 0, 0);
}

bool ToolBar::getItemRect(int nIndex, LPRECT lpRect) {
  int result = SendMessage(getHandle(), TB_GETITEMRECT, nIndex, (LPARAM)lpRect);
  return (result ? true : false);
}

bool ToolBar::getMaxSize(LPSIZE lpSize) {
  int result = SendMessage(getHandle(), TB_GETMAXSIZE, 0, (LPARAM)lpSize);
  return (result ? true : false);
}

int ToolBar::getMaxTextRows() {
  return SendMessage(getHandle(), TB_GETTEXTROWS, 0, 0);
}

bool ToolBar::getRect(int idButton, LPRECT lpRect) {
  int result = SendMessage(getHandle(), TB_GETRECT, idButton, (LPARAM)lpRect);
  return (result ? true : false);
}

int ToolBar::getRows() {
  return SendMessage(getHandle(), TB_GETROWS, 0, 0);
}

HWND ToolBar::getToolTips() {
  return (HWND)SendMessage(getHandle(), TB_GETTOOLTIPS, 0, 0);
}

int ToolBar::hitTest(LPPOINT pptHitTest) {
  return SendMessage(getHandle(), TB_HITTEST, 0, (LPARAM)pptHitTest);
}

bool ToolBar::insertMarkHitTest(LPPOINT ppt, LPTBINSERTMARK ptbim) {
  int result = SendMessage(getHandle(), TB_INSERTMARKHITTEST, (WPARAM)ppt, 0);
  return (result ? true : false);
}

bool ToolBar::isButtonChecked(int idButton) {
  int result = SendMessage(getHandle(), TB_ISBUTTONCHECKED, 
    (WPARAM)idButton, 0);
  return (result ? true : false);
}

bool ToolBar::isButtonEnabled(int idButton) {
  int result = SendMessage(getHandle(), TB_ISBUTTONENABLED, 
    (WPARAM)idButton, 0);
  return (result ? true : false);
}

bool ToolBar::isButtonHidden(int idButton) {
  int result = SendMessage(getHandle(), TB_ISBUTTONHIDDEN, 
    (WPARAM)idButton, 0);
  return (result ? true : false);
}

bool ToolBar::isButtonHighlighted(int idButton) {
  int result = SendMessage(getHandle(), TB_ISBUTTONHIGHLIGHTED, 
    (WPARAM)idButton, 0);
  return (result ? true : false);
}

bool ToolBar::isButtonIndeterminate(int idButton) {
  int result = SendMessage(getHandle(), TB_ISBUTTONINDETERMINATE, 
    (WPARAM)idButton, 0);
  return (result ? true : false);
}

bool ToolBar::isButtonPressed(int idButton) {
  int result = SendMessage(getHandle(), TB_ISBUTTONPRESSED,
    (WPARAM)idButton, 0);
  return (result ? true : false);
}

bool ToolBar::mapAccelerator(TCHAR chAccel, UINT* pIDBtn) {
  int result = SendMessage(getHandle(), TB_MAPACCELERATOR,
    (WPARAM)chAccel, (LPARAM)pIDBtn);
  return (result ? true : false);
}

bool ToolBar::moveButton(UINT oldPos, UINT newPos) {
  int result = SendMessage(getHandle(), TB_MOVEBUTTON,
    (WPARAM)oldPos, (LPARAM)newPos);
  return (result ? true : false);
}

bool ToolBar::setAnchorHighlight(bool anchor) {
  int result = SendMessage(getHandle(), 
    TB_SETANCHORHIGHLIGHT, (WPARAM)anchor, 0);
  return (result ? true : false);
}

bool ToolBar::setBitmapSize(int width, int height) {
  int result = SendMessage(getHandle(), TB_SETBITMAPSIZE, 
    0, MAKELONG(width, height));
  return (result ? true : false);
}

bool ToolBar::setButtonWidth(int cxMin, int cxMax) {
  int result = SendMessage((HWND) getHandle(), 
    TB_SETBUTTONWIDTH, 0, MAKELONG(cxMin, cxMax));
  return (result ? true : false);
}

bool ToolBar::setButtonId(int nIndex, UINT idButton) {
  int result = SendMessage(getHandle(), TB_SETCMDID, nIndex, idButton);
  return (result ? true : false);
}

HIMAGELIST ToolBar::setDisabledImageList(HIMAGELIST pImageList) {
  return (HIMAGELIST)SendMessage(getHandle(), 
    TB_SETDISABLEDIMAGELIST, 0, (LPARAM)pImageList);
}

HIMAGELIST ToolBar::setHotImageList(HIMAGELIST pImageList) {
  return (HIMAGELIST)SendMessage(getHandle(), 
    TB_SETHOTIMAGELIST, 0, (LPARAM)pImageList);
}

int ToolBar::setHotItem(int nIndex) {
  return SendMessage(getHandle(), TB_SETHOTITEM, nIndex, 0);
}

HIMAGELIST ToolBar::setImageList(HIMAGELIST pImageList) {
  return (HIMAGELIST)SendMessage(getHandle(), 
    TB_SETIMAGELIST, 0, (LPARAM)pImageList);
}

bool ToolBar::setIndent(int nPixel) {
  int result = SendMessage(getHandle(), TB_SETINDENT, nPixel, 0);
  return (result ? true : false);
}

void ToolBar::setInsertMark(TBINSERTMARK* ptbim) {
  SendMessage(getHandle(), TB_SETINSERTMARK, 0, (LPARAM)ptbim);
}

COLORREF ToolBar::setInsertMarkColor(COLORREF clrNew) {
  return (COLORREF)SendMessage(getHandle(), 
    TB_SETINSERTMARKCOLOR, 0, (LPARAM)clrNew);
}

bool ToolBar::setMaxTextRows(int iMaxRows) {
  int result = SendMessage(getHandle(), TB_SETMAXTEXTROWS, iMaxRows, 0);
  return (result ? true : false);
}

HWND ToolBar::setOwner(HWND owner) {
  return (HWND)SendMessage(getHandle(), TB_SETPARENT, (WPARAM)owner, 0);
}

void ToolBar::setRows(int nRows, bool fLarger, LPRECT lpRect) {
  SendMessage(getHandle(), TB_SETROWS, 
    MAKEWPARAM(nRows, fLarger), (LPARAM)lpRect);
}

void ToolBar::setToolTips(HWND hToolTip) {
  SendMessage(getHandle(), TB_SETTOOLTIPS, (WPARAM)hToolTip, 0);
}

LRESULT ToolBar::processNotify(HWND hwnd, WPARAM wParam, LPARAM lParam) {
  LPNMHDR pnmh = (NMHDR*)lParam;
  int tbId = (int)wParam ;

  switch (pnmh->code) {

    // Allow toolbar to be customized
  case TBN_QUERYDELETE:
  case TBN_QUERYINSERT:
    return TRUE; // We always say "yes"

    // Provide details of avalaible toolbar's buttons
  case TBN_GETBUTTONINFO:
    {
      LPNMTOOLBAR lptbn = (LPNMTOOLBAR)lParam;
      list <TBBUTTON>::iterator tbbi = tbButtons.begin();
      if (lptbn->iItem >= tbButtons.size()) return FALSE;
      for (int i = 0; i < lptbn->iItem; i++) tbbi++;
      lptbn->tbButton.iBitmap   = tbbi->iBitmap;
      lptbn->tbButton.idCommand = tbbi->idCommand;
      lptbn->tbButton.fsState   = tbbi->fsState;
      lptbn->tbButton.fsStyle   = tbbi->fsStyle;
      lptbn->tbButton.dwData    = tbbi->dwData;
      lptbn->tbButton.iString   = tbbi->iString;
      return TRUE;
    }
    break;
  }
  return FALSE;
}
