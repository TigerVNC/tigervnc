/* Copyright 2019-2020 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright 2019 Aaron Sowry for Cendio AB
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

#include <assert.h>

#include <map>

#if defined(WIN32)
#include <windows.h>
#include <commctrl.h>
#elif !defined(__APPLE__)
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#endif

#include <FL/Fl.H>
#include <FL/x.H>

#include <rfb/Exception.h>
#include <rfb/LogWriter.h>

#include "i18n.h"
#include "vncviewer.h"
#include "BaseTouchHandler.h"
#if defined(WIN32)
#include "Win32TouchHandler.h"
#elif !defined(__APPLE__)
#include "XInputTouchHandler.h"
#endif

#include "touch.h"

static rfb::LogWriter vlog("Touch");

#if !defined(WIN32) && !defined(__APPLE__)
static int xi_major;
#endif

typedef std::map<Window, class BaseTouchHandler*> HandlerMap;
static HandlerMap handlers;

#if defined(WIN32)
LRESULT CALLBACK win32WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam,
                                 LPARAM lParam, UINT_PTR uIdSubclass,
                                 DWORD_PTR dwRefData)
{
  bool handled = false;

  if (uMsg == WM_NCDESTROY) {
    delete handlers[hWnd];
    handlers.erase(hWnd);
    RemoveWindowSubclass(hWnd, &win32WindowProc, 1);
  } else {
    if (handlers.count(hWnd) == 0) {
      vlog.error(_("Got message (0x%x) for an unhandled window"), uMsg);
    } else {
      handled = dynamic_cast<Win32TouchHandler*>
        (handlers[hWnd])->processEvent(uMsg, wParam, lParam);
    }
  }

  // Only run the normal WndProc handlers for unhandled events
  if (handled)
    return 0;
  else
    return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

#elif !defined(__APPLE__)
static void x11_change_touch_ownership(bool enable)
{
  HandlerMap::const_iterator iter;

  XIEventMask *curmasks;
  int num_masks;

  XIEventMask newmask;
  unsigned char mask[XIMaskLen(XI_LASTEVENT)] = { 0 };

  newmask.mask = mask;
  newmask.mask_len = sizeof(mask);

  for (iter = handlers.begin(); iter != handlers.end(); ++iter) {
    curmasks = XIGetSelectedEvents(fl_display, iter->first, &num_masks);
    if (curmasks == NULL) {
      if (num_masks == -1)
        vlog.error(_("Unable to get X Input 2 event mask for window 0x%08lx"), iter->first);
      continue;
    }

    // Our windows should only have a single mask, which allows us to
    // simplify all the code handling the masks
    if (num_masks > 1) {
      vlog.error(_("Window 0x%08lx has more than one X Input 2 event mask"), iter->first);
      continue;
    }

    newmask.deviceid = curmasks[0].deviceid;

    assert(newmask.mask_len >= curmasks[0].mask_len);
    memcpy(newmask.mask, curmasks[0].mask, curmasks[0].mask_len);
    if (enable)
      XISetMask(newmask.mask, XI_TouchOwnership);
    else
      XIClearMask(newmask.mask, XI_TouchOwnership);

    XISelectEvents(fl_display, iter->first, &newmask, 1);

    XFree(curmasks);
  }
}

bool x11_grab_pointer(Window window)
{
  bool ret;

  if (handlers.count(window) == 0) {
    vlog.error(_("Invalid window 0x%08lx specified for pointer grab"), window);
    return BadWindow;
  }

  // We need to remove XI_TouchOwnership from our event masks while
  // grabbing as otherwise we will get double events (one for the grab,
  // and one because of XI_TouchOwnership) with no way of telling them
  // apart. See XInputTouchHandler constructor for why we use this
  // event.
  x11_change_touch_ownership(false);

  ret = dynamic_cast<XInputTouchHandler*>(handlers[window])->grabPointer();

  if (!ret)
    x11_change_touch_ownership(true);

  return ret;
}

void x11_ungrab_pointer(Window window)
{
  if (handlers.count(window) == 0) {
    vlog.error(_("Invalid window 0x%08lx specified for pointer grab"), window);
    return;
  }

  dynamic_cast<XInputTouchHandler*>(handlers[window])->ungrabPointer();

  // Restore XI_TouchOwnership now that the grab is gone
  x11_change_touch_ownership(true);
}
#endif

static int handleTouchEvent(void *event, void *data)
{
#if defined(WIN32)
  MSG *msg = (MSG*)event;

  // Trigger on the first WM_PAINT event. We can't trigger on WM_CREATE
  // events since FLTK's system handlers trigger before WndProc.
  // WM_CREATE events are sent directly to WndProc.
  if (msg->message == WM_PAINT && handlers.count(msg->hwnd) == 0) {
    try {
      handlers[msg->hwnd] = new Win32TouchHandler(msg->hwnd);
    } catch (rfb::Exception& e) {
      vlog.error(_("Failed to create touch handler: %s"), e.str());
      abort_vncviewer(_("Failed to create touch handler: %s"), e.str());
    }
    // Add a special hook-in for handling events sent directly to WndProc
    if (!SetWindowSubclass(msg->hwnd, &win32WindowProc, 1, 0)) {
      vlog.error(_("Couldn't attach event handler to window (error 0x%x)"),
                 (int)GetLastError());
    }
  }
#elif !defined(__APPLE__)
  XEvent *xevent = (XEvent*)event;

  if (xevent->type == MapNotify) {
    handlers[xevent->xmap.window] = new XInputTouchHandler(xevent->xmap.window);

    // Fall through as we don't want to interfere with whatever someone
    // else might want to do with this event

  } else if (xevent->type == UnmapNotify) {
    delete handlers[xevent->xunmap.window];
    handlers.erase(xevent->xunmap.window);
  } else if (xevent->type == DestroyNotify) {
    delete handlers[xevent->xdestroywindow.window];
    handlers.erase(xevent->xdestroywindow.window);
  } else if (xevent->type == GenericEvent) {
    if (xevent->xgeneric.extension == xi_major) {
      XIDeviceEvent *devev;

      if (!XGetEventData(fl_display, &xevent->xcookie)) {
        vlog.error(_("Failed to get event data for X Input event"));
        return 1;
      }

      devev = (XIDeviceEvent*)xevent->xcookie.data;

      if (handlers.count(devev->event) == 0) {
        // We get these when the mouse is grabbed implicitly, so just
        // ignore them
        // https://gitlab.freedesktop.org/xorg/xserver/-/issues/1026
        if ((devev->evtype == XI_Enter) || (devev->evtype == XI_Leave))
          ;
        else
          vlog.error(_("X Input event for unknown window"));
        XFreeEventData(fl_display, &xevent->xcookie);
        return 1;
      }

      dynamic_cast<XInputTouchHandler*>(handlers[devev->event])->processEvent(devev);

      XFreeEventData(fl_display, &xevent->xcookie);

      return 1;
    }
  }
#endif

  return 0;
}

void enable_touch()
{
#if !defined(WIN32) && !defined(__APPLE__)
  int ev, err;
  int major_ver, minor_ver;

  fl_open_display();

  if (!XQueryExtension(fl_display, "XInputExtension", &xi_major, &ev, &err)) {
    abort_vncviewer(_("X Input extension not available."));
    return; // Not reached
  }

  major_ver = 2;
  minor_ver = 2;
  if (XIQueryVersion(fl_display, &major_ver, &minor_ver) != Success) {
    abort_vncviewer(_("X Input 2 (or newer) is not available."));
    return; // Not reached
  }

  if ((major_ver == 2) && (minor_ver < 2))
    vlog.error(_("X Input 2.2 (or newer) is not available. Touch gestures will not be supported."));
#endif

  Fl::add_system_handler(handleTouchEvent, NULL);
}

void disable_touch()
{
  Fl::remove_system_handler(handleTouchEvent);
}

