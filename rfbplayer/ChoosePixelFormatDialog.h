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

// -=- ChoosePixelFormatDialog.h

#include <rfb_win32/Dialog.h>

class ChoosePixelFormatDialog : public rfb::win32::Dialog {
public:
  ChoosePixelFormatDialog(long _pf) : Dialog(GetModuleHandle(0)), 
    pf(_pf), combo(0) {}
  // - Show the dialog and return true if OK was clicked,
  //   false in case of error or Cancel
  virtual bool showDialog(HWND parent) {
    return Dialog::showDialog(MAKEINTRESOURCE(IDD_PIXELFORMAT), parent);
  }
  const long getPF() const {return pf;}
protected:

  // Dialog methods (protected)
  virtual void initDialog() {
    combo = GetDlgItem(handle, IDC_PIXELFORMAT);
    SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("Auto"));
    SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("8 bit depth (RGB332)"));
    SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("16 bit depth (RGB655)"));
    SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("24 bit depth (RGB888)"));
    SendMessage(combo, CB_SETCURSEL, pf, 0);
  }
  virtual bool onOk() {
    pf = SendMessage(combo, CB_GETCURSEL, 0, 0);
    return true;
  }
  virtual bool onCancel() {
    pf = -1;
    return false;
  }

  long pf;
  HWND combo;
};