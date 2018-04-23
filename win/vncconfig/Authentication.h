/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#include <windows.h>
#include <commctrl.h>

#include <vncconfig/PasswordDialog.h>
#include <rfb_win32/Registry.h>
#include <rfb_win32/SecurityPage.h>
#include <rfb_win32/MsgBox.h>
#include <rfb/ServerCore.h>
#include <rfb/Security.h>
#include <rfb/SecurityServer.h>
#include <rfb/SSecurityVncAuth.h>
#ifdef HAVE_GNUTLS
#include <rfb/SSecurityTLS.h>
#endif
#include <rfb/Password.h>

static rfb::BoolParameter queryOnlyIfLoggedOn("QueryOnlyIfLoggedOn",
  "Only prompt for a local user to accept incoming connections if there is a user logged on", false);

namespace rfb {

  namespace win32 {

    class SecPage : public SecurityPage {
    public:
      SecPage(const RegKey& rk)
        : SecurityPage(NULL), regKey(rk) {
        security = new SecurityServer();
      }

      void initDialog() {
        SecurityPage::initDialog();

        setItemChecked(IDC_QUERY_CONNECT, rfb::Server::queryConnect);
        setItemChecked(IDC_QUERY_LOGGED_ON, queryOnlyIfLoggedOn);
        onCommand(IDC_AUTH_NONE, 0);
      }

      bool onCommand(int id, int cmd) {
        SecurityPage::onCommand(id, cmd);

        setChanged(true);

        if (id == IDC_AUTH_VNC_PASSWD) {
          PasswordDialog passwdDlg(regKey, registryInsecure);
          passwdDlg.showDialog(handle);
        } else if (id == IDC_LOAD_CERT) {
          const TCHAR* title = _T("X509Cert");
          const TCHAR* filter =
             _T("X.509 Certificates (*.crt;*.cer;*.pem)\0*.crt;*.cer;*.pem\0All\0*.*\0");
          showFileChooser(regKey, title, filter, handle);
        } else if (id == IDC_LOAD_CERTKEY) {
          const TCHAR* title = _T("X509Key");
          const TCHAR* filter = _T("X.509 Keys (*.key;*.pem)\0*.key;*.pem\0All\0*.*\0");
          showFileChooser(regKey, title, filter, handle);
        } else if (id == IDC_QUERY_LOGGED_ON) {
          enableItem(IDC_QUERY_LOGGED_ON, enableQueryOnlyIfLoggedOn());
        }

        return true;
      }
      bool onOk() {
        SecurityPage::onOk();

        if (isItemChecked(IDC_AUTH_VNC))
          verifyVncPassword(regKey);
        else if (haveVncPassword() && 
            MsgBox(0, _T("The VNC authentication method is disabled, but a password is still stored for it.\n")
                      _T("Do you want to remove the VNC authentication password from the registry?"),
                      MB_ICONWARNING | MB_YESNO) == IDYES) {
          regKey.setBinary(_T("Password"), 0, 0);
        }

#ifdef HAVE_GNUTLS
        if (isItemChecked(IDC_ENC_X509)) {
          SSecurityTLS::X509_CertFile.setParam(regKey.getString("X509Cert"));
          SSecurityTLS::X509_CertFile.setParam(regKey.getString("X509Key"));
        }
#endif

        regKey.setString(_T("SecurityTypes"), security->ToString());
        regKey.setBool(_T("QueryConnect"), isItemChecked(IDC_QUERY_CONNECT));
        regKey.setBool(_T("QueryOnlyIfLoggedOn"), isItemChecked(IDC_QUERY_LOGGED_ON));

        return true;
      }
      void setWarnPasswdInsecure(bool warn) {
        registryInsecure = warn;
      }
      bool enableQueryOnlyIfLoggedOn() {
        return isItemChecked(IDC_QUERY_CONNECT);
      }


      static bool haveVncPassword() {
        PlainPasswd password, passwordReadOnly;
        SSecurityVncAuth::vncAuthPasswd.getVncAuthPasswd(&password, &passwordReadOnly);
        return password.buf && strlen(password.buf) != 0;
      }

      static void verifyVncPassword(const RegKey& regKey) {
        if (!haveVncPassword()) {
          MsgBox(0, _T("The VNC authentication method is enabled, but no password is specified.\n")
                    _T("The password dialog will now be shown."), MB_ICONINFORMATION | MB_OK);
          PasswordDialog passwd(regKey, registryInsecure);
          passwd.showDialog();
        }
      }

      virtual void loadX509Certs(void) {}
      virtual void enableX509Dialogs(void) {
        enableItem(IDC_LOAD_CERT, true);
        enableItem(IDC_LOAD_CERTKEY, true);
      }
      virtual void disableX509Dialogs(void) {
        enableItem(IDC_LOAD_CERT, false);
        enableItem(IDC_LOAD_CERTKEY, false);
      }
      virtual void loadVncPasswd() {
        enableItem(IDC_AUTH_VNC_PASSWD, isItemChecked(IDC_AUTH_VNC));
      }

    protected:
      RegKey regKey;
      static bool registryInsecure;
    private:
      inline void modifyAuthMethod(int enc_idc, int auth_idc, bool enable)
      {
        setItemChecked(enc_idc, enable);
        setItemChecked(auth_idc, enable);
      }
      inline bool showFileChooser(const RegKey& rk,
                                  const char* title,
                                  const char* filter,
                                  HWND hwnd)
      {
        OPENFILENAME ofn;
        char filename[MAX_PATH];

        ZeroMemory(&ofn, sizeof(ofn));
        ZeroMemory(&filename, sizeof(filename));
        filename[0] = '\0';
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd;
        ofn.lpstrFile = filename;
        ofn.nMaxFile = sizeof(filename);
        ofn.lpstrFilter = (char*)filter;
        ofn.nFilterIndex = 1;
        ofn.lpstrFileTitle = NULL;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrTitle = (char*)title;
        ofn.lpstrInitialDir = NULL;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileName(&ofn)==TRUE) {
          regKey.setString(title, filename);
          return true;
        }
        return false;
      }
    };

  };

  bool SecPage::registryInsecure = false;

};

#endif
