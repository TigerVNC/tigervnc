/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2014 Pierre Ossman for Cendio AB
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

#include <rdr/types.h>

#include <rfb/InputHandler.h>

namespace rdr { class OutStream; }

namespace rfb {

  class PixelFormat;
  class ConnParams;
  struct ScreenSet;
  struct Rect;

  class CMsgWriter : public InputHandler {
  public:
    CMsgWriter(ConnParams* cp, rdr::OutStream* os);
    virtual ~CMsgWriter();

    void writeClientInit(bool shared);

    void writeSetPixelFormat(const PixelFormat& pf);
    void writeSetEncodings(int nEncodings, rdr::U32* encodings);
    void writeSetEncodings(int preferredEncoding, bool useCopyRect);
    void writeSetDesktopSize(int width, int height, const ScreenSet& layout);

    void writeFramebufferUpdateRequest(const Rect& r,bool incremental);
    void writeEnableContinuousUpdates(bool enable, int x, int y, int w, int h);

    void writeFence(rdr::U32 flags, unsigned len, const char data[]);

    // InputHandler implementation

    virtual void keyEvent(rdr::U32 key, bool down);
    virtual void pointerEvent(const Point& pos, int buttonMask);
    virtual void clientCutText(const char* str, int len);

  protected:
    void startMsg(int type);
    void endMsg();

    ConnParams* cp;
    rdr::OutStream* os;
  };
}
#endif
