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

// -=- WMHooks.h

#ifndef __RFB_WIN32_WM_HOOKS_H__
#define __RFB_WIN32_WM_HOOKS_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <rfb/UpdateTracker.h>
#include <rdr/Exception.h>
#include <rfb_win32/MsgWindow.h>

namespace rfb {

  namespace win32 {

    class WMHooks : public MsgWindow {
    public:
      WMHooks();
      ~WMHooks();

      bool setClipRect(const Rect& cr);
      bool setUpdateTracker(UpdateTracker* ut);

      virtual LRESULT processMessage(UINT msg, WPARAM wParam, LPARAM lParam);

#ifdef _DEBUG
      // Get notifications of any messages in the given range, to any hooked window
      void setDiagnosticRange(UINT min, UINT max);
#endif

    protected:
      ClippedUpdateTracker* clipper;
      Region clip_region;

      void* fg_window;
      Rect fg_window_rect;

    public:
      SimpleUpdateTracker new_changes;
      bool notified;
    };

    class WMBlockInput {
    public:
      WMBlockInput();
      ~WMBlockInput();
      bool blockInputs(bool block);
    protected:
      bool active;
    };

    // - Legacy cursor handling support
    class WMCursorHooks {
    public:
      WMCursorHooks();
      ~WMCursorHooks();

      bool start();

      HCURSOR getCursor() const;
    };

  };

};

#endif // __RFB_WIN32_WM_HOOKS_H__
