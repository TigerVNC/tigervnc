/* Copyright 2019 Aaron Sowry for Cendio AB
 * Copyright 2019-2020 Pierre Ossman for Cendio AB
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
#include <string.h>

#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI2.h>
#include <X11/XKBlib.h>

#include <FL/x.H>

#ifndef XK_MISCELLANY
#define XK_MISCELLANY
#include <rfb/keysymdef.h>
#endif
#include <rfb/LogWriter.h>

#include "i18n.h"
#include "XInputTouchHandler.h"

static rfb::LogWriter vlog("XInputTouchHandler");

static bool grabbed = false;

XInputTouchHandler::XInputTouchHandler(Window wnd)
  : wnd(wnd), fakeStateMask(0)
{
  XIEventMask eventmask;
  unsigned char flags[XIMaskLen(XI_LASTEVENT)] = { 0 };

  // Event delivery is broken when somebody else does a pointer grab,
  // so we need to listen to all devices and do filtering of master
  // devices manually
  eventmask.deviceid = XIAllDevices;
  eventmask.mask_len = sizeof(flags);
  eventmask.mask = flags;

  XISetMask(flags, XI_ButtonPress);
  XISetMask(flags, XI_Motion);
  XISetMask(flags, XI_ButtonRelease);

  XISetMask(flags, XI_TouchBegin);
  XISetMask(flags, XI_TouchUpdate);
  XISetMask(flags, XI_TouchEnd);

  // If something has a passive grab of touches (e.g. the window
  // manager wants to have its own gestures) then we won't get the
  // touch events until everyone who has a grab has indicated they
  // don't want these touches (via XIAllowTouchEvents()).
  // Unfortunately the touches are then replayed one touch point at
  // a time, meaning things will be delayed and out of order,
  // completely screwing up our gesture detection. Listening for
  // XI_TouchOwnership has the effect of giving us the touch events
  // right away, even if grabbing clients are also getting them.
  //
  // FIXME: We should really wait for the XI_TouchOwnership event
  //        before it is safe to react to the gesture, otherwise we
  //        might react to something that the window manager will
  //        also react to.
  //
  if (!grabbed)
    XISetMask(flags, XI_TouchOwnership);

  XISelectEvents(fl_display, wnd, &eventmask, 1);
}

bool XInputTouchHandler::grabPointer()
{
  XIEventMask *curmasks;
  int num_masks;

  int ret, ndevices;

  XIDeviceInfo *devices, *device;
  bool gotGrab;

  // We grab for the same events as the window is currently interested in
  curmasks = XIGetSelectedEvents(fl_display, wnd, &num_masks);
  if (curmasks == NULL) {
    if (num_masks == -1)
      vlog.error(_("Unable to get X Input 2 event mask for window 0x%08lx"), wnd);
    else
      vlog.error(_("Window 0x%08lx has no X Input 2 event mask"), wnd);

    return false;
  }

  // Our windows should only have a single mask, which allows us to
  // simplify all the code handling the masks
  if (num_masks > 1) {
    vlog.error(_("Window 0x%08lx has more than one X Input 2 event mask"), wnd);
    return false;
  }

  devices = XIQueryDevice(fl_display, XIAllMasterDevices, &ndevices);

  // Iterate through available devices to find those which
  // provide pointer input, and attempt to grab all such devices.
  gotGrab = false;
  for (int i = 0; i < ndevices; i++) {
    device = &devices[i];

    if (device->use != XIMasterPointer)
      continue;

    curmasks[0].deviceid = device->deviceid;

    ret = XIGrabDevice(fl_display,
                       device->deviceid,
                       wnd,
                       CurrentTime,
                       None,
                       XIGrabModeAsync,
                       XIGrabModeAsync,
                       True,
                       &(curmasks[0]));

    if (ret) {
      if (ret == XIAlreadyGrabbed) {
        continue;
      } else {
        vlog.error(_("Failure grabbing device %i"), device->deviceid);
        continue;
      }
    }

    gotGrab = true;
  }

  XIFreeDeviceInfo(devices);

  // Did we not even grab a single device?
  if (!gotGrab)
    return false;

  grabbed = true;

  return true;
}

void XInputTouchHandler::ungrabPointer()
{
  int ndevices;
  XIDeviceInfo *devices, *device;

  devices = XIQueryDevice(fl_display, XIAllMasterDevices, &ndevices);

  // Release all devices, hoping they are the same as when we
  // grabbed things
  for (int i = 0; i < ndevices; i++) {
    device = &devices[i];

    if (device->use != XIMasterPointer)
      continue;

    XIUngrabDevice(fl_display, device->deviceid, CurrentTime);
  }

  XIFreeDeviceInfo(devices);

  grabbed = false;
}

void XInputTouchHandler::processEvent(const XIDeviceEvent* devev)
{
  // FLTK doesn't understand X Input events, and we've stopped
  // delivery of Core events by enabling the X Input ones. Make
  // FLTK happy by faking Core events based on the X Input ones.

  bool isMaster = devev->deviceid != devev->sourceid;

  // We're only interested in events from master devices
  if (!isMaster) {
    // However we need to accept TouchEnd from slave devices as they
    // might not get delivered if there is a pointer grab, see:
    // https://gitlab.freedesktop.org/xorg/xserver/-/issues/1016
    if (devev->evtype != XI_TouchEnd)
      return;
  }

  // Avoid duplicate TouchEnd events, see above
  // FIXME: Doesn't handle floating slave devices
  if (isMaster && devev->evtype == XI_TouchEnd)
    return;

  if (devev->flags & XIPointerEmulated) {
    // We still want the server to do the scroll wheel to button thing
    // though, so keep those
    if (((devev->evtype == XI_ButtonPress) ||
         (devev->evtype == XI_ButtonRelease)) &&
        (devev->detail >= 4) && (devev->detail <= 7)) {
      ;
    } else {
      // Sometimes the server incorrectly sends us various events with
      // this flag set, see:
      // https://gitlab.freedesktop.org/xorg/xserver/-/issues/1027
      return;
    }
  }

  switch (devev->evtype) {
  case XI_Enter:
  case XI_Leave:
    // We get these when the mouse is grabbed implicitly, so just ignore them
    // https://gitlab.freedesktop.org/xorg/xserver/-/issues/1026
    break;
  case XI_Motion:
    // FIXME: We also get XI_Motion for scroll wheel events, which
    //        we might want to ignore
    fakeMotionEvent(devev);
    break;
  case XI_ButtonPress:
    fakeButtonEvent(true, devev->detail, devev);
    break;
  case XI_ButtonRelease:
    fakeButtonEvent(false, devev->detail, devev);
    break;
  case XI_TouchBegin:
    // XInput2 wants us to explicitly accept touch sequences
    // for grabbed devices before it will pass events
    if (grabbed) {
      XIAllowTouchEvents(fl_display,
                         devev->deviceid,
                         devev->detail,
                         devev->event,
                         XIAcceptTouch);
    }

    handleTouchBegin(devev->detail, devev->event_x, devev->event_y);
    break;
  case XI_TouchUpdate:
    handleTouchUpdate(devev->detail, devev->event_x, devev->event_y);
    break;
  case XI_TouchEnd:
    handleTouchEnd(devev->detail);
    break;
  case XI_TouchOwnership:
    // FIXME: Currently ignored, see constructor
    break;
  }
}

void XInputTouchHandler::preparePointerEvent(XEvent* dst, const XIDeviceEvent* src)
{
  // XButtonEvent and XMotionEvent are almost identical, so we
  // don't have to care which it is for these fields
  dst->xbutton.serial = src->serial;
  dst->xbutton.display = src->display;
  dst->xbutton.window = src->event;
  dst->xbutton.root = src->root;
  dst->xbutton.subwindow = src->child;
  dst->xbutton.time = src->time;
  dst->xbutton.x = src->event_x;
  dst->xbutton.y = src->event_y;
  dst->xbutton.x_root = src->root_x;
  dst->xbutton.y_root = src->root_y;
  dst->xbutton.state = src->mods.effective;
  dst->xbutton.state |= ((src->buttons.mask[0] >> 1) & 0x1f) << 8;
  dst->xbutton.same_screen = True;
}

void XInputTouchHandler::fakeMotionEvent(const XIDeviceEvent* origEvent)
{
  XEvent fakeEvent;

  memset(&fakeEvent, 0, sizeof(XEvent));

  fakeEvent.type = MotionNotify;
  fakeEvent.xmotion.is_hint = False;
  preparePointerEvent(&fakeEvent, origEvent);

  pushFakeEvent(&fakeEvent);
}

void XInputTouchHandler::fakeButtonEvent(bool press, int button,
                                         const XIDeviceEvent* origEvent)
{
  XEvent fakeEvent;

  memset(&fakeEvent, 0, sizeof(XEvent));

  fakeEvent.type = press ? ButtonPress : ButtonRelease;
  fakeEvent.xbutton.button = button;

  // Apply the fake mask before pushing so it will be in sync
  fakeEvent.xbutton.state |= fakeStateMask;
  preparePointerEvent(&fakeEvent, origEvent);

  pushFakeEvent(&fakeEvent);
}

void XInputTouchHandler::preparePointerEvent(XEvent* dst, const GestureEvent src)
{
  Window root, child;
  int rootX, rootY;
  XkbStateRec state;

  // We don't have a real event to steal things from, so we'll have
  // to fake these events based on the current state of things

  root = XDefaultRootWindow(fl_display);
  XTranslateCoordinates(fl_display, wnd, root,
                        src.eventX,
                        src.eventY,
                        &rootX, &rootY, &child);
  XkbGetState(fl_display, XkbUseCoreKbd, &state);

  // XButtonEvent and XMotionEvent are almost identical, so we
  // don't have to care which it is for these fields
  dst->xbutton.serial = XLastKnownRequestProcessed(fl_display);
  dst->xbutton.display = fl_display;
  dst->xbutton.window = wnd;
  dst->xbutton.root = root;
  dst->xbutton.subwindow = None;
  dst->xbutton.time = fl_event_time;
  dst->xbutton.x = src.eventX;
  dst->xbutton.y = src.eventY;
  dst->xbutton.x_root = rootX;
  dst->xbutton.y_root = rootY;
  dst->xbutton.state = state.mods;
  dst->xbutton.state |= ((state.ptr_buttons >> 1) & 0x1f) << 8;
  dst->xbutton.same_screen = True;
}

void XInputTouchHandler::fakeMotionEvent(const GestureEvent origEvent)
{
  XEvent fakeEvent;

  memset(&fakeEvent, 0, sizeof(XEvent));

  fakeEvent.type = MotionNotify;
  fakeEvent.xmotion.is_hint = False;
  preparePointerEvent(&fakeEvent, origEvent);

  fakeEvent.xbutton.state |= fakeStateMask;

  pushFakeEvent(&fakeEvent);
}

void XInputTouchHandler::fakeButtonEvent(bool press, int button,
                                         const GestureEvent origEvent)
{
  XEvent fakeEvent;

  memset(&fakeEvent, 0, sizeof(XEvent));

  fakeEvent.type = press ? ButtonPress : ButtonRelease;
  fakeEvent.xbutton.button = button;
  preparePointerEvent(&fakeEvent, origEvent);

  fakeEvent.xbutton.state |= fakeStateMask;

  // The button mask should indicate the button state just prior to
  // the event, we update the button mask after pushing
  pushFakeEvent(&fakeEvent);

  // Set/unset the bit for the correct button
  if (press) {
    fakeStateMask |= (1<<(7+button));
  } else {
    fakeStateMask &= ~(1<<(7+button));
  }
}

void XInputTouchHandler::fakeKeyEvent(bool press, int keysym,
                                      const GestureEvent origEvent)
{
  XEvent fakeEvent;

  Window root, child;
  int rootX, rootY;
  XkbStateRec state;

  int modmask;

  root = XDefaultRootWindow(fl_display);
  XTranslateCoordinates(fl_display, wnd, root,
                        origEvent.eventX,
                        origEvent.eventY,
                        &rootX, &rootY, &child);
  XkbGetState(fl_display, XkbUseCoreKbd, &state);

  KeyCode kc = XKeysymToKeycode(fl_display, keysym);

  memset(&fakeEvent, 0, sizeof(XEvent));

  fakeEvent.type = press ? KeyPress : KeyRelease;
  fakeEvent.xkey.type = press ? KeyPress : KeyRelease;
  fakeEvent.xkey.keycode = kc;
  fakeEvent.xkey.serial = XLastKnownRequestProcessed(fl_display);
  fakeEvent.xkey.display = fl_display;
  fakeEvent.xkey.window = wnd;
  fakeEvent.xkey.root = root;
  fakeEvent.xkey.subwindow = None;
  fakeEvent.xkey.time = fl_event_time;
  fakeEvent.xkey.x = origEvent.eventX;
  fakeEvent.xkey.y = origEvent.eventY;
  fakeEvent.xkey.x_root = rootX;
  fakeEvent.xkey.y_root = rootY;
  fakeEvent.xkey.state = state.mods;
  fakeEvent.xkey.state |= ((state.ptr_buttons >> 1) & 0x1f) << 8;
  fakeEvent.xkey.same_screen = True;

  // Apply our fake mask
  fakeEvent.xkey.state |= fakeStateMask;

  pushFakeEvent(&fakeEvent);

  switch(keysym) {
    case XK_Shift_L:
    case XK_Shift_R:
      modmask = ShiftMask;
      break;
    case XK_Caps_Lock:
      modmask = LockMask;
      break;
    case XK_Control_L:
    case XK_Control_R:
      modmask = ControlMask;
      break;
    default:
      modmask = 0;
  }

  if (press)
    fakeStateMask |= modmask;
  else
    fakeStateMask &= ~modmask;
}

void XInputTouchHandler::handleGestureEvent(const GestureEvent& event)
{
  BaseTouchHandler::handleGestureEvent(event);
}

void XInputTouchHandler::pushFakeEvent(XEvent* event)
{
  // Perhaps use XPutBackEvent() to avoid round trip latency?
  XSendEvent(fl_display, event->xany.window, true, 0, event);
}
