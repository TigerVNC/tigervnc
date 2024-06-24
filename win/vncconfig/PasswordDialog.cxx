/* Copyright (C) 2004-2005 RealVNC Ltd.  All Rights Reserved.
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

#include <vncconfig/resource.h>
#include <vncconfig/PasswordDialog.h>
#include <rfb_win32/MsgBox.h>
#include <rfb/obfuscate.h>

using namespace rfb;
using namespace win32;

PasswordDialog::PasswordDialog(const RegKey& rk, bool registryInsecure_)
  : Dialog(GetModuleHandle(nullptr)), regKey(rk), registryInsecure(registryInsecure_) {
}

bool PasswordDialog::showDialog(HWND owner) {
  return Dialog::showDialog(MAKEINTRESOURCE(IDD_AUTH_VNC_PASSWD), owner);
}

bool PasswordDialog::onOk() {
  std::string password1(getItemString(IDC_PASSWORD1));
  std::string password2(getItemString(IDC_PASSWORD2));
  if (password1 != password2) {
    MsgBox(nullptr, "The supplied passwords do not match",
           MB_ICONEXCLAMATION | MB_OK);
    return false;
  }
  if (registryInsecure &&
      (MsgBox(nullptr, "Please note that your password cannot be stored securely on this system.  "
                 "Are you sure you wish to continue?",
              MB_YESNO | MB_ICONWARNING) == IDNO))
    return false;
  std::vector<uint8_t> obfPwd = obfuscate(password1.c_str());
  regKey.setBinary("Password", obfPwd.data(), obfPwd.size());
  return true;
}
