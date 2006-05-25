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

// -=- EditPixelFormatDialog.h

#include <rfb_win32/Dialog.h>

#define MAX_STR_LEN 256

class EditPixelFormatDialog : public rfb::win32::Dialog {
public:
  EditPixelFormatDialog(PixelFormatList *_supportedPF, char *_format_name,
                        PixelFormat *_pf) 
  : Dialog(GetModuleHandle(0)), format_name(_format_name), 
  supportedPF(_supportedPF), pf(_pf) {}
  // - Show the dialog and return true if OK was clicked,
  //   false in case of error or Cancel
  virtual bool showDialog(HWND parent) {
    return Dialog::showDialog(MAKEINTRESOURCE(IDD_UPF_EDIT), parent);
  }

protected:
  // Dialog methods (protected)
  virtual void initDialog() {
    HWND bppCombo = GetDlgItem(handle, IDC_BPP_COMBO);
    SendMessage(bppCombo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("8"));
    SendMessage(bppCombo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("16"));
    SendMessage(bppCombo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("32"));
    SendMessage(bppCombo, CB_SETCURSEL, min(2, (pf->bpp - 8) / 8), 0);

    HWND bigendianCombo = GetDlgItem(handle, IDC_BIGENDIAN_COMBO);
    SendMessage(bigendianCombo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("No"));
    SendMessage(bigendianCombo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("Yes"));
    SendMessage(bigendianCombo, CB_SETCURSEL, pf->bigEndian, 0);

    setItemString(IDC_NAME_EDIT, format_name);
    setItemInt(IDC_DEPTH_EDIT, pf->depth);
    setItemInt(IDC_REDMAX_EDIT, pf->redMax);
    setItemInt(IDC_GREENMAX_EDIT, pf->greenMax);
    setItemInt(IDC_BLUEMAX_EDIT, pf->blueMax);
    setItemInt(IDC_REDSHIFT_EDIT, pf->redShift);
    setItemInt(IDC_GREENSHIFT_EDIT, pf->greenShift);
    setItemInt(IDC_BLUESHIFT_EDIT, pf->blueShift);
  }
  virtual bool onOk() {
    // Check for the errors
    char err_msg[256] = "\0";
    if (((getItemString(IDC_NAME_EDIT))[0] == '\0') || 
        ((getItemString(IDC_DEPTH_EDIT))[0] == '\0') || 
        ((getItemString(IDC_REDMAX_EDIT))[0] == '\0') || 
        ((getItemString(IDC_GREENMAX_EDIT))[0] == '\0') ||
        ((getItemString(IDC_BLUEMAX_EDIT))[0] == '\0') ||
        ((getItemString(IDC_REDSHIFT_EDIT))[0] == '\0') || 
        ((getItemString(IDC_GREENSHIFT_EDIT))[0] == '\0') || 
        ((getItemString(IDC_BLUESHIFT_EDIT))[0] == '\0')) {
      strcpy(err_msg, "Please fill the all fields in the dialog.");
    }
    int newIndex = supportedPF->getIndexByPFName(getItemString(IDC_NAME_EDIT));
    if ((supportedPF->getIndexByPFName(format_name) != newIndex) && 
        (newIndex != -1)) {
      strcpy(err_msg, "The pixel format with that name is already exist.");
    }
    if (getItemInt(IDC_DEPTH_EDIT) <= 0) {
      strcpy(err_msg, "The pixel depth must be larger than 0.");
    }
    if (err_msg[0] != 0) {
      MessageBox(handle, err_msg, "RfbPlayer", MB_OK | MB_ICONWARNING);
      return false;
    }
    // Fill the pixel format structure
    strCopy(format_name, getItemString(IDC_NAME_EDIT), MAX_STR_LEN);
    pf->bpp = getItemInt(IDC_BPP_COMBO);
    pf->depth = getItemInt(IDC_DEPTH_EDIT);
    pf->bigEndian = (SendMessage(GetDlgItem(handle, IDC_BIGENDIAN_COMBO), 
      CB_GETCURSEL, 0, 0)) ? true : false;
    pf->trueColour = true;
    pf->redMax = getItemInt(IDC_REDMAX_EDIT);
    pf->greenMax = getItemInt(IDC_GREENMAX_EDIT);
    pf->blueMax = getItemInt(IDC_BLUEMAX_EDIT);
    pf->redShift = getItemInt(IDC_REDSHIFT_EDIT);
    pf->greenShift = getItemInt(IDC_GREENSHIFT_EDIT);
    pf->blueShift = getItemInt(IDC_BLUESHIFT_EDIT);
    return true;
  }

  char *format_name;
  PixelFormatList *supportedPF;
  PixelFormat *pf;
};
