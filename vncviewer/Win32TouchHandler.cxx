/* Copyright 2020 Samuel Mannehed for Cendio AB
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>

#define XK_MISCELLANY
#include <rfb/keysymdef.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>

#include "i18n.h"
#include "Win32TouchHandler.h"

static rfb::LogWriter vlog("Win32TouchHandler");

static const DWORD MOUSEMOVE_FLAGS = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE |
                                     MOUSEEVENTF_VIRTUALDESK;

static const unsigned SINGLE_PAN_THRESHOLD = 50;

Win32TouchHandler::Win32TouchHandler(HWND hWnd) :
  hWnd(hWnd), gesturesConfigured(false), gestureActive(false),
  ignoringGesture(false), fakeButtonMask(0)
{
  // If window is registered as touch we can not receive gestures,
  // this should not happen
  if (IsTouchWindow(hWnd, NULL))
    throw rfb::Exception(_("Window is registered for touch instead of gestures"));

  // We will not receive any touch/gesture events if this service
  // isn't running - Logging is enough
  if (!GetSystemMetrics(SM_DIGITIZER))
    vlog.debug("The 'Tablet PC Input' service is required for touch");

  // When we have less than two touch points we won't receive all
  // gesture events - Logging is enough
  int supportedTouches = GetSystemMetrics(SM_MAXIMUMTOUCHES);
  if (supportedTouches < 2)
    vlog.debug("Two touch points required, system currently supports: %d",
               supportedTouches);
}

bool Win32TouchHandler::processEvent(UINT Msg, WPARAM wParam, LPARAM lParam)
{
  GESTUREINFO gi;

  DWORD panWant = GC_PAN_WITH_SINGLE_FINGER_VERTICALLY   |
                  GC_PAN_WITH_SINGLE_FINGER_HORIZONTALLY |
                  GC_PAN;
  DWORD panBlock = GC_PAN_WITH_INERTIA | GC_PAN_WITH_GUTTER;

  GESTURECONFIG gc[] = {{GID_ZOOM, GC_ZOOM, 0},
                        {GID_PAN, panWant, panBlock},
                        {GID_TWOFINGERTAP, GC_TWOFINGERTAP, 0}};

  switch(Msg) {
  case WM_GESTURENOTIFY:
    if (gesturesConfigured)
        return false;

    if (!SetGestureConfig(hWnd, 0, 3, gc, sizeof(GESTURECONFIG))) {
      vlog.error(_("Failed to set gesture configuration (error 0x%x)"),
                 (int)GetLastError());
    }
    gesturesConfigured = true;
    // Windows expects all handler functions to always
    // pass this message on, and not consume it
    return false;
  case WM_GESTURE:
    ZeroMemory(&gi, sizeof(GESTUREINFO));
    gi.cbSize = sizeof(GESTUREINFO);

    if (!GetGestureInfo((HGESTUREINFO)lParam, &gi)) {
      vlog.error(_("Failed to get gesture information (error 0x%x)"),
                 (int)GetLastError());
      return true;
    }

    handleWin32GestureEvent(gi);

    CloseGestureInfoHandle((HGESTUREINFO)lParam);
    return true;
  }

  return false;
}

void Win32TouchHandler::handleWin32GestureEvent(GESTUREINFO gi)
{
  GestureEvent gev;
  POINT pos;

  if (gi.dwID == GID_BEGIN) {
    // FLTK gets very confused if the cursor position is outside
    // of the window when getting mouse events, so we start by
    // moving the cursor to something proper.
    // FIXME: Only do this when necessary?
    // FIXME: There is some odd delay before Windows fully updates
    //        the state of the cursor position. By doing it here in
    //        GID_BEGIN we hope to do it early enough that we don't
    //        get any odd effects.
    // FIXME: GF_BEGIN position can differ from GID_BEGIN pos.

    SetCursorPos(gi.ptsLocation.x, gi.ptsLocation.y);
    return;
  } else if (gi.dwID == GID_END) {
    gestureActive = false;
    ignoringGesture = false;
    return;
  }

  // The GID_BEGIN msg means that no fingers were previously touching,
  // and a completely new set of gestures is beginning.
  // The GF_BEGIN flag means a new type of gesture was detected. This
  // flag can be set on a msg when changing between gestures within
  // one set of touches.
  //
  // We don't support dynamically changing between gestures
  // without lifting the finger(s).
  if ((gi.dwFlags & GF_BEGIN) && gestureActive)
    ignoringGesture = true;
  if (ignoringGesture)
    return;

  if (gi.dwFlags & GF_BEGIN) {
    gev.type = GestureBegin;
  } else if (gi.dwFlags & GF_END) {
    gev.type = GestureEnd;
  } else {
    gev.type = GestureUpdate;
  }

  // Convert to relative coordinates
  pos.x = gi.ptsLocation.x;
  pos.y = gi.ptsLocation.y;
  ScreenToClient(gi.hwndTarget, &pos);
  gev.eventX = pos.x;
  gev.eventY = pos.y;

  switch(gi.dwID) {

  case GID_ZOOM:
    gev.gesture = GesturePinch;
    if (gi.dwFlags & GF_BEGIN) {
      gestureStart.x = pos.x;
      gestureStart.y = pos.y;
    } else {
      gev.eventX = gestureStart.x;
      gev.eventY = gestureStart.y;
    }
    gev.magnitudeX = gi.ullArguments;
    gev.magnitudeY = 0;
    break;

  case GID_PAN:
    if (isSinglePan(gi)) {
      if (gi.dwFlags & GF_BEGIN) {
        gestureStart.x = pos.x;
        gestureStart.y = pos.y;
        startedSinglePan = false;

      }

      // FIXME: Add support for sending a OneFingerTap gesture here.
      //        When the movement was very small and we get a GF_END
      //        within a short time we should consider it a tap.

      if (!startedSinglePan &&
          ((unsigned)abs(gestureStart.x - pos.x) < SINGLE_PAN_THRESHOLD) &&
          ((unsigned)abs(gestureStart.y - pos.y) < SINGLE_PAN_THRESHOLD))
         return;

      // Here we know we got a single pan!

      // Change the first GestureUpdate to GestureBegin
      // after we passed the threshold
      if (!startedSinglePan) {
        startedSinglePan = true;
        gev.type = GestureBegin;
        gev.eventX = gestureStart.x;
        gev.eventY = gestureStart.y;
      }

      gev.gesture = GestureDrag;

    } else {
      if (gi.dwFlags & GF_BEGIN) {
        gestureStart.x = pos.x;
        gestureStart.y = pos.y;
        gev.magnitudeX = 0;
        gev.magnitudeY = 0;
      } else {
        gev.eventX = gestureStart.x;
        gev.eventY = gestureStart.y;
        gev.magnitudeX = pos.x - gestureStart.x;
        gev.magnitudeY = pos.y - gestureStart.y;
      }

      gev.gesture = GestureTwoDrag;
    }
    break;

  case GID_TWOFINGERTAP:
    gev.gesture = GestureTwoTap;
    break;

  }

  gestureActive = true;

  BaseTouchHandler::handleGestureEvent(gev);

  // Since we have a threshold for GestureDrag we need to generate
  // a second event right away with the current position
  if ((gev.type == GestureBegin) && (gev.gesture == GestureDrag)) {
    gev.type = GestureUpdate;
    gev.eventX = pos.x;
    gev.eventY = pos.y;
    BaseTouchHandler::handleGestureEvent(gev);
  }

  // FLTK tends to reset the cursor to the real position so we
  // need to make sure that we update that position
  if (gev.type == GestureEnd) {
    POINT expectedPos;
    POINT currentPos;

    expectedPos = lastFakeMotionPos;
    ClientToScreen(hWnd, &expectedPos);
    GetCursorPos(&currentPos);

    if ((expectedPos.x != currentPos.x) ||
        (expectedPos.y != currentPos.y))
      SetCursorPos(expectedPos.x, expectedPos.y);
  }
}

bool Win32TouchHandler::isSinglePan(GESTUREINFO gi)
{
  // To differentiate between a single and a double pan we can look
  // at ullArguments. This shows the distance between the touch points,
  // but in the case of single pan, it seems to show the monitor's
  // origin value (this is not documented by microsoft). This origin
  // value seems to be relative to the screen's position in a multi
  // monitor setup. For example if the touch monitor is secondary and
  // positioned to the left of the primary, the origin is negative.
  //
  // To use this we need to get the monitor's origin value and check
  // if it is the same as ullArguments. If they match, we have a
  // single pan.

  POINT coordinates;
  HMONITOR monitorHandler;
  MONITORINFO mi;
  LONG lowestX;

  // Find the monitor with the touch event
  coordinates.x = gi.ptsLocation.x;
  coordinates.y = gi.ptsLocation.y;
  monitorHandler = MonitorFromPoint(coordinates,
                                    MONITOR_DEFAULTTOPRIMARY);

  // Find the monitor's origin
  ZeroMemory(&mi, sizeof(MONITORINFO));
  mi.cbSize = sizeof(MONITORINFO);
  GetMonitorInfo(monitorHandler, &mi);
  lowestX = mi.rcMonitor.left;

  return lowestX == (LONG)gi.ullArguments;
}

void Win32TouchHandler::fakeMotionEvent(const GestureEvent origEvent)
{
  UINT Msg = WM_MOUSEMOVE;
  WPARAM wParam = MAKEWPARAM(fakeButtonMask, 0);
  LPARAM lParam = MAKELPARAM(origEvent.eventX, origEvent.eventY);

  pushFakeEvent(Msg, wParam, lParam);
  lastFakeMotionPos.x = origEvent.eventX;
  lastFakeMotionPos.y = origEvent.eventY;
}

void Win32TouchHandler::fakeButtonEvent(bool press, int button,
                                        const GestureEvent origEvent)
{
  UINT Msg;
  WPARAM wParam;
  LPARAM lParam;
  int delta;

  switch (button) {

  case 1: // left mousebutton
    if (press) {
      Msg = WM_LBUTTONDOWN;
      fakeButtonMask |= MK_LBUTTON;
    } else {
      Msg = WM_LBUTTONUP;
      fakeButtonMask &= ~MK_LBUTTON;
    }
    break;
  case 2: // middle mousebutton
    if (press) {
      Msg = WM_MBUTTONDOWN;
      fakeButtonMask |= MK_MBUTTON;
    } else {
      Msg = WM_MBUTTONUP;
      fakeButtonMask &= ~MK_MBUTTON;
    }
    break;
  case 3: // right mousebutton
    if (press) {
      Msg = WM_RBUTTONDOWN;
      fakeButtonMask |= MK_RBUTTON;
    } else {
      Msg = WM_RBUTTONUP;
      fakeButtonMask &= ~MK_RBUTTON;
    }
    break;

  case 4: // scroll up
    Msg = WM_MOUSEWHEEL;
    delta = WHEEL_DELTA;
    break;
  case 5: // scroll down
    Msg = WM_MOUSEWHEEL;
    delta = -WHEEL_DELTA;
    break;
  case 6: // scroll left
    Msg = WM_MOUSEHWHEEL;
    delta = -WHEEL_DELTA;
    break;
  case 7: // scroll right
    Msg = WM_MOUSEHWHEEL;
    delta = WHEEL_DELTA;
    break;

  default:
    vlog.error(_("Invalid mouse button %d, must be a number between 1 and 7."),
               button);
    return;
  }

  if (1 <= button && button <= 3) {
    wParam = MAKEWPARAM(fakeButtonMask, 0);

    // Regular mouse events expect client coordinates
    lParam = MAKELPARAM(origEvent.eventX, origEvent.eventY);
  } else {
    POINT pos;

    // Only act on wheel press, not on release
    if (!press)
      return;

    wParam = MAKEWPARAM(fakeButtonMask, delta);

    // Wheel events require screen coordinates
    pos.x = (LONG)origEvent.eventX;
    pos.y = (LONG)origEvent.eventY;

    ClientToScreen(hWnd, &pos);
    lParam = MAKELPARAM(pos.x, pos.y);
  }

  pushFakeEvent(Msg, wParam, lParam);
}

void Win32TouchHandler::fakeKeyEvent(bool press, int keysym,
                                     const GestureEvent origEvent)
{
  UINT Msg = press ? WM_KEYDOWN : WM_KEYUP;
  WPARAM wParam;
  LPARAM lParam;
  int vKey;
  int scanCode;
  int previousKeyState = press ? 0 : 1;
  int transitionState = press ? 0 : 1;

  switch(keysym) {

  case XK_Control_L:
    vKey = VK_CONTROL;
    scanCode = 0x1d;
    if (press)
      fakeButtonMask |= MK_CONTROL;
    else
      fakeButtonMask &= ~MK_CONTROL;
    break;

  // BaseTouchHandler will currently not send SHIFT but we keep it for
  // completeness sake. This way we have coverage for all wParam's MK_-bits.
  case XK_Shift_L:
    vKey = VK_SHIFT;
    scanCode = 0x2a;
    if (press)
      fakeButtonMask |= MK_SHIFT;
    else
      fakeButtonMask &= ~MK_SHIFT;
    break;

  default:
    //FIXME: consider adding generic handling
    vlog.error(_("Unhandled key 0x%x - can't generate keyboard event."),
               keysym);
    return;
  }

  wParam = MAKEWPARAM(vKey, 0);

  scanCode         <<= 0;
  previousKeyState <<= 14;
  transitionState  <<= 15;
  lParam = MAKELPARAM(1, // RepeatCount
                      (scanCode | previousKeyState | transitionState));

  pushFakeEvent(Msg, wParam, lParam);
}

void Win32TouchHandler::pushFakeEvent(UINT Msg, WPARAM wParam, LPARAM lParam)
{
  PostMessage(hWnd, Msg, wParam, lParam);
}
