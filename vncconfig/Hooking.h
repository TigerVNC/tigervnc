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
#ifndef WINVNCCONF_HOOKING
#define WINVNCCONF_HOOKING

#include <rfb_win32/Registry.h>
#include <rfb_win32/Dialog.h>
#include <rfb_win32/SDisplay.h>
#include <rfb/ServerCore.h>

namespace rfb {

  namespace win32 {

    class HookingPage : public PropSheetPage {
    public:
      HookingPage(const RegKey& rk)
        : PropSheetPage(GetModuleHandle(0), MAKEINTRESOURCE(IDD_HOOKING)), regKey(rk) {}
      void initDialog() {
        setItemChecked(IDC_USEHOOKS, rfb::win32::SDisplay::use_hooks);
        setItemChecked(IDC_POLLCONSOLES, rfb::win32::WMPoller::poll_console_windows);
        setItemChecked(IDC_COMPAREFB, rfb::Server::compareFB);
      }
      bool onCommand(int id, int cmd) {
        switch (id) {
        case IDC_USEHOOKS:
        case IDC_POLLCONSOLES:
        case IDC_COMPAREFB:
          setChanged((rfb::win32::SDisplay::use_hooks != isItemChecked(IDC_USEHOOKS)) ||
            (rfb::win32::WMPoller::poll_console_windows != isItemChecked(IDC_POLLCONSOLES)) ||
            (rfb::Server::compareFB != isItemChecked(IDC_COMPAREFB)));
          break;
        }
        return false;
      }
      bool onOk() {
        regKey.setBool(_T("UseHooks"), isItemChecked(IDC_USEHOOKS));
        regKey.setBool(_T("PollConsoleWindows"), isItemChecked(IDC_POLLCONSOLES));
        regKey.setBool(_T("CompareFB"), isItemChecked(IDC_COMPAREFB));
        return true;
      }
    protected:
      RegKey regKey;
    };

  };

};

#endif