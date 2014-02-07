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

#ifndef __X11PIXELBUFFER_H__
#define __X11PIXELBUFFER_H__

#include <X11/Xlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <X11/extensions/XShm.h>

#include "PlatformPixelBuffer.h"

class X11PixelBuffer: public PlatformPixelBuffer {
public:
  X11PixelBuffer(int width, int height);
  ~X11PixelBuffer();

  virtual void draw(int src_x, int src_y, int x, int y, int w, int h);

  int getStride() const;

protected:
  int setupShm();

protected:
  XShmSegmentInfo *shminfo;
  XImage *xim;
};


#endif
