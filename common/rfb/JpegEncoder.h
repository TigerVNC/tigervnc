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
#ifndef __RFB_JPEGENCODER_H__
#define __RFB_JPEGENCODER_H__

#include <rfb/SMsgWriter.h>
#include <rfb/PixelBuffer.h>
#include <rfb/JpegCompressor.h>
#include <rfb/Configuration.h>

//
// JPEG Encoder.
// NOTE: JpegEncoder class is NOT derived from Encoder, because we need
//       access to the original server's framebuffer, without extra pixel
//       copying and with no pixel format translation.
//

namespace rfb {

  class JpegEncoder {
  public:
    JpegEncoder(SMsgWriter* writer);
    virtual ~JpegEncoder();

    virtual void setQualityLevel(int level);
    virtual bool isPixelFormatSupported(PixelBuffer* pb) const;
    virtual void writeRect(PixelBuffer* pb, const Rect& r);

    static BoolParameter useHardwareJPEG;

  private:
    SMsgWriter* writer;
    JpegCompressor* jcomp;

    static const int qualityMap[10];
  };

}

#endif
