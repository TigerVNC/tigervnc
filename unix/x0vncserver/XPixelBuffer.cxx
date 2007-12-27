/* Copyright (C) 2007 Constantin Kaplinsky.  All Rights Reserved.
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

#include <X11/Xlib.h>
#include <x0vncserver/XPixelBuffer.h>

using namespace rfb;

XPixelBuffer::XPixelBuffer(Display *dpy, Image* image,
                           int offsetLeft, int offsetTop,
                           const PixelFormat& pf, ColourMap* cm)
  : FullFramePixelBuffer(pf, image->xim->width, image->xim->height,
                         (rdr::U8 *)image->xim->data, cm),
    m_dpy(dpy),
    m_image(image),
    m_offsetLeft(offsetLeft),
    m_offsetTop(offsetTop),
    m_stride(image->xim->bytes_per_line * 8 / image->xim->bits_per_pixel)
{
}

XPixelBuffer::~XPixelBuffer()
{
}

void
XPixelBuffer::grabRegion(const rfb::Region& region)
{
  // m_image->get(DefaultRootWindow(m_dpy), m_offsetLeft, m_offsetTop);
}

