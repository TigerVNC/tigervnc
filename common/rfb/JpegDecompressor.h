/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2004-2005 Cendio AB. All rights reserved.
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
// JpegDecompressor decompresses a JPEG image into RGB output
// an underlying MemOutStream
//

#ifndef __RFB_JPEGDECOMPRESSOR_H__
#define __RFB_JPEGDECOMPRESSOR_H__

#include <rfb/PixelFormat.h>
#include <rfb/Rect.h>

struct jpeg_decompress_struct;

struct JPEG_ERROR_MGR;
struct JPEG_SRC_MGR;

namespace rfb {

  class JpegDecompressor {

  public:

    JpegDecompressor(void);
    virtual ~JpegDecompressor();

    void decompress(const rdr::U8 *, int, rdr::U8 *, int, const Rect&,
                    const PixelFormat&);

  private:

    struct jpeg_decompress_struct *dinfo;

    struct JPEG_ERROR_MGR *err;
    struct JPEG_SRC_MGR *src;

  };

} // end of namespace rfb

#endif
