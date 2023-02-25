/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2019 Pierre Ossman for Cendio AB
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
// CMsgWriter - class for writing RFB messages on the server side.
//

#ifndef __RFB_CMSGWRITER_H__
#define __RFB_CMSGWRITER_H__

#include <list>

#include <stdint.h>

namespace rdr { class OutStream; }

namespace rfb {

  class PixelFormat;
  class ServerParams;
  struct ScreenSet;
  struct Point;
  struct Rect;

  class CMsgWriter {
  public:
    CMsgWriter(ServerParams* server, rdr::OutStream* os);
    virtual ~CMsgWriter();

    void writeClientInit(bool shared);

    void writeSetPixelFormat(const PixelFormat& pf);
    void writeSetEncodings(const std::list<uint32_t> encodings);
    void writeSetDesktopSize(int width, int height, const ScreenSet& layout);

    void writeFramebufferUpdateRequest(const Rect& r,bool incremental);
    void writeEnableContinuousUpdates(bool enable, int x, int y, int w, int h);

    void writeFence(uint32_t flags, unsigned len, const uint8_t data[]);

    void writeKeyEvent(uint32_t keysym, uint32_t keycode, bool down);
    void writePointerEvent(const Point& pos, uint8_t buttonMask);

    void writeClientCutText(const char* str);

    void writeClipboardCaps(uint32_t caps, const uint32_t* lengths);
    void writeClipboardRequest(uint32_t flags);
    void writeClipboardPeek(uint32_t flags);
    void writeClipboardNotify(uint32_t flags);
    void writeClipboardProvide(uint32_t flags, const size_t* lengths,
                               const uint8_t* const* data);

  protected:
    void startMsg(int type);
    void endMsg();

    ServerParams* server;
    rdr::OutStream* os;
  };
}
#endif
