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

// -=- FTProgress.h

#ifndef __RFB_WIN32_FTPROGRESS_H__
#define __RFB_WIN32_FTPROGRESS_H__

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>

#include <rfb_win32/ProgressControl.h>
#include <vncviewer/resource.h>

namespace rfb {
  namespace win32 {
    class FTProgress
    {
    public:
      FTProgress(HWND hwndParent);
      ~FTProgress();
      
      bool initialize(DWORD64 totalMaxValue, DWORD maxValue);
      void increase(DWORD value);
      void clearSingle();
      void clearAll();

    private:
      ProgressControl *m_pSingleProgress;
      ProgressControl *m_pGeneralProgress;

      HWND m_hwndParent;
      HWND m_hwndSinglePercent;
      HWND m_hwndGeneralPercent;

      bool m_bInitialized;

      bool initProgressControls(DWORD64 totalMaxValue, DWORD maxValue);

      void setProgressText();
      bool createProgressBarObjects();
      bool destroyProgressBarObjects();
    };
  }
}

#endif // __RFB_WIN32_FTPROGRESS_H__
