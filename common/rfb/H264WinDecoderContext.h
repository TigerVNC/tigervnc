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

#ifndef __RFB_H264WINDECODER_H__
#define __RFB_H264WINDECODER_H__

#include <windows.h>
#include <mfidl.h>
#include <mftransform.h>

#include <rfb/H264DecoderContext.h>

namespace rfb {
  class H264WinDecoderContext : public H264DecoderContext {
    public:
      H264WinDecoderContext(const Rect &r) : H264DecoderContext(r) {};
      ~H264WinDecoderContext() { freeCodec(); }

      virtual void decode(const rdr::U8* h264_buffer, rdr::U32 len, rdr::U32 flags, ModifiablePixelBuffer* pb);

    protected:
      virtual bool initCodec();
      virtual void freeCodec();

    private:
      LONG stride;
      rdr::U32 full_width = 0;
      rdr::U32 full_height = 0;
      rdr::U32 crop_width = 0;
      rdr::U32 crop_height = 0;
      rdr::U32 offset_x = 0;
      rdr::U32 offset_y = 0;
      IMFTransform *decoder = NULL;
      IMFTransform *converter = NULL;
      IMFSample *input_sample = NULL;
      IMFSample *decoded_sample = NULL;
      IMFSample *converted_sample = NULL;
      IMFMediaBuffer *input_buffer = NULL;
      IMFMediaBuffer *decoded_buffer = NULL;
      IMFMediaBuffer *converted_buffer = NULL;

      void ParseSPS(const rdr::U8* buffer, int length);
  };
}

#endif
