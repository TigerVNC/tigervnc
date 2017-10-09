/* Copyright (C) 2007-2008 Constantin Kaplinsky.  All Rights Reserved.
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
// XPixelBuffer.h
//

#ifndef __XPIXELBUFFER_H__
#define __XPIXELBUFFER_H__

#include <rfb/PixelBuffer.h>
#include <rfb/VNCServer.h>
#include <x0vncserver/Image.h>
#include <x0vncserver/PollingManager.h>

//
// XPixelBuffer is an Image-based implementation of FullFramePixelBuffer.
//

class XPixelBuffer : public rfb::FullFramePixelBuffer
{
public:
  XPixelBuffer(Display *dpy, ImageFactory &factory, const rfb::Rect &rect);
  virtual ~XPixelBuffer();

  // Provide access to the underlying Image object.
  const Image *getImage() const { return m_image; }

  // Detect changed pixels, notify the server.
  inline void poll(rfb::VNCServer *server) { m_poller->poll(server); }

  // Override PixelBuffer::grabRegion().
  virtual void grabRegion(const rfb::Region& region);

protected:
  PollingManager *m_poller;

  Display *m_dpy;
  Image* m_image;
  int m_offsetLeft;
  int m_offsetTop;

  // Copy pixels from the screen to the pixel buffer,
  // for the specified rectangular area of the buffer.
  inline void grabRect(const rfb::Rect &r) {
    m_image->get(DefaultRootWindow(m_dpy),
		 m_offsetLeft + r.tl.x, m_offsetTop + r.tl.y,
		 r.width(), r.height(), r.tl.x, r.tl.y);
  }
};

#endif // __XPIXELBUFFER_H__

