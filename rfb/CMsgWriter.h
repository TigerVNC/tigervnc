/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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

namespace rdr { class OutStream; }

namespace rfb {

  class PixelFormat;
  class ConnParams;
  struct Rect;

  class CMsgWriter {
  public:
    virtual ~CMsgWriter();

    virtual void writeClientInit(bool shared)=0;

    virtual void writeSetPixelFormat(const PixelFormat& pf);
    virtual void writeSetEncodings(int nEncodings, rdr::U32* encodings);
    virtual void writeSetEncodings(int preferredEncoding, bool useCopyRect);
    virtual void writeFramebufferUpdateRequest(const Rect& r,bool incremental);
    virtual void writeKeyEvent(rdr::U32 key, bool down);
    virtual void writePointerEvent(int x, int y, int buttonMask);
    virtual void writeClientCutText(const char* str, int len);

    virtual void startMsg(int type)=0;
    virtual void endMsg()=0;

    virtual void setOutStream(rdr::OutStream* os);

    ConnParams* getConnParams() { return cp; }
    rdr::OutStream* getOutStream() { return os; }

  protected:
    CMsgWriter(ConnParams* cp, rdr::OutStream* os);

    ConnParams* cp;
    rdr::OutStream* os;
  };
}
#endif
