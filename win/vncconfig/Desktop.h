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
#ifndef WINVNCCONF_DESKTOP
#define WINVNCCONF_DESKTOP

#include <rfb_win32/Registry.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/SDisplay.h>

namespace rfb {

  namespace win32 {

    class DesktopPage : public PropSheetPage {
    public:
      DesktopPage(const RegKey& rk)
        : PropSheetPage(GetModuleHandle(nullptr), MAKEINTRESOURCE(IDD_DESKTOP)), regKey(rk) {}
      void initDialog() override {
        const char *action(rfb::win32::SDisplay::disconnectAction);
        bool disconnectLock = stricmp(action, "Lock") == 0;
        bool disconnectLogoff = stricmp(action, "Logoff") == 0;
        setItemChecked(IDC_DISCONNECT_LOGOFF, disconnectLogoff);
        setItemChecked(IDC_DISCONNECT_LOCK, disconnectLock);
        setItemChecked(IDC_DISCONNECT_NONE, !disconnectLock && !disconnectLogoff);
        setItemChecked(IDC_REMOVE_WALLPAPER, rfb::win32::SDisplay::removeWallpaper);
        setItemChecked(IDC_DISABLE_EFFECTS, rfb::win32::SDisplay::disableEffects);
      }
      bool onCommand(int id, int /*cmd*/) override {
        switch (id) {
        case IDC_DISCONNECT_LOGOFF:
        case IDC_DISCONNECT_LOCK:
        case IDC_DISCONNECT_NONE:
        case IDC_REMOVE_WALLPAPER:
        case IDC_DISABLE_EFFECTS:
          const char *action(rfb::win32::SDisplay::disconnectAction);
          bool disconnectLock = stricmp(action, "Lock") == 0;
          bool disconnectLogoff = stricmp(action, "Logoff") == 0;
          setChanged((disconnectLogoff != isItemChecked(IDC_DISCONNECT_LOGOFF)) ||
                     (disconnectLock != isItemChecked(IDC_DISCONNECT_LOCK)) ||
                     (isItemChecked(IDC_REMOVE_WALLPAPER) != rfb::win32::SDisplay::removeWallpaper) ||
                     (isItemChecked(IDC_DISABLE_EFFECTS) != rfb::win32::SDisplay::disableEffects));
          break;
        }
        return false;
      }
      bool onOk() override {
        const char* action = "None";
        if (isItemChecked(IDC_DISCONNECT_LOGOFF))
          action = "Logoff";
        else if (isItemChecked(IDC_DISCONNECT_LOCK))
          action = "Lock";
        regKey.setString("DisconnectAction", action);
        regKey.setBool("RemoveWallpaper", isItemChecked(IDC_REMOVE_WALLPAPER));
        regKey.setBool("DisableEffects", isItemChecked(IDC_DISABLE_EFFECTS));
        return true;
      }
    protected:
      RegKey regKey;
    };

  };

};

#endif
