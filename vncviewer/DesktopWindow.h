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

#include <sys/time.h>

#include <rfb/Rect.h>
#include <rfb/Pixel.h>

#include <FL/Fl_Window.H>

namespace rfb { class ModifiablePixelBuffer; }

class CConn;
class Surface;
class Viewport;

class Fl_Scrollbar;

#ifdef __GNUC__
#  define __printf_attr(a, b) __attribute__((__format__ (__printf__, a, b)))
#else
#  define __printf_attr(a, b)
#endif // __GNUC__

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
                 const rdr::U8* data);

  // Server-provided cursor position
  void setCursorPos(const rfb::Point& pos);

  // Change client LED state
  void setLEDState(unsigned int state);

  // Clipboard events
  void handleClipboardRequest();
  void handleClipboardAnnounce(bool available);
  void handleClipboardData(const char* data);

  // Fl_Window callback methods
  virtual void show();
  virtual void draw();
  virtual void resize(int x, int y, int w, int h);

  virtual int handle(int event);

  void fullscreen_on();

private:
  static void menuOverlay(void *data);

  void setOverlay(const char *text, ...) __printf_attr(2, 3);
  static void updateOverlay(void *data);

  static int fltkDispatch(int event, Fl_Window *win);
  static int fltkHandle(int event);

  bool hasFocus();

  void maybeGrabKeyboard();
  void grabKeyboard();
  void ungrabKeyboard();
  void grabPointer();
  void ungrabPointer();

  static void handleGrab(void *data);

  void maximizeWindow();

  void handleDesktopSize();
  static void handleResizeTimeout(void *data);
  static void reconfigureFullscreen(void *data);
  void remoteResize(int width, int height);

  void repositionWidgets();

  static void handleClose(Fl_Widget *wnd, void *data);

  static void handleOptions(void *data);

  static void handleFullscreenTimeout(void *data);

  void scrollTo(int x, int y);
  static void handleScroll(Fl_Widget *wnd, void *data);
  static void handleEdgeScroll(void *data);

  static void handleStatsTimeout(void *data);

private:
  CConn* cc;
  Fl_Scrollbar *hscroll, *vscroll;
  Viewport *viewport;
  Surface *offscreen;
  Surface *overlay;
  unsigned char overlayAlpha;
  struct timeval overlayStart;

  bool firstUpdate;
  bool delayedFullscreen;
  bool delayedDesktopSize;

  bool keyboardGrabbed;
  bool mouseGrabbed;

  struct statsEntry {
    unsigned ups;
    unsigned pps;
    unsigned bps;
  };
  struct statsEntry stats[100];

  struct timeval statsLastTime;
  unsigned statsLastUpdates;
  unsigned statsLastPixels;
  unsigned statsLastPosition;

  Surface *statsGraph;
};

#endif
