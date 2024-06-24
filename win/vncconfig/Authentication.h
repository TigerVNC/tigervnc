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

static rfb::BoolParameter queryOnlyIfLoggedOn("QueryOnlyIfLoggedOn",
  "Only prompt for a local user to accept incoming connections if there is a user logged on", false);

namespace rfb {

  namespace win32 {

    class SecPage : public SecurityPage {
    public:
      SecPage(const RegKey& rk)
        : SecurityPage(nullptr), regKey(rk) {
        security = new SecurityServer();
      }

      void initDialog() override {
        SecurityPage::initDialog();

        setItemChecked(IDC_QUERY_CONNECT, rfb::Server::queryConnect);
        setItemChecked(IDC_QUERY_LOGGED_ON, queryOnlyIfLoggedOn);
        onCommand(IDC_AUTH_NONE, 0);
      }

      bool onCommand(int id, int cmd) override {
        SecurityPage::onCommand(id, cmd);

        setChanged(true);

        if (id == IDC_AUTH_VNC_PASSWD) {
          PasswordDialog passwdDlg(regKey, registryInsecure);
          passwdDlg.showDialog(handle);
        } else if (id == IDC_LOAD_CERT) {
          const char* title = "X509Cert";
          const char* filter =
             "X.509 Certificates (*.crt;*.cer;*.pem)\0*.crt;*.cer;*.pem\0All\0*.*\0";
          showFileChooser(regKey, title, filter, handle);
        } else if (id == IDC_LOAD_CERTKEY) {
          const char* title = "X509Key";
          const char* filter = "X.509 Keys (*.key;*.pem)\0*.key;*.pem\0All\0*.*\0";
          showFileChooser(regKey, title, filter, handle);
        } else if (id == IDC_QUERY_LOGGED_ON) {
          enableItem(IDC_QUERY_LOGGED_ON, enableQueryOnlyIfLoggedOn());
        }

        return true;
      }
      bool onOk() override {
        SecurityPage::onOk();

        if (isItemChecked(IDC_AUTH_VNC))
          verifyVncPassword(regKey);
        else if (haveVncPassword() && 
            MsgBox(nullptr, "The VNC authentication method is disabled, but a password is still stored for it.\n"
                      "Do you want to remove the VNC authentication password from the registry?",
                      MB_ICONWARNING | MB_YESNO) == IDYES) {
          regKey.setBinary("Password", nullptr, 0);
        }

#ifdef HAVE_GNUTLS
        if (isItemChecked(IDC_ENC_X509)) {
          SSecurityTLS::X509_CertFile.setParam(regKey.getString("X509Cert").c_str());
          SSecurityTLS::X509_CertFile.setParam(regKey.getString("X509Key").c_str());
        }
#endif

        regKey.setString("SecurityTypes", security->ToString());
        regKey.setBool("QueryConnect", isItemChecked(IDC_QUERY_CONNECT));
        regKey.setBool("QueryOnlyIfLoggedOn", isItemChecked(IDC_QUERY_LOGGED_ON));

        return true;
      }
      void setWarnPasswdInsecure(bool warn) {
        registryInsecure = warn;
      }
      bool enableQueryOnlyIfLoggedOn() {
        return isItemChecked(IDC_QUERY_CONNECT);
      }


      static bool haveVncPassword() {
        std::string password, passwordReadOnly;
        SSecurityVncAuth::vncAuthPasswd.getVncAuthPasswd(&password, &passwordReadOnly);
        return !password.empty();
      }

      static void verifyVncPassword(const RegKey& regKey) {
        if (!haveVncPassword()) {
          MsgBox(nullptr, "The VNC authentication method is enabled, but no password is specified.\n"
                    "The password dialog will now be shown.", MB_ICONINFORMATION | MB_OK);
          PasswordDialog passwd(regKey, registryInsecure);
          passwd.showDialog();
        }
      }

      void loadX509Certs(void) override {}
      void enableX509Dialogs(void) override {
        enableItem(IDC_LOAD_CERT, true);
        enableItem(IDC_LOAD_CERTKEY, true);
      }
      void disableX509Dialogs(void) override {
        enableItem(IDC_LOAD_CERT, false);
        enableItem(IDC_LOAD_CERTKEY, false);
      }
      void loadVncPasswd() override {
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
      inline bool showFileChooser(const RegKey& /*rk*/,
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
        ofn.lpstrFileTitle = nullptr;
        ofn.nMaxFileTitle = 0;
        ofn.lpstrTitle = (char*)title;
        ofn.lpstrInitialDir = nullptr;
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
