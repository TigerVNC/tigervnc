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

#ifndef __VIEWPORT_H__
#define __VIEWPORT_H__

#include <map>

#include <FL/Fl.H>
#include <FL/Fl_Widget.H>
#include <FL/Fl_Menu_Button.H>
#include <FL/Fl_RGB_Image.H>

#include <rfb/Rect.h>
#include <rfb/Region.h>
#include <rfb/Timer.h>
#include <rfb/PixelBuffer.h>
#include <rfb/PixelTransformer.h>

#if defined(WIN32)
#include "PlatformPixelBuffer.h"
#elif defined(__APPLE__)
#include "PlatformPixelBuffer.h"
#else
#include "X11PixelBuffer.h"
#endif

class CConn;

class Viewport : public Fl_Widget {
public:

  Viewport(int w, int h, const rfb::PixelFormat& serverPF, CConn* cc_);
  ~Viewport();

  // PixelFormat of incoming write operations
  void setServerPF(const rfb::PixelFormat& pf);
  // Most efficient format (from Viewport's point of view)
  const rfb::PixelFormat &getPreferredPF();

  // Flush updates to screen
  void updateWindow();

  // Methods forwarded from CConn
  void setName(const char *name);

  void setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs);

  void fillRect(const rfb::Rect& r, rfb::Pixel pix) {
    if (pixelTrans) {
      rfb::Pixel pix2;
      pixelTrans->translatePixels(&pix, &pix2, 1);
      pix = pix2;
    }

    frameBuffer->fillRect(r, pix);
    damageRect(r);
  }
  void imageRect(const rfb::Rect& r, void* pixels) {
    if (pixelTrans)
      pixelTrans->translateRect(pixels, r.width(),
                                rfb::Rect(0, 0, r.width(), r.height()),
                                frameBuffer->data, frameBuffer->getStride(),
                                r.tl);
    else
      frameBuffer->imageRect(r, pixels);
    damageRect(r);
  }
  void copyRect(const rfb::Rect& r, int srcX, int srcY) {
    frameBuffer->copyRect(r, rfb::Point(r.tl.x-srcX, r.tl.y-srcY));
    damageRect(r);
  }

  void setCursor(int width, int height, const rfb::Point& hotspot,
                 void* data, void* mask);

  // Fl_Widget callback methods

  void draw();

  void resize(int x, int y, int w, int h);

  int handle(int event);

private:

  void damageRect(const rfb::Rect& r) {
    damage.assign_union(rfb::Region(r));
    if (!Fl::has_timeout(handleUpdateTimeout, this))
      Fl::add_timeout(0.100, handleUpdateTimeout, this);
  };

  static void handleUpdateTimeout(void *data);
  static void handleColourMap(void *data);

  static void handleClipboardChange(int source, void *data);

  void handlePointerEvent(const rfb::Point& pos, int buttonMask);
  static void handlePointerTimeout(void *data);

  rdr::U32 translateKeyEvent(int keyCode, int origKeyCode, const char *keyText);
  void handleKeyEvent(int keyCode, int origKeyCode, const char *keyText, bool down);

  void initContextMenu();
  void popupContextMenu();

  void setMenuKey();

  static void handleOptions(void *data);

private:
  CConn* cc;

  PlatformPixelBuffer* frameBuffer;

  rfb::PixelTransformer *pixelTrans;
  rfb::SimpleColourMap colourMap;

  rfb::Region damage;

  rfb::Point lastPointerPos;
  int lastButtonMask;

  typedef std::map<int, rdr::U32> DownMap;
  DownMap downKeySym;

  int menuKeyCode;
  Fl_Menu_Button *contextMenu;

  Fl_RGB_Image *cursor;
  rfb::Point cursorHotspot;
};

#endif
