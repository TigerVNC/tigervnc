/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <FL/fl_draw.H>

#include <rfb/CMsgWriter.h>
#include <rfb/LogWriter.h>

// FLTK can pull in the X11 headers on some systems
#ifndef XK_VoidSymbol
#define XK_MISCELLANY
#include <rfb/keysymdef.h>
#endif

#include "DesktopWindow.h"
#include "CConn.h"
#include "i18n.h"
#include "parameters.h"
#include "keysym2ucs.h"

using namespace rfb;

extern void exit_vncviewer();

static rfb::LogWriter vlog("DesktopWindow");

DesktopWindow::DesktopWindow(int w, int h, const char *name,
                             const rfb::PixelFormat& serverPF,
                             CConn* cc_)
  : Fl_Window(w, h), cc(cc_), frameBuffer(NULL), pixelTrans(NULL),
    lastPointerPos(0, 0), lastButtonMask(0)
{
  callback(handleClose, this);

  setName(name);

  frameBuffer = new ManagedPixelBuffer(getPreferredPF(), w, h);
  assert(frameBuffer);

  setServerPF(serverPF);

  show();
}


DesktopWindow::~DesktopWindow()
{
  // Unregister all timeouts in case they get a change tro trigger
  // again later when this object is already gone.
  Fl::remove_timeout(handleUpdateTimeout, this);
  Fl::remove_timeout(handleColourMap, this);
  Fl::remove_timeout(handlePointerTimeout, this);

  delete frameBuffer;

  if (pixelTrans)
    delete pixelTrans;
}


void DesktopWindow::setServerPF(const rfb::PixelFormat& pf)
{
  if (pixelTrans)
    delete pixelTrans;
  pixelTrans = NULL;

  if (pf.equal(getPreferredPF()))
    return;

  pixelTrans = new PixelTransformer();
  pixelTrans->init(pf, &colourMap, getPreferredPF());
}


const rfb::PixelFormat &DesktopWindow::getPreferredPF()
{
  static PixelFormat prefPF(32, 24, false, true, 255, 255, 255, 0, 8, 16);

  return prefPF;
}


// Cursor stuff

void DesktopWindow::setCursor(int width, int height, const Point& hotspot,
                              void* data, void* mask)
{
}


void DesktopWindow::setName(const char *name)
{
  CharArray windowNameStr;
  windowNameStr.replaceBuf(new char[256]);

  snprintf(windowNameStr.buf, 256, _("TigerVNC: %.240s"), name);

  copy_label(windowNameStr.buf);
}

// setColourMapEntries() changes some of the entries in the colourmap.
// Unfortunately these messages are often sent one at a time, so we delay the
// settings taking effect by 100ms.  This is because recalculating the internal
// translation table can be expensive.
void DesktopWindow::setColourMapEntries(int firstColour, int nColours,
                                        rdr::U16* rgbs)
{
  for (int i = 0; i < nColours; i++)
    colourMap.set(firstColour+i, rgbs[i*3], rgbs[i*3+1], rgbs[i*3+2]);

  if (!Fl::has_timeout(handleColourMap, this))
    Fl::add_timeout(0.100, handleColourMap, this);
}


// Copy the areas of the framebuffer that have been changed (damaged)
// to the displayed window.

void DesktopWindow::updateWindow()
{
  Rect r;

  Fl::remove_timeout(handleUpdateTimeout, this);

  r = damage.get_bounding_rect();
  Fl_Window::damage(FL_DAMAGE_USER1, r.tl.x, r.tl.y, r.width(), r.height());

  damage.clear();
}


void DesktopWindow::draw()
{
  int X, Y, W, H;

  int pixel_bytes, stride_bytes;
  const uchar *buf_start;

  // Check what actually needs updating
  fl_clip_box(0, 0, w(), h(), X, Y, W, H);
  if ((W == 0) || (H == 0))
    return;

  pixel_bytes = frameBuffer->getPF().bpp/8;
  stride_bytes = pixel_bytes * frameBuffer->getStride();
  buf_start = frameBuffer->data +
              pixel_bytes * X +
              stride_bytes * Y;

  // FIXME: Check how efficient this thing really is
  fl_draw_image(buf_start, X, Y, W, H, pixel_bytes, stride_bytes);
}


int DesktopWindow::handle(int event)
{
  int buttonMask, wheelMask;

  switch (event) {
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
      if (Fl::event_dy() < 0)
        wheelMask = 8;
      else
        wheelMask = 16;

      // A quick press of the wheel "button", followed by a immediate
      // release below
      handlePointerEvent(Point(Fl::event_x(), Fl::event_y()),
                         buttonMask | wheelMask);
    } 

    handlePointerEvent(Point(Fl::event_x(), Fl::event_y()), buttonMask);
    return 1;

  case FL_FOCUS:
    // Yes, we would like some focus please!
    return 1;

  case FL_KEYDOWN:
    handleKeyEvent(Fl::event_key(), Fl::event_text(), true);
    return 1;

  case FL_KEYUP:
    handleKeyEvent(Fl::event_key(), Fl::event_text(), false);
    return 1;
  }

  return Fl_Window::handle(event);
}


void DesktopWindow::handleUpdateTimeout(void *data)
{
  DesktopWindow *self = (DesktopWindow *)data;

  assert(self);

  self->updateWindow();
}


void DesktopWindow::handleColourMap(void *data)
{
  DesktopWindow *self = (DesktopWindow *)data;

  assert(self);

  if (self->pixelTrans != NULL)
    self->pixelTrans->setColourMapEntries(0, 0);

  self->Fl_Window::damage(FL_DAMAGE_ALL);
}

void DesktopWindow::handleClose(Fl_Widget *wnd, void *data)
{
  exit_vncviewer();
}


void DesktopWindow::handlePointerEvent(const rfb::Point& pos, int buttonMask)
{
  if (!viewOnly) {
    if (pointerEventInterval == 0 || buttonMask != lastButtonMask) {
      cc->writer()->pointerEvent(pos, buttonMask);
    } else {
      if (!Fl::has_timeout(handlePointerTimeout, this))
        Fl::add_timeout((double)pointerEventInterval/1000.0,
                        handlePointerTimeout, this);
    }
    lastPointerPos = pos;
    lastButtonMask = buttonMask;
  }
}


void DesktopWindow::handlePointerTimeout(void *data)
{
  DesktopWindow *self = (DesktopWindow *)data;

  assert(self);

  self->cc->writer()->pointerEvent(self->lastPointerPos, self->lastButtonMask);
}

void DesktopWindow::handleKeyEvent(int keyCode, const char *keyText, bool down)
{
  rdr::U32 keySym;

  if (viewOnly)
    return;

  if (keyCode > 0xFFFF) {
      vlog.error(_("Too large FLTK key code %d (0x%08x)"), keyCode, keyCode);
      return;
  }

  // Because of the way keyboards work, we cannot expect to have the same
  // symbol on release as when pressed. This breaks the VNC protocol however,
  // so we need to keep track of what keysym a key _code_ generated on press
  // and send the same on release.
  if (!down) {
    cc->writer()->keyEvent(downKeySym[keyCode], false);
    return;
  }

  // Special key
  if (keyText[0] == '\0') {
    if ((keyCode >= FL_F) && (keyCode <= FL_F_Last))
      keySym = XK_F1 + (keyCode - FL_F);
#if 0
      case FL_KP		0xff80	///< One of the keypad numbers; use FL_KP + n for number n.
      case FL_KP_Enter	0xff8d	///< The enter key on the keypad, same as Fl_KP+'\\r'.
      case FL_KP_Last	0xffbd	///< The last keypad key; use to range-check keypad.
#endif
    else {
      switch (keyCode) {
      case FL_BackSpace:
        keySym = XK_BackSpace;
        break;
      case FL_Tab:
        keySym = XK_Tab;
        break;
      case FL_Enter:
        keySym = XK_Return;
        break;
      case FL_Pause:
        keySym = XK_Pause;
        break;
      case FL_Scroll_Lock:
        keySym = XK_Scroll_Lock;
        break;
      case FL_Escape:
        keySym = XK_Escape;
        break;
      case FL_Home:
        keySym = XK_Home;
        break;
      case FL_Left:
        keySym = XK_Left;
        break;
      case FL_Up:
        keySym = XK_Up;
        break;
      case FL_Right:
        keySym = XK_Right;
        break;
      case FL_Down:
        keySym = XK_Down;
        break;
      case FL_Page_Up:
        keySym = XK_Page_Up;
        break;
      case FL_Page_Down:
        keySym = XK_Page_Down;
        break;
      case FL_End:
        keySym = XK_End;
        break;
      case FL_Print:
        keySym = XK_Print;
        break;
      case FL_Insert:
        keySym = XK_Insert;
        break;
      case FL_Menu:
        keySym = XK_Menu;
        break;
      case FL_Help:
        keySym = XK_Help;
        break;
      case FL_Num_Lock:
        keySym = XK_Num_Lock;
        break;
      case FL_Shift_L:
        keySym = XK_Shift_L;
        break;
      case FL_Shift_R:
        keySym = XK_Shift_R;
        break;
      case FL_Control_L:
        keySym = XK_Control_L;
        break;
      case FL_Control_R:
        keySym = XK_Control_R;
        break;
      case FL_Caps_Lock:
        keySym = XK_Caps_Lock;
        break;
      case FL_Meta_L:
        keySym = XK_Super_L;
        break;
      case FL_Meta_R:
        keySym = XK_Super_R;
        break;
      case FL_Alt_L:
        keySym = XK_Alt_L;
        break;
      case FL_Alt_R:
        keySym = XK_Alt_R;
        break;
      case FL_Delete:
        keySym = XK_Delete;
        break;
      default:
        vlog.error(_("Unknown FLTK key code %d (0x%04x)"), keyCode, keyCode);
        return;
      }
    }
  } else {
    unsigned ucs;

    if (fl_utf_nb_char((const unsigned char*)keyText, strlen(keyText)) != 1) {
      vlog.error(_("Multiple characters given for key code %d (0x%04x): '%s'"),
                 keyCode, keyCode, keyText);
      return;
    }

    ucs = fl_utf8decode(keyText, NULL, NULL);
    keySym = ucs2keysym(ucs);
  }

  downKeySym[keyCode] = keySym;
  cc->writer()->keyEvent(keySym, down);
}
