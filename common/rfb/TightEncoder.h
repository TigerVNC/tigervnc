/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
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
#ifndef __RFB_TIGHTENCODER_H__
#define __RFB_TIGHTENCODER_H__

#include <rdr/MemOutStream.h>
#include <rdr/ZlibOutStream.h>
#include <rfb/Encoder.h>

// FIXME: Check if specifying extern "C" is really necessary.
#include <stdio.h>
extern "C" {
#include <jpeglib.h>
}

namespace rfb {

  struct TIGHT_CONF {
    unsigned int maxRectSize, maxRectWidth;
    unsigned int monoMinRectSize;
    int idxZlibLevel, monoZlibLevel, rawZlibLevel;
    int idxMaxColorsDivisor;
    int jpegQuality;
  };

  //
  // Compression level stuff. The following array contains various
  // encoder parameters for each of 10 compression levels (0..9).
  // Last three parameters correspond to JPEG quality levels (0..9).
  //
  // NOTE: s_conf[9].maxRectSize should be >= s_conf[i].maxRectSize,
  // where i in [0..8]. RequiredBuffSize() method depends on this.
  // FIXME: Is this comment obsolete?
  //

  
  class TightEncoder : public Encoder {
  public:
    static Encoder* create(SMsgWriter* writer);
    virtual void setCompressLevel(int level);
    virtual void setQualityLevel(int level);
    virtual int getNumRects(const Rect &r);
    virtual bool writeRect(const Rect& r, ImageGetter* ig, Rect* actual);
    virtual ~TightEncoder();

  private:
    TightEncoder(SMsgWriter* writer);
    void writeSubrect(const Rect& r, ImageGetter* ig);

    SMsgWriter* writer;
    rdr::MemOutStream mos;
    rdr::ZlibOutStream zos[4];

    static const int defaultCompressLevel;
    static const TIGHT_CONF conf[];

    const TIGHT_CONF* pconf;
    const TIGHT_CONF* pjconf;
  };

}

#endif
