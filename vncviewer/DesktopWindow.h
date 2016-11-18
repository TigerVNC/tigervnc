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
#include <rfb/Pixel.h>

#include <FL/Fl_Window.H>

namespace rfb { class ModifiablePixelBuffer; }

class CConn;
class Viewport;

class Fl_Scroll;

class DesktopWindow : public Fl_Window {
public:

  DesktopWindow(int w, int h, const char *name,
                const rfb::PixelFormat& serverPF, CConn* cc_);
  ~DesktopWindow();

  // Most efficient format (from DesktopWindow's point of view)
  const rfb::PixelFormat &getPreferredPF();

  // Flush updates to screen
  void updateWindow();

  // Updated session title
  void setName(const char *name);

  // Resize the current framebuffer, but retain the contents
  void resizeFramebuffer(int new_w, int new_h);

  // New image for the locally rendered cursor
  void setCursor(int width, int height, const rfb::Point& hotspot,
                 void* data, void* mask);

  // Fl_Window callback methods
  void resize(int x, int y, int w, int h);

  int handle(int event);

  void fullscreen_on();
  void grabKeyboard();
  void ungrabKeyboard();
  void toggleGrabKeys();
  bool isGrabActive();

private:
  static int fltkHandle(int event, Fl_Window *win);


  static void handleGrab(void *data);

  void maximizeWindow();

  void handleDesktopSize();
  static void handleResizeTimeout(void *data);
  void remoteResize(int width, int height);

  void repositionViewport();

  static void handleClose(Fl_Widget *wnd, void *data);

  static void handleOptions(void *data);

  static void handleFullscreenTimeout(void *data);

  static void handleEdgeScroll(void *data);

private:
  CConn* cc;
  Fl_Scroll *scroll;
  Viewport *viewport;

  bool firstUpdate;
  bool delayedFullscreen;
  bool delayedDesktopSize;
  bool grabActive;
};

#endif
