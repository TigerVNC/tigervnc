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

// -=- RegConfig.h

// Class which monitors the registry and reads in the registry settings
// whenever they change, or are added or removed.

#ifndef __RFB_WIN32_REG_CONFIG_H__
#define __RFB_WIN32_REG_CONFIG_H__

#include <rfb/Threading.h>
#include <rfb/Configuration.h>

#include <rfb_win32/Registry.h>

namespace rfb {

  namespace win32 {

    class RegistryReader {
    public:
      RegistryReader();
      ~RegistryReader();
      bool setKey(const HKEY rootkey, const TCHAR* keyname);
      bool setNotifyThread(Thread* thread, UINT winMsg, WPARAM wParam=0, LPARAM lParam=0);
      bool setNotifyWindow(HWND window, UINT winMsg, WPARAM wParam=0, LPARAM lParam=0);
      static void loadRegistryConfig(RegKey& key);
    protected:
      friend class RegReaderThread;
      Thread* thread;
      Thread* notifyThread;
      HWND notifyWindow;
      MSG notifyMsg;
    };

  };

};

#endif // __RFB_WIN32_REG_CONFIG_H__
