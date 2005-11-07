/* Copyright (C) 2005 TightVNC Team.  All Rights Reserved.
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
 *
 * TightVNC distribution homepage on the Web: http://www.tightvnc.com/
 *
 */

// -=- ProgressControl.h

#ifndef __RFB_WIN32_PROGRESSCONTROL_H__
#define __RFB_WIN32_PROGRESSCONTROL_H__

#include <windows.h>
#include <commctrl.h>

namespace rfb {
  namespace win32 {
    class ProgressControl
    {
    public:
      ProgressControl(HWND hwndProgress);
      ~ProgressControl();
      
      bool init(DWORD64 maxValue, DWORD64 position);
      
      bool increase(DWORD64 value);
      bool clear();

      int getCurrentPercent();
      
    private:
      HWND m_hwndProgress;
      
      DWORD64 m_dw64MaxValue;
      DWORD64 m_dw64CurrentValue;
      
      bool show();
    };
  }
}

#endif // __RFB_WIN32_PROGRESSCONTROL_H__
