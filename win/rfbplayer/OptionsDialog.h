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

// -=- OptionsDialog.h

#include <rfbplayer/PlayerOptions.h>
#include <rfbplayer/UserPixelFormatsDialog.h>

class OptionsDialog : public rfb::win32::Dialog {
public:
  OptionsDialog(PlayerOptions *_options, PixelFormatList *_supportedPF) 
  : Dialog(GetModuleHandle(0)), options(_options), combo(0),
  supportedPF(_supportedPF) {}
  // - Show the dialog and return true if OK was clicked,
  //   false in case of error or Cancel
  virtual bool showDialog(HWND parent) {
    return Dialog::showDialog(MAKEINTRESOURCE(IDD_OPTIONS), parent);
  }
protected:

  // Dialog methods (protected)
  virtual void initDialog() {
    combo = GetDlgItem(handle, IDC_PIXELFORMAT);
    SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("Auto"));
    for (int i = 0; i < supportedPF->count(); i++) {
      SendMessage(combo, CB_ADDSTRING, 
        0, (LPARAM)(LPCTSTR)(((*supportedPF)[i])->format_name));
    }
    SendMessage(combo, CB_SETCURSEL, options->pixelFormatIndex + 1, 0);
    setItemChecked(IDC_ACCEPT_BELL, options->acceptBell);
    setItemChecked(IDC_ACCEPT_CUT_TEXT, options->acceptCutText);
    setItemChecked(IDC_AUTOPLAY, options->autoPlay);
    setItemChecked(IDC_BIG_ENDIAN, options->bigEndianFlag);
    if (options->askPixelFormat) {
      setItemChecked(IDC_ASK_PF, true);
      enableItem(IDC_PIXELFORMAT, false);
      enableItem(IDC_BIG_ENDIAN, false);
    }
  }
  virtual bool onOk() {
    options->askPixelFormat = isItemChecked(IDC_ASK_PF);
    options->acceptBell = isItemChecked(IDC_ACCEPT_BELL);
    options->acceptCutText = isItemChecked(IDC_ACCEPT_CUT_TEXT);
    options->autoPlay = isItemChecked(IDC_AUTOPLAY);
    options->bigEndianFlag = isItemChecked(IDC_BIG_ENDIAN);
    if (!options->askPixelFormat) {
      options->pixelFormatIndex = int(SendMessage(combo, CB_GETCURSEL, 0, 0)) - 1;
      if (options->pixelFormatIndex < 0) {
        options->autoDetectPF = true;
      } else {
        options->setPF(&((*supportedPF)[options->pixelFormatIndex])->PF);
        options->pixelFormat.bigEndian = options->bigEndianFlag;
        options->autoDetectPF = false;
      }
    }
    options->writeToRegistry();
    return true;
  }
  virtual bool onCommand(int item, int cmd) { 
    if (item == IDC_ASK_PF) {
      enableItem(IDC_PIXELFORMAT, !isItemChecked(IDC_ASK_PF));
      enableItem(IDC_BIG_ENDIAN, !isItemChecked(IDC_ASK_PF));
    }
    if (item == IDC_DEFAULT) {
      SendMessage(combo, CB_SETCURSEL, DEFAULT_PF_INDEX + 1, 0);
      enableItem(IDC_PIXELFORMAT, !DEFAULT_ASK_PF);
      enableItem(IDC_BIG_ENDIAN, !DEFAULT_ASK_PF);
      setItemChecked(IDC_ASK_PF, DEFAULT_ASK_PF);
      setItemChecked(IDC_ACCEPT_BELL, DEFAULT_ACCEPT_BELL);
      setItemChecked(IDC_ACCEPT_CUT_TEXT, DEFAULT_ACCEPT_CUT_TEXT);
      setItemChecked(IDC_AUTOPLAY, DEFAULT_AUTOPLAY);
      setItemChecked(IDC_BIG_ENDIAN, DEFAULT_BIG_ENDIAN);
    }
    if (item == IDC_EDIT_UPF) {
      UserPixelFormatsDialog UpfListDialog(supportedPF);
      if (UpfListDialog.showDialog(handle)) {
        int index = SendMessage(combo, CB_GETCURSEL, 0, 0);
        SendMessage(combo, CB_RESETCONTENT, 0, 0);
        SendMessage(combo, CB_ADDSTRING, 0, (LPARAM)(LPCTSTR)("Auto"));
        for (int i = 0; i < supportedPF->count(); i++) {
          SendMessage(combo, CB_ADDSTRING, 
            0, (LPARAM)(LPCTSTR)(((*supportedPF)[i])->format_name));
        }
        if ( index > (SendMessage(combo, CB_GETCOUNT, 0, 0) - 1)) {
          index = SendMessage(combo, CB_GETCOUNT, 0, 0) - 1;
        }
        SendMessage(combo, CB_SETCURSEL, index, 0);
        options->pixelFormatIndex = index - 1;
      }
    }
    return false;
  }

  HWND combo;
  PlayerOptions *options;
  PixelFormatList *supportedPF;
};