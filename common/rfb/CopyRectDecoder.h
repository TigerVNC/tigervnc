/* Copyright 2014 Pierre Ossman <ossman@cendio.se> for Cendio AB
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
#ifndef __RFB_COPYRECTDECODER_H__
#define __RFB_COPYRECTDECODER_H__

#include <rfb/Decoder.h>

namespace rfb {

  class CopyRectDecoder : public Decoder {
  public:
    CopyRectDecoder();
    virtual ~CopyRectDecoder();
    virtual void readRect(const Rect& r, rdr::InStream* is,
                          const ConnParams& cp, rdr::OutStream* os);
    virtual void getAffectedRegion(const Rect& rect, const void* buffer,
                                   size_t buflen, const ConnParams& cp,
                                   Region* region);
    virtual void decodeRect(const Rect& r, const void* buffer,
                            size_t buflen, const ConnParams& cp,
                            ModifiablePixelBuffer* pb);
  };
}
#endif
