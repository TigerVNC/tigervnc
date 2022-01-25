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

#ifndef __RFB_H264DECODERCONTEXT_H__
#define __RFB_H264DECODERCONTEXT_H__

#include <os/Mutex.h>
#include <rdr/types.h>
#include <rfb/Rect.h>
#include <rfb/Decoder.h>

namespace rfb {
  class H264DecoderContext {
    public:
      static H264DecoderContext *createContext(const Rect &r);

      virtual ~H264DecoderContext() = 0;

      virtual void decode(const rdr::U8* h264_buffer, rdr::U32 len, rdr::U32 flags, ModifiablePixelBuffer* pb) {}
      void reset();

      inline bool isEqualRect(const Rect &r) const { return r.equals(rect); }
      bool isReady();

    protected:
      os::Mutex mutex;
      rfb::Rect rect;
      bool initialized;

      H264DecoderContext(const Rect &r) : rect(r) { initialized = false; }

      virtual bool initCodec() { return false; }
      virtual void freeCodec() {}
  };
}

#endif
