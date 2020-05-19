/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
// JpegCompressor compresses RGB input into a JPEG image and stores it in
// an underlying MemOutStream
//

#ifndef __RFB_JPEGCOMPRESSOR_H__
#define __RFB_JPEGCOMPRESSOR_H__

#include <rdr/MemOutStream.h>
#include <rfb/PixelFormat.h>
#include <rfb/Rect.h>

struct jpeg_compress_struct;

struct JPEG_ERROR_MGR;
struct JPEG_DEST_MGR;

namespace rfb {

  class JpegCompressor : public rdr::MemOutStream {

  public:

    JpegCompressor(int bufferLen = 128*1024);
    virtual ~JpegCompressor();

    void compress(const rdr::U8 *, int, const Rect&, const PixelFormat&, int, int);

    void writeBytes(const void*, int);

  private:

    struct jpeg_compress_struct *cinfo;

    struct JPEG_ERROR_MGR *err;
    struct JPEG_DEST_MGR *dest;

  };

} // end of namespace rfb

#endif
