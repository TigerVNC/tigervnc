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

#include <rdr/types.h>

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
    void writeSetEncodings(const std::list<rdr::U32> encodings);
    void writeSetDesktopSize(int width, int height, const ScreenSet& layout);

    void writeFramebufferUpdateRequest(const Rect& r,bool incremental);
    void writeEnableContinuousUpdates(bool enable, int x, int y, int w, int h);

    void writeFence(rdr::U32 flags, unsigned len, const char data[]);

    void writeKeyEvent(rdr::U32 keysym, rdr::U32 keycode, bool down);
    void writePointerEvent(const Point& pos, int buttonMask);

    void writeClientCutText(const char* str);

    void writeClipboardCaps(rdr::U32 caps, const rdr::U32* lengths);
    void writeClipboardRequest(rdr::U32 flags);
    void writeClipboardPeek(rdr::U32 flags);
    void writeClipboardNotify(rdr::U32 flags);
    void writeClipboardProvide(rdr::U32 flags, const size_t* lengths,
                               const rdr::U8* const* data);

  protected:
    void startMsg(int type);
    void endMsg();

    ServerParams* server;
    rdr::OutStream* os;
  };
}
#endif
