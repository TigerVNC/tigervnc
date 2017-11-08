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

#include <x0vncserver/Geometry.h>
#include <x0vncserver/XPixelBuffer.h>

using namespace rfb;

extern const unsigned short code_map_qnum_to_xorgevdev[];
extern const unsigned int code_map_qnum_to_xorgevdev_len;

extern const unsigned short code_map_qnum_to_xorgkbd[];
extern const unsigned int code_map_qnum_to_xorgkbd_len;

extern rfb::BoolParameter useShm;
extern rfb::BoolParameter rawKeyboard;

static rfb::LogWriter vlog("XDesktop");

// order is important as it must match RFB extension
static const char * ledNames[XDESKTOP_N_LEDS] = {
  "Scroll Lock", "Num Lock", "Caps Lock"
};

XDesktop::XDesktop(Display* dpy_, Geometry *geometry_)
    : dpy(dpy_), geometry(geometry_), pb(0), server(0),
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

    TXWindow::setGlobalEventHandler(this);
}

XDesktop::~XDesktop() {
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
      server->setCursorPos(rfb::Point(x, y));
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

    server = (VNCServerST *)vs;
    server->setPixelBuffer(pb);

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

    delete pb;
    pb = 0;
}

bool XDesktop::isRunning() {
    return running;
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
    unsigned keycode;

    xkb = XkbGetMap(dpy, XkbAllComponentsMask, XkbUseCoreKbd);
    if (!xkb)
      return 0;

    XkbGetState(dpy, XkbUseCoreKbd, &state);

    for (keycode = xkb->min_key_code;
         keycode <= xkb->max_key_code;
         keycode++) {
      KeySym cursym;
      unsigned int mods, out_mods;
      // XkbStateFieldFromRec() doesn't work properly because
      // state.lookup_mods isn't properly updated, so we do this manually
      mods = XkbBuildCoreState(XkbStateMods(&state), state.group);
      XkbTranslateKeyCode(xkb, keycode, mods, &out_mods, &cursym);
      if (cursym == keysym)
        break;
    }

    if (keycode > xkb->max_key_code)
      keycode = 0;

    XkbFreeKeyboard(xkb, XkbAllComponentsMask, True);

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

    if (!keycode)
      return;

    if (down)
      pressedKeys[keysym] = keycode;
    else
      pressedKeys.erase(keysym);

    XTestFakeKeyEvent(dpy, keycode, down, CurrentTime);
#endif
}

void XDesktop::clientCutText(const char* str, int len) {
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
    }

    return false;
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
          rdr::U8 *in = (rdr::U8 *) &pixel;

          alpha = in[3];
          if (alpha == 0)
            alpha = 1; // Avoid division by zero

          *out++ = (unsigned)*in++ * 255/alpha;
          *out++ = (unsigned)*in++ * 255/alpha;
          *out++ = (unsigned)*in++ * 255/alpha;
          *out++ = *in++;
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
