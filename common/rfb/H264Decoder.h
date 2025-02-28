/* Copyright (C) 2021 Vladimir Sukhonosov <xornet@xornet.org>
 * Copyright (C) 2021 Martins Mozeiko <martins.mozeiko@gmail.com>
 * All Rights Reserved.
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

#ifndef __RFB_H264DECODER_H__
#define __RFB_H264DECODER_H__

#include <list>

#include <rfb/Decoder.h>

namespace rfb {
  class H264DecoderContext;

  class H264Decoder : public Decoder {
  public:
    H264Decoder();
    virtual ~H264Decoder();
    bool readRect(const core::Rect& r, rdr::InStream* is,
                  const ServerParams& server,
                  rdr::OutStream* os) override;
    void decodeRect(const core::Rect& r, const uint8_t* buffer,
                    size_t buflen, const ServerParams& server,
                    ModifiablePixelBuffer* pb) override;

  private:
    void resetContexts();
    H264DecoderContext* findContext(const core::Rect& r);

    std::list<H264DecoderContext*> contexts;
  };
}

#endif
