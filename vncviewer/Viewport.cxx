/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2014 Pierre Ossman for Cendio AB
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
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <rfb/CMsgWriter.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>

// FLTK can pull in the X11 headers on some systems
#ifndef XK_VoidSymbol
#define XK_LATIN1
#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <rfb/keysymdef.h>
#endif

#ifndef XF86XK_ModeLock
#include <rfb/XF86keysym.h>
#endif

#ifndef NoSymbol
#define NoSymbol 0
#endif

// Missing in at least some versions of MinGW
#ifndef MAPVK_VK_TO_VSC
#define MAPVK_VK_TO_VSC 0
#endif

#include "Viewport.h"
#include "CConn.h"
#include "OptionsDialog.h"
#include "DesktopWindow.h"
#include "i18n.h"
#include "fltk_layout.h"
#include "parameters.h"
#include "keysym2ucs.h"
#include "menukey.h"
#include "vncviewer.h"

#include "PlatformPixelBuffer.h"
#include "FLTKPixelBuffer.h"

#if defined(WIN32)
#include "Win32PixelBuffer.h"
#elif defined(__APPLE__)
#include "OSXPixelBuffer.h"
#else
#include "X11PixelBuffer.h"
#endif

#include <FL/fl_draw.H>
#include <FL/fl_ask.H>

#include <FL/Fl_Menu.H>
#include <FL/Fl_Menu_Button.H>

#ifdef __APPLE__
#include "cocoa.h"
#endif

#ifdef WIN32
#include "win32.h"
#endif


using namespace rfb;
using namespace rdr;

static rfb::LogWriter vlog("Viewport");

// Menu constants

enum { ID_EXIT, ID_FULLSCREEN, ID_RESIZE,
       ID_CTRL, ID_ALT, ID_MENUKEY, ID_CTRLALTDEL,
       ID_REFRESH, ID_OPTIONS, ID_INFO, ID_ABOUT, ID_DISMISS };

// Fake key presses use this value and above
static const int fakeKeyBase = 0x200;

Viewport::Viewport(int w, int h, const rfb::PixelFormat& serverPF, CConn* cc_)
  : Fl_Widget(0, 0, w, h), cc(cc_), frameBuffer(NULL),
    lastPointerPos(0, 0), lastButtonMask(0),
    cursor(NULL), menuCtrlKey(false), menuAltKey(false)
{
// FLTK STR #2636 gives us the ability to monitor clipboard changes
#ifdef HAVE_FLTK_CLIPBOARD
  Fl::add_clipboard_notify(handleClipboardChange, this);
#endif

#ifdef HAVE_FLTK_XHANDLERS
  // We need to intercept keyboard events early
  Fl::add_system_handler(handleSystemEvent, this);
#endif

  frameBuffer = createFramebuffer(w, h);
  assert(frameBuffer);

  contextMenu = new Fl_Menu_Button(0, 0, 0, 0);
  // Setting box type to FL_NO_BOX prevents it from trying to draw the
  // button component (which we don't want)
  contextMenu->box(FL_NO_BOX);

  // The (invisible) button associated with this widget can mess with
  // things like Fl_Scroll so we need to get rid of any parents.
  // Unfortunately that's not possible because of STR #2654, but
  // reparenting to the current window works for most cases.
  window()->add(contextMenu);

  setMenuKey();

  OptionsDialog::addCallback(handleOptions, this);

  // Send a fake pointer event so that the server will stop rendering
  // a server-side cursor. Ideally we'd like to send the actual pointer
  // position, but we can't really tell when the window manager is done
  // placing us so we don't have a good time for that.
  handlePointerEvent(Point(w/2, h/2), 0);
}


Viewport::~Viewport()
{
  // Unregister all timeouts in case they get a change tro trigger
  // again later when this object is already gone.
  Fl::remove_timeout(handlePointerTimeout, this);

#ifdef HAVE_FLTK_XHANDLERS
  Fl::remove_system_handler(handleSystemEvent);
#endif

#ifdef HAVE_FLTK_CLIPBOARD
  Fl::remove_clipboard_notify(handleClipboardChange);
#endif

  OptionsDialog::removeCallback(handleOptions);

  delete frameBuffer;

  if (cursor) {
    if (!cursor->alloc_array)
      delete [] cursor->array;
    delete cursor;
  }

  // FLTK automatically deletes all child widgets, so we shouldn't touch
  // them ourselves here
}


const rfb::PixelFormat &Viewport::getPreferredPF()
{
  return frameBuffer->getPF();
}


// Copy the areas of the framebuffer that have been changed (damaged)
// to the displayed window.
// FIXME: Make sure this gets called on slow updates

void Viewport::updateWindow()
{
  Rect r;

  r = frameBuffer->getDamage();
  damage(FL_DAMAGE_USER1, r.tl.x + x(), r.tl.y + y(), r.width(), r.height());
}

rfb::ModifiablePixelBuffer* Viewport::getFramebuffer(void)
{
  return frameBuffer;
}

#ifdef HAVE_FLTK_CURSOR
static const char * dotcursor_xpm[] = {
  "5 5 2 1",
  ".	c #000000",
  " 	c #FFFFFF",
  "     ",
  " ... ",
  " ... ",
  " ... ",
  "     "};
#endif

void Viewport::setCursor(int width, int height, const Point& hotspot,
                              void* data, void* mask)
{
#ifdef HAVE_FLTK_CURSOR
  if (cursor) {
    if (!cursor->alloc_array)
      delete [] cursor->array;
    delete cursor;
  }

  int mask_len = ((width+7)/8) * height;
  int i;

  for (i = 0; i < mask_len; i++)
    if (((rdr::U8*)mask)[i]) break;

  if ((i == mask_len) && dotWhenNoCursor) {
    vlog.debug("cursor is empty - using dot");

    Fl_Pixmap pxm(dotcursor_xpm);
    cursor = new Fl_RGB_Image(&pxm);
    cursorHotspot.x = cursorHotspot.y = 2;
  } else {
    if ((width == 0) || (height == 0)) {
      U8 *buffer = new U8[4];
      memset(buffer, 0, 4);
      cursor = new Fl_RGB_Image(buffer, 1, 1, 4);
      cursorHotspot.x = cursorHotspot.y = 0;
    } else {
      U8 *buffer = new U8[width*height*4];
      U8 *i, *o, *m;
      int m_width;

      const PixelFormat *pf;
      
      pf = &cc->cp.pf();

      i = (U8*)data;
      o = buffer;
      m = (U8*)mask;
      m_width = (width+7)/8;
      for (int y = 0;y < height;y++) {
        for (int x = 0;x < width;x++) {
          pf->rgbFromBuffer(o, i, 1);

          if (m[(m_width*y)+(x/8)] & 0x80>>(x%8))
            o[3] = 255;
          else
            o[3] = 0;

          o += 4;
          i += pf->bpp/8;
        }
      }

      cursor = new Fl_RGB_Image(buffer, width, height, 4);

      cursorHotspot = hotspot;
    }
  }

  if (Fl::belowmouse() == this)
    window()->cursor(cursor, cursorHotspot.x, cursorHotspot.y);
#endif
}


void Viewport::draw()
{
  int X, Y, W, H;

  // Check what actually needs updating
  fl_clip_box(x(), y(), w(), h(), X, Y, W, H);
  if ((W == 0) || (H == 0))
    return;

  frameBuffer->draw(X - x(), Y - y(), X, Y, W, H);
}


void Viewport::resize(int x, int y, int w, int h)
{
  PlatformPixelBuffer* newBuffer;
  rfb::Rect rect;

  const rdr::U8* data;
  int stride;

  // FIXME: Resize should probably be a feature of the pixel buffer itself

  if ((w == frameBuffer->width()) && (h == frameBuffer->height()))
    goto end;

  vlog.debug("Resizing framebuffer from %dx%d to %dx%d",
             frameBuffer->width(), frameBuffer->height(), w, h);

  newBuffer = createFramebuffer(w, h);
  assert(newBuffer);

  rect.setXYWH(0, 0,
               __rfbmin(newBuffer->width(), frameBuffer->width()),
               __rfbmin(newBuffer->height(), frameBuffer->height()));
  data = frameBuffer->getBuffer(frameBuffer->getRect(), &stride);
  newBuffer->imageRect(rect, data, stride);

  // Black out any new areas

  if (newBuffer->width() > frameBuffer->width()) {
    rect.setXYWH(frameBuffer->width(), 0,
                 newBuffer->width() - frameBuffer->width(),
                 newBuffer->height());
    newBuffer->fillRect(rect, 0);
  }

  if (newBuffer->height() > frameBuffer->height()) {
    rect.setXYWH(0, frameBuffer->height(),
                 newBuffer->width(),
                 newBuffer->height() - frameBuffer->height());
    newBuffer->fillRect(rect, 0);
  }

  delete frameBuffer;
  frameBuffer = newBuffer;

end:
  Fl_Widget::resize(x, y, w, h);
}


int Viewport::handle(int event)
{
  char *buffer;
  int ret;
  int buttonMask, wheelMask;
  DownMap::const_iterator iter;

  switch (event) {
  case FL_PASTE:
    buffer = new char[Fl::event_length() + 1];

    // This is documented as to ASCII, but actually does to 8859-1
    ret = fl_utf8toa(Fl::event_text(), Fl::event_length(), buffer,
                     Fl::event_length() + 1);
    assert(ret < (Fl::event_length() + 1));

    vlog.debug("Sending clipboard data (%d bytes)", strlen(buffer));

    try {
      cc->writer()->clientCutText(buffer, ret);
    } catch (rdr::Exception& e) {
      vlog.error("%s", e.str());
      exit_vncviewer(e.str());
    }

    delete [] buffer;

    return 1;

  case FL_ENTER:
    // Yes, we would like some pointer events please!
#ifdef HAVE_FLTK_CURSOR
    if (cursor)
      window()->cursor(cursor, cursorHotspot.x, cursorHotspot.y);
#endif
    return 1;

  case FL_LEAVE:
#ifdef HAVE_FLTK_CURSOR
    window()->cursor(FL_CURSOR_DEFAULT);
#endif
    // Fall through as we want a last move event to help trigger edge stuff
  case FL_PUSH:
  case FL_RELEASE:
  case FL_DRAG:
  case FL_MOVE:
  case FL_MOUSEWHEEL:
    buttonMask = 0;
    if (Fl::event_button1())
      buttonMask |= 1;
    if (Fl::event_button2())
      buttonMask |= 2;
    if (Fl::event_button3())
      buttonMask |= 4;

    if (event == FL_MOUSEWHEEL) {
      wheelMask = 0;
      if (Fl::event_dy() < 0)
        wheelMask |= 8;
      if (Fl::event_dy() > 0)
        wheelMask |= 16;
      if (Fl::event_dx() < 0)
        wheelMask |= 32;
      if (Fl::event_dx() > 0)
        wheelMask |= 64;

      // A quick press of the wheel "button", followed by a immediate
      // release below
      handlePointerEvent(Point(Fl::event_x() - x(), Fl::event_y() - y()),
                         buttonMask | wheelMask);
    } 

    handlePointerEvent(Point(Fl::event_x() - x(), Fl::event_y() - y()), buttonMask);
    return 1;

  case FL_FOCUS:
#ifdef HAVE_FLTK_IM
    Fl::disable_im();
#endif
    // Yes, we would like some focus please!
    return 1;

  case FL_UNFOCUS:
    // Release all keys that were pressed as that generally makes most
    // sense (e.g. Alt+Tab where we only see the Alt press)
    while (!downKeySym.empty())
      handleKeyRelease(downKeySym.begin()->first);
#ifdef HAVE_FLTK_IM
    Fl::enable_im();
#endif
    return 1;

  case FL_KEYDOWN:
    handleFLTKKeyPress();
    return 1;

  case FL_KEYUP:
    handleKeyRelease(Fl::event_original_key());
    return 1;
  }

  return Fl_Widget::handle(event);
}


PlatformPixelBuffer* Viewport::createFramebuffer(int w, int h)
{
  PlatformPixelBuffer *fb;

  try {
#if defined(WIN32)
    fb = new Win32PixelBuffer(w, h);
#elif defined(__APPLE__)
    fb = new OSXPixelBuffer(w, h);
#else
    fb = new X11PixelBuffer(w, h);
#endif
  } catch (rdr::Exception& e) {
    vlog.error(_("Unable to create platform specific framebuffer: %s"), e.str());
    vlog.error(_("Using platform independent framebuffer"));
    fb = new FLTKPixelBuffer(w, h);
  }

  return fb;
}


void Viewport::handleClipboardChange(int source, void *data)
{
  Viewport *self = (Viewport *)data;

  assert(self);

  if (!sendPrimary && (source == 0))
    return;

  Fl::paste(*self, source);
}


void Viewport::handlePointerEvent(const rfb::Point& pos, int buttonMask)
{
  if (!viewOnly) {
    if (pointerEventInterval == 0 || buttonMask != lastButtonMask) {
      try {
        cc->writer()->pointerEvent(pos, buttonMask);
      } catch (rdr::Exception& e) {
        vlog.error("%s", e.str());
        exit_vncviewer(e.str());
      }
    } else {
      if (!Fl::has_timeout(handlePointerTimeout, this))
        Fl::add_timeout((double)pointerEventInterval/1000.0,
                        handlePointerTimeout, this);
    }
    lastPointerPos = pos;
    lastButtonMask = buttonMask;
  }
}


void Viewport::handlePointerTimeout(void *data)
{
  Viewport *self = (Viewport *)data;

  assert(self);

  try {
    self->cc->writer()->pointerEvent(self->lastPointerPos, self->lastButtonMask);
  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
    exit_vncviewer(e.str());
  }
}


void Viewport::handleKeyPress(int keyCode, rdr::U32 keySym)
{
  static bool menuRecursion = false;

  // Prevent recursion if the menu wants to send its own
  // activation key.
  if (menuKeySym && (keySym == menuKeySym) && !menuRecursion) {
    menuRecursion = true;
    popupContextMenu();
    menuRecursion = false;
    return;
  }

  if (viewOnly)
    return;

#ifdef __APPLE__
  // Alt on OS X behaves more like AltGr on other systems, and to get
  // sane behaviour we should translate things in that manner for the
  // remote VNC server. However that means we lose the ability to use
  // Alt as a shortcut modifier. Do what RealVNC does and hijack the
  // left command key as an Alt replacement.
  switch (keySym) {
  case XK_Super_L:
    keySym = XK_Alt_L;
    break;
  case XK_Super_R:
    keySym = XK_Super_L;
    break;
  case XK_Alt_L:
    keySym = XK_Mode_switch;
    break;
  case XK_Alt_R:
    keySym = XK_ISO_Level3_Shift;
    break;
  }
#endif

#ifdef WIN32
  // Ugly hack alert!
  //
  // Windows doesn't have a proper AltGr, but handles it using fake
  // Ctrl+Alt. Unfortunately X11 doesn't generally like the combination
  // Ctrl+Alt+AltGr, which we usually end up with when Xvnc tries to
  // get everything in the correct state. Cheat and temporarily release
  // Ctrl and Alt when we send some other symbol.
  bool ctrlPressed, altPressed;
  DownMap::iterator iter;

  ctrlPressed = false;
  altPressed = false;
  for (iter = downKeySym.begin();iter != downKeySym.end();++iter) {
    if (iter->second == XK_Control_L)
      ctrlPressed = true;
    else if (iter->second == XK_Alt_R)
      altPressed = true;
  }

  if (ctrlPressed && altPressed) {
    vlog.debug("Faking release of AltGr (Ctrl_L+Alt_R)");
    try {
      cc->writer()->keyEvent(XK_Control_L, false);
      cc->writer()->keyEvent(XK_Alt_R, false);
    } catch (rdr::Exception& e) {
      vlog.error("%s", e.str());
      exit_vncviewer(e.str());
    }
  }
#endif

  // Because of the way keyboards work, we cannot expect to have the same
  // symbol on release as when pressed. This breaks the VNC protocol however,
  // so we need to keep track of what keysym a key _code_ generated on press
  // and send the same on release.
  downKeySym[keyCode] = keySym;

#if defined(WIN32) || defined(__APPLE__)
  vlog.debug("Key pressed: 0x%04x => 0x%04x", keyCode, keySym);
#else
  vlog.debug("Key pressed: 0x%04x => XK_%s (0x%04x)",
             keyCode, XKeysymToString(keySym), keySym);
#endif

  try {
    cc->writer()->keyEvent(keySym, true);
  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
    exit_vncviewer(e.str());
  }

#ifdef WIN32
  // Ugly hack continued...
  if (ctrlPressed && altPressed) {
    vlog.debug("Restoring AltGr state");
    try {
      cc->writer()->keyEvent(XK_Control_L, true);
      cc->writer()->keyEvent(XK_Alt_R, true);
    } catch (rdr::Exception& e) {
      vlog.error("%s", e.str());
      exit_vncviewer(e.str());
    }
  }
#endif
}


void Viewport::handleKeyRelease(int keyCode)
{
  DownMap::iterator iter;

  if (viewOnly)
    return;

  iter = downKeySym.find(keyCode);
  if (iter == downKeySym.end()) {
    // These occur somewhat frequently so let's not spam them unless
    // logging is turned up.
    vlog.debug("Unexpected release of key code %d", keyCode);
    return;
  }

#if defined(WIN32) || defined(__APPLE__)
  vlog.debug("Key released: 0x%04x => 0x%04x", keyCode, iter->second);
#else
  vlog.debug("Key released: 0x%04x => XK_%s (0x%04x)",
             keyCode, XKeysymToString(iter->second), iter->second);
#endif

  try {
    cc->writer()->keyEvent(iter->second, false);
  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
    exit_vncviewer(e.str());
  }

  downKeySym.erase(iter);
}


int Viewport::handleSystemEvent(void *event, void *data)
{
  Viewport *self = (Viewport *)data;
  Fl_Widget *focus;

  assert(self);

  focus = Fl::grab();
  if (!focus)
    focus = Fl::focus();
  if (!focus)
    return 0;

  if (focus != self)
    return 0;

  assert(event);

#if defined(WIN32)
  MSG *msg = (MSG*)event;

  if ((msg->message == WM_KEYDOWN) || (msg->message == WM_SYSKEYDOWN)) {
    UINT vKey;
    bool isExtended;
    int keyCode;
    rdr::U32 keySym;

    vKey = msg->wParam;
    isExtended = (msg->lParam & (1 << 24)) != 0;

    keyCode = ((msg->lParam >> 16) & 0xff);

    // Windows sets the scan code to 0x00 for multimedia keys, so we
    // have to do a reverse lookup based on the vKey.
    if (keyCode == 0x00) {
      keyCode = MapVirtualKey(vKey, MAPVK_VK_TO_VSC);
      if (keyCode == 0x00) {
        if (isExtended)
          vlog.error(_("No scan code for extended virtual key 0x%02x"), (int)vKey);
        else
          vlog.error(_("No scan code for virtual key 0x%02x"), (int)vKey);
        return 1;
      }
    }

    if (isExtended)
      keyCode |= 0x100;

    // VK_SNAPSHOT sends different scan codes depending on the state of
    // Alt. This means that we can get different scan codes on press and
    // release. Force it to be something standard.
    if (vKey == VK_SNAPSHOT)
      keyCode = 0x137;

    keySym = win32_vkey_to_keysym(vKey, isExtended);
    if (keySym == NoSymbol) {
      if (isExtended)
        vlog.error(_("No symbol for extended virtual key 0x%02x"), (int)vKey);
      else
        vlog.error(_("No symbol for virtual key 0x%02x"), (int)vKey);
      return 1;
    }

    self->handleKeyPress(keyCode, keySym);

    return 1;
  } else if ((msg->message == WM_KEYUP) || (msg->message == WM_SYSKEYUP)) {
    UINT vKey;
    bool isExtended;
    int keyCode;

    vKey = msg->wParam;
    isExtended = (msg->lParam & (1 << 24)) != 0;

    keyCode = ((msg->lParam >> 16) & 0xff);
    if (keyCode == 0x00)
      keyCode = MapVirtualKey(vKey, MAPVK_VK_TO_VSC);
    if (isExtended)
      keyCode |= 0x100;
    if (vKey == VK_SNAPSHOT)
      keyCode = 0x137;

    self->handleKeyRelease(keyCode);

    return 1;
  }
#elif defined(__APPLE__)
  if (cocoa_is_keyboard_event(event)) {
    int keyCode;

    keyCode = cocoa_event_keycode(event);

    if (cocoa_is_key_press(event)) {
      rdr::U32 keySym;

      keySym = cocoa_event_keysym(event);
      if (keySym == NoSymbol) {
        vlog.error(_("No symbol for key code 0x%02x (in the current state)"),
                   (int)keyCode);
        return 1;
      }

      self->handleKeyPress(keyCode, keySym);

      // We don't get any release events for CapsLock, so we have to
      // send the release right away.
      if (keySym == XK_Caps_Lock)
        self->handleKeyRelease(keyCode);
    } else {
      self->handleKeyRelease(keyCode);
    }

    return 1;
  }
#else
  XEvent *xevent = (XEvent*)event;

  if (xevent->type == KeyPress) {
    char str;
    KeySym keysym;

    XLookupString(&xevent->xkey, &str, 1, &keysym, NULL);
    if (keysym == NoSymbol) {
      vlog.error(_("No symbol for key code %d (in the current state)"),
                 (int)xevent->xkey.keycode);
      return 1;
    }

    switch (keysym) {
    // For the first few years, there wasn't a good consensus on what the
    // Windows keys should be mapped to for X11. So we need to help out a
    // bit and map all variants to the same key...
    case XK_Hyper_L:
      keysym = XK_Super_L;
      break;
    case XK_Hyper_R:
      keysym = XK_Super_R;
      break;
    // There has been several variants for Shift-Tab over the years.
    // RFB states that we should always send a normal tab.
    case XK_ISO_Left_Tab:
      keysym = XK_Tab;
      break;
    }

    self->handleKeyPress(xevent->xkey.keycode, keysym);
    return 1;
  } else if (xevent->type == KeyRelease) {
    self->handleKeyRelease(xevent->xkey.keycode);
    return 1;
  }
#endif

  return 0;
}


rdr::U32 Viewport::translateKeyEvent(void)
{
  unsigned ucs;
  int keyCode, origKeyCode;
  const char *keyText;
  int keyTextLen;

  keyCode = Fl::event_key();
  origKeyCode = Fl::event_original_key();
  keyText = Fl::event_text();
  keyTextLen = Fl::event_length();

  vlog.debug("FLTK key %d (%d) '%s'[%d]", origKeyCode, keyCode, keyText, keyTextLen);

  // First check for function keys
  if ((keyCode > FL_F) && (keyCode <= FL_F_Last))
    return XK_F1 + (keyCode - FL_F - 1);

  // Numpad numbers
  if ((keyCode >= (FL_KP + '0')) && (keyCode <= (FL_KP + '9')))
    return XK_KP_0 + (keyCode - (FL_KP + '0'));

  // FLTK does some special remapping of numpad keys when numlock is off
  if ((origKeyCode >= FL_KP) && (origKeyCode <= FL_KP_Last)) {
    switch (keyCode) {
    case FL_F+1:
      return XK_KP_F1;
    case FL_F+2:
      return XK_KP_F2;
    case FL_F+3:
      return XK_KP_F3;
    case FL_F+4:
      return XK_KP_F4;
    case FL_Home:
      return XK_KP_Home;
    case FL_Left:
      return XK_KP_Left;
    case FL_Up:
      return XK_KP_Up;
    case FL_Right:
      return XK_KP_Right;
    case FL_Down:
      return XK_KP_Down;
    case FL_Page_Up:
      return XK_KP_Page_Up;
    case FL_Page_Down:
      return XK_KP_Page_Down;
    case FL_End:
      return XK_KP_End;
    case FL_Insert:
      return XK_KP_Insert;
    case FL_Delete:
      return XK_KP_Delete;
    }
  }

#if defined(WIN32) || defined(__APPLE__)
  // X11 fairly consistently uses XK_KP_Separator for comma and
  // XK_KP_Decimal for period. Windows and OS X are a different matter
  // though.
  //
  // OS X will consistently generate the same key code no matter what
  // layout is being used.
  //
  // Windows is terribly inconcistent, and is not something that's
  // likely to change:
  // http://blogs.msdn.com/michkap/archive/2006/09/13/752377.aspx
  //
  // To get X11 behaviour, we instead look at the text generated by
  // they key.
  if ((keyCode == (FL_KP + ',')) || (keyCode == (FL_KP + '.'))) {
    switch (keyText[0]) {
    case ',':
      return XK_KP_Separator;
    case '.':
      return XK_KP_Decimal;
    default:
      vlog.error(_("Unknown decimal separator: '%s'"), keyText);
      return XK_KP_Decimal;
    }
  }
#endif

  // Then other special keys
  switch (keyCode) {
  case FL_BackSpace:
    return XK_BackSpace;
  case FL_Tab:
    return XK_Tab;
  case FL_Enter:
    return XK_Return;
  case FL_Pause:
    return XK_Pause;
  case FL_Scroll_Lock:
    return XK_Scroll_Lock;
  case FL_Escape:
    return XK_Escape;
  case FL_Home:
    return XK_Home;
  case FL_Left:
    return XK_Left;
  case FL_Up:
    return XK_Up;
  case FL_Right:
    return XK_Right;
  case FL_Down:
    return XK_Down;
  case FL_Page_Up:
    return XK_Page_Up;
  case FL_Page_Down:
    return XK_Page_Down;
  case FL_End:
    return XK_End;
  case FL_Print:
    return XK_Print;
  case FL_Insert:
    return XK_Insert;
  case FL_Menu:
    return XK_Menu;
  case FL_Help:
    return XK_Help;
  case FL_Num_Lock:
    return XK_Num_Lock;
  case FL_Shift_L:
    return XK_Shift_L;
  case FL_Shift_R:
    return XK_Shift_R;
  case FL_Control_L:
    return XK_Control_L;
  case FL_Control_R:
    return XK_Control_R;
  case FL_Caps_Lock:
    return XK_Caps_Lock;
  case FL_Meta_L:
    return XK_Super_L;
  case FL_Meta_R:
    return XK_Super_R;
  case FL_Alt_L:
    return XK_Alt_L;
  case FL_Alt_R:
    return XK_Alt_R;
  case FL_Delete:
    return XK_Delete;
  case FL_KP_Enter:
    return XK_KP_Enter;
  case FL_KP + '=':
    return XK_KP_Equal;
  case FL_KP + '*':
    return XK_KP_Multiply;
  case FL_KP + '+':
    return XK_KP_Add;
  case FL_KP + ',':
    return XK_KP_Separator;
  case FL_KP + '-':
    return XK_KP_Subtract;
  case FL_KP + '.':
    return XK_KP_Decimal;
  case FL_KP + '/':
    return XK_KP_Divide;
#ifdef HAVE_FLTK_MEDIAKEYS
  case FL_Volume_Down:
    return XF86XK_AudioLowerVolume;
  case FL_Volume_Mute:
    return XF86XK_AudioMute;
  case FL_Volume_Up:
    return XF86XK_AudioRaiseVolume;
  case FL_Media_Play:
    return XF86XK_AudioPlay;
  case FL_Media_Stop:
    return XF86XK_AudioStop;
  case FL_Media_Prev:
    return XF86XK_AudioPrev;
  case FL_Media_Next:
    return XF86XK_AudioNext;
  case FL_Home_Page:
    return XF86XK_HomePage;
  case FL_Mail:
    return XF86XK_Mail;
  case FL_Search:
    return XF86XK_Search;
  case FL_Back:
    return XF86XK_Back;
  case FL_Forward:
    return XF86XK_Forward;
  case FL_Stop:
    return XF86XK_Stop;
  case FL_Refresh:
    return XF86XK_Refresh;
  case FL_Sleep:
    return XF86XK_Sleep;
  case FL_Favorites:
    return XF86XK_Favorites;
#endif
  case XK_ISO_Level3_Shift:
    // FLTK tends to let this one leak through on X11...
    return XK_ISO_Level3_Shift;
  case XK_Multi_key:
    // Same for this...
    return XK_Multi_key;
  }

  // Unknown special key?
  if (keyTextLen == 0) {
    vlog.error(_("Unknown FLTK key code %d (0x%04x)"), keyCode, keyCode);
    return NoSymbol;
  }

  // Control character?
  if ((keyTextLen == 1) && ((keyText[0] < 0x20) | (keyText[0] == 0x7f))) {
    if (keyText[0] == 0x00)
      return XK_2;
    else if (keyText[0] < 0x1b) {
      if (!!Fl::event_state(FL_SHIFT) != !!Fl::event_state(FL_CAPS_LOCK))
        return keyText[0] + XK_A - 0x01;
      else
        return keyText[0] + XK_a - 0x01;
    } else if (keyText[0] < 0x20)
      return keyText[0] + XK_3 - 0x1b;
    else
      return XK_8;
  }

  // Look up the symbol the key produces and translate that from Unicode
  // to a X11 keysym.
  if (fl_utf_nb_char((const unsigned char*)keyText, strlen(keyText)) != 1) {
    vlog.error(_("Multiple characters given for key code %d (0x%04x): '%s'"),
               keyCode, keyCode, keyText);
    return NoSymbol;
  }

  ucs = fl_utf8decode(keyText, NULL, NULL);
  return ucs2keysym(ucs);
}


void Viewport::handleFLTKKeyPress(void)
{
  rdr::U32 keySym;

#ifdef HAVE_FLTK_XHANDLERS
  return;
#endif

  keySym = translateKeyEvent();
  if (keySym == NoSymbol)
    return;

  handleKeyPress(Fl::event_original_key(), keySym);
}


void Viewport::initContextMenu()
{
  size_t i;
  Fl_Menu_Item items[] = {
    { strdup(_("E&xit viewer")), 0, NULL, (void*)ID_EXIT, FL_MENU_DIVIDER },

#ifdef HAVE_FLTK_FULLSCREEN
    { strdup(_("&Full screen")), 0, NULL, (void*)ID_FULLSCREEN, FL_MENU_TOGGLE },
#endif
    { strdup(_("Resize &window to session")), 0, NULL, (void*)ID_RESIZE, FL_MENU_DIVIDER },

    { strdup(_("&Ctrl")), 0, NULL, (void*)ID_CTRL, FL_MENU_TOGGLE },
    { strdup(_("&Alt")), 0, NULL, (void*)ID_ALT, FL_MENU_TOGGLE },

    { strdup("Menu key"), 0, NULL, (void*)ID_MENUKEY, 0 },
    { strdup("Secret shortcut menu key"), menuKeyCode, NULL, (void*)ID_MENUKEY, FL_MENU_INVISIBLE },
    { strdup(_("Send Ctrl-Alt-&Del")), 0, NULL, (void*)ID_CTRLALTDEL, FL_MENU_DIVIDER },

    { strdup(_("&Refresh screen")), 0, NULL, (void*)ID_REFRESH, FL_MENU_DIVIDER },

    { strdup(_("&Options...")), 0, NULL, (void*)ID_OPTIONS, 0 },
    { strdup(_("Connection &info...")), 0, NULL, (void*)ID_INFO, 0 },
    { strdup(_("About &TigerVNC viewer...")), 0, NULL, (void*)ID_ABOUT, FL_MENU_DIVIDER },

    { strdup(_("Dismiss &menu")), 0, NULL, (void*)ID_DISMISS, 0 },

    {0}
  };

  // Update any state
  for (i = 0;i < sizeof(items)/sizeof(items[0]);i++) {
    switch ((intptr_t)items[i].user_data_) {
#ifdef HAVE_FLTK_FULLSCREEN
    case ID_FULLSCREEN:
      if (window()->fullscreen_active())
        items[i].flags |= FL_MENU_VALUE;
      break;
    case ID_RESIZE:
      if (window()->fullscreen_active())
        items[i].flags |= FL_MENU_INACTIVE;
      break;
#endif
    case ID_CTRL:
      if (menuCtrlKey)
        items[i].flags |= FL_MENU_VALUE;
      break;
    case ID_ALT:
      if (menuAltKey)
        items[i].flags |= FL_MENU_VALUE;
      break;
    case ID_MENUKEY:
      // The menu key needs to have its fields updated, or the entries hidden
      if (!menuKeySym)
        items[i].flags |= FL_MENU_INACTIVE | FL_MENU_INVISIBLE;
      else if (!(items[i].flags & FL_MENU_INVISIBLE)) {
        char sendMenuKey[64];
        snprintf(sendMenuKey, 64, _("Send %s"), (const char *)menuKey);
        free((void*)items[i].text);
        items[i].text = strdup(sendMenuKey);
      }
      break;
    }
  }

  contextMenu->copy(items);
}


void Viewport::popupContextMenu()
{
  const Fl_Menu_Item *m;
  char buffer[1024];

  // Make sure the menu is reset to its initial state between goes or
  // it will start up highlighting the previously selected entry.
  contextMenu->value(-1);

  // initialize context menu before display
  initContextMenu();

  // Unfortunately FLTK doesn't reliably restore the mouse pointer for
  // menus, so we have to help it out.
#ifdef HAVE_FLTK_CURSOR
  if (Fl::belowmouse() == this)
    window()->cursor(FL_CURSOR_DEFAULT);
#endif

  m = contextMenu->popup();

  // Back to our proper mouse pointer.
#ifdef HAVE_FLTK_CURSOR
  if ((Fl::belowmouse() == this) && cursor)
    window()->cursor(cursor, cursorHotspot.x, cursorHotspot.y);
#endif

  if (m == NULL)
    return;

  switch (m->argument()) {
  case ID_EXIT:
    exit_vncviewer();
    break;
#ifdef HAVE_FLTK_FULLSCREEN
  case ID_FULLSCREEN:
    if (window()->fullscreen_active())
      window()->fullscreen_off();
    else
      ((DesktopWindow*)window())->fullscreen_on();
    break;
#endif
  case ID_RESIZE:
#ifdef HAVE_FLTK_FULLSCREEN
    if (window()->fullscreen_active())
      break;
#endif
    window()->size(w(), h());
    break;
  case ID_CTRL:
    if (m->value())
      handleKeyPress(fakeKeyBase + 0, XK_Control_L);
    else
      handleKeyRelease(fakeKeyBase + 0);
    menuCtrlKey = !menuCtrlKey;
    break;
  case ID_ALT:
    if (m->value())
      handleKeyPress(fakeKeyBase + 1, XK_Alt_L);
    else
      handleKeyRelease(fakeKeyBase + 1);
    menuAltKey = !menuAltKey;
    break;
  case ID_MENUKEY:
    handleKeyPress(fakeKeyBase + 2, menuKeySym);
    handleKeyRelease(fakeKeyBase + 2);
    break;
  case ID_CTRLALTDEL:
    handleKeyPress(fakeKeyBase + 3, XK_Control_L);
    handleKeyPress(fakeKeyBase + 4, XK_Alt_L);
    handleKeyPress(fakeKeyBase + 5, XK_Delete);

    handleKeyRelease(fakeKeyBase + 5);
    handleKeyRelease(fakeKeyBase + 4);
    handleKeyRelease(fakeKeyBase + 3);
    break;
  case ID_REFRESH:
    cc->refreshFramebuffer();
    break;
  case ID_OPTIONS:
    OptionsDialog::showDialog();
    break;
  case ID_INFO:
    if (fltk_escape(cc->connectionInfo(), buffer, sizeof(buffer)) < sizeof(buffer)) {
      fl_message_title(_("VNC connection info"));
      fl_message("%s", buffer);
    }
    break;
  case ID_ABOUT:
    about_vncviewer();
    break;
  case ID_DISMISS:
    // Don't need to do anything
    break;
  }
}


void Viewport::setMenuKey()
{
  getMenuKey(&menuKeyCode, &menuKeySym);
}


void Viewport::handleOptions(void *data)
{
  Viewport *self = (Viewport*)data;

  self->setMenuKey();
}
