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

#include <FL/Fl_Scroll.H>
#include <FL/x.H>

#include <rfb/LogWriter.h>

#include "DesktopWindow.h"
#include "OptionsDialog.h"
#include "i18n.h"
#include "parameters.h"

#ifdef WIN32
#include "win32.h"
#endif

#ifdef __APPLE__
#include "cocoa.h"
#endif

using namespace rfb;

extern void exit_vncviewer();

static rfb::LogWriter vlog("DesktopWindow");

DesktopWindow::DesktopWindow(int w, int h, const char *name,
                             const rfb::PixelFormat& serverPF,
                             CConn* cc_)
  : Fl_Window(w, h)
{
  // Allow resize
  size_range(100, 100, w, h);

  Fl_Scroll *scroll = new Fl_Scroll(0, 0, w, h);
  scroll->color(FL_BLACK);

  // Automatically adjust the scroll box to the window
  resizable(scroll);

  viewport = new Viewport(w, h, serverPF, cc_);

  scroll->end();

  callback(handleClose, this);

  setName(name);

  OptionsDialog::addCallback(handleOptions, this);

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


void DesktopWindow::setColourMapEntries(int firstColour, int nColours,
                                        rdr::U16* rgbs)
{
  viewport->setColourMapEntries(firstColour, nColours, rgbs);
}


// Copy the areas of the framebuffer that have been changed (damaged)
// to the displayed window.

void DesktopWindow::updateWindow()
{
  viewport->updateWindow();
}


void DesktopWindow::resize(int x, int y, int w, int h)
{
  // Deal with some scrolling corner cases:
  //
  // a) If the window is larger then the viewport, center the viewport.
  // b) If the window is smaller than the viewport, make sure there is
  //    no wasted space on the sides.
  //
  // FIXME: Doesn't compensate for scroll widget size properly.
  if (w > viewport->w())
    viewport->position((w - viewport->w()) / 2, viewport->y());
  else {
    if (viewport->x() > 0)
      viewport->position(0, viewport->y());
    else if (w > (viewport->x() + viewport->w()))
      viewport->position(w - viewport->w(), viewport->y());
  }

  // Same thing for y axis
  if (h > viewport->h())
    viewport->position(viewport->x(), (h - viewport->h()) / 2);
  else {
    if (viewport->y() > 0)
      viewport->position(viewport->x(), 0);
    else if (h > (viewport->y() + viewport->h()))
      viewport->position(viewport->x(), h - viewport->h());
  }

  Fl_Window::resize(x, y, w, h);
}


int DesktopWindow::handle(int event)
{
  switch (event) {
#ifdef HAVE_FLTK_FULLSCREEN
  case FL_FOCUS:
    // FIXME: We reassert the keyboard grabbing on focus/unfocus as FLTK
    //        releases the grab when someone calls Fl::grab(0)
  case FL_FULLSCREEN:
    if (event == FL_FULLSCREEN)
      fullScreen.setParam(fullscreen_active());

    if (!fullscreenSystemKeys)
      break;

    if (fullscreen_active())
      grabKeyboard();
    else
      ungrabKeyboard();

    break;
  case FL_UNFOCUS:
    // FIXME: We need to relinquish control when the entire window loses
    //        focus as it seems to interfere with the WM:s ability to handle
    //        interactions with popups' window decorations.
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
                      GrabModeAsync, GrabModeAsync, fl_event_time);
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
#endif
}
