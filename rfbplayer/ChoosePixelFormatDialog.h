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
  ChoosePixelFormatDialog(long _pfIndex, PixelFormatList *_supportedPF) 
  : Dialog(GetModuleHandle(0)), supportedPF(_supportedPF), pfIndex(_pfIndex), 
    combo(0) {}
  // - Show the dialog and return true if OK was clicked,
  //   false in case of error or Cancel
  virtual bool showDialog(HWND parent) {
    return Dialog::showDialog(MAKEINTRESOURCE(IDD_PIXELFORMAT), parent);
  }
  const long getPF() const {return pfIndex;}
protected:

  // Dialog methods (protected)
  virtual void initDialog() {
    combo = GetDlgItem(handle, IDC_PIXELFORMAT);
    SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("Auto"));
    for (int i = 0; i < supportedPF->count(); i++) {
      SendMessage(combo, CB_ADDSTRING, 
        0, (LPARAM)(LPCTSTR)(((*supportedPF)[i]).format_name));
    }
    SendMessage(combo, CB_SETCURSEL, pfIndex + 1, 0);
  }
  virtual bool onOk() {
    pfIndex = SendMessage(combo, CB_GETCURSEL, 0, 0) - 1;
    return true;
  }
  virtual bool onCancel() {
    return false;
  }
  long pfIndex;
  PixelFormatList *supportedPF;
  HWND combo;
};