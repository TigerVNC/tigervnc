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

#ifndef WINVNC_TRAYICON_H
#define WINVNC_TRAYICON_H

#include <winvnc/VNCServerWin32.h>
#include <rfb_win32/TCharArray.h>
#include <rfb/Configuration.h>

namespace os {
  class Mutex;
  class Thread;
}

namespace winvnc {

  class STrayIconThread : os::Thread {
  public:
    STrayIconThread(VNCServerWin32& sm, UINT inactiveIcon,
      UINT activeIcon, UINT dis_inactiveIcon, UINT dis_activeIcon, UINT menu);
    virtual ~STrayIconThread();

    void setToolTip(const TCHAR* text);

    static rfb::BoolParameter disableOptions;
    static rfb::BoolParameter disableClose;

    friend class STrayIcon;
  protected:
    virtual void worker();

    os::Mutex* lock;
    DWORD thread_id;
    HWND windowHandle;
    rfb::TCharArray toolTip;
    VNCServerWin32& server;
    UINT inactiveIcon;
    UINT activeIcon;
    UINT dis_inactiveIcon;
    UINT dis_activeIcon;
    UINT menu;
    bool runTrayIcon;
  };

};

#endif
