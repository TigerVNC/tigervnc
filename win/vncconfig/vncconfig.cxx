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

#include <winsock2.h>
#include <windows.h>
#include <commctrl.h>
#include <string.h>

#include "resource.h"

#include <core/Logger_stdio.h>
#include <core/LogWriter.h>

#include <rfb_win32/Dialog.h>
#include <rfb_win32/RegConfig.h>
#include <rfb_win32/CurrentUser.h>

using namespace core;
using namespace rfb;
using namespace rfb::win32;

static LogWriter vlog("main");


#include <vncconfig/Authentication.h>
#include <vncconfig/Connections.h>
#include <vncconfig/Sharing.h>
#include <vncconfig/Hooking.h>
#include <vncconfig/Inputs.h>
#include <vncconfig/Legacy.h>
#include <vncconfig/Desktop.h>


const char* rfb::win32::AppName = "TigerVNC Configuration";


#ifdef _DEBUG
BoolParameter captureDialogs("CaptureDialogs", "", false);
#endif

HKEY configKey = HKEY_CURRENT_USER;


void
processParams(int argc, char* argv[]) {
  for (int i=1; i<argc; i++) {
    if (strcasecmp(argv[i], "-service") == 0) {
      configKey = HKEY_LOCAL_MACHINE;
    } else if (strcasecmp(argv[i], "-user") == 0) {
      configKey = HKEY_CURRENT_USER;
    } else {
      int ret;

      ret = Configuration::handleParamArg(argc, argv, i);
      if (ret > 0) {
        i += ret - 1;
        continue;
      }
    }
  }
}


int WINAPI WinMain(HINSTANCE inst, HINSTANCE /*prev*/, char* /*cmdLine*/, int /*cmdShow*/) {

  // Configure debugging output
#ifdef _DEBUG
  AllocConsole();
  freopen("CONIN$","rb",stdin);
  freopen("CONOUT$","wb",stdout);
  freopen("CONOUT$","wb",stderr);
  setbuf(stderr, nullptr);
  initStdIOLoggers();
  logParams.setParam("*:stderr:100");
  vlog.info("Starting vncconfig applet");
#endif

  try {
    try {
      // Process command-line args
      int argc = __argc;
      char** argv = __argv;
      processParams(argc, argv);

      /* *** Required if we wish to use IP address control
      INITCOMMONCONTROLSEX icce;
      icce.dwSize = sizeof(icce);
      icce.dwICC = ICC_INTERNET_CLASSES;
      InitCommonControlsEx(&icce);
      */

      // Create the required configuration registry key
      RegKey rootKey;
      rootKey.createKey(configKey, "Software\\TigerVNC\\WinVNC4");
  
      // Override whatever security it already had (NT only)
      bool warnOnChangePassword = false;
      try {
        AccessEntries access;
        Sid::Administrators adminSID;
        Sid::SYSTEM systemSID;
        access.addEntry(adminSID, KEY_ALL_ACCESS, GRANT_ACCESS);
        access.addEntry(systemSID, KEY_ALL_ACCESS, GRANT_ACCESS);
        UserSID userSID;
        if (configKey == HKEY_CURRENT_USER)
          access.addEntry(userSID, KEY_ALL_ACCESS, GRANT_ACCESS);
        AccessControlList acl(CreateACL(access));

        // Set the DACL, and don't allow the key to inherit its parent's DACL
        rootKey.setDACL(acl, false);
      } catch (core::win32_error& e) {
        // Something weird happens on NT 4.0 SP5 but I can't reproduce it on other
        // NT 4.0 service pack revisions.
        if (e.err == ERROR_INVALID_PARAMETER) {
          MsgBox(nullptr, "Windows reported an error trying to secure the VNC server settings for this user.  "
                    "Your settings may not be secure!", MB_ICONWARNING | MB_OK);
        } else if (e.err != ERROR_CALL_NOT_IMPLEMENTED &&
                   e.err != ERROR_NOT_LOGGED_ON) {
          // If the call is not implemented, ignore the error and continue
          throw;
        }
        warnOnChangePassword = true;
      }

      // Start a RegConfig thread, to load in existing settings
      RegConfigThread config;
      config.start(configKey, "Software\\TigerVNC\\WinVNC4");

      // Build the dialog
      std::list<PropSheetPage*> pages;
      SecPage auth(rootKey); pages.push_back(&auth);
      auth.setWarnPasswdInsecure(warnOnChangePassword);
      ConnectionsPage conn(rootKey); pages.push_back(&conn);
      InputsPage inputs(rootKey); pages.push_back(&inputs);
      SharingPage sharing(rootKey); pages.push_back(&sharing);
      DesktopPage desktop(rootKey); pages.push_back(&desktop);
      HookingPage hooks(rootKey); pages.push_back(&hooks);
      LegacyPage legacy(rootKey, configKey == HKEY_CURRENT_USER); pages.push_back(&legacy);

      // Load the default icon to use
      HICON icon = (HICON)LoadImage(inst, MAKEINTRESOURCE(IDI_ICON), IMAGE_ICON, 0, 0, LR_SHARED);

      // Create the PropertySheet handler
      const char* propSheetTitle = "VNC server properties (service-mode)";
      if (configKey == HKEY_CURRENT_USER)
        propSheetTitle = "VNC server properties (user-mode)";
      PropSheet sheet(inst, propSheetTitle, pages, icon);

#ifdef _DEBUG
      vlog.debug("Capture dialogs=%s", captureDialogs ? "true" : "false");
      sheet.showPropSheet(nullptr, true, false, captureDialogs);
#else
      sheet.showPropSheet(nullptr, true, false);
#endif
    } catch (core::win32_error& e) {
      switch (e.err) {
      case ERROR_ACCESS_DENIED:
        MsgBox(nullptr, "You do not have sufficient access rights to run the VNC Configuration applet",
               MB_ICONSTOP | MB_OK);
        return 1;
      };
      throw;
    }

  } catch (std::exception& e) {
    MsgBox(nullptr, e.what(), MB_ICONEXCLAMATION | MB_OK);
    return 1;
  }

  return 0;
}
