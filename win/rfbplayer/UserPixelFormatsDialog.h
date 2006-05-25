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

#include <rfbplayer/EditPixelFormatDialog.h>

#define UPF_REGISTRY_PATH "Software\\TightVnc\\RfbPlayer\\UserDefinedPF"

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
        0, (LPARAM)(LPCTSTR)(((*supportedPF)[i])->format_name));
    }
    SendMessage(pfList, LB_SETCURSEL, 0, 0);
  }
  virtual bool onCommand(int item, int cmd) { 
    switch (item) {
    case IDC_ADD_BUTTON:
      { 
        char format_name[MAX_STR_LEN] = "";
        PixelFormat pf(32, 24, 0, 1, 0, 0, 0, 0, 0, 0);
        EditPixelFormatDialog edit(supportedPF, format_name, &pf);
        if (edit.showDialog(handle)) {
          supportedPF->add(format_name, pf);
          SendMessage(pfList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)format_name);
        }
      }
      break;
    case IDC_REMOVE_BUTTON:
      {
        int index = SendMessage(pfList, LB_GETCURSEL, 0, 0);
        if (index == LB_ERR) {
          MessageBox(handle, "You must select the pixel format for remove.", 
                     "RfbPlayer", MB_OK | MB_ICONWARNING);
          return false;
        } else {
          supportedPF->remove(supportedPF->getDefaultPFCount() + index);
          SendMessage(pfList, LB_DELETESTRING, index, 0);
        }
      }
      break;
    case IDC_PF_LIST:
      if (cmd != LBN_DBLCLK) break;
    case IDC_EDIT_BUTTON:
      {
        int index = SendMessage(pfList, LB_GETCURSEL, 0, 0);
        if (index == LB_ERR) {
          MessageBox(handle, "You must select the pixel format for remove.", 
                     "RfbPlayer", MB_OK | MB_ICONWARNING);
          return false;
        }
        PixelFormat *pf = 
          &(supportedPF->operator[](index + supportedPF->getDefaultPFCount())->PF);
        char *format_name = 
          (supportedPF)->operator[](index + supportedPF->getDefaultPFCount())->format_name;
        EditPixelFormatDialog edit(supportedPF, format_name, pf);
        if (edit.showDialog(handle)) {
          SendMessage(pfList, LB_DELETESTRING, index, 0);
          SendMessage(pfList, LB_INSERTSTRING, index, (LPARAM)(LPCTSTR)format_name);
          SendMessage(pfList, LB_SETCURSEL, index, 0);
        }
      }
      break;
    default:
      break;
    }
    return false;
  }
  virtual bool onOk() {
    supportedPF->writeUserDefinedPF(HKEY_CURRENT_USER, UPF_REGISTRY_PATH);
    return true;
  }

  HWND pfList;
  PixelFormatList *supportedPF;
};