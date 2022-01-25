/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2004-2008 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2017 Peter Astrand <astrand@cendio.se> for Cendio AB
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
#include <signal.h>
#include <unistd.h>

#include <rfb/LogWriter.h>

#include <x0vncserver/XDesktop.h>

#include <X11/XKBlib.h>
#ifdef HAVE_XTEST
#include <X11/extensions/XTest.h>
#endif
#ifdef HAVE_XDAMAGE
#include <X11/extensions/Xdamage.h>
#endif
#ifdef HAVE_XFIXES
#include <X11/extensions/Xfixes.h>
#endif
#ifdef HAVE_XRANDR
#include <X11/extensions/Xrandr.h>
#include <RandrGlue.h>
extern "C" {
void vncSetGlueContext(Display *dpy, void *res);
}
#endif
#include <x0vncserver/Geometry.h>
#include <x0vncserver/XPixelBuffer.h>

using namespace rfb;

extern const unsigned short code_map_qnum_to_xorgevdev[];
extern const unsigned int code_map_qnum_to_xorgevdev_len;

extern const unsigned short code_map_qnum_to_xorgkbd[];
extern const unsigned int code_map_qnum_to_xorgkbd_len;

BoolParameter useShm("UseSHM", "Use MIT-SHM extension if available", true);
BoolParameter rawKeyboard("RawKeyboard",
                          "Send keyboard events straight through and "
                          "avoid mapping them to the current keyboard "
                          "layout", false);
IntParameter queryConnectTimeout("QueryConnectTimeout",
                                 "Number of seconds to show the Accept Connection dialog before "
                                 "rejecting the connection",
                                 10);

static rfb::LogWriter vlog("XDesktop");

// order is important as it must match RFB extension
static const char * ledNames[XDESKTOP_N_LEDS] = {
  "Scroll Lock", "Num Lock", "Caps Lock"
};

XDesktop::XDesktop(Display* dpy_, Geometry *geometry_)
  : dpy(dpy_), geometry(geometry_), pb(0), server(0),
    queryConnectDialog(0), queryConnectSock(0),
    oldButtonMask(0), haveXtest(false), haveDamage(false),
    maxButtons(0), running(false), ledMasks(), ledState(0),
    codeMap(0), codeMapLen(0)
{
  int major, minor;

  int xkbOpcode, xkbErrorBase;

  major = XkbMajorVersion;
  minor = XkbMinorVersion;
  if (!XkbQueryExtension(dpy, &xkbOpcode, &xkbEventBase,
                         &xkbErrorBase, &major, &minor)) {
    vlog.error("XKEYBOARD extension not present");
    throw Exception();
  }

  XkbSelectEvents(dpy, XkbUseCoreKbd, XkbIndicatorStateNotifyMask,
                  XkbIndicatorStateNotifyMask);

  // figure out bit masks for the indicators we are interested in
  for (int i = 0; i < XDESKTOP_N_LEDS; i++) {
    Atom a;
    int shift;
    Bool on;

    a = XInternAtom(dpy, ledNames[i], True);
    if (!a || !XkbGetNamedIndicator(dpy, a, &shift, &on, NULL, NULL))
      continue;

    ledMasks[i] = 1u << shift;
    vlog.debug("Mask for '%s' is 0x%x", ledNames[i], ledMasks[i]);
    if (on)
      ledState |= 1u << i;
  }

  // X11 unfortunately uses keyboard driver specific keycodes and provides no
  // direct way to query this, so guess based on the keyboard mapping
  XkbDescPtr desc = XkbGetKeyboard(dpy, XkbAllComponentsMask, XkbUseCoreKbd);
  if (desc && desc->names) {
    char *keycodes = XGetAtomName(dpy, desc->names->keycodes);

    if (keycodes) {
      if (strncmp("evdev", keycodes, strlen("evdev")) == 0) {
        codeMap = code_map_qnum_to_xorgevdev;
        codeMapLen = code_map_qnum_to_xorgevdev_len;
        vlog.info("Using evdev codemap\n");
      } else if (strncmp("xfree86", keycodes, strlen("xfree86")) == 0) {
        codeMap = code_map_qnum_to_xorgkbd;
        codeMapLen = code_map_qnum_to_xorgkbd_len;
        vlog.info("Using xorgkbd codemap\n");
      } else {
        vlog.info("Unknown keycode '%s', no codemap\n", keycodes);
      }
      XFree(keycodes);
    } else {
      vlog.debug("Unable to get keycode map\n");
    }

    XkbFreeKeyboard(desc, XkbAllComponentsMask, True);
  }

#ifdef HAVE_XTEST
  int xtestEventBase;
  int xtestErrorBase;

  if (XTestQueryExtension(dpy, &xtestEventBase,
                          &xtestErrorBase, &major, &minor)) {
    XTestGrabControl(dpy, True);
    vlog.info("XTest extension present - version %d.%d",major,minor);
    haveXtest = true;
  } else {
#endif
    vlog.info("XTest extension not present");
    vlog.info("Unable to inject events or display while server is grabbed");
#ifdef HAVE_XTEST
  }
#endif

#ifdef HAVE_XDAMAGE
  int xdamageErrorBase;

  if (XDamageQueryExtension(dpy, &xdamageEventBase, &xdamageErrorBase)) {
    haveDamage = true;
  } else {
#endif
    vlog.info("DAMAGE extension not present");
    vlog.info("Will have to poll screen for changes");
#ifdef HAVE_XDAMAGE
  }
#endif

#ifdef HAVE_XFIXES
  int xfixesErrorBase;

  if (XFixesQueryExtension(dpy, &xfixesEventBase, &xfixesErrorBase)) {
    XFixesSelectCursorInput(dpy, DefaultRootWindow(dpy),
                            XFixesDisplayCursorNotifyMask);
  } else {
#endif
    vlog.info("XFIXES extension not present");
    vlog.info("Will not be able to display cursors");
#ifdef HAVE_XFIXES
  }
#endif

#ifdef HAVE_XRANDR
  int xrandrErrorBase;

  randrSyncSerial = 0;
  if (XRRQueryExtension(dpy, &xrandrEventBase, &xrandrErrorBase)) {
    XRRSelectInput(dpy, DefaultRootWindow(dpy),
                   RRScreenChangeNotifyMask | RRCrtcChangeNotifyMask);
    /* Override TXWindow::init input mask */
    XSelectInput(dpy, DefaultRootWindow(dpy),
                 PropertyChangeMask | StructureNotifyMask | ExposureMask);
  } else {
#endif
    vlog.info("RANDR extension not present");
    vlog.info("Will not be able to handle session resize");
#ifdef HAVE_XRANDR
  }
#endif

  TXWindow::setGlobalEventHandler(this);
}

XDesktop::~XDesktop() {
  if (running)
    stop();
}


void XDesktop::poll() {
  if (pb and not haveDamage)
    pb->poll(server);
  if (running) {
    Window root, child;
    int x, y, wx, wy;
    unsigned int mask;
    XQueryPointer(dpy, DefaultRootWindow(dpy), &root, &child,
                  &x, &y, &wx, &wy, &mask);
    x -= geometry->offsetLeft();
    y -= geometry->offsetTop();
    server->setCursorPos(rfb::Point(x, y), false);
  }
}


void XDesktop::start(VNCServer* vs) {

  // Determine actual number of buttons of the X pointer device.
  unsigned char btnMap[8];
  int numButtons = XGetPointerMapping(dpy, btnMap, 8);
  maxButtons = (numButtons > 8) ? 8 : numButtons;
  vlog.info("Enabling %d button%s of X pointer device",
            maxButtons, (maxButtons != 1) ? "s" : "");

  // Create an ImageFactory instance for producing Image objects.
  ImageFactory factory((bool)useShm);

  // Create pixel buffer and provide it to the server object.
  pb = new XPixelBuffer(dpy, factory, geometry->getRect());
  vlog.info("Allocated %s", pb->getImage()->classDesc());

  server = vs;
  server->setPixelBuffer(pb, computeScreenLayout());

#ifdef HAVE_XDAMAGE
  if (haveDamage) {
    damage = XDamageCreate(dpy, DefaultRootWindow(dpy),
                           XDamageReportRawRectangles);
  }
#endif

#ifdef HAVE_XFIXES
  setCursor();
#endif

  server->setLEDState(ledState);

  running = true;
}

void XDesktop::stop() {
  running = false;

#ifdef HAVE_XDAMAGE
  if (haveDamage)
    XDamageDestroy(dpy, damage);
#endif

  delete queryConnectDialog;
  queryConnectDialog = 0;

  server->setPixelBuffer(0);
  server = 0;

  delete pb;
  pb = 0;
}

void XDesktop::terminate() {
  kill(getpid(), SIGTERM);
}

bool XDesktop::isRunning() {
  return running;
}

void XDesktop::queryConnection(network::Socket* sock,
                               const char* userName)
{
  assert(isRunning());

  // Someone already querying?
  if (queryConnectSock) {
    std::list<network::Socket*> sockets;
    std::list<network::Socket*>::iterator i;

    // Check if this socket is still valid
    server->getSockets(&sockets);
    for (i = sockets.begin(); i != sockets.end(); i++) {
      if (*i == queryConnectSock) {
        server->approveConnection(sock, false, "Another connection is currently being queried.");
        return;
      }
    }
  }

  if (!userName)
    userName = "(anonymous)";

  queryConnectSock = sock;

  CharArray address(sock->getPeerAddress());
  delete queryConnectDialog;
  queryConnectDialog = new QueryConnectDialog(dpy, address.buf,
                                              userName,
                                              queryConnectTimeout,
                                              this);
  queryConnectDialog->map();
}

void XDesktop::pointerEvent(const Point& pos, int buttonMask) {
#ifdef HAVE_XTEST
  if (!haveXtest) return;
  XTestFakeMotionEvent(dpy, DefaultScreen(dpy),
                       geometry->offsetLeft() + pos.x,
                       geometry->offsetTop() + pos.y,
                       CurrentTime);
  if (buttonMask != oldButtonMask) {
    for (int i = 0; i < maxButtons; i++) {
      if ((buttonMask ^ oldButtonMask) & (1<<i)) {
        if (buttonMask & (1<<i)) {
          XTestFakeButtonEvent(dpy, i+1, True, CurrentTime);
        } else {
          XTestFakeButtonEvent(dpy, i+1, False, CurrentTime);
        }
      }
    }
  }
  oldButtonMask = buttonMask;
#endif
}

#ifdef HAVE_XTEST
KeyCode XDesktop::XkbKeysymToKeycode(Display* dpy, KeySym keysym) {
  XkbDescPtr xkb;
  XkbStateRec state;
  unsigned int mods;
  unsigned keycode;

  xkb = XkbGetMap(dpy, XkbAllComponentsMask, XkbUseCoreKbd);
  if (!xkb)
    return 0;

  XkbGetState(dpy, XkbUseCoreKbd, &state);
  // XkbStateFieldFromRec() doesn't work properly because
  // state.lookup_mods isn't properly updated, so we do this manually
  mods = XkbBuildCoreState(XkbStateMods(&state), state.group);

  for (keycode = xkb->min_key_code;
       keycode <= xkb->max_key_code;
       keycode++) {
    KeySym cursym;
    unsigned int out_mods;
    XkbTranslateKeyCode(xkb, keycode, mods, &out_mods, &cursym);
    if (cursym == keysym)
      break;
  }

  if (keycode > xkb->max_key_code)
    keycode = 0;

  XkbFreeKeyboard(xkb, XkbAllComponentsMask, True);

  // Shift+Tab is usually ISO_Left_Tab, but RFB hides this fact. Do
  // another attempt if we failed the initial lookup
  if ((keycode == 0) && (keysym == XK_Tab) && (mods & ShiftMask))
    return XkbKeysymToKeycode(dpy, XK_ISO_Left_Tab);

  return keycode;
}
#endif

void XDesktop::keyEvent(rdr::U32 keysym, rdr::U32 xtcode, bool down) {
#ifdef HAVE_XTEST
  int keycode = 0;

  if (!haveXtest)
    return;

  // Use scan code if provided and mapping exists
  if (codeMap && rawKeyboard && xtcode < codeMapLen)
      keycode = codeMap[xtcode];

  if (!keycode) {
    if (pressedKeys.find(keysym) != pressedKeys.end())
      keycode = pressedKeys[keysym];
    else {
      // XKeysymToKeycode() doesn't respect state, so we have to use
      // something slightly more complex
      keycode = XkbKeysymToKeycode(dpy, keysym);
    }
  }

  if (!keycode) {
    vlog.error("Could not map key event to X11 key code");
    return;
  }

  if (down)
    pressedKeys[keysym] = keycode;
  else
    pressedKeys.erase(keysym);

  vlog.debug("%d %s", keycode, down ? "down" : "up");

  XTestFakeKeyEvent(dpy, keycode, down, CurrentTime);
#endif
}

void XDesktop::clientCutText(const char* str) {
}

ScreenSet XDesktop::computeScreenLayout()
{
  ScreenSet layout;
  char buffer[2048];

#ifdef HAVE_XRANDR
  XRRScreenResources *res = XRRGetScreenResources(dpy, DefaultRootWindow(dpy));
  if (!res) {
    vlog.error("XRRGetScreenResources failed");
    return layout;
  }
  vncSetGlueContext(dpy, res);

  layout = ::computeScreenLayout(&outputIdMap);
  XRRFreeScreenResources(res);

  // Adjust the layout relative to the geometry
  ScreenSet::iterator iter, iter_next;
  Point offset(-geometry->offsetLeft(), -geometry->offsetTop());
  for (iter = layout.begin();iter != layout.end();iter = iter_next) {
    iter_next = iter; ++iter_next;
    iter->dimensions = iter->dimensions.intersect(geometry->getRect());
    if (iter->dimensions.is_empty())
      layout.remove_screen(iter->id);
    else
      iter->dimensions = iter->dimensions.translate(offset);
  }
#endif

  // Make sure that we have at least one screen
  if (layout.num_screens() == 0)
    layout.add_screen(rfb::Screen(0, 0, 0, geometry->width(),
                                  geometry->height(), 0));

  vlog.debug("Detected screen layout:");
  layout.print(buffer, sizeof(buffer));
  vlog.debug("%s", buffer);

  return layout;
}

#ifdef HAVE_XRANDR
/* Get the biggest mode which is equal or smaller to requested
   size. If no such mode exists, return the smallest. */
static void GetSmallerMode(XRRScreenResources *res,
                    XRROutputInfo *output,
                    unsigned int *width, unsigned int *height)
{
  XRRModeInfo best = {};
  XRRModeInfo smallest = {};
  smallest.width = -1;
  smallest.height = -1;

  for (int i = 0; i < res->nmode; i++) {
    for (int j = 0; j < output->nmode; j++) {
      if (output->modes[j] == res->modes[i].id) {
        if ((res->modes[i].width > best.width && res->modes[i].width <= *width) &&
            (res->modes[i].height > best.height && res->modes[i].height <= *height)) {
          best = res->modes[i];
        }
        if ((res->modes[i].width < smallest.width) && res->modes[i].height < smallest.height) {
          smallest = res->modes[i];
        }
      }
    }
  }

  if (best.id == 0 && smallest.id != 0) {
    best = smallest;
  }

  *width = best.width;
  *height = best.height;
}
#endif /* HAVE_XRANDR */

unsigned int XDesktop::setScreenLayout(int fb_width, int fb_height,
                                       const rfb::ScreenSet& layout)
{
#ifdef HAVE_XRANDR
  XRRScreenResources *res = XRRGetScreenResources(dpy, DefaultRootWindow(dpy));
  if (!res) {
    vlog.error("XRRGetScreenResources failed");
    return rfb::resultProhibited;
  }
  vncSetGlueContext(dpy, res);

  /* The client may request a screen layout which is not supported by
     the Xserver. This happens, for example, when adjusting the size
     of a non-fullscreen vncviewer window. To handle this and other
     cases, we first call tryScreenLayout. If this fails, we try to
     adjust the request to one screen with a smaller mode. */
  vlog.debug("Testing screen layout");
  unsigned int tryresult = ::tryScreenLayout(fb_width, fb_height, layout, &outputIdMap);
  rfb::ScreenSet adjustedLayout;
  if (tryresult == rfb::resultSuccess) {
    adjustedLayout = layout;
  } else {
    vlog.debug("Impossible layout - trying to adjust");

    ScreenSet::const_iterator firstscreen = layout.begin();
    adjustedLayout.add_screen(*firstscreen);
    ScreenSet::iterator iter = adjustedLayout.begin();
    RROutput outputId = None;

    for (int i = 0;i < vncRandRGetOutputCount();i++) {
      unsigned int oi = vncRandRGetOutputId(i);

      /* Known? */
      if (outputIdMap.count(oi) == 0)
        continue;

      /* Find the corresponding screen... */
      if (iter->id == outputIdMap[oi]) {
        outputId = oi;
      } else {
        outputIdMap.erase(oi);
      }
    }

    /* New screen */
    if (outputId == None) {
      int i = getPreferredScreenOutput(&outputIdMap, std::set<unsigned int>());
      if (i != -1) {
        outputId = vncRandRGetOutputId(i);
      }
    }
    if (outputId == None) {
      vlog.debug("Resize adjust: Could not find corresponding screen");
      XRRFreeScreenResources(res);
      return rfb::resultInvalid;
    }
    XRROutputInfo *output = XRRGetOutputInfo(dpy, res, outputId);
    if (!output) {
      vlog.debug("Resize adjust: XRRGetOutputInfo failed");
      XRRFreeScreenResources(res);
      return rfb::resultInvalid;
    }
    if (!output->crtc) {
      vlog.debug("Resize adjust: Selected output has no CRTC");
      XRRFreeScreenResources(res);
      XRRFreeOutputInfo(output);
      return rfb::resultInvalid;
    }
    XRRCrtcInfo *crtc = XRRGetCrtcInfo(dpy, res, output->crtc);
    if (!crtc) {
      vlog.debug("Resize adjust: XRRGetCrtcInfo failed");
      XRRFreeScreenResources(res);
      XRRFreeOutputInfo(output);
      return rfb::resultInvalid;
    }

    unsigned int swidth = iter->dimensions.width();
    unsigned int sheight = iter->dimensions.height();

    switch (crtc->rotation) {
    case RR_Rotate_90:
    case RR_Rotate_270:
      unsigned int swap = swidth;
      swidth = sheight;
      sheight = swap;
      break;
    }

    GetSmallerMode(res, output, &swidth, &sheight);
    XRRFreeOutputInfo(output);

    switch (crtc->rotation) {
    case RR_Rotate_90:
    case RR_Rotate_270:
      unsigned int swap = swidth;
      swidth = sheight;
      sheight = swap;
      break;
    }

    XRRFreeCrtcInfo(crtc);

    if (sheight != 0 && swidth != 0) {
      vlog.debug("Adjusted resize request to %dx%d", swidth, sheight);
      iter->dimensions.setXYWH(0, 0, swidth, sheight);
      fb_width = swidth;
      fb_height = sheight;
    } else {
      vlog.error("Failed to find smaller or equal screen size");
      XRRFreeScreenResources(res);
      return rfb::resultInvalid;
    }
  }

  vlog.debug("Changing screen layout");
  unsigned int ret = ::setScreenLayout(fb_width, fb_height, adjustedLayout, &outputIdMap);
  XRRFreeScreenResources(res);

  /* Send a dummy event to the root window. When this event is seen,
     earlier change events (ConfigureNotify and/or CrtcChange) have
     been processed. An Expose event is used for simplicity; does not
     require any Atoms, and will not affect other applications. */
  unsigned long serial = XNextRequest(dpy);
  XExposeEvent ev = {}; /* zero x, y, width, height, count */
  ev.type = Expose;
  ev.display = dpy;
  ev.window = DefaultRootWindow(dpy);
  if (XSendEvent(dpy, DefaultRootWindow(dpy), False, ExposureMask, (XEvent*)&ev)) {
    while (randrSyncSerial < serial) {
      TXWindow::handleXEvents(dpy);
    }
  } else {
    vlog.error("XSendEvent failed");
  }

  /* The protocol requires that an error is returned if the requested
     layout could not be set. This is checked by
     VNCSConnectionST::setDesktopSize. Another ExtendedDesktopSize
     with reason=0 will be sent in response to the changes seen by the
     event handler. */
  if (adjustedLayout != layout)
    return rfb::resultInvalid;

  // Explicitly update the server state with the result as there
  // can be corner cases where we don't get feedback from the X server
  server->setScreenLayout(computeScreenLayout());

  return ret;

#else
  return rfb::resultProhibited;
#endif /* HAVE_XRANDR */
}


bool XDesktop::handleGlobalEvent(XEvent* ev) {
  if (ev->type == xkbEventBase + XkbEventCode) {
    XkbEvent *kb = (XkbEvent *)ev;

    if (kb->any.xkb_type != XkbIndicatorStateNotify)
      return false;

    vlog.debug("Got indicator update, mask is now 0x%x", kb->indicators.state);

    ledState = 0;
    for (int i = 0; i < XDESKTOP_N_LEDS; i++) {
      if (kb->indicators.state & ledMasks[i])
        ledState |= 1u << i;
    }

    if (running)
      server->setLEDState(ledState);

    return true;
#ifdef HAVE_XDAMAGE
  } else if (ev->type == xdamageEventBase) {
    XDamageNotifyEvent* dev;
    Rect rect;

    if (!running)
      return true;

    dev = (XDamageNotifyEvent*)ev;
    rect.setXYWH(dev->area.x, dev->area.y, dev->area.width, dev->area.height);
    rect = rect.translate(Point(-geometry->offsetLeft(),
                                -geometry->offsetTop()));
    server->add_changed(rect);

    return true;
#endif
#ifdef HAVE_XFIXES
  } else if (ev->type == xfixesEventBase + XFixesCursorNotify) {
    XFixesCursorNotifyEvent* cev;

    if (!running)
      return true;

    cev = (XFixesCursorNotifyEvent*)ev;

    if (cev->subtype != XFixesDisplayCursorNotify)
      return false;

    return setCursor();
#endif
#ifdef HAVE_XRANDR
  } else if (ev->type == Expose) {
    XExposeEvent* eev = (XExposeEvent*)ev;
    randrSyncSerial = eev->serial;

    return false;

  } else if (ev->type == ConfigureNotify) {
    XConfigureEvent* cev = (XConfigureEvent*)ev;

    if (cev->window != DefaultRootWindow(dpy)) {
      return false;
    }

    XRRUpdateConfiguration(ev);
    geometry->recalc(cev->width, cev->height);

    if (!running) {
      return false;
    }

    if ((cev->width != pb->width() || (cev->height != pb->height()))) {
      // Recreate pixel buffer
      ImageFactory factory((bool)useShm);
      delete pb;
      pb = new XPixelBuffer(dpy, factory, geometry->getRect());
      server->setPixelBuffer(pb, computeScreenLayout());

      // Mark entire screen as changed
      server->add_changed(rfb::Region(Rect(0, 0, cev->width, cev->height)));
    }

    return true;

  } else if (ev->type == xrandrEventBase + RRNotify) {
    XRRNotifyEvent* rev = (XRRNotifyEvent*)ev;

    if (rev->window != DefaultRootWindow(dpy)) {
      return false;
    }

    if (!running)
      return false;

    if (rev->subtype == RRNotify_CrtcChange) {
      server->setScreenLayout(computeScreenLayout());
    }

    return true;
#endif
  }

  return false;
}

void XDesktop::queryApproved()
{
  assert(isRunning());
  server->approveConnection(queryConnectSock, true, 0);
  queryConnectSock = 0;
}

void XDesktop::queryRejected()
{
  assert(isRunning());
  server->approveConnection(queryConnectSock, false,
                            "Connection rejected by local user");
  queryConnectSock = 0;
}

bool XDesktop::setCursor()
{
  XFixesCursorImage *cim;

  cim = XFixesGetCursorImage(dpy);
  if (cim == NULL)
    return false;

  // Copied from XserverDesktop::setCursor() in
  // unix/xserver/hw/vnc/XserverDesktop.cc and adapted to
  // handle long -> U32 conversion for 64-bit Xlib
  rdr::U8* cursorData;
  rdr::U8 *out;
  const unsigned long *pixels;

  cursorData = new rdr::U8[cim->width * cim->height * 4];

  // Un-premultiply alpha
  pixels = cim->pixels;
  out = cursorData;
  for (int y = 0; y < cim->height; y++) {
    for (int x = 0; x < cim->width; x++) {
      rdr::U8 alpha;
      rdr::U32 pixel = *pixels++;

      alpha = (pixel >> 24) & 0xff;
      if (alpha == 0)
        alpha = 1; // Avoid division by zero

      *out++ = ((pixel >> 16) & 0xff) * 255/alpha;
      *out++ = ((pixel >>  8) & 0xff) * 255/alpha;
      *out++ = ((pixel >>  0) & 0xff) * 255/alpha;
      *out++ = ((pixel >> 24) & 0xff);
    }
  }

  try {
    server->setCursor(cim->width, cim->height, Point(cim->xhot, cim->yhot),
                      cursorData);
  } catch (rdr::Exception& e) {
    vlog.error("XserverDesktop::setCursor: %s",e.str());
  }

  delete [] cursorData;
  XFree(cim);
  return true;
}
