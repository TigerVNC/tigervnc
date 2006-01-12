/* Copyright (C) 2002-2004 RealVNC Ltd.  All Rights Reserved.
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

// -=- WinVNC Version 4.0 Tray Icon implementation

#include <winvnc/STrayIcon.h>
#include <winvnc/resource.h>

#include <rfb/LogWriter.h>
#include <rfb/Configuration.h>
#include <rfb_win32/LaunchProcess.h>
#include <rfb_win32/TrayIcon.h>
#include <rfb_win32/AboutDialog.h>
#include <rfb_win32/Win32Util.h>
#include <rfb_win32/Service.h>
#include <rfb_win32/CurrentUser.h>
#include <winvnc/ControlPanel.h>

using namespace rfb;
using namespace win32;
using namespace winvnc;

static LogWriter vlog("STrayIcon");

BoolParameter STrayIconThread::disableOptions("DisableOptions", "Disable the Options entry in the VNC Server tray menu.", false);


//
// -=- AboutDialog global values
//

const WORD rfb::win32::AboutDialog::DialogId = IDD_ABOUT;
const WORD rfb::win32::AboutDialog::Copyright = IDC_COPYRIGHT;
const WORD rfb::win32::AboutDialog::Version = IDC_VERSION;
const WORD rfb::win32::AboutDialog::BuildTime = IDC_BUILDTIME;
const WORD rfb::win32::AboutDialog::Description = IDC_DESCRIPTION;


//
// -=- Internal tray icon class
//

const UINT WM_SET_TOOLTIP = WM_USER + 1;


class winvnc::STrayIcon : public TrayIcon {
public:
  STrayIcon(STrayIconThread& t) : thread(t),
    vncConfig(_T("vncconfig.exe"), isServiceProcess() ? _T("-noconsole -service") : _T("-noconsole")),
    vncConnect(_T("winvnc4.exe"), _T("-connect")) {

    // ***
    SetWindowText(getHandle(), _T("winvnc::IPC_Interface"));
    // ***

    SetTimer(getHandle(), 1, 3000, 0);
    PostMessage(getHandle(), WM_TIMER, 1, 0);
    PostMessage(getHandle(), WM_SET_TOOLTIP, 0, 0);
    CPanel = new ControlPanel(getHandle());
  }

  virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {

    case WM_USER:
      {
        bool userKnown = CurrentUserToken().isValid();
        bool allowOptions = !STrayIconThread::disableOptions && userKnown;

        switch (lParam) {
        case WM_LBUTTONDBLCLK:
          SendMessage(getHandle(), WM_COMMAND, ID_CONTR0L_PANEL, 0);
          break;
        case WM_RBUTTONUP:
          HMENU menu = LoadMenu(GetModuleHandle(0), MAKEINTRESOURCE(thread.menu));
          HMENU trayMenu = GetSubMenu(menu, 0);


          // Default item is Options, if available, or About if not
          SetMenuDefaultItem(trayMenu, ID_CONTR0L_PANEL, FALSE);
          
          // Enable/disable options as required
          EnableMenuItem(trayMenu, ID_OPTIONS, (!allowOptions ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);
          EnableMenuItem(trayMenu, ID_CONNECT, (!userKnown ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);
          EnableMenuItem(trayMenu, ID_CLOSE, (isServiceProcess() ? MF_GRAYED : MF_ENABLED) | MF_BYCOMMAND);

          thread.server.getClientsInfo(&LCInfo);
          CheckMenuItem(trayMenu, ID_DISABLE_NEW_CLIENTS, (LCInfo.getDisable() ? MF_CHECKED : MF_UNCHECKED) | MF_BYCOMMAND);

          // SetForegroundWindow is required, otherwise Windows ignores the
          // TrackPopupMenu because the window isn't the foreground one, on
          // some older Windows versions...
          SetForegroundWindow(getHandle());

          // Display the menu
          POINT pos;
          GetCursorPos(&pos);
          TrackPopupMenu(trayMenu, 0, pos.x, pos.y, 0, getHandle(), 0);

          break;
		} 
        return 0;
      }
    
      // Handle tray icon menu commands
    case WM_COMMAND:
      switch (LOWORD(wParam)) {
      case ID_CONTR0L_PANEL:
        CPanel->showDialog();
        break;
      case ID_DISABLE_NEW_CLIENTS:
        {
          thread.server.getClientsInfo(&LCInfo);
          LCInfo.setDisable(!LCInfo.getDisable());
          thread.server.setClientsStatus(&LCInfo);
          CPanel->UpdateListView(&LCInfo);
        }
        break;
      case ID_OPTIONS:
        {
          CurrentUserToken token;
          if (token.isValid())
            vncConfig.start(isServiceProcess() ? (HANDLE)token : 0);
          else
            vlog.error("Options: unknown current user");
        }
        break;
      case ID_CONNECT:
        {
          CurrentUserToken token;
          if (token.isValid())
            vncConnect.start(isServiceProcess() ? (HANDLE)token : 0);
          else
            vlog.error("Options: unknown current user");
        }
        break;
      case ID_DISCONNECT:
        thread.server.disconnectClients("tray menu disconnect");
        break;
      case ID_CLOSE:
        if (!isServiceProcess())
          thread.server.stop();
        break;
      case ID_ABOUT:
        AboutDialog::instance.showDialog();
        break;
      }
      return 0;

      // Handle commands send by other processes
    case WM_COPYDATA:
      {
        COPYDATASTRUCT* command = (COPYDATASTRUCT*)lParam;
        switch (command->dwData) {
        case 1:
          {
            CharArray viewer = new char[command->cbData + 1];
            memcpy(viewer.buf, command->lpData, command->cbData);
            viewer.buf[command->cbData] = 0;
            thread.server.addNewClient(viewer.buf);
          }
          break;
        case 2:
          thread.server.disconnectClients("IPC disconnect");
          break;
        case 3:
          thread.server.setClientsStatus((rfb::ListConnInfo *)command->cbData);
        case 4:
          thread.server.getClientsInfo(&LCInfo);
          CPanel->UpdateListView(&LCInfo);
          break;
        };
      };
      break;

    case WM_CLOSE:
      PostQuitMessage(0);
      break;

    case WM_TIMER:
      if (rfb::win32::desktopChangeRequired()) {
        SendMessage(getHandle(), WM_CLOSE, 0, 0);
        return 0;
      }

      thread.server.getClientsInfo(&LCInfo);
      CPanel->UpdateListView(&LCInfo);

      setIcon(thread.server.isServerInUse() ?
              (!LCInfo.getDisable() ? thread.activeIcon : thread.dis_activeIcon) : 
              (!LCInfo.getDisable() ? thread.inactiveIcon : thread.dis_inactiveIcon));

      return 0;

    case WM_SET_TOOLTIP:
      {
        rfb::Lock l(thread.lock);
        if (thread.toolTip.buf)
          setToolTip(thread.toolTip.buf);
      }
      return 0;

    }

    return TrayIcon::processMessage(msg, wParam, lParam);
  }

protected:
  LaunchProcess vncConfig;
  LaunchProcess vncConnect;
  STrayIconThread& thread;
  ControlPanel * CPanel;
  rfb::ListConnInfo LCInfo;
};


STrayIconThread::STrayIconThread(VNCServerWin32& sm, UINT inactiveIcon_, UINT activeIcon_, 
                                 UINT dis_inactiveIcon_, UINT dis_activeIcon_, UINT menu_)
: server(sm), inactiveIcon(inactiveIcon_), activeIcon(activeIcon_),
  dis_inactiveIcon(dis_inactiveIcon_), dis_activeIcon(dis_activeIcon_),menu(menu_),
  windowHandle(0), runTrayIcon(true) {
  start();
}


void STrayIconThread::run() {
  while (runTrayIcon) {
    if (rfb::win32::desktopChangeRequired() && 
      !rfb::win32::changeDesktop())
      Sleep(2000);

    STrayIcon icon(*this);
    windowHandle = icon.getHandle();

    MSG msg;
    while (runTrayIcon && ::GetMessage(&msg, 0, 0, 0) > 0) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }

    windowHandle = 0;
  }
}

void STrayIconThread::setToolTip(const TCHAR* text) {
  if (!windowHandle) return;
  Lock l(lock);
  delete [] toolTip.buf;
  toolTip.buf = tstrDup(text);
  PostMessage(windowHandle, WM_SET_TOOLTIP, 0, 0);
}


