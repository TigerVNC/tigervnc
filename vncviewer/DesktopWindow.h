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

#ifndef __DESKTOPWINDOW_H__
#define __DESKTOPWINDOW_H__

#include <map>

#include <rfb/Rect.h>

#include "Viewport.h"

#include <FL/Fl.H>
#include <FL/Fl_Window.H>

class CConn;

class DesktopWindow : public Fl_Window {
public:

  DesktopWindow(int w, int h, const char *name,
                const rfb::PixelFormat& serverPF, CConn* cc_);
  ~DesktopWindow();

  // PixelFormat of incoming write operations
  void setServerPF(const rfb::PixelFormat& pf);
  // Most efficient format (from DesktopWindow's point of view)
  const rfb::PixelFormat &getPreferredPF();

  // Flush updates to screen
  void updateWindow();

  // Methods forwarded from CConn
  void setName(const char *name);

  void setColourMapEntries(int firstColour, int nColours, rdr::U16* rgbs);

  void fillRect(const rfb::Rect& r, rfb::Pixel pix) {
    viewport->fillRect(r, pix);
  }
  void imageRect(const rfb::Rect& r, void* pixels) {
    viewport->imageRect(r, pixels);
  }
  void copyRect(const rfb::Rect& r, int srcX, int srcY) {
    viewport->copyRect(r, srcX, srcY);
  }

  rdr::U8* getPixelsRW(const rfb::Rect& r, int* stride) {
    return viewport->getPixelsRW(r, stride);
  }
  void damageRect(const rfb::Rect& r) {
    viewport->damageRect(r);
  }

  void resizeFramebuffer(int new_w, int new_h);

  void setCursor(int width, int height, const rfb::Point& hotspot,
                 void* data, void* mask);

  // Fl_Window callback methods
  void resize(int x, int y, int w, int h);

  int handle(int event);

  void fullscreen_on();

private:
  static int fltkHandle(int event, Fl_Window *win);

  void grabKeyboard();
  void ungrabKeyboard();

  static void handleGrab(void *data);

  void maximizeWindow();

  void handleDesktopSize();
  static void handleResizeTimeout(void *data);
  void remoteResize(int width, int height);

  void repositionViewport();

  static void handleClose(Fl_Widget *wnd, void *data);

  static void handleOptions(void *data);

  static void handleFullscreenTimeout(void *data);

private:
  CConn* cc;
  Viewport *viewport;

  bool firstUpdate;
  bool delayedFullscreen;
  bool delayedDesktopSize;
};

#endif
