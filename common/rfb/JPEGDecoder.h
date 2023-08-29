/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2009-2025 Pierre Ossman for Cendio AB
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
#ifndef __RFB_JPEGDECODER_H__
#define __RFB_JPEGDECODER_H__

#include <vector>

#include <rfb/Decoder.h>
#include <rfb/JpegDecompressor.h>

namespace rfb {

  class JPEGDecoder : public Decoder {

  public:
    JPEGDecoder();
    virtual ~JPEGDecoder();
    bool readRect(const core::Rect& r, rdr::InStream* is,
                  const ServerParams& server,
                  rdr::OutStream* os) override;
    void decodeRect(const core::Rect& r, const uint8_t* buffer,
                    size_t buflen, const ServerParams& server,
                    ModifiablePixelBuffer* pb) override;

  private:
    bool readSegment(rdr::InStream* is, rdr::OutStream* os);
    bool readStream(rdr::InStream* is, rdr::OutStream* os);

  private:
    enum {
      IDLE,
      SEGMENT,
      STREAM,
    } state;
    bool seenHuffman, seenQuant;
    std::vector< std::vector<uint8_t> > lastHuffmanTables;
    std::vector< std::vector<uint8_t> > lastQuantTables;
  };
}

#endif
