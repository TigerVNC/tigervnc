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

#include <FL/fl_draw.H>

#include <rfb/CMsgWriter.h>
#include <rfb/LogWriter.h>

#include "DesktopWindow.h"
#include "CConn.h"
#include "i18n.h"
#include "parameters.h"

using namespace rfb;

extern void exit_vncviewer();

static rfb::LogWriter vlog("DesktopWindow");

DesktopWindow::DesktopWindow(int w, int h, const char *name,
                             const rfb::PixelFormat& serverPF,
                             CConn* cc_)
  : Fl_Window(w, h), cc(cc_), frameBuffer(NULL), pixelTrans(NULL),
    lastPointerPos(0, 0), lastButtonMask(0)
{
  callback(handleClose, this);

  setName(name);

  frameBuffer = new ManagedPixelBuffer(getPreferredPF(), w, h);
  assert(frameBuffer);

  setServerPF(serverPF);

  show();
}


DesktopWindow::~DesktopWindow()
{
  delete frameBuffer;

  if (pixelTrans)
    delete pixelTrans;
}


void DesktopWindow::setServerPF(const rfb::PixelFormat& pf)
{
  if (pixelTrans)
    delete pixelTrans;
  pixelTrans = NULL;

  if (pf.equal(getPreferredPF()))
    return;

  pixelTrans = new PixelTransformer();
  pixelTrans->init(pf, &colourMap, getPreferredPF());
}


const rfb::PixelFormat &DesktopWindow::getPreferredPF()
{
  static PixelFormat prefPF(32, 24, false, true, 255, 255, 255, 0, 8, 16);

  return prefPF;
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

// setColourMapEntries() changes some of the entries in the colourmap.
// Unfortunately these messages are often sent one at a time, so we delay the
// settings taking effect by 100ms.  This is because recalculating the internal
// translation table can be expensive.
void DesktopWindow::setColourMapEntries(int firstColour, int nColours,
                                        rdr::U16* rgbs)
{
  for (int i = 0; i < nColours; i++)
    colourMap.set(firstColour+i, rgbs[i*3], rgbs[i*3+1], rgbs[i*3+2]);

  if (!Fl::has_timeout(handleColourMap, this))
    Fl::add_timeout(0.100, handleColourMap, this);
}


// Copy the areas of the framebuffer that have been changed (damaged)
// to the displayed window.

void DesktopWindow::updateWindow()
{
  Rect r;

  Fl::remove_timeout(handleUpdateTimeout, this);

  r = damage.get_bounding_rect();
  Fl_Window::damage(FL_DAMAGE_USER1, r.tl.x, r.tl.y, r.width(), r.height());

  damage.clear();
}


void DesktopWindow::draw()
{
  int X, Y, W, H;

  int pixel_bytes, stride_bytes;
  const uchar *buf_start;

  // Check what actually needs updating
  fl_clip_box(0, 0, w(), h(), X, Y, W, H);
  if ((W == 0) || (H == 0))
    return;

  pixel_bytes = frameBuffer->getPF().bpp/8;
  stride_bytes = pixel_bytes * frameBuffer->getStride();
  buf_start = frameBuffer->data +
              pixel_bytes * X +
              stride_bytes * Y;

  // FIXME: Check how efficient this thing really is
  fl_draw_image(buf_start, X, Y, W, H, pixel_bytes, stride_bytes);
}


int DesktopWindow::handle(int event)
{
  int buttonMask, wheelMask;

  switch (event) {
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
      if (Fl::event_dy() < 0)
        wheelMask = 8;
      else
        wheelMask = 16;

      // A quick press of the wheel "button", followed by a immediate
      // release below
      handlePointerEvent(Point(Fl::event_x(), Fl::event_y()),
                         buttonMask | wheelMask);
    } 

    handlePointerEvent(Point(Fl::event_x(), Fl::event_y()), buttonMask);
    return 1;
  }

  return Fl_Window::handle(event);
}


void DesktopWindow::handleUpdateTimeout(void *data)
{
  DesktopWindow *self = (DesktopWindow *)data;

  assert(self);

  self->updateWindow();
}


void DesktopWindow::handleColourMap(void *data)
{
  DesktopWindow *self = (DesktopWindow *)data;

  assert(self);

  if (self->pixelTrans != NULL)
    self->pixelTrans->setColourMapEntries(0, 0);

  self->Fl_Window::damage(FL_DAMAGE_ALL);
}

void DesktopWindow::handleClose(Fl_Widget *wnd, void *data)
{
  exit_vncviewer();
}


void DesktopWindow::handlePointerEvent(const rfb::Point& pos, int buttonMask)
{
  if (!viewOnly) {
    if (pointerEventInterval == 0 || buttonMask != lastButtonMask) {
      cc->writer()->pointerEvent(pos, buttonMask);
    } else {
      if (!Fl::has_timeout(handlePointerTimeout, this))
        Fl::add_timeout((double)pointerEventInterval/1000.0,
                        handlePointerTimeout, this);
    }
    lastPointerPos = pos;
    lastButtonMask = buttonMask;
  }
}


void DesktopWindow::handlePointerTimeout(void *data)
{
  DesktopWindow *self = (DesktopWindow *)data;

  assert(self);

  self->cc->writer()->pointerEvent(self->lastPointerPos, self->lastButtonMask);
}
