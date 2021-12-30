/* Copyright (C) 2007-2008 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2014 Pierre Ossman for Cendio AB
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

//
// XPixelBuffer.cxx
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <rfb/Region.h>
#include <X11/Xlib.h>
#include <x0vncserver/XPixelBuffer.h>

using namespace rfb;

XPixelBuffer::XPixelBuffer(Display *dpy, ImageFactory &factory,
                           const Rect &rect)
  : FullFramePixelBuffer(),
    m_poller(0),
    m_dpy(dpy),
    m_image(factory.newImage(dpy, rect.width(), rect.height())),
    m_offsetLeft(rect.tl.x),
    m_offsetTop(rect.tl.y)
{
  // Fill in the PixelFormat structure of the parent class.
  format = PixelFormat(m_image->xim->bits_per_pixel,
                       m_image->xim->depth,
                       (m_image->xim->byte_order == MSBFirst),
                       true,
                       m_image->xim->red_mask   >> (ffs(m_image->xim->red_mask) - 1),
                       m_image->xim->green_mask >> (ffs(m_image->xim->green_mask) - 1),
                       m_image->xim->blue_mask  >> (ffs(m_image->xim->blue_mask) - 1),
                       ffs(m_image->xim->red_mask) - 1,
                       ffs(m_image->xim->green_mask) - 1,
                       ffs(m_image->xim->blue_mask) - 1);

  // Set up the remaining data of the parent class.
  setBuffer(rect.width(), rect.height(), (rdr::U8 *)m_image->xim->data,
            m_image->xim->bytes_per_line * 8 / m_image->xim->bits_per_pixel);

  // Get initial screen image from the X display.
  m_image->get(DefaultRootWindow(m_dpy), m_offsetLeft, m_offsetTop);

  // PollingManager will detect changed pixels.
  m_poller = new PollingManager(dpy, getImage(), factory,
                                m_offsetLeft, m_offsetTop);
}

XPixelBuffer::~XPixelBuffer()
{
  delete m_poller;
  delete m_image;
}

void
XPixelBuffer::grabRegion(const rfb::Region& region)
{
  std::vector<Rect> rects;
  std::vector<Rect>::const_iterator i;
  region.get_rects(&rects);
  for (i = rects.begin(); i != rects.end(); i++) {
    grabRect(*i);
  }
}

