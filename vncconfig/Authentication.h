/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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
#ifndef WINVNCCONF_AUTHENTICATION
#define WINVNCCONF_AUTHENTICATION

#include <rfb_win32/Registry.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/Win32Util.h>
#include <rfb/ServerCore.h>
#include <rfb/secTypes.h>
#include <rfb/vncAuth.h>


extern rfb::VncAuthPasswdConfigParameter vncAuthPasswd;

namespace rfb {

  namespace win32 {

    class VncPasswdDialog : public Dialog {
    public:
      VncPasswdDialog(const RegKey& rk) : Dialog(GetModuleHandle(0)), regKey(rk), warnPasswdInsecure(false) {}
      bool showDialog() {
        return Dialog::showDialog(MAKEINTRESOURCE(IDD_AUTH_VNC_PASSWD));
      }
      bool onOk() {
        TCharArray password1 = getItemString(IDC_PASSWORD1);
        TCharArray password2 = getItemString(IDC_PASSWORD2);;
        if (_tcscmp(password1.buf, password2.buf) != 0) {
          MsgBox(0, _T("The supplied passwords do not match"),
                 MB_ICONEXCLAMATION | MB_OK);
          return false;
        }
        if (warnPasswdInsecure &&
          (MsgBox(0, _T("Please note that your VNC password cannot be stored securely on this system.  ")
                     _T("Are you sure you wish to continue?"),
                  MB_YESNO | MB_ICONWARNING) == IDNO))
          return false;
        char passwd[9];
        memset(passwd, 0, sizeof(passwd));
        strCopy(passwd, CStr(password1.buf), sizeof(passwd));
        vncAuthObfuscatePasswd(passwd);
        regKey.setBinary(_T("Password"), passwd, 8);
        return true;
      }
      void setWarnPasswdInsecure(bool warn) {
        warnPasswdInsecure = warn;
      }
    protected:
      const RegKey& regKey;
      bool warnPasswdInsecure;
    };

    class AuthenticationPage : public PropSheetPage {
    public:
      AuthenticationPage(const RegKey& rk)
        : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_AUTHENTICATION)),
        passwd(rk), regKey(rk) {}
      void initDialog() {
        CharArray sec_types_str;
        sec_types_str.buf = rfb::Server::sec_types.getData();
        std::list<int> sec_types = parseSecTypes(sec_types_str.buf);

        useNone = useVNC = false;
        std::list<int>::iterator i;
        for (i=sec_types.begin(); i!=sec_types.end(); i++) {
          if ((*i) == secTypeNone) useNone = true;
          else if ((*i) == secTypeVncAuth) useVNC = true;
        }

        setItemChecked(IDC_AUTH_NONE, useNone);
        setItemChecked(IDC_AUTH_VNC, useVNC);
        setItemChecked(IDC_QUERY_CONNECT, rfb::Server::queryConnect);
      }
      bool onCommand(int id, int cmd) {
        switch (id) {
        case IDC_AUTH_VNC_PASSWD:
          passwd.showDialog();
          return true;
        case IDC_AUTH_NONE:
        case IDC_AUTH_VNC:
        case IDC_QUERY_CONNECT:
          setChanged((rfb::Server::queryConnect != isItemChecked(IDC_QUERY_CONNECT)) ||
                     (useNone != isItemChecked(IDC_AUTH_NONE)) ||
                     (useVNC != isItemChecked(IDC_AUTH_VNC)));
          return false;
        };
        return false;
      }
      bool onOk() {
        useVNC = isItemChecked(IDC_AUTH_VNC);
        useNone = isItemChecked(IDC_AUTH_NONE);
        if (useVNC) {
          CharArray password = vncAuthPasswd.getVncAuthPasswd();
          if (!password.buf || strlen(password.buf) == 0) {
            MsgBox(0, _T("The VNC authentication method is enabled, but no password is specified!  ")
                      _T("The password dialog will now be shown."), MB_ICONEXCLAMATION | MB_OK);
            passwd.showDialog();
          }
          regKey.setString(_T("SecurityTypes"), _T("VncAuth"));
        } else if (useNone) {
          regKey.setString(_T("SecurityTypes"), _T("None"));
        }
        regKey.setString(_T("ReverseSecurityTypes"), _T("None"));
        regKey.setBool(_T("QueryConnect"), isItemChecked(IDC_QUERY_CONNECT));
        return true;
      }
      void setWarnPasswdInsecure(bool warn) {
        passwd.setWarnPasswdInsecure(warn);
      }
    protected:
      RegKey regKey;
      VncPasswdDialog passwd;
      bool useNone;
      bool useVNC;
    };

  };

};

#endif
