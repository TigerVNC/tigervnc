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

// -=- UserPixelFormatsDialog.h

#include <rfb_win32/Dialog.h>

class UserPixelFormatsDialog : public rfb::win32::Dialog {
public:
  UserPixelFormatsDialog(PixelFormatList *_supportedPF) 
  : Dialog(GetModuleHandle(0)), supportedPF(_supportedPF), pfList(0) {}
  // - Show the dialog and return true if OK was clicked,
  //   false in case of error or Cancel
  virtual bool showDialog(HWND parent) {
    return Dialog::showDialog(MAKEINTRESOURCE(IDD_USERPF_LIST), parent);
  }

protected:
  // Dialog methods (protected)
  virtual void initDialog() {
    pfList = GetDlgItem(handle, IDC_PF_LIST);
    for (int i = supportedPF->getDefaultPFCount(); i < supportedPF->count(); i++) {
      SendMessage(pfList, LB_ADDSTRING, 
        0, (LPARAM)(LPCTSTR)(((*supportedPF)[i]).format_name));
    }
    SendMessage(pfList, LB_SETCURSEL, 0, 0);
  }
  virtual bool onCommand(int item, int cmd) { 
    switch (item) {
    case IDC_ADD_BUTTON:
      break;
    case IDC_REMOVE_BUTTON:
      break;
    case IDC_EDIT_BUTTON:
      break;
    default:
      break;
    }
    return false;
  }

  HWND pfList;
  PixelFormatList *supportedPF;
};