/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2019 Pierre Ossman for Cendio AB
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

#ifndef __RFB_SDISPLAY_H__
#define __RFB_SDISPLAY_H__

#include <rfb/SDesktop.h>
#include <rfb/UpdateTracker.h>
#include <rfb/Configuration.h>
#include <rfb_win32/Handle.h>
#include <rfb_win32/EventManager.h>
#include <rfb_win32/SInput.h>
#include <rfb_win32/Clipboard.h>
#include <rfb_win32/CleanDesktop.h>
#include <rfb_win32/WMCursor.h>
#include <rfb_win32/WMNotifier.h>
#include <rfb_win32/DeviceFrameBuffer.h>
#include <rfb_win32/DeviceContext.h>

namespace rfb {

  namespace win32 {

    //
    // -=- SDisplay
    //

    class SDisplayCore {
    public:
      virtual ~SDisplayCore() {};
      virtual void setScreenRect(const Rect& screenRect_) = 0;
      virtual void flushUpdates() = 0;
      virtual const char* methodName() const = 0;
    };

    class QueryConnectionHandler {
    public:
      virtual ~QueryConnectionHandler() {}
      virtual void queryConnection(network::Socket* sock,
                                   const char* userName) = 0;
    };

    class SDisplay : public SDesktop,
      WMMonitor::Notifier,
      Clipboard::Notifier,
      public EventHandler
    {
    public:
      SDisplay();
      virtual ~SDisplay();

      // -=- SDesktop interface

      virtual void start(VNCServer* vs);
      virtual void stop();
      virtual void terminate();
      virtual void queryConnection(network::Socket* sock,
                                   const char* userName);
      virtual void handleClipboardRequest();
      virtual void handleClipboardAnnounce(bool available);
      virtual void handleClipboardData(const char* data);
      virtual void pointerEvent(const Point& pos, int buttonmask);
      virtual void keyEvent(rdr::U32 keysym, rdr::U32 keycode, bool down);

      // -=- Clipboard events
      
      virtual void notifyClipboardChanged(bool available);

      // -=- Display events
      
      virtual void notifyDisplayEvent(WMMonitor::Notifier::DisplayEventType evt);

      // -=- EventHandler interface

      HANDLE getUpdateEvent() {return updateEvent;}
      HANDLE getTerminateEvent() {return terminateEvent;}
      virtual void processEvent(HANDLE event);

      // -=- Notification of whether or not SDisplay is started

      void setStatusLocation(bool* status) {statusLocation = status;}

      // -=- Set handler for incoming connections

      void setQueryConnectionHandler(QueryConnectionHandler* qch) {
        queryConnectionHandler = qch;
      }

      static IntParameter updateMethod;
      static BoolParameter disableLocalInputs;
      static StringParameter disconnectAction;
      static BoolParameter removeWallpaper;
      static BoolParameter disableEffects;

      // -=- Use by VNC Config to determine whether hooks are available
      static bool areHooksAvailable();


    protected:
      bool isRestartRequired();
      void startCore();
      void stopCore();
      void restartCore();
      void recreatePixelBuffer(bool force=false);
      bool flushChangeTracker();  // true if flushed, false if empty
      bool checkLedState();

      VNCServer* server;

      // -=- Display pixel buffer
      DeviceFrameBuffer* pb;
      DeviceContext* device;

      // -=- The coordinates of Window's entire virtual Screen
      Rect screenRect;

      // -=- All changes are collected in UN-CLIPPED Display coords and merged
      //     When they are to be flushed to the VNCServer, they are changed
      //     to server coords and clipped appropriately.
      SimpleUpdateTracker updates;
      ClippingUpdateTracker clipper;

      // -=- Internal SDisplay implementation
      SDisplayCore* core;
      int updateMethod_;

      // Inputs
      SPointer* ptr;
      SKeyboard* kbd;
      Clipboard* clipboard;
      WMBlockInput* inputs;

      // Desktop state
      WMMonitor* monitor;

      // Desktop optimisation
      CleanDesktop* cleanDesktop;
      bool isWallpaperRemoved;
      bool areEffectsDisabled;

      // Cursor
      WMCursor* cursor;
      WMCursor::Info old_cursor;
      Region old_cursor_region;
      Point cursor_renderpos;

      // -=- Event signalled to trigger an update to be flushed
      Handle updateEvent;
      // -=- Event signalled to terminate the server
      Handle terminateEvent;

      // -=- Where to write the active/inactive indicator to
      bool* statusLocation;

      // -=- Whom to query incoming connections
      QueryConnectionHandler* queryConnectionHandler;

      unsigned ledState;
    };

  }
}

#endif // __RFB_SDISPLAY_H__
