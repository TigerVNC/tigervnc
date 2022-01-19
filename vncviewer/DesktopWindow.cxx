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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <algorithm>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <rfb/LogWriter.h>
#include <rfb/CMsgWriter.h>

#include "DesktopWindow.h"
#include "OptionsDialog.h"
#include "i18n.h"
#include "parameters.h"
#include "vncviewer.h"
#include "CConn.h"
#include "Surface.h"
#include "Viewport.h"
#include "touch.h"

#include <FL/Fl.H>
#include <FL/Fl_Image_Surface.H>
#include <FL/Fl_Scrollbar.H>
#include <FL/fl_draw.H>
#include <FL/x.H>

#ifdef WIN32
#include "win32.h"
#endif

#ifdef __APPLE__
#include "cocoa.h"
#include <Carbon/Carbon.h>
#endif

// width of each "edge" region where scrolling happens,
// as a ratio compared to the viewport size
// default: 1/16th of the viewport size
#define EDGE_SCROLL_SIZE 16
// edge width is calculated at runtime; these values are just examples
static int edge_scroll_size_x = 128;
static int edge_scroll_size_y = 96;
// maximum pixels to scroll per frame
#define EDGE_SCROLL_SPEED 16
// how long to wait between viewport scroll position changes
// default: roughly 60 fps for smooth motion
#define EDGE_SCROLL_SECONDS_PER_FRAME 0.016666

using namespace rfb;

static rfb::LogWriter vlog("DesktopWindow");

// Global due to http://www.fltk.org/str.php?L2177 and the similar
// issue for Fl::event_dispatch.
static std::set<DesktopWindow *> instances;

DesktopWindow::DesktopWindow(int w, int h, const char *name,
                             const rfb::PixelFormat& serverPF,
                             CConn* cc_)
  : Fl_Window(w, h), cc(cc_), offscreen(NULL), overlay(NULL),
    firstUpdate(true),
    delayedFullscreen(false), delayedDesktopSize(false),
    keyboardGrabbed(false), mouseGrabbed(false),
    statsLastUpdates(0), statsLastPixels(0), statsLastPosition(0),
    statsGraph(NULL)
{
  Fl_Group* group;

  // Dummy group to prevent FLTK from moving our widgets around
  group = new Fl_Group(0, 0, w, h);
  group->resizable(NULL);
  resizable(group);

  viewport = new Viewport(w, h, serverPF, cc);

  // Position will be adjusted later
  hscroll = new Fl_Scrollbar(0, 0, 0, 0);
  vscroll = new Fl_Scrollbar(0, 0, 0, 0);
  hscroll->type(FL_HORIZONTAL);
  hscroll->callback(handleScroll, this);
  vscroll->callback(handleScroll, this);

  group->end();

  callback(handleClose, this);

  setName(name);

  OptionsDialog::addCallback(handleOptions, this);

  // Some events need to be caught globally
  if (instances.size() == 0)
    Fl::add_handler(fltkHandle);
  instances.insert(this);

  // Hack. See below...
  Fl::event_dispatch(fltkDispatch);

  // Support for -geometry option. Note that although we do support
  // negative coordinates, we do not support -XOFF-YOFF (ie
  // coordinates relative to the right edge / bottom edge) at this
  // time.
  int geom_x = 0, geom_y = 0;
  if (strcmp(geometry, "") != 0) {
    int matched;
    matched = sscanf((const char*)geometry, "+%d+%d", &geom_x, &geom_y);
    if (matched == 2) {
      force_position(1);
    } else {
      int geom_w, geom_h;
      matched = sscanf((const char*)geometry, "%dx%d+%d+%d", &geom_w, &geom_h, &geom_x, &geom_y);
      switch (matched) {
      case 4:
        force_position(1);
        /* fall through */
      case 2:
        w = geom_w;
        h = geom_h;
        break;
      default:
        geom_x = geom_y = 0;
        vlog.error(_("Invalid geometry specified!"));
      }
    }
  }

  // Many window managers don't properly resize overly large windows,
  // so we'll have to do some sanity checks ourselves here
  int sx, sy, sw, sh;
  if (force_position()) {
    Fl::screen_work_area(sx, sy, sw, sh, geom_x, geom_y);
  } else {
    int mx, my;

    // If we don't explicitly request a position then we don't know which
    // monitor the window manager might place us on. Assume the popular
    // behaviour of following the cursor.

    Fl::get_mouse(mx, my);
    Fl::screen_work_area(sx, sy, sw, sh, mx, my);
  }
  if ((w > sw) || (h > sh)) {
    vlog.info(_("Reducing window size to fit on current monitor"));
    if (w > sw)
      w = sw;
    if (h > sh)
      h = sh;
  }

#ifdef __APPLE__
  // On OS X we can do the maximize thing properly before the
  // window is showned. Other platforms handled further down...
  if (maximize) {
    int dummy;
    Fl::screen_work_area(dummy, dummy, w, h, geom_x, geom_y);
  }
#endif

  if (force_position()) {
    resize(geom_x, geom_y, w, h);
  } else {
    size(w, h);
  }

  if (fullScreen) {
    // Hack: Window managers seem to be rather crappy at respecting
    // fullscreen hints on initial windows. So on X11 we'll have to
    // wait until after we've been mapped.
#if defined(WIN32) || defined(__APPLE__)
    fullscreen_on();
#else
    delayedFullscreen = true;
#endif
  }

  show();

  // Full screen events are not sent out for a hidden window,
  // so send a fake one here to set up things properly.
  if (fullscreen_active())
    handle(FL_FULLSCREEN);

  // Unfortunately, current FLTK does not allow us to set the
  // maximized property on Windows and X11 before showing the window.
  // See STR #2083 and STR #2178
#ifndef __APPLE__
  if (maximize) {
    maximizeWindow();
  }
#endif

  // Adjust layout now that we're visible and know our final size
  repositionWidgets();

  if (delayedFullscreen) {
    // Hack: Fullscreen requests may be ignored, so we need a timeout for
    // when we should stop waiting. We also really need to wait for the
    // resize, which can come after the fullscreen event.
    Fl::add_timeout(0.5, handleFullscreenTimeout, this);
    fullscreen_on();
  }

  // Throughput graph for debugging
  if (vlog.getLevel() >= LogWriter::LEVEL_DEBUG) {
    memset(&stats, 0, sizeof(stats));
    Fl::add_timeout(0, handleStatsTimeout, this);
  }

  // Show hint about menu key
  Fl::add_timeout(0.5, menuOverlay, this);

  // By default we get a slight delay when we warp the pointer, something
  // we don't want or we'll get jerky movement
#ifdef __APPLE__
  CGEventSourceRef event = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
  CGEventSourceSetLocalEventsSuppressionInterval(event, 0);
  CFRelease(event);
#endif
}


DesktopWindow::~DesktopWindow()
{
  // Unregister all timeouts in case they get a change tro trigger
  // again later when this object is already gone.
  Fl::remove_timeout(handleGrab, this);
  Fl::remove_timeout(handleResizeTimeout, this);
  Fl::remove_timeout(handleFullscreenTimeout, this);
  Fl::remove_timeout(handleEdgeScroll, this);
  Fl::remove_timeout(handleStatsTimeout, this);
  Fl::remove_timeout(menuOverlay, this);
  Fl::remove_timeout(updateOverlay, this);

  OptionsDialog::removeCallback(handleOptions);

  delete overlay;
  delete offscreen;

  delete statsGraph;

  instances.erase(this);

  if (instances.size() == 0)
    Fl::remove_handler(fltkHandle);

  Fl::event_dispatch(Fl::handle_);

  // FLTK automatically deletes all child widgets, so we shouldn't touch
  // them ourselves here
}


const rfb::PixelFormat &DesktopWindow::getPreferredPF()
{
  return viewport->getPreferredPF();
}


void DesktopWindow::setName(const char *name)
{
  CharArray windowNameStr;
  windowNameStr.replaceBuf(new char[256]);

  snprintf(windowNameStr.buf, 256, "%.240s - TigerVNC", name);

  copy_label(windowNameStr.buf);
}


// Copy the areas of the framebuffer that have been changed (damaged)
// to the displayed window.

void DesktopWindow::updateWindow()
{
  if (firstUpdate) {
    if (cc->server.supportsSetDesktopSize) {
      // Hack: Wait until we're in the proper mode and position until
      // resizing things, otherwise we might send the wrong thing.
      if (delayedFullscreen)
        delayedDesktopSize = true;
      else
        handleDesktopSize();
    }
    firstUpdate = false;
  }

  viewport->updateWindow();
}


void DesktopWindow::resizeFramebuffer(int new_w, int new_h)
{
  bool maximized;

  if ((new_w == viewport->w()) && (new_h == viewport->h()))
    return;

  maximized = false;

#ifdef WIN32
  WINDOWPLACEMENT wndpl;
  memset(&wndpl, 0, sizeof(WINDOWPLACEMENT));
  wndpl.length = sizeof(WINDOWPLACEMENT);
  GetWindowPlacement(fl_xid(this), &wndpl);
  if (wndpl.showCmd == SW_SHOWMAXIMIZED)
    maximized = true;
#elif defined(__APPLE__)
  if (cocoa_win_is_zoomed(this))
    maximized = true;
#else
  Atom net_wm_state = XInternAtom (fl_display, "_NET_WM_STATE", 0);
  Atom net_wm_state_maximized_vert = XInternAtom (fl_display, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
  Atom net_wm_state_maximized_horz = XInternAtom (fl_display, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);

  Atom type;
  int format;
  unsigned long nitems, remain;
  Atom *atoms;

  XGetWindowProperty(fl_display, fl_xid(this), net_wm_state, 0, 1024, False, XA_ATOM, &type, &format, &nitems, &remain, (unsigned char**)&atoms);

  for (unsigned long i = 0;i < nitems;i++) {
    if ((atoms[i] == net_wm_state_maximized_vert) ||
        (atoms[i] == net_wm_state_maximized_horz)) {
      maximized = true;
      break;
    }
  }

  XFree(atoms);
#endif

  // If we're letting the viewport match the window perfectly, then
  // keep things that way for the new size, otherwise just keep things
  // like they are.
  if (!fullscreen_active() && !maximized) {
    if ((w() == viewport->w()) && (h() == viewport->h()))
      size(new_w, new_h);
    else {
      // Make sure the window isn't too big. We do this manually because
      // we have to disable the window size restriction (and it isn't
      // entirely trustworthy to begin with).
      if ((w() > new_w) || (h() > new_h))
        size(__rfbmin(w(), new_w), __rfbmin(h(), new_h));
    }
  }

  viewport->size(new_w, new_h);

  repositionWidgets();
}


void DesktopWindow::setCursor(int width, int height,
                              const rfb::Point& hotspot,
                              const rdr::U8* data)
{
  viewport->setCursor(width, height, hotspot, data);
}


void DesktopWindow::setCursorPos(const rfb::Point& pos)
{
  if (!mouseGrabbed) {
    // Do nothing if we do not have the mouse captured.
    return;
  }
#if defined(WIN32)
  SetCursorPos(pos.x + x_root() + viewport->x(),
               pos.y + y_root() + viewport->y());
#elif defined(__APPLE__)
  CGPoint new_pos;
  new_pos.x = pos.x + x_root() + viewport->x();
  new_pos.y = pos.y + y_root() + viewport->y();
  CGWarpMouseCursorPosition(new_pos);
#else // Assume this is Xlib
  Window rootwindow = DefaultRootWindow(fl_display);
  XWarpPointer(fl_display, rootwindow, rootwindow, 0, 0, 0, 0,
               pos.x + x_root() + viewport->x(),
               pos.y + y_root() + viewport->y());
#endif
}


void DesktopWindow::show()
{
  Fl_Window::show();

#if !defined(WIN32) && !defined(__APPLE__)
  XEvent e;

  // Request ability to grab keyboard under Xwayland
  e.xany.type = ClientMessage;
  e.xany.window = fl_xid(this);
  e.xclient.message_type = XInternAtom (fl_display, "_XWAYLAND_MAY_GRAB_KEYBOARD", 0);
  e.xclient.format = 32;
  e.xclient.data.l[0] = 1;
  e.xclient.data.l[1] = 0;
  e.xclient.data.l[2] = 0;
  e.xclient.data.l[3] = 0;
  e.xclient.data.l[4] = 0;
  XSendEvent(fl_display, RootWindow(fl_display, fl_screen), 0, SubstructureNotifyMask | SubstructureRedirectMask, &e);
#endif
}


void DesktopWindow::draw()
{
  bool redraw;

  int X, Y, W, H;

  // X11 needs an off screen buffer for compositing to avoid flicker,
  // and alpha blending doesn't work for windows on Win32
#if !defined(__APPLE__)

  // Adjust offscreen surface dimensions
  if ((offscreen == NULL) ||
      (offscreen->width() != w()) || (offscreen->height() != h())) {
    delete offscreen;
    offscreen = new Surface(w(), h());
  }

#endif

  // Active area inside scrollbars
  W = w() - (vscroll->visible() ? vscroll->w() : 0);
  H = h() - (hscroll->visible() ? hscroll->h() : 0);

  // Full redraw?
  redraw = (damage() & ~FL_DAMAGE_CHILD);

  // Simplify the clip region to a simple rectangle in order to
  // properly draw all the layers even if they only partially overlap
  if (redraw)
    X = Y = 0;
  else
    fl_clip_box(0, 0, W, H, X, Y, W, H);
  fl_push_no_clip();
  fl_push_clip(X, Y, W, H);

  // Redraw background only on full redraws
  if (redraw) {
    if (offscreen)
      offscreen->clear(40, 40, 40);
    else
      fl_rectf(0, 0, W, H, 40, 40, 40);
  }

  if (offscreen) {
    viewport->draw(offscreen);
    viewport->clear_damage();
  } else {
    if (redraw)
      draw_child(*viewport);
    else
      update_child(*viewport);
  }

  // Debug graph (if active)
  if (statsGraph) {
    int ox, oy, ow, oh;

    ox = X = w() - statsGraph->width() - 30;
    oy = Y = h() - statsGraph->height() - 30;
    ow = statsGraph->width();
    oh = statsGraph->height();

    fl_clip_box(ox, oy, ow, oh, ox, oy, ow, oh);

    if ((ow != 0) && (oh != 0)) {
      if (offscreen)
        statsGraph->blend(offscreen, ox - X, oy - Y, ox, oy, ow, oh, 204);
      else
        statsGraph->blend(ox - X, oy - Y, ox, oy, ow, oh, 204);
    }
  }

  // Overlay (if active)
  if (overlay) {
    int ox, oy, ow, oh;
    int sx, sy, sw, sh;

    // Make sure it's properly seen by adjusting it relative to the
    // primary screen rather than the entire window
    if (fullscreen_active()) {
      assert(Fl::screen_count() >= 1);

      rfb::Rect windowRect, screenRect;
      windowRect.setXYWH(x(), y(), w(), h());

      bool foundEnclosedScreen = false;
      for (int i = 0; i < Fl::screen_count(); i++) {
        Fl::screen_xywh(sx, sy, sw, sh, i);

        // The screen with the smallest index that are enclosed by
        // the viewport will be used for showing the overlay.
        screenRect.setXYWH(sx, sy, sw, sh);
        if (screenRect.enclosed_by(windowRect)) {
          foundEnclosedScreen = true;
          break;
        }
      }

      // If no monitor inside the viewport was found,
      // use the one primary instead.
      if (!foundEnclosedScreen)
        Fl::screen_xywh(sx, sy, sw, sh, 0);

      // Adjust the coordinates so they are relative to the viewport.
      sx -= x();
      sy -= y();

    } else {
      sx = 0;
      sy = 0;
      sw = w();
    }

    ox = X = sx + (sw - overlay->width()) / 2;
    oy = Y = sy + 50;
    ow = overlay->width();
    oh = overlay->height();

    fl_clip_box(ox, oy, ow, oh, ox, oy, ow, oh);

    if ((ow != 0) && (oh != 0)) {
      if (offscreen)
        overlay->blend(offscreen, ox - X, oy - Y, ox, oy, ow, oh, overlayAlpha);
      else
        overlay->blend(ox - X, oy - Y, ox, oy, ow, oh, overlayAlpha);
    }
  }

  // Flush offscreen surface to screen
  if (offscreen) {
    fl_clip_box(0, 0, w(), h(), X, Y, W, H);
    offscreen->draw(X, Y, X, Y, W, H);
  }

  fl_pop_clip();
  fl_pop_clip();

  // Finally the scrollbars

  if (redraw) {
    draw_child(*hscroll);
    draw_child(*vscroll);
  } else {
    update_child(*hscroll);
    update_child(*vscroll);
  }
}


void DesktopWindow::setLEDState(unsigned int state)
{
  viewport->setLEDState(state);
}


void DesktopWindow::handleClipboardRequest()
{
  viewport->handleClipboardRequest();
}

void DesktopWindow::handleClipboardAnnounce(bool available)
{
  viewport->handleClipboardAnnounce(available);
}

void DesktopWindow::handleClipboardData(const char* data)
{
  viewport->handleClipboardData(data);
}


void DesktopWindow::resize(int x, int y, int w, int h)
{
  bool resizing;

#if ! (defined(WIN32) || defined(__APPLE__))
  // X11 window managers will treat a resize to cover the entire
  // monitor as a request to go full screen. Make sure we avoid this.
  if (!fullscreen_active()) {
    bool resize_req;

    // If there is no X11 window, then this must be a resize request,
    // not a notification from the X server.
    if (!shown())
      resize_req = true;
    else {
      // Otherwise we need to get the real window coordinates to tell
      // the difference
      XWindowAttributes actual;
      Window cr;
      int wx, wy;

      XGetWindowAttributes(fl_display, fl_xid(this), &actual);
      XTranslateCoordinates(fl_display, fl_xid(this), actual.root,
                            0, 0, &wx, &wy, &cr);

      // Actual resize request?
      if ((wx != x) || (wy != y) ||
          (actual.width != w) || (actual.height != h))
        resize_req = true;
      else
        resize_req = false;
    }

    if (resize_req) {
      for (int i = 0;i < Fl::screen_count();i++) {
        int sx, sy, sw, sh;

        Fl::screen_xywh(sx, sy, sw, sh, i);

        // We can't trust x and y if the window isn't mapped as the
        // window manager might adjust those numbers
        if (shown() && ((sx != x) || (sy != y)))
            continue;

        if ((sw != w) || (sh != h))
            continue;

        vlog.info(_("Adjusting window size to avoid accidental full-screen request"));
        // Assume a panel of some form and adjust the height
        h -= 40;
      }
    }
  }
#endif

  if ((this->w() != w) || (this->h() != h))
    resizing = true;
  else
    resizing = false;

  Fl_Window::resize(x, y, w, h);

  if (resizing) {
    // Try to get the remote size to match our window size, provided
    // the following conditions are true:
    //
    // a) The user has this feature turned on
    // b) The server supports it
    // c) We're not still waiting for a chance to handle DesktopSize
    // d) We're not still waiting for startup fullscreen to kick in
    //
    if (not firstUpdate and not delayedFullscreen and
        ::remoteResize and cc->server.supportsSetDesktopSize) {
      // We delay updating the remote desktop as we tend to get a flood
      // of resize events as the user is dragging the window.
      Fl::remove_timeout(handleResizeTimeout, this);
      Fl::add_timeout(0.5, handleResizeTimeout, this);
    }

    repositionWidgets();
  }

  // Some systems require a grab after the window size has been changed.
  // Otherwise they might hold on to displays, resulting in them being unusable.
  maybeGrabKeyboard();
}


void DesktopWindow::menuOverlay(void* data)
{
  DesktopWindow *self;

  self = (DesktopWindow*)data;

  if (strcmp((const char*)menuKey, "") != 0) {
    self->setOverlay(_("Press %s to open the context menu"),
                     (const char*)menuKey);
  }
}

void DesktopWindow::setOverlay(const char* text, ...)
{
  const Fl_Fontsize fontsize = 16;
  const int margin = 10;

  va_list ap;
  char textbuf[1024];

  Fl_Image_Surface *surface;

  Fl_RGB_Image* imageText;
  Fl_RGB_Image* image;

  unsigned char* buffer;

  int x, y;
  int w, h;

  unsigned char* a;
  const unsigned char* b;

  delete overlay;
  Fl::remove_timeout(updateOverlay, this);

  va_start(ap, text);
  vsnprintf(textbuf, sizeof(textbuf), text, ap);
  textbuf[sizeof(textbuf)-1] = '\0';
  va_end(ap);

#if !defined(WIN32) && !defined(__APPLE__)
  // FLTK < 1.3.5 crashes if fl_gc is unset
  if (!fl_gc)
    fl_gc = XDefaultGC(fl_display, 0);
#endif

  fl_font(FL_HELVETICA, fontsize);
  w = 0;
  fl_measure(textbuf, w, h);

  // Margins
  w += margin * 2 * 2;
  h += margin * 2;

  surface = new Fl_Image_Surface(w, h);
  surface->set_current();

  fl_rectf(0, 0, w, h, 0, 0, 0);

  fl_font(FL_HELVETICA, fontsize);
  fl_color(FL_WHITE);
  fl_draw(textbuf, 0, 0, w, h, FL_ALIGN_CENTER);

  imageText = surface->image();
  delete surface;

  Fl_Display_Device::display_device()->set_current();

  buffer = new unsigned char[w * h * 4];
  image = new Fl_RGB_Image(buffer, w, h, 4);

  a = buffer;
  for (x = 0;x < image->w() * image->h();x++) {
    a[0] = a[1] = a[2] = 0x40;
    a[3] = 0xcc;
    a += 4;
  }

  a = buffer;
  b = (const unsigned char*)imageText->data()[0];
  for (y = 0;y < h;y++) {
    for (x = 0;x < w;x++) {
      unsigned char alpha;
      alpha = *b;
      a[0] = (unsigned)a[0] * (255 - alpha) / 255 + alpha;
      a[1] = (unsigned)a[1] * (255 - alpha) / 255 + alpha;
      a[2] = (unsigned)a[2] * (255 - alpha) / 255 + alpha;
      a[3] = 255 - (255 - a[3]) * (255 - alpha) / 255;
      a += 4;
      b += imageText->d();
    }
    if (imageText->ld() != 0)
      b += imageText->ld() - w * imageText->d();
  }

  delete imageText;

  overlay = new Surface(image);
  overlayAlpha = 0;
  gettimeofday(&overlayStart, NULL);

  delete image;
  delete [] buffer;

  Fl::add_timeout(1.0/60, updateOverlay, this);
}

void DesktopWindow::updateOverlay(void *data)
{
  DesktopWindow *self;
  unsigned elapsed;

  self = (DesktopWindow*)data;

  elapsed = msSince(&self->overlayStart);

  if (elapsed < 500) {
    self->overlayAlpha = (unsigned)255 * elapsed / 500;
    Fl::add_timeout(1.0/60, updateOverlay, self);
  } else if (elapsed < 3500) {
    self->overlayAlpha = 255;
    Fl::add_timeout(3.0, updateOverlay, self);
  } else if (elapsed < 4000) {
    self->overlayAlpha = (unsigned)255 * (4000 - elapsed) / 500;
    Fl::add_timeout(1.0/60, updateOverlay, self);
  } else {
    delete self->overlay;
    self->overlay = NULL;
  }

  self->damage(FL_DAMAGE_USER1);
}


int DesktopWindow::handle(int event)
{
  switch (event) {
  case FL_FULLSCREEN:
    fullScreen.setParam(fullscreen_active());

    // Update scroll bars
    repositionWidgets();

    if (fullscreen_active())
      maybeGrabKeyboard();
    else
      ungrabKeyboard();

    break;

  case FL_ENTER:
      if (keyboardGrabbed)
          grabPointer();
  case FL_LEAVE:
  case FL_DRAG:
  case FL_MOVE:
    // We don't get FL_LEAVE with a grabbed pointer, so check manually
    if (mouseGrabbed) {
      if ((Fl::event_x() < 0) || (Fl::event_x() >= w()) ||
          (Fl::event_y() < 0) || (Fl::event_y() >= h())) {
        ungrabPointer();
      }
    }
    if (fullscreen_active()) {
      // calculate width of "edge" regions
      edge_scroll_size_x = viewport->w() / EDGE_SCROLL_SIZE;
      edge_scroll_size_y = viewport->h() / EDGE_SCROLL_SIZE;
      // if cursor is near the edge of the viewport, scroll
      if (((viewport->x() < 0) && (Fl::event_x() < edge_scroll_size_x)) ||
          ((viewport->x() + viewport->w() >= w()) && (Fl::event_x() >= w() - edge_scroll_size_x)) ||
          ((viewport->y() < 0) && (Fl::event_y() < edge_scroll_size_y)) ||
          ((viewport->y() + viewport->h() >= h()) && (Fl::event_y() >= h() - edge_scroll_size_y))) {
        if (!Fl::has_timeout(handleEdgeScroll, this))
          Fl::add_timeout(EDGE_SCROLL_SECONDS_PER_FRAME, handleEdgeScroll, this);
      }
    }
    // Continue processing so that the viewport also gets mouse events
    break;
  }

  return Fl_Window::handle(event);
}


int DesktopWindow::fltkDispatch(int event, Fl_Window *win)
{
  int ret;

  // FLTK keeps spamming bogus FL_MOVE events if _any_ X event is
  // received with the mouse pointer outside our windows
  // https://github.com/fltk/fltk/issues/76
  if ((event == FL_MOVE) && (win == NULL))
    return 0;

  ret = Fl::handle_(event, win);

  // This is hackish and the result of the dodgy focus handling in FLTK.
  // The basic problem is that FLTK's view of focus and the system's tend
  // to differ, and as a result we do not see all the FL_FOCUS events we
  // need. Fortunately we can grab them here...

  DesktopWindow *dw = dynamic_cast<DesktopWindow*>(win);

  if (dw) {
    switch (event) {
    // Focus might not stay with us just because we have grabbed the
    // keyboard. E.g. we might have sub windows, or we're not using
    // all monitors and the user clicked on another application.
    // Make sure we update our grabs with the focus changes.
    case FL_FOCUS:
      dw->maybeGrabKeyboard();
      break;
    case FL_UNFOCUS:
      if (fullscreenSystemKeys) {
        dw->ungrabKeyboard();
      }
      break;

    case FL_RELEASE:
      // We usually fail to grab the mouse if a mouse button was
      // pressed when we gained focus (e.g. clicking on our window),
      // so we may need to try again when the button is released.
      // (We do it here rather than handle() because a window does not
      // see FL_RELEASE events if a child widget grabs it first)
      if (dw->keyboardGrabbed && !dw->mouseGrabbed)
        dw->grabPointer();
      break;
    }
  }

  return ret;
}

int DesktopWindow::fltkHandle(int event)
{
  switch (event) {
  case FL_SCREEN_CONFIGURATION_CHANGED:
    // Screens removed or added. Recreate fullscreen window if
    // necessary. On Windows, adding a second screen only works
    // reliable if we are using a timer. Otherwise, the window will
    // not be resized to cover the new screen. A timer makes sense
    // also on other systems, to make sure that whatever desktop
    // environment has a chance to deal with things before we do.
    // Please note that when using FullscreenSystemKeys on macOS, the
    // display configuration cannot be changed: macOS will not detect
    // added or removed screens and there will be no
    // FL_SCREEN_CONFIGURATION_CHANGED event. This is by design:
    // "When you capture a display, you have exclusive use of the
    // display. Other applications and system services are not allowed
    // to use the display or change its configuration. In addition,
    // they are not notified of display changes"
    Fl::remove_timeout(reconfigureFullscreen);
    Fl::add_timeout(0.5, reconfigureFullscreen);
  }

  return 0;
}

void DesktopWindow::fullscreen_on()
{
  bool allMonitors = !strcasecmp(fullScreenMode, "all");
  bool selectedMonitors = !strcasecmp(fullScreenMode, "selected");
  int top, bottom, left, right;

  if (not selectedMonitors and not allMonitors) {
    top = bottom = left = right = Fl::screen_num(x(), y(), w(), h());
  } else {
    int top_y, bottom_y, left_x, right_x;

    int sx, sy, sw, sh;

    std::set<int> monitors;

    if (selectedMonitors and not allMonitors) {
      std::set<int> selected = fullScreenSelectedMonitors.getParam();
      monitors.insert(selected.begin(), selected.end());
    } else {
      for (int i = 0; i < Fl::screen_count(); i++) {
        monitors.insert(i);
      }
    }

    // If no monitors were found in the selected monitors case, we want
    // to explicitly use the window's current monitor.
    if (monitors.size() == 0) {
      monitors.insert(Fl::screen_num(x(), y(), w(), h()));
    }

    // If there are monitors selected, calculate the dimensions
    // of the frame buffer, expressed in the monitor indices that
    // limits it.
    std::set<int>::iterator it = monitors.begin();

    // Get first monitor dimensions.
    Fl::screen_xywh(sx, sy, sw, sh, *it);
    top = bottom = left = right = *it;
    top_y = sy;
    bottom_y = sy + sh;
    left_x = sx;
    right_x = sx + sw;

    // Keep going through the rest of the monitors.
    for (; it != monitors.end(); it++) {
      Fl::screen_xywh(sx, sy, sw, sh, *it);

      if (sy < top_y) {
        top = *it;
        top_y = sy;
      }

      if ((sy + sh) > bottom_y) {
        bottom = *it;
        bottom_y = sy + sh;
      }

      if (sx < left_x) {
        left = *it;
        left_x = sx;
      }

      if ((sx + sw) > right_x) {
        right = *it;
        right_x = sx + sw;
      }
    }

  }
#ifdef __APPLE__
  // This is a workaround for a bug in FLTK, see: https://github.com/fltk/fltk/pull/277
  int savedLevel;
  savedLevel = cocoa_get_level(this);
#endif
  fullscreen_screens(top, bottom, left, right);
#ifdef __APPLE__
  // This is a workaround for a bug in FLTK, see: https://github.com/fltk/fltk/pull/277
  if (cocoa_get_level(this) != savedLevel)
    cocoa_set_level(this, savedLevel);
#endif

  if (!fullscreen_active())
    fullscreen();
}

#if !defined(WIN32) && !defined(__APPLE__)
Bool eventIsFocusWithSerial(Display *display, XEvent *event, XPointer arg)
{
  unsigned long serial;

  serial = *(unsigned long*)arg;

  if (event->xany.serial != serial)
    return False;

  if ((event->type != FocusIn) && (event->type != FocusOut))
    return False;

  return True;
}
#endif

bool DesktopWindow::hasFocus()
{
  Fl_Widget* focus;

  focus = Fl::grab();
  if (!focus)
    focus = Fl::focus();

  if (!focus)
    return false;

  return focus->window() == this;
}

void DesktopWindow::maybeGrabKeyboard()
{
  if (fullscreenSystemKeys && fullscreen_active() && hasFocus())
    grabKeyboard();
}

void DesktopWindow::grabKeyboard()
{
  // Grabbing the keyboard is fairly safe as FLTK reroutes events to the
  // correct widget regardless of which low level window got the system
  // event.

  // FIXME: Push this stuff into FLTK.

#if defined(WIN32)
  int ret;
  
  ret = win32_enable_lowlevel_keyboard(fl_xid(this));
  if (ret != 0) {
    vlog.error(_("Failure grabbing keyboard"));
    return;
  }
#elif defined(__APPLE__)
  int ret;
  
  ret = cocoa_capture_displays(this);
  if (ret != 0) {
    vlog.error(_("Failure grabbing keyboard"));
    return;
  }
#else
  int ret;

  XEvent xev;
  unsigned long serial;

  serial = XNextRequest(fl_display);

  ret = XGrabKeyboard(fl_display, fl_xid(this), True,
                      GrabModeAsync, GrabModeAsync, CurrentTime);
  if (ret) {
    if (ret == AlreadyGrabbed) {
      // It seems like we can race with the WM in some cases.
      // Try again in a bit.
      if (!Fl::has_timeout(handleGrab, this))
        Fl::add_timeout(0.500, handleGrab, this);
    } else {
      vlog.error(_("Failure grabbing keyboard"));
    }
    return;
  }

  // Xorg 1.20+ generates FocusIn/FocusOut even when there is no actual
  // change of focus. This causes us to get stuck in an endless loop
  // grabbing and ungrabbing the keyboard. Avoid this by filtering out
  // any focus events generated by XGrabKeyboard().
  XSync(fl_display, False);
  while (XCheckIfEvent(fl_display, &xev, &eventIsFocusWithSerial,
                       (XPointer)&serial) == True) {
    vlog.debug("Ignored synthetic focus event cause by grab change");
  }
#endif

  keyboardGrabbed = true;

  if (contains(Fl::belowmouse()))
    grabPointer();
}


void DesktopWindow::ungrabKeyboard()
{
  Fl::remove_timeout(handleGrab, this);

  keyboardGrabbed = false;

  ungrabPointer();

#if defined(WIN32)
  win32_disable_lowlevel_keyboard(fl_xid(this));
#elif defined(__APPLE__)
  cocoa_release_displays(this);
#else
  // FLTK has a grab so lets not mess with it
  if (Fl::grab())
    return;

  XEvent xev;
  unsigned long serial;

  serial = XNextRequest(fl_display);

  XUngrabKeyboard(fl_display, CurrentTime);

  // See grabKeyboard()
  XSync(fl_display, False);
  while (XCheckIfEvent(fl_display, &xev, &eventIsFocusWithSerial,
                       (XPointer)&serial) == True) {
    vlog.debug("Ignored synthetic focus event cause by grab change");
  }
#endif
}


void DesktopWindow::grabPointer()
{
#if !defined(WIN32) && !defined(__APPLE__)
  // We also need to grab the pointer as some WMs like to grab buttons
  // combined with modifies (e.g. Alt+Button0 in metacity).

  // Having a button pressed prevents us from grabbing, we make
  // a new attempt in fltkHandle()
  if (!x11_grab_pointer(fl_xid(this)))
    return;
#endif

  mouseGrabbed = true;
}


void DesktopWindow::ungrabPointer()
{
  mouseGrabbed = false;

#if !defined(WIN32) && !defined(__APPLE__)
  x11_ungrab_pointer(fl_xid(this));
#endif
}


void DesktopWindow::handleGrab(void *data)
{
  DesktopWindow *self = (DesktopWindow*)data;

  assert(self);

  self->maybeGrabKeyboard();
}


#define _NET_WM_STATE_ADD           1  /* add/set property */
void DesktopWindow::maximizeWindow()
{
#if defined(WIN32)
  // We cannot use ShowWindow() in full screen mode as it will
  // resize things implicitly. Fortunately modifying the style
  // directly results in a maximized state once we leave full screen.
  if (fullscreen_active()) {
    WINDOWINFO wi;
    wi.cbSize = sizeof(WINDOWINFO);
    GetWindowInfo(fl_xid(this), &wi);
    SetWindowLongPtr(fl_xid(this), GWL_STYLE, wi.dwStyle | WS_MAXIMIZE);
  } else
    ShowWindow(fl_xid(this), SW_MAXIMIZE);
#elif defined(__APPLE__)
  if (fullscreen_active())
    return;
  cocoa_win_zoom(this);
#else
  // X11
  fl_open_display();
  Atom net_wm_state = XInternAtom (fl_display, "_NET_WM_STATE", 0);
  Atom net_wm_state_maximized_vert = XInternAtom (fl_display, "_NET_WM_STATE_MAXIMIZED_VERT", 0);
  Atom net_wm_state_maximized_horz = XInternAtom (fl_display, "_NET_WM_STATE_MAXIMIZED_HORZ", 0);

  XEvent e;
  e.xany.type = ClientMessage;
  e.xany.window = fl_xid(this);
  e.xclient.message_type = net_wm_state;
  e.xclient.format = 32;
  e.xclient.data.l[0] = _NET_WM_STATE_ADD;
  e.xclient.data.l[1] = net_wm_state_maximized_vert;
  e.xclient.data.l[2] = net_wm_state_maximized_horz;
  e.xclient.data.l[3] = 0;
  e.xclient.data.l[4] = 0;
  XSendEvent(fl_display, RootWindow(fl_display, fl_screen), 0, SubstructureNotifyMask | SubstructureRedirectMask, &e);
#endif
}


void DesktopWindow::handleDesktopSize()
{
  if (strcmp(desktopSize, "") != 0) {
    int width, height;

    // An explicit size has been requested

    if (sscanf(desktopSize, "%dx%d", &width, &height) != 2)
      return;

    remoteResize(width, height);
  } else if (::remoteResize) {
    // No explicit size, but remote resizing is on so make sure it
    // matches whatever size the window ended up being
    remoteResize(w(), h());
  }
}


void DesktopWindow::handleResizeTimeout(void *data)
{
  DesktopWindow *self = (DesktopWindow *)data;

  assert(self);

  self->remoteResize(self->w(), self->h());
}


void DesktopWindow::reconfigureFullscreen(void *data)
{
  std::set<DesktopWindow *>::iterator iter;

  for (iter = instances.begin(); iter != instances.end(); ++iter) {
    if ((*iter)->fullscreen_active())
      (*iter)->fullscreen_on();
  }
}


void DesktopWindow::remoteResize(int width, int height)
{
  ScreenSet layout;
  ScreenSet::const_iterator iter;

  if (!fullscreen_active() || (width > w()) || (height > h())) {
    // In windowed mode (or the framebuffer is so large that we need
    // to scroll) we just report a single virtual screen that covers
    // the entire framebuffer.

    layout = cc->server.screenLayout();

    // Not sure why we have no screens, but adding a new one should be
    // safe as there is nothing to conflict with...
    if (layout.num_screens() == 0)
      layout.add_screen(rfb::Screen());
    else if (layout.num_screens() != 1) {
      // More than one screen. Remove all but the first (which we
      // assume is the "primary").

      while (true) {
        iter = layout.begin();
        ++iter;

        if (iter == layout.end())
          break;

        layout.remove_screen(iter->id);
      }
    }

    // Resize the remaining single screen to the complete framebuffer
    layout.begin()->dimensions.tl.x = 0;
    layout.begin()->dimensions.tl.y = 0;
    layout.begin()->dimensions.br.x = width;
    layout.begin()->dimensions.br.y = height;
  } else {
    int i;
    rdr::U32 id;
    int sx, sy, sw, sh;
    rfb::Rect viewport_rect, screen_rect;

    // In full screen we report all screens that are fully covered.

    viewport_rect.setXYWH(x() + (w() - width)/2, y() + (h() - height)/2,
                          width, height);

    // If we can find a matching screen in the existing set, we use
    // that, otherwise we create a brand new screen.
    //
    // FIXME: We should really track screens better so we can handle
    //        a resized one.
    //
    for (i = 0;i < Fl::screen_count();i++) {
      Fl::screen_xywh(sx, sy, sw, sh, i);

      // Check that the screen is fully inside the framebuffer
      screen_rect.setXYWH(sx, sy, sw, sh);
      if (!screen_rect.enclosed_by(viewport_rect))
        continue;

      // Adjust the coordinates so they are relative to our viewport
      sx -= viewport_rect.tl.x;
      sy -= viewport_rect.tl.y;

      // Look for perfectly matching existing screen that is not yet present in
      // in the screen layout...
      for (iter = cc->server.screenLayout().begin();
           iter != cc->server.screenLayout().end(); ++iter) {
        if ((iter->dimensions.tl.x == sx) &&
            (iter->dimensions.tl.y == sy) &&
            (iter->dimensions.width() == sw) &&
            (iter->dimensions.height() == sh) &&
            (std::find(layout.begin(), layout.end(), *iter) == layout.end()))
          break;
      }

      // Found it?
      if (iter != cc->server.screenLayout().end()) {
        layout.add_screen(*iter);
        continue;
      }

      // Need to add a new one, which means we need to find an unused id
      while (true) {
        id = rand();
        for (iter = cc->server.screenLayout().begin();
             iter != cc->server.screenLayout().end(); ++iter) {
          if (iter->id == id)
            break;
        }

        if (iter == cc->server.screenLayout().end())
          break;
      }

      layout.add_screen(rfb::Screen(id, sx, sy, sw, sh, 0));
    }

    // If the viewport doesn't match a physical screen, then we might
    // end up with no screens in the layout. Add a fake one...
    if (layout.num_screens() == 0)
      layout.add_screen(rfb::Screen(0, 0, 0, width, height, 0));
  }

  // Do we actually change anything?
  if ((width == cc->server.width()) &&
      (height == cc->server.height()) &&
      (layout == cc->server.screenLayout()))
    return;

  vlog.debug("Requesting framebuffer resize from %dx%d to %dx%d",
             cc->server.width(), cc->server.height(), width, height);

  char buffer[2048];
  layout.print(buffer, sizeof(buffer));
  if (!layout.validate(width, height)) {
    vlog.error(_("Invalid screen layout computed for resize request!"));
    vlog.error("%s", buffer);
    return;
  } else {
    vlog.debug("%s", buffer);
  }

  cc->writer()->writeSetDesktopSize(width, height, layout);
}


void DesktopWindow::repositionWidgets()
{
  int new_x, new_y;

  // Viewport position

  new_x = viewport->x();
  new_y = viewport->y();

  if (w() > viewport->w())
    new_x = (w() - viewport->w()) / 2;
  else {
    if (viewport->x() > 0)
      new_x = 0;
    else if (w() > (viewport->x() + viewport->w()))
      new_x = w() - viewport->w();
  }

  // Same thing for y axis
  if (h() > viewport->h())
    new_y = (h() - viewport->h()) / 2;
  else {
    if (viewport->y() > 0)
      new_y = 0;
    else if (h() > (viewport->y() + viewport->h()))
      new_y = h() - viewport->h();
  }

  if ((new_x != viewport->x()) || (new_y != viewport->y())) {
    viewport->position(new_x, new_y);
    damage(FL_DAMAGE_SCROLL);
  }

  // Scrollbars visbility

  if (fullscreen_active()) {
    hscroll->hide();
    vscroll->hide();
  } else {
    // Decide whether to show a scrollbar by checking if the window
    // size (possibly minus scrollbar_size) is less than the viewport
    // (remote framebuffer) size.
    //
    // We decide whether to subtract scrollbar_size on an axis by
    // checking if the other axis *definitely* needs a scrollbar.  You
    // might be tempted to think that this becomes a weird recursive
    // problem, but it isn't: If the window size is less than the
    // viewport size (without subtracting the scrollbar_size), then
    // that axis *definitely* needs a scrollbar; if the check changes
    // when we subtract scrollbar_size, then that axis only *maybe*
    // needs a scrollbar.  If both axes only "maybe" need a scrollbar,
    // then neither does; so we don't need to recurse on the "maybe"
    // cases.

    if (w() - (h() < viewport->h() ? Fl::scrollbar_size() : 0) < viewport->w())
      hscroll->show();
    else
      hscroll->hide();

    if (h() - (w() < viewport->w() ? Fl::scrollbar_size() : 0) < viewport->h())
      vscroll->show();
    else
      vscroll->hide();
  }

  // Scrollbars positions

  hscroll->resize(0, h() - Fl::scrollbar_size(),
                  w() - (vscroll->visible() ? Fl::scrollbar_size() : 0),
                  Fl::scrollbar_size());
  vscroll->resize(w() - Fl::scrollbar_size(), 0,
                  Fl::scrollbar_size(),
                  h() - (hscroll->visible() ? Fl::scrollbar_size() : 0));

  // Scrollbars range

  hscroll->value(-viewport->x(),
                 w() - (vscroll->visible() ? vscroll->w() : 0),
                 0, viewport->w());
  vscroll->value(-viewport->y(),
                 h() - (hscroll->visible() ? hscroll->h() : 0),
                 0, viewport->h());
  hscroll->value(hscroll->clamp(hscroll->value()));
  vscroll->value(vscroll->clamp(vscroll->value()));
}

void DesktopWindow::handleClose(Fl_Widget *wnd, void *data)
{
  disconnect();
}


void DesktopWindow::handleOptions(void *data)
{
  DesktopWindow *self = (DesktopWindow*)data;

  if (fullscreenSystemKeys)
    self->maybeGrabKeyboard();
  else
    self->ungrabKeyboard();

  // Call fullscreen_on even if active since it handles
  // fullScreenMode
  if (fullScreen)
    self->fullscreen_on();
  else if (!fullScreen && self->fullscreen_active())
    self->fullscreen_off();
}

void DesktopWindow::handleFullscreenTimeout(void *data)
{
  DesktopWindow *self = (DesktopWindow *)data;

  assert(self);

  self->delayedFullscreen = false;

  if (self->delayedDesktopSize) {
    self->handleDesktopSize();
    self->delayedDesktopSize = false;
  }
}

void DesktopWindow::scrollTo(int x, int y)
{
  x = hscroll->clamp(x);
  y = vscroll->clamp(y);

  hscroll->value(x);
  vscroll->value(y);

  // Scrollbar position results in inverse movement of
  // the viewport widget
  x = -x;
  y = -y;

  if ((viewport->x() == x) && (viewport->y() == y))
    return;

  viewport->position(x, y);
  damage(FL_DAMAGE_SCROLL);
}

void DesktopWindow::handleScroll(Fl_Widget *widget, void *data)
{
  DesktopWindow *self = (DesktopWindow *)data;

  self->scrollTo(self->hscroll->value(), self->vscroll->value());
}

void DesktopWindow::handleEdgeScroll(void *data)
{
  DesktopWindow *self = (DesktopWindow *)data;

  int mx, my;
  int dx, dy;

  assert(self);

  if (!self->fullscreen_active())
    return;

  mx = Fl::event_x();
  my = Fl::event_y();

  dx = dy = 0;

  // Clamp mouse position in case it is outside the window
  if (mx < 0)
    mx = 0;
  if (mx > self->w())
    mx = self->w();
  if (my < 0)
    my = 0;
  if (my > self->h())
    my = self->h();

  if ((self->viewport->x() < 0) && (mx < edge_scroll_size_x))
    dx = EDGE_SCROLL_SPEED -
         EDGE_SCROLL_SPEED * mx / edge_scroll_size_x;
  if ((self->viewport->x() + self->viewport->w() >= self->w()) &&
      (mx >= self->w() - edge_scroll_size_x))
    dx = EDGE_SCROLL_SPEED * (self->w() - mx) / edge_scroll_size_x -
         EDGE_SCROLL_SPEED - 1;
  if ((self->viewport->y() < 0) && (my < edge_scroll_size_y))
    dy = EDGE_SCROLL_SPEED -
         EDGE_SCROLL_SPEED * my / edge_scroll_size_y;
  if ((self->viewport->y() + self->viewport->h() >= self->h()) &&
      (my >= self->h() - edge_scroll_size_y))
    dy = EDGE_SCROLL_SPEED * (self->h() - my) / edge_scroll_size_y -
         EDGE_SCROLL_SPEED - 1;

  if ((dx == 0) && (dy == 0))
    return;

  self->scrollTo(self->hscroll->value() - dx, self->vscroll->value() - dy);

  Fl::repeat_timeout(EDGE_SCROLL_SECONDS_PER_FRAME, handleEdgeScroll, data);
}

void DesktopWindow::handleStatsTimeout(void *data)
{
  DesktopWindow *self = (DesktopWindow*)data;

  const size_t statsCount = sizeof(self->stats)/sizeof(self->stats[0]);

  unsigned updates, pixels, pos;
  unsigned elapsed;

  const unsigned statsWidth = 200;
  const unsigned statsHeight = 100;
  const unsigned graphWidth = statsWidth - 10;
  const unsigned graphHeight = statsHeight - 25;

  Fl_Image_Surface *surface;
  Fl_RGB_Image *image;

  unsigned maxUPS, maxPPS, maxBPS;
  size_t i;

  char buffer[256];

  updates = self->cc->getUpdateCount();
  pixels = self->cc->getPixelCount();
  pos = self->cc->getPosition();
  elapsed = msSince(&self->statsLastTime);
  if (elapsed < 1)
    elapsed = 1;

  memmove(&self->stats[0], &self->stats[1], sizeof(self->stats[0])*(statsCount-1));

  self->stats[statsCount-1].ups = (updates - self->statsLastUpdates) * 1000 / elapsed;
  self->stats[statsCount-1].pps = (pixels - self->statsLastPixels) * 1000 / elapsed;
  self->stats[statsCount-1].bps = (pos - self->statsLastPosition) * 1000 / elapsed;

  gettimeofday(&self->statsLastTime, NULL);
  self->statsLastUpdates = updates;
  self->statsLastPixels = pixels;
  self->statsLastPosition = pos;

#if !defined(WIN32) && !defined(__APPLE__)
  // FLTK < 1.3.5 crashes if fl_gc is unset
  if (!fl_gc)
    fl_gc = XDefaultGC(fl_display, 0);
#endif

  surface = new Fl_Image_Surface(statsWidth, statsHeight);
  surface->set_current();

  fl_rectf(0, 0, statsWidth, statsHeight, FL_BLACK);

  fl_rect(5, 5, graphWidth, graphHeight, FL_WHITE);

  maxUPS = maxPPS = maxBPS = 0;
  for (i = 0;i < statsCount;i++) {
    if (self->stats[i].ups > maxUPS)
      maxUPS = self->stats[i].ups;
    if (self->stats[i].pps > maxPPS)
      maxPPS = self->stats[i].pps;
    if (self->stats[i].bps > maxBPS)
      maxBPS = self->stats[i].bps;
  }

  if (maxUPS != 0) {
    fl_color(FL_GREEN);
    for (i = 0;i < statsCount-1;i++) {
      fl_line(5 + i * graphWidth / statsCount,
              5 + graphHeight - graphHeight * self->stats[i].ups / maxUPS,
              5 + (i+1) * graphWidth / statsCount,
              5 + graphHeight - graphHeight * self->stats[i+1].ups / maxUPS);
    }
  }

  if (maxPPS != 0) {
    fl_color(FL_YELLOW);
    for (i = 0;i < statsCount-1;i++) {
      fl_line(5 + i * graphWidth / statsCount,
              5 + graphHeight - graphHeight * self->stats[i].pps / maxPPS,
              5 + (i+1) * graphWidth / statsCount,
              5 + graphHeight - graphHeight * self->stats[i+1].pps / maxPPS);
    }
  }

  if (maxBPS != 0) {
    fl_color(FL_RED);
    for (i = 0;i < statsCount-1;i++) {
      fl_line(5 + i * graphWidth / statsCount,
              5 + graphHeight - graphHeight * self->stats[i].bps / maxBPS,
              5 + (i+1) * graphWidth / statsCount,
              5 + graphHeight - graphHeight * self->stats[i+1].bps / maxBPS);
    }
  }

  fl_font(FL_HELVETICA, 10);

  fl_color(FL_GREEN);
  snprintf(buffer, sizeof(buffer), "%u upd/s", self->stats[statsCount-1].ups);
  fl_draw(buffer, 5, statsHeight - 5);

  fl_color(FL_YELLOW);
  siPrefix(self->stats[statsCount-1].pps, "pix/s",
           buffer, sizeof(buffer), 3);
  fl_draw(buffer, 5 + (statsWidth-10)/3, statsHeight - 5);

  fl_color(FL_RED);
  siPrefix(self->stats[statsCount-1].bps * 8, "bps",
           buffer, sizeof(buffer), 3);
  fl_draw(buffer, 5 + (statsWidth-10)*2/3, statsHeight - 5);

  image = surface->image();
  delete surface;

  Fl_Display_Device::display_device()->set_current();

  delete self->statsGraph;
  self->statsGraph = new Surface(image);
  delete image;

  self->damage(FL_DAMAGE_CHILD, self->w() - statsWidth - 30,
               self->h() - statsHeight - 30,
               statsWidth, statsHeight);

  Fl::repeat_timeout(0.5, handleStatsTimeout, data);
}
