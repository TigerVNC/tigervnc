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

#ifndef __FLTKPIXELBUFFER_H__
#define __FLTKPIXELBUFFER_H__

#include "PlatformPixelBuffer.h"

class FLTKPixelBuffer: public PlatformPixelBuffer {
public:
  FLTKPixelBuffer(int width, int height);
  ~FLTKPixelBuffer();

  virtual void draw(int src_x, int src_y, int x, int y, int w, int h);
};


#endif
