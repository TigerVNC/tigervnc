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

using namespace rfb;

static rfb::LogWriter vlog("DesktopWindow");

DesktopWindow::DesktopWindow(int w, int h, const char *name,
                             const rfb::PixelFormat& serverPF,
                             CConn* cc_)
  : Fl_Window(w, h), cc(cc_), firstUpdate(true)
{
  Fl_Scroll *scroll = new Fl_Scroll(0, 0, w, h);
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

#ifdef HAVE_FLTK_FULLSCREEN
  if (fullScreen)
    fullscreen_on();
  else
#endif
  {
    // If we are creating a window which is equal to the size on the
    // screen on X11, many WMs will treat this as a legacy fullscreen
    // request. This is not what we want. Besides, it doesn't really
    // make sense to try to create a window which is larger than the
    // available work space. 
    size(__rfbmin(w, Fl::w()), __rfbmin(h, Fl::h()));
  }

  show();

  // The window manager might give us an initial window size that is different
  // than the one we requested, and in those cases we need to manually adjust
  // the scroll widget for things to behave sanely.
  if ((w != this->w()) || (h != this->h()))
    scroll->size(this->w(), this->h());
}


DesktopWindow::~DesktopWindow()
{
  // Unregister all timeouts in case they get a change tro trigger
  // again later when this object is already gone.
  Fl::remove_timeout(handleGrab, this);
  Fl::remove_timeout(handleResizeTimeout, this);

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
    if (cc->cp.supportsSetDesktopSize)
      remoteResize();
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
  Fl_Window::resize(x, y, w, h);

  // Try to get the remote size to match our window size, provided
  // the following conditions are true:
  //
  // a) The user has this feature turned on
  // b) The server supports it
  // c) We're not still waiting for a chance to handle DesktopSize
  //
  if (not firstUpdate and ::remoteResize and cc->cp.supportsSetDesktopSize) {
    // We delay updating the remote desktop as we tend to get a flood
    // of resize events as the user is dragging the window.
    Fl::remove_timeout(handleResizeTimeout, this);
    Fl::add_timeout(0.5, handleResizeTimeout, this);
  }

  // Deal with some scrolling corner cases
  repositionViewport();
}


int DesktopWindow::handle(int event)
{
  switch (event) {
#ifdef HAVE_FLTK_FULLSCREEN
  case FL_FULLSCREEN:
    fullScreen.setParam(fullscreen_active());

    if (!fullscreenSystemKeys)
      break;

    if (fullscreen_active())
      grabKeyboard();
    else
      ungrabKeyboard();

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
  
  ret = cocoa_capture_display(this);
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


void DesktopWindow::handleResizeTimeout(void *data)
{
  DesktopWindow *self = (DesktopWindow *)data;

  assert(self);

  self->remoteResize();
}


void DesktopWindow::remoteResize()
{
  int width, height;
  ScreenSet layout;
  ScreenSet::iterator iter;

  if (firstUpdate) {
    if (sscanf(desktopSize.getValueStr(), "%dx%d", &width, &height) != 2)
      return;
  } else {
    width = w();
    height = h();
  }

#ifdef HAVE_FLTK_FULLSCREEN
  if (!fullscreen_active()) {
#endif
    // In windowed mode we just report a single virtual screen that
    // covers the entire framebuffer.

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

    // In full screen we report all screens that are fully covered.

    // If we can find a matching screen in the existing set, we use
    // that, otherwise we create a brand new screen.
    //
    // FIXME: We should really track screens better so we can handle
    //        a resized one.
    //
    for (i = 0;i < Fl::screen_count();i++) {
      Fl::screen_xywh(sx, sy, sw, sh, i);

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
