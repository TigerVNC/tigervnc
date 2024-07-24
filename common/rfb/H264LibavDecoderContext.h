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

#ifndef __RFB_H264LIBAVDECODER_H__
#define __RFB_H264LIBAVDECODER_H__

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

#include <rfb/H264DecoderContext.h>

namespace rfb {
  class H264LibavDecoderContext : public H264DecoderContext {
    public:
      H264LibavDecoderContext(const Rect &r) : H264DecoderContext(r) {}
      ~H264LibavDecoderContext() { freeCodec(); }

      void decode(const uint8_t* h264_buffer, uint32_t len,
                  ModifiablePixelBuffer* pb) override;

    protected:
      bool initCodec() override;
      void freeCodec() override;

    private:
      uint8_t* makeH264WorkBuffer(const uint8_t* buffer, uint32_t len);

      AVCodecContext *avctx;
      AVCodecParserContext *parser;
      AVFrame* frame;
      AVFrame* rgbFrame = nullptr;
      SwsContext* sws;
      uint8_t* h264WorkBuffer;
      uint32_t h264WorkBufferLength;
  };
}

#endif
