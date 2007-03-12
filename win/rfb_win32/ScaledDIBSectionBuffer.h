/* Copyright (C) 2006 TightVNC Team.  All Rights Reserved.
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
 *
 * TightVNC distribution homepage on the Web: http://www.tightvnc.com/
 *
 */

// -=- ScaledDIBSectionBuffer.h

#ifndef __RFB_WIN32_SCALED_DIB_SECTION_BUFFER_H__
#define __RFB_WIN32_SCALED_DIB_SECTION_BUFFER_H__

#include <rfb/ScaledPixelBuffer.h>

#include <rfb_win32/DIBSectionBuffer.h>

namespace rfb {

  namespace win32 {

    //
    // -=- ScaledDIBSectionBuffer
    //

    class ScaledDIBSectionBuffer : public ScaledPixelBuffer, public DIBSectionBuffer {
    public:
      ScaledDIBSectionBuffer(HWND window);
      virtual ~ScaledDIBSectionBuffer();

      int width()  const { return scaled_width; }
      int height() const { return scaled_height; }
      int area() const { return scaled_width * scaled_height; }
      bool isScaling() const { return scaling; }

      virtual void setPF(const PixelFormat &pf);
      virtual const PixelFormat& getPixelFormat() const { return pf; }
      virtual const PixelFormat& getScaledPixelFormat() const { return getPF(); }
      virtual void setSize(int w, int h);
      virtual void setScale(int scale);
      virtual void setScaleWindowSize(int width, int height);
      
      virtual void calculateScaledBufferSize();

      Rect getRect() const { return ScaledPixelBuffer::getRect(); }
      Rect getRect(const Point& pos) const { return ScaledPixelBuffer::getRect(pos); }

      // -=- Overrides basic rendering operations of 
      //     FullFramePixelBuffer class

      virtual void fillRect(const Rect &dest, Pixel pix);
      virtual void imageRect(const Rect &dest, const void* pixels, int stride=0);
      virtual void copyRect(const Rect &dest, const Point &move_by_delta);
      virtual void maskRect(const Rect& r, const void* pixels, const void* mask_);
      virtual void maskRect(const Rect& r, Pixel pixel, const void* mask_);

    protected:
      virtual void recreateScaledBuffer();
      virtual void recreateBuffers();
      virtual void recreateBuffer() {
        recreateScaledBuffer();
      };

      ManagedPixelBuffer *src_buffer;
      bool scaling;
    };

  };

};

#endif // __RFB_WIN32_SCALED_DIB_SECTION_BUFFER_H__
