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

#include <rfb/CMsgWriter.h>
#include <rfb/LogWriter.h>

// FLTK can pull in the X11 headers on some systems
#ifndef XK_VoidSymbol
#define XK_MISCELLANY
#define XK_XKB_KEYS
#include <rfb/keysymdef.h>
#endif

#ifndef XF86XK_ModeLock
#include <rfb/XF86keysym.h>
#endif

#include "Viewport.h"
#include "CConn.h"
#include "OptionsDialog.h"
#include "i18n.h"
#include "fltk_layout.h"
#include "parameters.h"
#include "keysym2ucs.h"
#include "vncviewer.h"

#include <FL/fl_draw.H>
#include <FL/fl_ask.H>

#ifdef WIN32
#include "win32.h"
#endif

using namespace rfb;
using namespace rdr;

static rfb::LogWriter vlog("Viewport");

// Menu constants

enum { ID_EXIT, ID_FULLSCREEN, ID_CTRL, ID_ALT, ID_MENUKEY, ID_CTRLALTDEL,
       ID_REFRESH, ID_OPTIONS, ID_INFO, ID_ABOUT, ID_DISMISS };

Viewport::Viewport(int w, int h, const rfb::PixelFormat& serverPF, CConn* cc_)
  : Fl_Widget(0, 0, w, h), cc(cc_), frameBuffer(NULL), pixelTrans(NULL),
    colourMapChange(false), lastPointerPos(0, 0), lastButtonMask(0),
    cursor(NULL)
{
// FLTK STR #2599 must be fixed for proper dead keys support
#ifdef HAVE_FLTK_DEAD_KEYS
  set_simple_keyboard();
#endif

// FLTK STR #2636 gives us the ability to monitor clipboard changes
#ifdef HAVE_FLTK_CLIPBOARD
  Fl::add_clipboard_notify(handleClipboardChange, this);
#endif

  frameBuffer = new PlatformPixelBuffer(w, h);
  assert(frameBuffer);

  setServerPF(serverPF);

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
  initContextMenu();

  OptionsDialog::addCallback(handleOptions, this);
}


Viewport::~Viewport()
{
  // Unregister all timeouts in case they get a change tro trigger
  // again later when this object is already gone.
  Fl::remove_timeout(handleUpdateTimeout, this);
  Fl::remove_timeout(handlePointerTimeout, this);

#ifdef HAVE_FLTK_CLIPBOARD
  Fl::remove_clipboard_notify(handleClipboardChange);
#endif

  OptionsDialog::removeCallback(handleOptions);

  delete frameBuffer;

  if (pixelTrans)
    delete pixelTrans;

  if (cursor) {
    if (!cursor->alloc_array)
      delete [] cursor->array;
    delete cursor;
  }

  // FLTK automatically deletes all child widgets, so we shouldn't touch
  // them ourselves here
}


void Viewport::setServerPF(const rfb::PixelFormat& pf)
{
  if (pixelTrans)
    delete pixelTrans;
  pixelTrans = NULL;

  if (pf.equal(getPreferredPF()))
    return;

  pixelTrans = new PixelTransformer();

  // FIXME: This is an ugly (temporary) hack to get around a corner
  //        case during startup. The conversion routines cannot handle
  //        non-native source formats, and we can sometimes get that
  //        as the initial format. We will switch to a better format
  //        before getting any updates, but we need something for now.
  //        Our old client used something completely bogus and just
  //        hoped nothing would ever go wrong. We try to at least match
  //        the pixel size so that we don't get any memory access issues
  //        should a stray update appear.
  static rdr::U32 endianTest = 1;
  static bool nativeBigEndian = *(rdr::U8*)(&endianTest) != 1;
  if ((pf.bpp > 8) && (pf.bigEndian != nativeBigEndian)) {
    PixelFormat fake_pf(pf.bpp, pf.depth, nativeBigEndian, pf.trueColour,
                        pf.redMax, pf.greenMax, pf.blueMax,
                        pf.redShift, pf.greenShift, pf.blueShift);
    pixelTrans->init(fake_pf, &colourMap, getPreferredPF());
    return;
  }

  pixelTrans->init(pf, &colourMap, getPreferredPF());
}


const rfb::PixelFormat &Viewport::getPreferredPF()
{
  return frameBuffer->getPF();
}


// setColourMapEntries() changes some of the entries in the colourmap.
// We don't actually act on these changes until we need to. This is
// because recalculating the internal translation table can be expensive.
// This also solves the issue of silly servers sending colour maps in
// multiple pieces.
void Viewport::setColourMapEntries(int firstColour, int nColours,
                                   rdr::U16* rgbs)
{
  for (int i = 0; i < nColours; i++)
    colourMap.set(firstColour+i, rgbs[i*3], rgbs[i*3+1], rgbs[i*3+2]);

  colourMapChange = true;
}


// Copy the areas of the framebuffer that have been changed (damaged)
// to the displayed window.

void Viewport::updateWindow()
{
  Rect r;

  Fl::remove_timeout(handleUpdateTimeout, this);

  r = damage.get_bounding_rect();
  Fl_Widget::damage(FL_DAMAGE_USER1, r.tl.x + x(), r.tl.y + y(), r.width(), r.height());

  damage.clear();
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
      
      if (pixelTrans)
        pf = &pixelTrans->getInPF();
      else
        pf = &frameBuffer->getPF();

      i = (U8*)data;
      o = buffer;
      m = (U8*)mask;
      m_width = (width+7)/8;
      for (int y = 0;y < height;y++) {
        for (int x = 0;x < width;x++) {
          pf->rgbFromBuffer(o, i, 1, &colourMap);

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

  // FIXME: Resize should probably be a feature of the pixel buffer itself

  if ((w == frameBuffer->width()) && (h == frameBuffer->height()))
    goto end;

  newBuffer = new PlatformPixelBuffer(w, h);
  assert(newBuffer);

  rect.setXYWH(0, 0,
               __rfbmin(newBuffer->width(), frameBuffer->width()),
               __rfbmin(newBuffer->height(), frameBuffer->height()));
  newBuffer->imageRect(rect, frameBuffer->data, frameBuffer->getStride());

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

    vlog.debug("Sending clipboard data: '%s'", buffer);

    try {
      cc->writer()->clientCutText(buffer, ret);
    } catch (rdr::Exception& e) {
      vlog.error(e.str());
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
    // Yes, we would like some focus please!
    return 1;

  case FL_UNFOCUS:
    // Release all keys that were pressed as that generally makes most
    // sense (e.g. Alt+Tab where we only see the Alt press)
    while (!downKeySym.empty())
      handleKeyEvent(downKeySym.begin()->first, downKeySym.begin()->first,
                     "", false);
    return 1;

  case FL_KEYDOWN:
    if (menuKeyCode && (Fl::event_key() == menuKeyCode)) {
      popupContextMenu();
      return 1;
    }

    handleKeyEvent(Fl::event_key(), Fl::event_original_key(),
                   Fl::event_text(), true);
    return 1;

  case FL_KEYUP:
    if (menuKeyCode && (Fl::event_key() == menuKeyCode))
      return 1;

    handleKeyEvent(Fl::event_key(), Fl::event_original_key(),
                   Fl::event_text(), false);
    return 1;
  }

  return Fl_Widget::handle(event);
}


void Viewport::handleUpdateTimeout(void *data)
{
  Viewport *self = (Viewport *)data;

  assert(self);

  self->updateWindow();
}


void Viewport::commitColourMap()
{
  if (pixelTrans == NULL)
    return;
  if (!colourMapChange)
    return;

  colourMapChange = false;

  pixelTrans->setColourMapEntries(0, 0);
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
        vlog.error(e.str());
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
    vlog.error(e.str());
    exit_vncviewer(e.str());
  }
}


rdr::U32 Viewport::translateKeyEvent(int keyCode, int origKeyCode, const char *keyText)
{
  unsigned ucs;

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

#ifdef __APPLE__
  // Alt on OS X behaves more like AltGr on other systems, and to get
  // sane behaviour we should translate things in that manner for the
  // remote VNC server. However that means we lose the ability to use
  // Alt as a shortcut modifier. Do what RealVNC does and hijack the
  // left command key as an Alt replacement.
  switch (keyCode) {
  case FL_Meta_L:
    return XK_Alt_L;
  case FL_Meta_R:
    return XK_Super_L;
  case FL_Alt_L:
  case FL_Alt_R:
    return XK_ISO_Level3_Shift;
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
  if (keyText[0] == '\0') {
    vlog.error(_("Unknown FLTK key code %d (0x%04x)"), keyCode, keyCode);
    return XK_VoidSymbol;
  }

  // Look up the symbol the key produces and translate that from Unicode
  // to a X11 keysym.
  if (fl_utf_nb_char((const unsigned char*)keyText, strlen(keyText)) != 1) {
    vlog.error(_("Multiple characters given for key code %d (0x%04x): '%s'"),
               keyCode, keyCode, keyText);
    return XK_VoidSymbol;
  }

  ucs = fl_utf8decode(keyText, NULL, NULL);
  return ucs2keysym(ucs);
}


void Viewport::handleKeyEvent(int keyCode, int origKeyCode, const char *keyText, bool down)
{
  rdr::U32 keySym;

  if (viewOnly)
    return;

  // Because of the way keyboards work, we cannot expect to have the same
  // symbol on release as when pressed. This breaks the VNC protocol however,
  // so we need to keep track of what keysym a key _code_ generated on press
  // and send the same on release.
  if (!down) {
    DownMap::iterator iter;

    iter = downKeySym.find(origKeyCode);
    if (iter == downKeySym.end()) {
      vlog.error(_("Unexpected release of FLTK key code %d (0x%04x)"),
                 origKeyCode, origKeyCode);
      return;
    }

    vlog.debug("Key released: 0x%04x => 0x%04x", origKeyCode, iter->second);

    try {
      cc->writer()->keyEvent(iter->second, false);
    } catch (rdr::Exception& e) {
      vlog.error(e.str());
      exit_vncviewer(e.str());
    }

    downKeySym.erase(iter);

    return;
  }

  keySym = translateKeyEvent(keyCode, origKeyCode, keyText);
  if (keySym == XK_VoidSymbol)
    return;

#ifdef WIN32
  // Ugly hack alert!
  //
  // Windows doesn't have a proper AltGr, but handles it using fake
  // Ctrl+Alt. Unfortunately X11 doesn't generally like the combination
  // Ctrl+Alt+AltGr, which we usually end up with when Xvnc tries to
  // get everything in the correct state. Cheat and temporarily release
  // Ctrl and Alt whenever we get a key with a symbol.
  bool need_cheat = true;

  if (keyText[0] == '\0')
    need_cheat = false;
  else if ((downKeySym.find(FL_Control_L) == downKeySym.end()) &&
           (downKeySym.find(FL_Control_R) == downKeySym.end()))
    need_cheat = false;
  else if ((downKeySym.find(FL_Alt_L) == downKeySym.end()) &&
           (downKeySym.find(FL_Alt_R) == downKeySym.end()))
    need_cheat = false;

  if (need_cheat) {
    vlog.debug("Faking release of AltGr (Ctrl+Alt)");
    try {
      if (downKeySym.find(FL_Control_L) != downKeySym.end())
        cc->writer()->keyEvent(XK_Control_L, false);
      if (downKeySym.find(FL_Control_R) != downKeySym.end())
        cc->writer()->keyEvent(XK_Control_R, false);
      if (downKeySym.find(FL_Alt_L) != downKeySym.end())
        cc->writer()->keyEvent(XK_Alt_L, false);
      if (downKeySym.find(FL_Alt_R) != downKeySym.end())
        cc->writer()->keyEvent(XK_Alt_R, false);
    } catch (rdr::Exception& e) {
      vlog.error(e.str());
      exit_vncviewer(e.str());
    }
  }
#endif

  vlog.debug("Key pressed: 0x%04x (0x%04x) '%s' => 0x%04x",
             origKeyCode, keyCode, keyText, keySym);

  downKeySym[origKeyCode] = keySym;

  try {
    cc->writer()->keyEvent(keySym, down);
  } catch (rdr::Exception& e) {
    vlog.error(e.str());
    exit_vncviewer(e.str());
  }

#ifdef WIN32
  // Ugly hack continued...
  if (need_cheat) {
    vlog.debug("Restoring AltGr state");
    try {
      if (downKeySym.find(FL_Control_L) != downKeySym.end())
        cc->writer()->keyEvent(XK_Control_L, true);
      if (downKeySym.find(FL_Control_R) != downKeySym.end())
        cc->writer()->keyEvent(XK_Control_R, true);
      if (downKeySym.find(FL_Alt_L) != downKeySym.end())
        cc->writer()->keyEvent(XK_Alt_L, true);
      if (downKeySym.find(FL_Alt_R) != downKeySym.end())
        cc->writer()->keyEvent(XK_Alt_R, true);
    } catch (rdr::Exception& e) {
      vlog.error(e.str());
      exit_vncviewer(e.str());
    }
  }
#endif
}


void Viewport::initContextMenu()
{
  contextMenu->clear();

  contextMenu->add(_("Exit viewer"), 0, NULL, (void*)ID_EXIT, FL_MENU_DIVIDER);

#ifdef HAVE_FLTK_FULLSCREEN
  contextMenu->add(_("Full screen"), 0, NULL, (void*)ID_FULLSCREEN, FL_MENU_DIVIDER);
#endif

  contextMenu->add(_("Ctrl"), 0, NULL, (void*)ID_CTRL, FL_MENU_TOGGLE);
  contextMenu->add(_("Alt"), 0, NULL, (void*)ID_ALT, FL_MENU_TOGGLE);

  if (menuKeyCode) {
    char sendMenuKey[64];
    snprintf(sendMenuKey, 64, _("Send %s"), (const char *)menuKey);
    contextMenu->add(sendMenuKey, 0, NULL, (void*)ID_MENUKEY, 0);
    contextMenu->add("Secret shortcut menu key", menuKeyCode, NULL, (void*)ID_MENUKEY, FL_MENU_INVISIBLE);
  }

  contextMenu->add(_("Send Ctrl-Alt-Del"), 0, NULL, (void*)ID_CTRLALTDEL, FL_MENU_DIVIDER);

  contextMenu->add(_("Refresh screen"), 0, NULL, (void*)ID_REFRESH, FL_MENU_DIVIDER);

  contextMenu->add(_("Options..."), 0, NULL, (void*)ID_OPTIONS, 0);
  contextMenu->add(_("Connection info..."), 0, NULL, (void*)ID_INFO, 0);
  contextMenu->add(_("About TigerVNC viewer..."), 0, NULL, (void*)ID_ABOUT, FL_MENU_DIVIDER);

  contextMenu->add(_("Dismiss menu"), 0, NULL, (void*)ID_DISMISS, 0);
}


void Viewport::popupContextMenu()
{
  const Fl_Menu_Item *m;
  char buffer[1024];

  // Make sure the menu is reset to its initial state between goes or
  // it will start up highlighting the previously selected entry.
  contextMenu->value(-1);

  m = contextMenu->popup();
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
      window()->fullscreen();
    break;
#endif
  case ID_CTRL:
    handleKeyEvent(FL_Control_L, FL_Control_L, "", m->value());
    break;
  case ID_ALT:
    handleKeyEvent(FL_Alt_L, FL_Alt_L, "", m->value());
    break;
  case ID_MENUKEY:
    handleKeyEvent(menuKeyCode, menuKeyCode, "", true);
    handleKeyEvent(menuKeyCode, menuKeyCode, "", false);
    break;
  case ID_CTRLALTDEL:
    handleKeyEvent(FL_Control_L, FL_Control_L, "", true);
    handleKeyEvent(FL_Alt_L, FL_Alt_L, "", true);
    handleKeyEvent(FL_Delete, FL_Delete, "", true);

    handleKeyEvent(FL_Delete, FL_Delete, "", false);
    handleKeyEvent(FL_Alt_L, FL_Alt_L, "", false);
    handleKeyEvent(FL_Control_L, FL_Control_L, "", false);
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
      fl_message(buffer);
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
  const char *menuKeyStr;

  menuKeyCode = 0;

  menuKeyStr = menuKey;
  if (menuKeyStr[0] == 'F') {
    int num = atoi(menuKeyStr + 1);
    if ((num >= 1) && (num <= 12))
      menuKeyCode = FL_F + num;
  }

  // Need to repopulate the context menu as it contains references to
  // the menu key
  initContextMenu();
}


void Viewport::handleOptions(void *data)
{
  Viewport *self = (Viewport*)data;

  self->setMenuKey();
}
