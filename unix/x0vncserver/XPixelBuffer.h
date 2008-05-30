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
#include <x0vncserver/Image.h>

using namespace rfb;

//
// XPixelBuffer is an Image-based implementation of FullFramePixelBuffer.
//

class XPixelBuffer : public FullFramePixelBuffer
{
public:
  XPixelBuffer(Display *dpy, Image* image,
               int offsetLeft, int offsetTop,
               const PixelFormat& pf, ColourMap* cm);
  virtual ~XPixelBuffer();

  // We allow public access to the underlying Image object.
  // The image is heavily used by the PollingManager.
  // TODO: Allow read-only (const Image *) access only.
  //       Or better do not allow public access at all.
  virtual Image *getImage() const { return m_image; }

  virtual int getStride() const { return m_stride; }

  // Override PixelBuffer's function.
  virtual void grabRegion(const rfb::Region& region);

protected:
  Display *m_dpy;
  Image* m_image;
  int m_offsetLeft;
  int m_offsetTop;

  // The number of pixels in a row, with padding included.
  int m_stride;
};

#endif // __XPIXELBUFFER_H__

