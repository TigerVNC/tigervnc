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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <rfb/CMsgWriter.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rfb/ledStates.h>

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

#if ! (defined(WIN32) || defined(__APPLE__))
#include <X11/XKBlib.h>
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

#include <FL/fl_draw.H>
#include <FL/fl_ask.H>

#include <FL/Fl_Menu.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/x.H>

#if !defined(WIN32) && !defined(__APPLE__)
#include <X11/XKBlib.h>
extern const struct _code_map_xkb_to_qnum {
  const char * from;
  const unsigned short to;
} code_map_xkb_to_qnum[];
extern const unsigned int code_map_xkb_to_qnum_len;

static int code_map_keycode_to_qnum[256];
#endif

#ifdef __APPLE__
#include "cocoa.h"
extern const unsigned short code_map_osx_to_qnum[];
extern const unsigned int code_map_osx_to_qnum_len;
#endif

#ifdef WIN32
#include "win32.h"
#endif


using namespace rfb;
using namespace rdr;

static rfb::LogWriter vlog("Viewport");

// Menu constants

enum { ID_DISCONNECT, ID_FULLSCREEN, ID_MINIMIZE, ID_RESIZE,
       ID_CTRL, ID_ALT, ID_MENUKEY, ID_CTRLALTDEL,
       ID_REFRESH, ID_OPTIONS, ID_INFO, ID_ABOUT };

// Used to detect fake input (0xaa is not a real key)
#ifdef WIN32
static const WORD SCAN_FAKE = 0xaa;
#endif

Viewport::Viewport(int w, int h, const rfb::PixelFormat& serverPF, CConn* cc_)
  : Fl_Widget(0, 0, w, h), cc(cc_), frameBuffer(NULL),
    lastPointerPos(0, 0), lastButtonMask(0),
#ifdef WIN32
    altGrArmed(false),
#endif
    firstLEDState(true),
    pendingServerClipboard(false), pendingClientClipboard(false),
    menuCtrlKey(false), menuAltKey(false), cursor(NULL)
{
#if !defined(WIN32) && !defined(__APPLE__)
  XkbDescPtr xkb;
  Status status;

  xkb = XkbGetMap(fl_display, 0, XkbUseCoreKbd);
  if (!xkb)
    throw rfb::Exception("XkbGetMap");

  status = XkbGetNames(fl_display, XkbKeyNamesMask, xkb);
  if (status != Success)
    throw rfb::Exception("XkbGetNames");

  memset(code_map_keycode_to_qnum, 0, sizeof(code_map_keycode_to_qnum));
  for (KeyCode keycode = xkb->min_key_code;
       keycode < xkb->max_key_code;
       keycode++) {
    const char *keyname = xkb->names->keys[keycode].name;
    unsigned short rfbcode;

    if (keyname[0] == '\0')
      continue;

    rfbcode = 0;
    for (unsigned i = 0;i < code_map_xkb_to_qnum_len;i++) {
        if (strncmp(code_map_xkb_to_qnum[i].from,
                    keyname, XkbKeyNameLength) == 0) {
            rfbcode = code_map_xkb_to_qnum[i].to;
            break;
        }
    }
    if (rfbcode != 0)
        code_map_keycode_to_qnum[keycode] = rfbcode;
    else
        vlog.debug("No key mapping for key %.4s", keyname);
  }

  XkbFreeKeyboard(xkb, 0, True);
#endif

  Fl::add_clipboard_notify(handleClipboardChange, this);

  // We need to intercept keyboard events early
  Fl::add_system_handler(handleSystemEvent, this);

  frameBuffer = new PlatformPixelBuffer(w, h);
  assert(frameBuffer);
  cc->setFramebuffer(frameBuffer);

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

  // Make sure we have an initial blank cursor set
  setCursor(0, 0, rfb::Point(0, 0), NULL);
}


Viewport::~Viewport()
{
  // Unregister all timeouts in case they get a change tro trigger
  // again later when this object is already gone.
  Fl::remove_timeout(handlePointerTimeout, this);
#ifdef WIN32
  Fl::remove_timeout(handleAltGrTimeout, this);
#endif

  Fl::remove_system_handler(handleSystemEvent);

  Fl::remove_clipboard_notify(handleClipboardChange);

  OptionsDialog::removeCallback(handleOptions);

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

void Viewport::updateWindow()
{
  Rect r;

  r = frameBuffer->getDamage();
  damage(FL_DAMAGE_USER1, r.tl.x + x(), r.tl.y + y(), r.width(), r.height());
}

static const char * dotcursor_xpm[] = {
  "5 5 2 1",
  ".	c #000000",
  " 	c #FFFFFF",
  "     ",
  " ... ",
  " ... ",
  " ... ",
  "     "};

void Viewport::setCursor(int width, int height, const Point& hotspot,
                         const rdr::U8* data)
{
  int i;

  if (cursor) {
    if (!cursor->alloc_array)
      delete [] cursor->array;
    delete cursor;
  }

  for (i = 0; i < width*height; i++)
    if (data[i*4 + 3] != 0) break;

  if ((i == width*height) && dotWhenNoCursor) {
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
      U8 *buffer = new U8[width * height * 4];
      memcpy(buffer, data, width * height * 4);
      cursor = new Fl_RGB_Image(buffer, width, height, 4);
      cursorHotspot = hotspot;
    }
  }

  if (Fl::belowmouse() == this)
    window()->cursor(cursor, cursorHotspot.x, cursorHotspot.y);
}

void Viewport::handleClipboardRequest()
{
  Fl::paste(*this, clipboardSource);
}

void Viewport::handleClipboardAnnounce(bool available)
{
  if (!acceptClipboard)
    return;

  if (!available) {
    vlog.debug("Clipboard is no longer available on server");
    pendingServerClipboard = false;
    return;
  }

  pendingClientClipboard = false;

  if (!hasFocus()) {
    vlog.debug("Got notification of new clipboard on server whilst not focused, will request data later");
    pendingServerClipboard = true;
    return;
  }

  vlog.debug("Got notification of new clipboard on server, requesting data");
  cc->requestClipboard();
}

void Viewport::handleClipboardData(const char* data)
{
  size_t len;

  if (!hasFocus())
    return;

  len = strlen(data);

  vlog.debug("Got clipboard data (%d bytes)", (int)len);

  // RFB doesn't have separate selection and clipboard concepts, so we
  // dump the data into both variants.
#if !defined(WIN32) && !defined(__APPLE__)
  if (setPrimary)
    Fl::copy(data, len, 0);
#endif
  Fl::copy(data, len, 1);
}

void Viewport::setLEDState(unsigned int state)
{
  vlog.debug("Got server LED state: 0x%08x", state);

  // The first message is just considered to be the server announcing
  // support for this extension. We will push our state to sync up the
  // server when we get focus. If we already have focus we need to push
  // it here though.
  if (firstLEDState) {
    firstLEDState = false;
    if (hasFocus())
      pushLEDState();
    return;
  }

  if (!hasFocus())
    return;

#if defined(WIN32)
  INPUT input[6];
  UINT count;
  UINT ret;

  memset(input, 0, sizeof(input));
  count = 0;

  if (!!(state & ledCapsLock) != !!(GetKeyState(VK_CAPITAL) & 0x1)) {
    input[count].type = input[count+1].type = INPUT_KEYBOARD;
    input[count].ki.wVk = input[count+1].ki.wVk = VK_CAPITAL;
    input[count].ki.wScan = input[count+1].ki.wScan = SCAN_FAKE;
    input[count].ki.dwFlags = 0;
    input[count+1].ki.dwFlags = KEYEVENTF_KEYUP;
    count += 2;
  }

  if (!!(state & ledNumLock) != !!(GetKeyState(VK_NUMLOCK) & 0x1)) {
    input[count].type = input[count+1].type = INPUT_KEYBOARD;
    input[count].ki.wVk = input[count+1].ki.wVk = VK_NUMLOCK;
    input[count].ki.wScan = input[count+1].ki.wScan = SCAN_FAKE;
    input[count].ki.dwFlags = KEYEVENTF_EXTENDEDKEY;
    input[count+1].ki.dwFlags = KEYEVENTF_KEYUP | KEYEVENTF_EXTENDEDKEY;
    count += 2;
  }

  if (!!(state & ledScrollLock) != !!(GetKeyState(VK_SCROLL) & 0x1)) {
    input[count].type = input[count+1].type = INPUT_KEYBOARD;
    input[count].ki.wVk = input[count+1].ki.wVk = VK_SCROLL;
    input[count].ki.wScan = input[count+1].ki.wScan = SCAN_FAKE;
    input[count].ki.dwFlags = 0;
    input[count+1].ki.dwFlags = KEYEVENTF_KEYUP;
    count += 2;
  }

  if (count == 0)
    return;

  ret = SendInput(count, input, sizeof(*input));
  if (ret < count)
    vlog.error(_("Failed to update keyboard LED state: %lu"), GetLastError());
#elif defined(__APPLE__)
  int ret;

  ret = cocoa_set_caps_lock_state(state & ledCapsLock);
  if (ret != 0) {
    vlog.error(_("Failed to update keyboard LED state: %d"), ret);
    return;
  }

  ret = cocoa_set_num_lock_state(state & ledNumLock);
  if (ret != 0) {
    vlog.error(_("Failed to update keyboard LED state: %d"), ret);
    return;
  }

  // No support for Scroll Lock //

#else
  unsigned int affect, values;
  unsigned int mask;

  Bool ret;

  affect = values = 0;

  affect |= LockMask;
  if (state & ledCapsLock)
    values |= LockMask;

  mask = getModifierMask(XK_Num_Lock);
  affect |= mask;
  if (state & ledNumLock)
    values |= mask;

  mask = getModifierMask(XK_Scroll_Lock);
  affect |= mask;
  if (state & ledScrollLock)
    values |= mask;

  ret = XkbLockModifiers(fl_display, XkbUseCoreKbd, affect, values);
  if (!ret)
    vlog.error(_("Failed to update keyboard LED state"));
#endif
}

void Viewport::pushLEDState()
{
  unsigned int state;

  // Server support?
  if (cc->server.ledState() == ledUnknown)
    return;

  state = 0;

#if defined(WIN32)
  if (GetKeyState(VK_CAPITAL) & 0x1)
    state |= ledCapsLock;
  if (GetKeyState(VK_NUMLOCK) & 0x1)
    state |= ledNumLock;
  if (GetKeyState(VK_SCROLL) & 0x1)
    state |= ledScrollLock;
#elif defined(__APPLE__)
  int ret;
  bool on;

  ret = cocoa_get_caps_lock_state(&on);
  if (ret != 0) {
    vlog.error(_("Failed to get keyboard LED state: %d"), ret);
    return;
  }
  if (on)
    state |= ledCapsLock;

  ret = cocoa_get_num_lock_state(&on);
  if (ret != 0) {
    vlog.error(_("Failed to get keyboard LED state: %d"), ret);
    return;
  }
  if (on)
    state |= ledNumLock;

  // No support for Scroll Lock //
  state |= (cc->server.ledState() & ledScrollLock);

#else
  unsigned int mask;

  Status status;
  XkbStateRec xkbState;

  status = XkbGetState(fl_display, XkbUseCoreKbd, &xkbState);
  if (status != Success) {
    vlog.error(_("Failed to get keyboard LED state: %d"), status);
    return;
  }

  if (xkbState.locked_mods & LockMask)
    state |= ledCapsLock;

  mask = getModifierMask(XK_Num_Lock);
  if (xkbState.locked_mods & mask)
    state |= ledNumLock;

  mask = getModifierMask(XK_Scroll_Lock);
  if (xkbState.locked_mods & mask)
    state |= ledScrollLock;
#endif

  if ((state & ledCapsLock) != (cc->server.ledState() & ledCapsLock)) {
    vlog.debug("Inserting fake CapsLock to get in sync with server");
    handleKeyPress(0x3a, XK_Caps_Lock);
    handleKeyRelease(0x3a);
  }
  if ((state & ledNumLock) != (cc->server.ledState() & ledNumLock)) {
    vlog.debug("Inserting fake NumLock to get in sync with server");
    handleKeyPress(0x45, XK_Num_Lock);
    handleKeyRelease(0x45);
  }
  if ((state & ledScrollLock) != (cc->server.ledState() & ledScrollLock)) {
    vlog.debug("Inserting fake ScrollLock to get in sync with server");
    handleKeyPress(0x46, XK_Scroll_Lock);
    handleKeyRelease(0x46);
  }
}


void Viewport::draw(Surface* dst)
{
  int X, Y, W, H;

  // Check what actually needs updating
  fl_clip_box(x(), y(), w(), h(), X, Y, W, H);
  if ((W == 0) || (H == 0))
    return;

  frameBuffer->draw(dst, X - x(), Y - y(), X, Y, W, H);
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
  if ((w != frameBuffer->width()) || (h != frameBuffer->height())) {
    vlog.debug("Resizing framebuffer from %dx%d to %dx%d",
               frameBuffer->width(), frameBuffer->height(), w, h);

    frameBuffer = new PlatformPixelBuffer(w, h);
    assert(frameBuffer);
    cc->setFramebuffer(frameBuffer);
  }

  Fl_Widget::resize(x, y, w, h);
}


int Viewport::handle(int event)
{
  char *filtered;
  int buttonMask, wheelMask;
  DownMap::const_iterator iter;

  switch (event) {
  case FL_PASTE:
    filtered = convertLF(Fl::event_text(), Fl::event_length());

    vlog.debug("Sending clipboard data (%d bytes)", (int)strlen(filtered));

    try {
      cc->sendClipboardData(filtered);
    } catch (rdr::Exception& e) {
      vlog.error("%s", e.str());
      abort_connection_with_unexpected_error(e);
    }

    strFree(filtered);

    return 1;

  case FL_ENTER:
    window()->cursor(cursor, cursorHotspot.x, cursorHotspot.y);
    // Yes, we would like some pointer events please!
    return 1;

  case FL_LEAVE:
    window()->cursor(FL_CURSOR_DEFAULT);
    // We want a last move event to help trigger edge stuff
    handlePointerEvent(Point(Fl::event_x() - x(), Fl::event_y() - y()), 0);
    return 1;

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
    Fl::disable_im();

    flushPendingClipboard();

    // We may have gotten our lock keys out of sync with the server
    // whilst we didn't have focus. Try to sort this out.
    pushLEDState();

    // Resend Ctrl/Alt if needed
    if (menuCtrlKey)
      handleKeyPress(0x1d, XK_Control_L);
    if (menuAltKey)
      handleKeyPress(0x38, XK_Alt_L);

    // Yes, we would like some focus please!
    return 1;

  case FL_UNFOCUS:
    // Release all keys that were pressed as that generally makes most
    // sense (e.g. Alt+Tab where we only see the Alt press)
    while (!downKeySym.empty())
      handleKeyRelease(downKeySym.begin()->first);
    Fl::enable_im();
    return 1;

  case FL_KEYDOWN:
  case FL_KEYUP:
    // Just ignore these as keys were handled in the event handler
    return 1;
  }

  return Fl_Widget::handle(event);
}

void Viewport::sendPointerEvent(const rfb::Point& pos, int buttonMask)
{
  if (viewOnly)
      return;

  if ((pointerEventInterval == 0) || (buttonMask != lastButtonMask)) {
    try {
      cc->writer()->writePointerEvent(pos, buttonMask);
    } catch (rdr::Exception& e) {
      vlog.error("%s", e.str());
      abort_connection_with_unexpected_error(e);
    }
  } else {
    if (!Fl::has_timeout(handlePointerTimeout, this))
      Fl::add_timeout((double)pointerEventInterval/1000.0,
                      handlePointerTimeout, this);
  }
  lastPointerPos = pos;
  lastButtonMask = buttonMask;
}

bool Viewport::hasFocus()
{
  Fl_Widget* focus;

  focus = Fl::grab();
  if (!focus)
    focus = Fl::focus();

  return focus == this;
}

#if ! (defined(WIN32) || defined(__APPLE__))
unsigned int Viewport::getModifierMask(unsigned int keysym)
{
  XkbDescPtr xkb;
  unsigned int mask, keycode;
  XkbAction *act;

  mask = 0;

  xkb = XkbGetMap(fl_display, XkbAllComponentsMask, XkbUseCoreKbd);
  if (xkb == NULL)
    return 0;

  for (keycode = xkb->min_key_code; keycode <= xkb->max_key_code; keycode++) {
    unsigned int state_out;
    KeySym ks;

    XkbTranslateKeyCode(xkb, keycode, 0, &state_out, &ks);
    if (ks == NoSymbol)
      continue;

    if (ks == keysym)
      break;
  }

  // KeySym not mapped?
  if (keycode > xkb->max_key_code)
    goto out;

  act = XkbKeyAction(xkb, keycode, 0);
  if (act == NULL)
    goto out;
  if (act->type != XkbSA_LockMods)
    goto out;

  if (act->mods.flags & XkbSA_UseModMapMods)
    mask = xkb->map->modmap[keycode];
  else
    mask = act->mods.mask;

out:
  XkbFreeKeyboard(xkb, XkbAllComponentsMask, True);

  return mask;
}
#endif


void Viewport::handleClipboardChange(int source, void *data)
{
  Viewport *self = (Viewport *)data;

  assert(self);

  if (!sendClipboard)
    return;

#if !defined(WIN32) && !defined(__APPLE__)
  if (!sendPrimary && (source == 0))
    return;
#endif

  self->clipboardSource = source;

  self->pendingServerClipboard = false;

  if (!self->hasFocus()) {
    vlog.debug("Local clipboard changed whilst not focused, will notify server later");
    self->pendingClientClipboard = true;
    // Clear any older client clipboard from the server
    self->cc->announceClipboard(false);
    return;
  }

  vlog.debug("Local clipboard changed, notifying server");
  try {
    self->cc->announceClipboard(true);
  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
    abort_connection_with_unexpected_error(e);
  }
}


void Viewport::flushPendingClipboard()
{
  if (pendingServerClipboard) {
    vlog.debug("Focus regained after remote clipboard change, requesting data");
    try {
      cc->requestClipboard();
    } catch (rdr::Exception& e) {
      vlog.error("%s", e.str());
      abort_connection_with_unexpected_error(e);
    }
  }
  if (pendingClientClipboard) {
    vlog.debug("Focus regained after local clipboard change, notifying server");
    try {
      cc->announceClipboard(true);
    } catch (rdr::Exception& e) {
      vlog.error("%s", e.str());
      abort_connection_with_unexpected_error(e);
    }
  }

  pendingServerClipboard = false;
  pendingClientClipboard = false;
}


void Viewport::handlePointerEvent(const rfb::Point& pos, int buttonMask)
{
  filterPointerEvent(pos, buttonMask);
}


void Viewport::handlePointerTimeout(void *data)
{
  Viewport *self = (Viewport *)data;

  assert(self);

  try {
    self->cc->writer()->writePointerEvent(self->lastPointerPos,
                                          self->lastButtonMask);
  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
    abort_connection_with_unexpected_error(e);
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

  if (keyCode == 0) {
    vlog.error(_("No key code specified on key press"));
    return;
  }

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
    // Fake keycode?
    if (keyCode > 0xff)
      cc->writer()->writeKeyEvent(keySym, 0, true);
    else
      cc->writer()->writeKeyEvent(keySym, keyCode, true);
  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
    abort_connection_with_unexpected_error(e);
  }
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
    if (keyCode > 0xff)
      cc->writer()->writeKeyEvent(iter->second, 0, false);
    else
      cc->writer()->writeKeyEvent(iter->second, keyCode, false);
  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
    abort_connection_with_unexpected_error(e);
  }

  downKeySym.erase(iter);
}


int Viewport::handleSystemEvent(void *event, void *data)
{
  Viewport *self = (Viewport *)data;

  assert(self);

  if (!self->hasFocus())
    return 0;

  assert(event);

#if defined(WIN32)
  MSG *msg = (MSG*)event;

  if ((msg->message == WM_MOUSEMOVE) ||
      (msg->message == WM_LBUTTONDOWN) ||
      (msg->message == WM_LBUTTONUP) ||
      (msg->message == WM_RBUTTONDOWN) ||
      (msg->message == WM_RBUTTONUP) ||
      (msg->message == WM_MBUTTONDOWN) ||
      (msg->message == WM_MBUTTONUP) ||
      (msg->message == WM_MOUSEWHEEL) ||
      (msg->message == WM_MOUSEHWHEEL)) {
    // We can't get a mouse event in the middle of an AltGr sequence, so
    // abort that detection
    if (self->altGrArmed)
      self->resolveAltGrDetection(false);

    return 0; // We didn't really consume the mouse event
  } else if ((msg->message == WM_KEYDOWN) || (msg->message == WM_SYSKEYDOWN)) {
    UINT vKey;
    bool isExtended;
    int keyCode;
    rdr::U32 keySym;

    vKey = msg->wParam;
    isExtended = (msg->lParam & (1 << 24)) != 0;

    keyCode = ((msg->lParam >> 16) & 0xff);

    // Windows' touch keyboard doesn't set a scan code for the Alt
    // portion of the AltGr sequence, so we need to help it out
    if (!isExtended && (keyCode == 0x00) && (vKey == VK_MENU)) {
      isExtended = true;
      keyCode = 0x38;
    }

    // Windows doesn't have a proper AltGr, but handles it using fake
    // Ctrl+Alt. However the remote end might not be Windows, so we need
    // to merge those in to a single AltGr event. We detect this case
    // by seeing the two key events directly after each other with a very
    // short time between them (<50ms) and supress the Ctrl event.
    if (self->altGrArmed) {
      bool altPressed = isExtended &&
                        (keyCode == 0x38) &&
                        (vKey == VK_MENU) &&
                        ((msg->time - self->altGrCtrlTime) < 50);
      self->resolveAltGrDetection(altPressed);
    }

    if (keyCode == SCAN_FAKE) {
      vlog.debug("Ignoring fake key press (virtual key 0x%02x)", vKey);
      return 1;
    }

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

    if (keyCode & ~0x7f) {
      vlog.error(_("Invalid scan code 0x%02x"), (int)keyCode);
      return 1;
    }

    if (isExtended)
      keyCode |= 0x80;


    // Fortunately RFB and Windows use the same scan code set (mostly),
    // so there is no conversion needed
    // (as long as we encode the extended keys with the high bit)

    // However Pause sends a code that conflicts with NumLock, so use
    // the code most RFB implementations use (part of the sequence for
    // Ctrl+Pause, i.e. Break)
    if (keyCode == 0x45)
      keyCode = 0xc6;

    // And NumLock incorrectly has the extended bit set
    if (keyCode == 0xc5)
      keyCode = 0x45;

    // And Alt+PrintScreen (i.e. SysRq) sends a different code than
    // PrintScreen
    if (keyCode == 0xb7)
      keyCode = 0x54;

    keySym = win32_vkey_to_keysym(vKey, isExtended);
    if (keySym == NoSymbol) {
      if (isExtended)
        vlog.error(_("No symbol for extended virtual key 0x%02x"), (int)vKey);
      else
        vlog.error(_("No symbol for virtual key 0x%02x"), (int)vKey);
    }

    // Windows sends the same vKey for both shifts, so we need to look
    // at the scan code to tell them apart
    if ((keySym == XK_Shift_L) && (keyCode == 0x36))
      keySym = XK_Shift_R;

    // AltGr handling (see above)
    if (win32_has_altgr()) {
      if ((keyCode == 0xb8) && (keySym == XK_Alt_R))
        keySym = XK_ISO_Level3_Shift;

      // Possible start of AltGr sequence?
      if ((keyCode == 0x1d) && (keySym == XK_Control_L)) {
        self->altGrArmed = true;
        self->altGrCtrlTime = msg->time;
        Fl::add_timeout(0.1, handleAltGrTimeout, self);
        return 1;
      }
    }

    self->handleKeyPress(keyCode, keySym);

    // We don't get reliable WM_KEYUP for these
    switch (keySym) {
    case XK_Zenkaku_Hankaku:
    case XK_Eisu_toggle:
    case XK_Katakana:
    case XK_Hiragana:
    case XK_Romaji:
      self->handleKeyRelease(keyCode);
    }

    return 1;
  } else if ((msg->message == WM_KEYUP) || (msg->message == WM_SYSKEYUP)) {
    UINT vKey;
    bool isExtended;
    int keyCode;

    vKey = msg->wParam;
    isExtended = (msg->lParam & (1 << 24)) != 0;

    keyCode = ((msg->lParam >> 16) & 0xff);

    // Touch keyboard AltGr (see above)
    if (!isExtended && (keyCode == 0x00) && (vKey == VK_MENU)) {
      isExtended = true;
      keyCode = 0x38;
    }

    // We can't get a release in the middle of an AltGr sequence, so
    // abort that detection
    if (self->altGrArmed)
      self->resolveAltGrDetection(false);

    if (keyCode == SCAN_FAKE) {
      vlog.debug("Ignoring fake key release (virtual key 0x%02x)", vKey);
      return 1;
    }

    if (keyCode == 0x00)
      keyCode = MapVirtualKey(vKey, MAPVK_VK_TO_VSC);
    if (isExtended)
      keyCode |= 0x80;
    if (keyCode == 0x45)
      keyCode = 0xc6;
    if (keyCode == 0xc5)
      keyCode = 0x45;
    if (keyCode == 0xb7)
      keyCode = 0x54;

    self->handleKeyRelease(keyCode);

    // Windows has a rather nasty bug where it won't send key release
    // events for a Shift button if the other Shift is still pressed
    if ((keyCode == 0x2a) || (keyCode == 0x36)) {
      if (self->downKeySym.count(0x2a))
        self->handleKeyRelease(0x2a);
      if (self->downKeySym.count(0x36))
        self->handleKeyRelease(0x36);
    }

    return 1;
  }
#elif defined(__APPLE__)
  if (cocoa_is_keyboard_event(event)) {
    int keyCode;

    keyCode = cocoa_event_keycode(event);
    if ((unsigned)keyCode >= code_map_osx_to_qnum_len)
      keyCode = 0;
    else
      keyCode = code_map_osx_to_qnum[keyCode];

    if (cocoa_is_key_press(event)) {
      rdr::U32 keySym;

      keySym = cocoa_event_keysym(event);
      if (keySym == NoSymbol) {
        vlog.error(_("No symbol for key code 0x%02x (in the current state)"),
                   (int)keyCode);
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
    int keycode;
    char str;
    KeySym keysym;

    keycode = code_map_keycode_to_qnum[xevent->xkey.keycode];

    // Generate a fake keycode just for tracking if we can't figure
    // out the proper one
    if (keycode == 0)
        keycode = 0x100 | xevent->xkey.keycode;

    XLookupString(&xevent->xkey, &str, 1, &keysym, NULL);
    if (keysym == NoSymbol) {
      vlog.error(_("No symbol for key code %d (in the current state)"),
                 (int)xevent->xkey.keycode);
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

    self->handleKeyPress(keycode, keysym);
    return 1;
  } else if (xevent->type == KeyRelease) {
    int keycode = code_map_keycode_to_qnum[xevent->xkey.keycode];
    if (keycode == 0)
        keycode = 0x100 | xevent->xkey.keycode;
    self->handleKeyRelease(keycode);
    return 1;
  }
#endif

  return 0;
}

#ifdef WIN32
void Viewport::handleAltGrTimeout(void *data)
{
  Viewport *self = (Viewport *)data;

  assert(self);

  self->altGrArmed = false;
  self->handleKeyPress(0x1d, XK_Control_L);
}

void Viewport::resolveAltGrDetection(bool isAltGrSequence)
{
  altGrArmed = false;
  Fl::remove_timeout(handleAltGrTimeout);
  // when it's not an AltGr sequence we can't supress the Ctrl anymore
  if (!isAltGrSequence)
    handleKeyPress(0x1d, XK_Control_L);
}
#endif

void Viewport::initContextMenu()
{
  contextMenu->clear();

  fltk_menu_add(contextMenu, p_("ContextMenu|", "Dis&connect"),
                0, NULL, (void*)ID_DISCONNECT, FL_MENU_DIVIDER);

  fltk_menu_add(contextMenu, p_("ContextMenu|", "&Full screen"),
                0, NULL, (void*)ID_FULLSCREEN,
                FL_MENU_TOGGLE | (window()->fullscreen_active()?FL_MENU_VALUE:0));
  fltk_menu_add(contextMenu, p_("ContextMenu|", "Minimi&ze"),
                0, NULL, (void*)ID_MINIMIZE, 0);
  fltk_menu_add(contextMenu, p_("ContextMenu|", "Resize &window to session"),
                0, NULL, (void*)ID_RESIZE,
                (window()->fullscreen_active()?FL_MENU_INACTIVE:0) |
                FL_MENU_DIVIDER);

  fltk_menu_add(contextMenu, p_("ContextMenu|", "&Ctrl"),
                0, NULL, (void*)ID_CTRL,
                FL_MENU_TOGGLE | (menuCtrlKey?FL_MENU_VALUE:0));
  fltk_menu_add(contextMenu, p_("ContextMenu|", "&Alt"),
                0, NULL, (void*)ID_ALT,
                FL_MENU_TOGGLE | (menuAltKey?FL_MENU_VALUE:0));

  if (menuKeySym) {
    char sendMenuKey[64];
    snprintf(sendMenuKey, 64, p_("ContextMenu|", "Send %s"), (const char *)menuKey);
    fltk_menu_add(contextMenu, sendMenuKey, 0, NULL, (void*)ID_MENUKEY, 0);
    fltk_menu_add(contextMenu, "Secret shortcut menu key", menuKeyFLTK, NULL,
                  (void*)ID_MENUKEY, FL_MENU_INVISIBLE);
  }

  fltk_menu_add(contextMenu, p_("ContextMenu|", "Send Ctrl-Alt-&Del"),
                0, NULL, (void*)ID_CTRLALTDEL, FL_MENU_DIVIDER);

  fltk_menu_add(contextMenu, p_("ContextMenu|", "&Refresh screen"),
                0, NULL, (void*)ID_REFRESH, FL_MENU_DIVIDER);

  fltk_menu_add(contextMenu, p_("ContextMenu|", "&Options..."),
                0, NULL, (void*)ID_OPTIONS, 0);
  fltk_menu_add(contextMenu, p_("ContextMenu|", "Connection &info..."),
                0, NULL, (void*)ID_INFO, 0);
  fltk_menu_add(contextMenu, p_("ContextMenu|", "About &TigerVNC viewer..."),
                0, NULL, (void*)ID_ABOUT, 0);
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
  if (Fl::belowmouse() == this)
    window()->cursor(FL_CURSOR_DEFAULT);

  // FLTK also doesn't switch focus properly for menus
  handle(FL_UNFOCUS);

  m = contextMenu->popup();

  handle(FL_FOCUS);

  // Back to our proper mouse pointer.
  if (Fl::belowmouse())
    window()->cursor(cursor, cursorHotspot.x, cursorHotspot.y);

  if (m == NULL)
    return;

  switch (m->argument()) {
  case ID_DISCONNECT:
    disconnect();
    break;
  case ID_FULLSCREEN:
    if (window()->fullscreen_active())
      window()->fullscreen_off();
    else
      ((DesktopWindow*)window())->fullscreen_on();
    break;
  case ID_MINIMIZE:
    window()->iconize();
    break;
  case ID_RESIZE:
    if (window()->fullscreen_active())
      break;
    window()->size(w(), h());
    break;
  case ID_CTRL:
    if (m->value())
      handleKeyPress(0x1d, XK_Control_L);
    else
      handleKeyRelease(0x1d);
    menuCtrlKey = !menuCtrlKey;
    break;
  case ID_ALT:
    if (m->value())
      handleKeyPress(0x38, XK_Alt_L);
    else
      handleKeyRelease(0x38);
    menuAltKey = !menuAltKey;
    break;
  case ID_MENUKEY:
    handleKeyPress(menuKeyCode, menuKeySym);
    handleKeyRelease(menuKeyCode);
    break;
  case ID_CTRLALTDEL:
    handleKeyPress(0x1d, XK_Control_L);
    handleKeyPress(0x38, XK_Alt_L);
    handleKeyPress(0xd3, XK_Delete);

    handleKeyRelease(0xd3);
    handleKeyRelease(0x38);
    handleKeyRelease(0x1d);
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
  }
}


void Viewport::setMenuKey()
{
  getMenuKey(&menuKeyFLTK, &menuKeyCode, &menuKeySym);
}


void Viewport::handleOptions(void *data)
{
  Viewport *self = (Viewport*)data;

  self->setMenuKey();
  // FIXME: Need to recheck cursor for dotWhenNoCursor
}
