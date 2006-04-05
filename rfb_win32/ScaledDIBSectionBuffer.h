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

    class ScaledDIBSectionBuffer : public ScaledPixelBuffer, DIBSectionBuffer {
    public:
      ScaledDIBSectionBuffer(HWND window);
      virtual ~ScaledDIBSectionBuffer() {};

      int width()  const { return scaled_width; }
      int height() const { return scaled_height; }
      bool isScaling() const { return scaling; }

      virtual void setSrcPixelBuffer(U8 **src_data, int w, int h, PixelFormat pf=PixelFormat());
      virtual void setPF(const PixelFormat &pf);
      virtual void setSize(int w, int h);

    protected:
      virtual void recreateScaledBuffer();
      virtual void recreateBuffer() {
        recreateScaledBuffer();
      };

      bool scaling;
    };

  };

};

#endif // __RFB_WIN32_SCALED_DIB_SECTION_BUFFER_H__
