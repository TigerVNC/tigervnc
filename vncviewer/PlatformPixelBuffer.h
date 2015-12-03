/* Copyright 2011-2014 Pierre Ossman for Cendio AB
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

#ifndef __PLATFORMPIXELBUFFER_H__
#define __PLATFORMPIXELBUFFER_H__

#include <os/Mutex.h>

#include <rfb/PixelBuffer.h>
#include <rfb/Region.h>

class PlatformPixelBuffer: public rfb::FullFramePixelBuffer {
public:
  PlatformPixelBuffer(const rfb::PixelFormat& pf, int width, int height,
                      rdr::U8* data, int stride);

  virtual void commitBufferRW(const rfb::Rect& r);

  virtual void draw(int src_x, int src_y, int x, int y, int w, int h) = 0;
  rfb::Rect getDamage(void);

  virtual bool isRendering(void);

protected:
  os::Mutex mutex;
  rfb::Region damage;
};

#endif
