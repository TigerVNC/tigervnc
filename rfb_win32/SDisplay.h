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

// -=- SDisplay.h
//
// The SDisplay class encapsulates a system display.

// *** THIS INTERFACE NEEDS TIDYING TO SEPARATE COORDINATE SYSTEMS BETTER ***

#ifndef __RFB_SDISPLAY_H__
#define __RFB_SDISPLAY_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <rfb/SDesktop.h>
#include <rfb/UpdateTracker.h>
#include <rfb/Configuration.h>
#include <rfb/util.h>

#include <winsock2.h>
#include <rfb_win32/Win32Util.h>
#include <rfb_win32/SocketManager.h>
#include <rfb_win32/DeviceFrameBuffer.h>
#include <rfb_win32/SInput.h>
#include <rfb_win32/Clipboard.h>
#include <rfb_win32/MsgWindow.h>
#include <rfb_win32/WMCursor.h>
#include <rfb_win32/WMHooks.h>
#include <rfb_win32/WMNotifier.h>
#include <rfb_win32/WMWindowCopyRect.h>
#include <rfb_win32/WMPoller.h>

namespace rfb {

  namespace win32 {

    //
    // -=- SDisplay
    //

    class SDisplayCore;

    class SDisplay : public SDesktop,
      WMMonitor::Notifier,
      Clipboard::Notifier,
      UpdateTracker,
      public SocketManager::EventHandler
    {
    public:
      SDisplay(const TCHAR* device=0);
      virtual ~SDisplay();

      // -=- SDesktop interface

      virtual void start(VNCServer* vs);
      virtual void stop();
      virtual void pointerEvent(const Point& pos, rdr::U8 buttonmask);
      virtual void keyEvent(rdr::U32 key, bool down);
      virtual void clientCutText(const char* str, int len);
      virtual void framebufferUpdateRequest();
      virtual Point getFbSize();

      // -=- UpdateTracker

      virtual void add_changed(const Region& rgn);
      virtual void add_copied(const Region& dest, const Point& delta);

      // -=- Clipboard
      
      virtual void notifyClipboardChanged(const char* text, int len);

      // -=- Display events
      
      virtual void notifyDisplayEvent(WMMonitor::Notifier::DisplayEventType evt);

      // -=- EventHandler interface

      HANDLE getUpdateEvent() {return updateEvent;}
      virtual bool processEvent(HANDLE event);

      // -=- Notification of whether or not SDisplay is started

      void setStatusLocation(bool* status) {statusLocation = status;}

      friend class SDisplayCore;

      static BoolParameter use_hooks;
      static BoolParameter disableLocalInputs;
      static StringParameter disconnectAction;
      static BoolParameter removeWallpaper;
      static BoolParameter removePattern;
      static BoolParameter disableEffects;

    protected:
      void restart();
      void recreatePixelBuffer();
      bool flushChangeTracker();  // true if flushed, false if empty

      void triggerUpdate();

      VNCServer* server;

      // -=- Display pixel buffer
      DeviceFrameBuffer* pb;
      TCharArray deviceName;
      HDC device;
      bool releaseDevice;

      // -=- The coordinates of Window's entire virtual Screen
      Rect screenRect;

      // -=- All changes are collected in Display coords and merged
      SimpleUpdateTracker change_tracker;

      // -=- Internal SDisplay implementation
      SDisplayCore* core;

      // -=- Cursor
      WMCursor::Info old_cursor;
      Region old_cursor_region;
      Point cursor_renderpos;

      // -=- Event signalled to trigger an update to be flushed
      Handle updateEvent;

      // -=- Where to write the active/inactive indicator to
      bool* statusLocation;
    };

  }
}

#endif // __RFB_SDISPLAY_H__
