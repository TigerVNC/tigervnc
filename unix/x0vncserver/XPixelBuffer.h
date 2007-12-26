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
// XPixelBuffer.h
//

#ifndef __XPIXELBUFFER_H__
#define __XPIXELBUFFER_H__

#include <rfb/PixelBuffer.h>

using namespace rfb;

//
// XPixelBuffer is a modification of FullFramePixelBuffer that does
// not always return buffer width in getStride().
//

class XPixelBuffer : public FullFramePixelBuffer
{
public:
  XPixelBuffer(const PixelFormat& pf, int width, int height,
               rdr::U8* data_, ColourMap* cm, int stride_) :
    FullFramePixelBuffer(pf, width, height, data_, cm), stride(stride_)
  {
  }

  virtual int getStride() const { return stride; }

protected:
  int stride;
};

#endif // __XPIXELBUFFER_H__

