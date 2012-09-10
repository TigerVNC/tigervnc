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

#include <rfb/LogWriter.h>
#include <rfb/CMsgWriter.h>

#include "DesktopWindow.h"
#include "OptionsDialog.h"
#include "i18n.h"
#include "parameters.h"
#include "vncviewer.h"
#include "CConn.h"

#include <FL/Fl_Scroll.H>
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
  : Fl_Window(w, h), cc(cc_), firstUpdate(true),
    delayedFullscreen(false), delayedDesktopSize(false)
{
  scroll = new Fl_Scroll(0, 0, w, h);
  scroll->color(FL_BLACK);

  // Automatically adjust the scroll box to the window
  resizable(scroll);

  viewport = new Viewport(w, h, serverPF, cc);

  scroll->end();

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
  if (geometry.hasBeenSet()) {
    int matched;
    matched = sscanf(geometry.getValueStr(), "+%d+%d", &geom_x, &geom_y);
    if (matched == 2) {
      force_position(1);
    } else {
      int geom_w, geom_h;
      matched = sscanf(geometry.getValueStr(), "%dx%d+%d+%d", &geom_w, &geom_h, &geom_x, &geom_y);
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
	vlog.error("Invalid geometry specified!");	
      }
    }
  }

  if (force_position()) {
    resize(geom_x, geom_y, w, h);
  } else {
    size(w, h);
  }

#ifdef HAVE_FLTK_FULLSCREEN
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
#endif

  show();

  // Unfortunately, current FLTK does not allow us to set the
  // maximized property before showing the window. See STR #2083 and
  // STR #2178
  if (maximize) {
    maximizeWindow();
  }

  // The window manager might give us an initial window size that is different
  // than the one we requested, and in those cases we need to manually adjust
  // the scroll widget for things to behave sanely.
  if ((w != this->w()) || (h != this->h()))
    scroll->size(this->w(), this->h());

#ifdef HAVE_FLTK_FULLSCREEN
  if (delayedFullscreen) {
    // Hack: Fullscreen requests may be ignored, so we need a timeout for
    // when we should stop waiting. We also really need to wait for the
    // resize, which can come after the fullscreen event.
    Fl::add_timeout(0.5, handleFullscreenTimeout, this);
    fullscreen_on();
  }
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

  OptionsDialog::removeCallback(handleOptions);

  // FLTK automatically deletes all child widgets, so we shouldn't touch
  // them ourselves here
}


void DesktopWindow::setServerPF(const rfb::PixelFormat& pf)
{
  viewport->setServerPF(pf);
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


void DesktopWindow::setColourMapEntries(int firstColour, int nColours,
                                        rdr::U16* rgbs)
{
  viewport->setColourMapEntries(firstColour, nColours, rgbs);
}


// Copy the areas of the framebuffer that have been changed (damaged)
// to the displayed window.

void DesktopWindow::updateWindow()
{
  if (firstUpdate) {
    if (cc->cp.supportsSetDesktopSize) {
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
#ifdef HAVE_FLTK_FULLSCREEN
  if (!fullscreen_active()) {
#endif
    if ((w() == viewport->w()) && (h() == viewport->h()))
      size(new_w, new_h);
    else {
      // Make sure the window isn't too big. We do this manually because
      // we have to disable the window size restriction (and it isn't
      // entirely trustworthy to begin with).
      if ((w() > new_w) || (h() > new_h))
        size(__rfbmin(w(), new_w), __rfbmin(h(), new_h));
    }
#ifdef HAVE_FLTK_FULLSCREEN
  }
#endif

  viewport->size(new_w, new_h);

  // We might not resize the main window, so we need to manually call this
  // to make sure the viewport is centered.
  repositionViewport();

  // repositionViewport() makes sure the scroll widget notices any changes
  // in position, but it might be just the size that changes so we also
  // need a poke here as well.
  redraw();
}


void DesktopWindow::setCursor(int width, int height, const Point& hotspot,
                              void* data, void* mask)
{
  viewport->setCursor(width, height, hotspot, data, mask);
}


void DesktopWindow::resize(int x, int y, int w, int h)
{
  bool resizing;

#if ! (defined(WIN32) || defined(__APPLE__))
  // X11 window managers will treat a resize to cover the entire
  // monitor as a request to go full screen. Make sure we avoid this.
  //
  // FIXME: If this is an external event then this code will get
  //        FLTK into a confused state about the window's position/size.
  //        Unfortunately FLTK doesn't offer an easy way to tell
  //        what kind of resize it is. Let's hope this corner case
  //        isn't too common...
#ifdef HAVE_FLTK_FULLSCREEN
  if (!fullscreen_active())
#endif
  {
    for (int i = 0;i < Fl::screen_count();i++) {
      int sx, sy, sw, sh;

      Fl::screen_xywh(sx, sy, sw, sh, i);

      if ((sx == x) && (sy == y) && (sw == w) && (sh == h)) {
        vlog.info("Adjusting window size to avoid accidental full screen request");
        // Assume a panel of some form and adjust the height
        y += 20;
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
        ::remoteResize and cc->cp.supportsSetDesktopSize) {
      // We delay updating the remote desktop as we tend to get a flood
      // of resize events as the user is dragging the window.
      Fl::remove_timeout(handleResizeTimeout, this);
      Fl::add_timeout(0.5, handleResizeTimeout, this);
    }

    // Deal with some scrolling corner cases
    repositionViewport();
  }
}


int DesktopWindow::handle(int event)
{
  switch (event) {
#ifdef HAVE_FLTK_FULLSCREEN
  case FL_FULLSCREEN:
    fullScreen.setParam(fullscreen_active());

    if (fullscreen_active())
      scroll->type(0);
    else
      scroll->type(Fl_Scroll::BOTH);

    if (!fullscreenSystemKeys)
      break;

    if (fullscreen_active())
      grabKeyboard();
    else
      ungrabKeyboard();

    break;

  case FL_ENTER:
  case FL_LEAVE:
  case FL_DRAG:
  case FL_MOVE:
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
#endif

  case FL_SHORTCUT:
    // Sometimes the focus gets out of whack and we fall through to the
    // shortcut dispatching. Try to make things sane again...
    if (Fl::focus() == NULL) {
      take_focus();
      Fl::handle(FL_KEYDOWN, this);
    }
    return 1;
  }

  return Fl_Window::handle(event);
}


int DesktopWindow::fltkHandle(int event, Fl_Window *win)
{
  int ret;

  ret = Fl::handle_(event, win);

#ifdef HAVE_FLTK_FULLSCREEN
  // This is hackish and the result of the dodgy focus handling in FLTK.
  // The basic problem is that FLTK's view of focus and the system's tend
  // to differ, and as a result we do not see all the FL_FOCUS events we
  // need. Fortunately we can grab them here...

  DesktopWindow *dw = dynamic_cast<DesktopWindow*>(win);

  if (dw && fullscreenSystemKeys) {
    switch (event) {
    case FL_FOCUS:
      // FIXME: We reassert the keyboard grabbing on focus as FLTK there are
      //        some issues we need to work around:
      //        a) Fl::grab(0) on X11 will release the keyboard grab for us.
      //        b) Gaining focus on the system level causes FLTK to switch
      //           window level on OS X.
      if (dw->fullscreen_active())
        dw->grabKeyboard();
      break;

      case FL_UNFOCUS:
      // FIXME: We need to relinquish control when the entire window loses
      //        focus as it is very tied to this specific window on some
      //        platforms and we want to be able to open subwindows.
      dw->ungrabKeyboard();
      break;
    }
  }
#endif

  return ret;
}


void DesktopWindow::fullscreen_on()
{
#ifdef HAVE_FLTK_FULLSCREEN
#ifdef HAVE_FLTK_FULLSCREEN_SCREENS
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
#endif // HAVE_FLTK_FULLSCREEN_SCREENS

  fullscreen();
#endif // HAVE_FLTK_FULLSCREEN
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
  if (ret != 0)
    vlog.error(_("Failure grabbing keyboard"));
#elif defined(__APPLE__)
  int ret;
  
  ret = cocoa_capture_display(this,
#ifdef HAVE_FLTK_FULLSCREEN_SCREENS
                              fullScreenAllMonitors
#else
                              false
#endif
                              );
  if (ret != 0)
    vlog.error(_("Failure grabbing keyboard"));
#else
  int ret;

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
  }

  // We also need to grab the pointer as some WMs like to grab buttons
  // combined with modifies (e.g. Alt+Button0 in metacity).
  ret = XGrabPointer(fl_display, fl_xid(this), True,
                     ButtonPressMask|ButtonReleaseMask|
                     ButtonMotionMask|PointerMotionMask,
                     GrabModeAsync, GrabModeAsync,
                     None, None, CurrentTime);
  if (ret)
    vlog.error(_("Failure grabbing mouse"));
#endif
}


void DesktopWindow::ungrabKeyboard()
{
  Fl::remove_timeout(handleGrab, this);

#if defined(WIN32)
  win32_disable_lowlevel_keyboard(fl_xid(this));
#elif defined(__APPLE__)
  cocoa_release_display(this);
#else
  // FLTK has a grab so lets not mess with it
  if (Fl::grab())
    return;

  XUngrabPointer(fl_display, fl_event_time);
  XUngrabKeyboard(fl_display, fl_event_time);
#endif
}


void DesktopWindow::handleGrab(void *data)
{
  DesktopWindow *self = (DesktopWindow*)data;

  assert(self);

#ifdef HAVE_FLTK_FULLSCREEN
  if (!fullscreenSystemKeys)
    return;
  if (!self->fullscreen_active())
    return;

  self->grabKeyboard();
#endif
}


#define _NET_WM_STATE_ADD           1  /* add/set property */
void DesktopWindow::maximizeWindow()
{
#if defined(WIN32)
  WINDOWPLACEMENT wp;
  wp.length = sizeof(WINDOWPLACEMENT);
  GetWindowPlacement(fl_xid(this), &wp);
  wp.showCmd = SW_MAXIMIZE;
  SetWindowPlacement(fl_xid(this), &wp);
#elif defined(__APPLE__)
  /* OS X is somewhat strange and does not really have a concept of a
     maximized window, so we can simply resize the window to the workarea */
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
  int width, height;

  if (sscanf(desktopSize.getValueStr(), "%dx%d", &width, &height) != 2)
    return;

  remoteResize(width, height);
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
  ScreenSet::iterator iter;

#ifdef HAVE_FLTK_FULLSCREEN
  if (!fullscreen_active() || (width > w()) || (height > h())) {
#endif
    // In windowed mode (or the framebuffer is so large that we need
    // to scroll) we just report a single virtual screen that covers
    // the entire framebuffer.

    layout = cc->cp.screenLayout;

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
#ifdef HAVE_FLTK_FULLSCREEN
  } else {
    int i;
    rdr::U32 id;
    int sx, sy, sw, sh;
    Rect viewport_rect, screen_rect;

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
      for (iter = cc->cp.screenLayout.begin();
           iter != cc->cp.screenLayout.end(); ++iter) {
        if ((iter->dimensions.tl.x == sx) &&
            (iter->dimensions.tl.y == sy) &&
            (iter->dimensions.width() == sw) &&
            (iter->dimensions.height() == sh))
          break;
      }

      // Found it?
      if (iter != cc->cp.screenLayout.end()) {
        layout.add_screen(*iter);
        continue;
      }

      // Need to add a new one, which means we need to find an unused id
      while (true) {
        id = rand();
        for (iter = cc->cp.screenLayout.begin();
             iter != cc->cp.screenLayout.end(); ++iter) {
          if (iter->id == id)
            break;
        }

        if (iter == cc->cp.screenLayout.end())
          break;
      }

      layout.add_screen(rfb::Screen(id, sx, sy, sw, sh, 0));
    }

    // If the viewport doesn't match a physical screen, then we might
    // end up with no screens in the layout. Add a fake one...
    if (layout.num_screens() == 0)
      layout.add_screen(rfb::Screen(0, 0, 0, width, height, 0));
  }
#endif

  // Do we actually change anything?
  if ((width == cc->cp.width) &&
      (height == cc->cp.height) &&
      (layout == cc->cp.screenLayout))
    return;

  vlog.debug("Requesting framebuffer resize from %dx%d to %dx%d (%d screens)",
             cc->cp.width, cc->cp.height, width, height, layout.num_screens());

  if (!layout.validate(width, height)) {
    vlog.error("Invalid screen layout computed for resize request!");
    return;
  }

  cc->writer()->writeSetDesktopSize(width, height, layout);
}


void DesktopWindow::repositionViewport()
{
  int new_x, new_y;

  // Deal with some scrolling corner cases:
  //
  // a) If the window is larger then the viewport, center the viewport.
  // b) If the window is smaller than the viewport, make sure there is
  //    no wasted space on the sides.
  //
  // FIXME: Doesn't compensate for scroll widget size properly.

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

    // The scroll widget does not notice when you move around child widgets,
    // so redraw everything to make sure things update.
    redraw();
  }
}

void DesktopWindow::handleClose(Fl_Widget *wnd, void *data)
{
  exit_vncviewer();
}


void DesktopWindow::handleOptions(void *data)
{
  DesktopWindow *self = (DesktopWindow*)data;

#ifdef HAVE_FLTK_FULLSCREEN
  if (self->fullscreen_active() && fullscreenSystemKeys)
    self->grabKeyboard();
  else
    self->ungrabKeyboard();

  if (fullScreen && !self->fullscreen_active())
    self->fullscreen_on();
  else if (!fullScreen && self->fullscreen_active())
    self->fullscreen_off();
#endif
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

void DesktopWindow::handleEdgeScroll(void *data)
{
#ifdef HAVE_FLTK_FULLSCREEN
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

  // Make sure we don't move the viewport too much
  if (self->viewport->x() + dx > 0)
    dx = -self->viewport->x();
  if (self->viewport->x() + dx + self->viewport->w() < self->w())
    dx = self->w() - (self->viewport->x() + self->viewport->w());
  if (self->viewport->y() + dy > 0)
    dy = -self->viewport->y();
  if (self->viewport->y() + dy + self->viewport->h() < self->h())
    dy = self->h() - (self->viewport->y() + self->viewport->h());

  self->scroll->scroll_to(self->scroll->xposition() - dx, self->scroll->yposition() - dy);

  Fl::repeat_timeout(0.1, handleEdgeScroll, data);
#endif
}
