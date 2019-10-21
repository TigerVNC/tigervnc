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
#endif

#define EDGE_SCROLL_SIZE 32
#define EDGE_SCROLL_SPEED 20

using namespace rfb;

static rfb::LogWriter vlog("DesktopWindow");

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

  // Hack. See below...
  Fl::event_dispatch(&fltkHandle);

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
  if ((new_w == viewport->w()) && (new_h == viewport->h()))
    return;

  // If we're letting the viewport match the window perfectly, then
  // keep things that way for the new size, otherwise just keep things
  // like they are.
  if (!fullscreen_active()) {
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
    if (fullscreen_active() && fullScreenAllMonitors) {
      assert(Fl::screen_count() >= 1);
      Fl::screen_xywh(sx, sy, sw, sh, 0);
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

        if ((sx == x) && (sy == y) && (sw == w) && (sh == h)) {
          vlog.info(_("Adjusting window size to avoid accidental full screen request"));
          // Assume a panel of some form and adjust the height
          y += 20;
          h -= 40;
        }
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
}


void DesktopWindow::menuOverlay(void* data)
{
  DesktopWindow *self;

  self = (DesktopWindow*)data;
  self->setOverlay(_("Press %s to open the context menu"),
                   (const char*)menuKey);
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

    if (!fullscreenSystemKeys)
      break;

    if (fullscreen_active())
      grabKeyboard();
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
      if (((viewport->x() < 0) && (Fl::event_x() < EDGE_SCROLL_SIZE)) ||
          ((viewport->x() + viewport->w() > w()) && (Fl::event_x() > w() - EDGE_SCROLL_SIZE)) ||
          ((viewport->y() < 0) && (Fl::event_y() < EDGE_SCROLL_SIZE)) ||
          ((viewport->y() + viewport->h() > h()) && (Fl::event_y() > h() - EDGE_SCROLL_SIZE))) {
        if (!Fl::has_timeout(handleEdgeScroll, this))
          Fl::add_timeout(0.1, handleEdgeScroll, this);
      }
    }
    // Continue processing so that the viewport also gets mouse events
    break;
  }

  return Fl_Window::handle(event);
}


int DesktopWindow::fltkHandle(int event, Fl_Window *win)
{
  int ret;

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
      if (fullscreenSystemKeys) {
        if (dw->fullscreen_active())
          dw->grabKeyboard();
      }
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


void DesktopWindow::fullscreen_on()
{
  if (not fullScreenAllMonitors)
    fullscreen_screens(-1, -1, -1, -1);
  else {
    int top, bottom, left, right;
    int top_y, bottom_y, left_x, right_x;

    int sx, sy, sw, sh;

    top = bottom = left = right = 0;

    Fl::screen_xywh(sx, sy, sw, sh, 0);
    top_y = sy;
    bottom_y = sy + sh;
    left_x = sx;
    right_x = sx + sw;

    for (int i = 1;i < Fl::screen_count();i++) {
      Fl::screen_xywh(sx, sy, sw, sh, i);
      if (sy < top_y) {
        top = i;
        top_y = sy;
      }
      if ((sy + sh) > bottom_y) {
        bottom = i;
        bottom_y = sy + sh;
      }
      if (sx < left_x) {
        left = i;
        left_x = sx;
      }
      if ((sx + sw) > right_x) {
        right = i;
        right_x = sx + sw;
      }
    }

    fullscreen_screens(top, bottom, left, right);
  }

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
  
  ret = cocoa_capture_display(this, fullScreenAllMonitors);
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
  cocoa_release_display(this);
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
  int ret;

  // We also need to grab the pointer as some WMs like to grab buttons
  // combined with modifies (e.g. Alt+Button0 in metacity).
  ret = XGrabPointer(fl_display, fl_xid(this), True,
                     ButtonPressMask|ButtonReleaseMask|
                     ButtonMotionMask|PointerMotionMask,
                     GrabModeAsync, GrabModeAsync,
                     None, None, CurrentTime);
  if (ret) {
    // Having a button pressed prevents us from grabbing, we make
    // a new attempt in fltkHandle()
    if (ret == AlreadyGrabbed)
      return;
    vlog.error(_("Failure grabbing mouse"));
    return;
  }
#endif

  mouseGrabbed = true;
}


void DesktopWindow::ungrabPointer()
{
  mouseGrabbed = false;
#if !defined(WIN32) && !defined(__APPLE__)
  XUngrabPointer(fl_display, CurrentTime);
#endif
}


void DesktopWindow::handleGrab(void *data)
{
  DesktopWindow *self = (DesktopWindow*)data;

  assert(self);

  if (!fullscreenSystemKeys)
    return;
  if (!self->fullscreen_active())
    return;

  self->grabKeyboard();
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
  // OS X is somewhat strange and does not really have a concept of a
  // maximized window, so we can simply resize the window to the workarea.
  // Note that we shouldn't do this whilst in full screen as that will
  // incorrectly adjust things.
  if (fullscreen_active())
    return;
  int X, Y, W, H;
  Fl::screen_work_area(X, Y, W, H, this->x(), this->y());
  size(W, H);
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

      // Look for perfectly matching existing screen...
      for (iter = cc->server.screenLayout().begin();
           iter != cc->server.screenLayout().end(); ++iter) {
        if ((iter->dimensions.tl.x == sx) &&
            (iter->dimensions.tl.y == sy) &&
            (iter->dimensions.width() == sw) &&
            (iter->dimensions.height() == sh))
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

  char buffer[2048];
  vlog.debug("Requesting framebuffer resize from %dx%d to %dx%d",
             cc->server.width(), cc->server.height(), width, height);
  layout.print(buffer, sizeof(buffer));
  vlog.debug("%s", buffer);

  if (!layout.validate(width, height)) {
    vlog.error(_("Invalid screen layout computed for resize request!"));
    return;
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
  exit_vncviewer();
}


void DesktopWindow::handleOptions(void *data)
{
  DesktopWindow *self = (DesktopWindow*)data;

  if (self->fullscreen_active() && fullscreenSystemKeys)
    self->grabKeyboard();
  else
    self->ungrabKeyboard();

  // Call fullscreen_on even if active since it handles
  // fullScreenAllMonitors
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

  if ((self->viewport->x() < 0) && (mx < EDGE_SCROLL_SIZE))
    dx = EDGE_SCROLL_SPEED -
         EDGE_SCROLL_SPEED * mx / EDGE_SCROLL_SIZE;
  if ((self->viewport->x() + self->viewport->w() > self->w()) &&
      (mx > self->w() - EDGE_SCROLL_SIZE))
    dx = EDGE_SCROLL_SPEED * (self->w() - mx) / EDGE_SCROLL_SIZE -
         EDGE_SCROLL_SPEED;
  if ((self->viewport->y() < 0) && (my < EDGE_SCROLL_SIZE))
    dy = EDGE_SCROLL_SPEED -
         EDGE_SCROLL_SPEED * my / EDGE_SCROLL_SIZE;
  if ((self->viewport->y() + self->viewport->h() > self->h()) &&
      (my > self->h() - EDGE_SCROLL_SIZE))
    dy = EDGE_SCROLL_SPEED * (self->h() - my) / EDGE_SCROLL_SIZE -
         EDGE_SCROLL_SPEED;

  if ((dx == 0) && (dy == 0))
    return;

  self->scrollTo(self->hscroll->value() - dx, self->vscroll->value() - dy);

  Fl::repeat_timeout(0.1, handleEdgeScroll, data);
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
