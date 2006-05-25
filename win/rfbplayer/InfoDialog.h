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

// -=- InfoDialog.h

#include <rfb_win32/Dialog.h>

class InfoDialog : public rfb::win32::Dialog {
public:
  InfoDialog(char *_info_message, char *_title = "Information") 
  : Dialog(GetModuleHandle(0)), info_message(_info_message), title(_title) {}
  // - Show the dialog and return true if OK was clicked,
  //   false in case of error or Cancel
  virtual bool showDialog(HWND parent = 0) {
    return Dialog::showDialog(MAKEINTRESOURCE(IDD_INFO), parent);
  }
protected:

  // Dialog methods (protected)
  virtual void initDialog() {
    SetWindowText(handle, title);
    setItemString(IDC_INFO_EDIT, info_message);
  }
  char *info_message;
  char *title;
};