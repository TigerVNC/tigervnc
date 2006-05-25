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

// -=- GotoPosDialog.h

#include <rfb_win32/Dialog.h>

class GotoPosDialog : public rfb::win32::Dialog {
public:
  GotoPosDialog() : Dialog(GetModuleHandle(0)) {}
  // - Show the dialog and return true if OK was clicked,
  //   false in case of error or Cancel
  virtual bool showDialog(HWND parent) {
    return Dialog::showDialog(MAKEINTRESOURCE(IDD_GOTO), parent);
  }
  const long getPos() const {return pos;}
protected:

  // Dialog methods (protected)
  virtual void initDialog() {
    setItemString(IDC_GOTO_EDIT, rfb::TStr("0"));
  }
  virtual bool onOk() {
    pos = atol(rfb::CStr(getItemString(IDC_GOTO_EDIT)));
    return true;
  }

  long pos;
};