/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2019 Pierre Ossman for Cendio AB
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
// CMsgHandler - class to handle incoming messages on the client side.
//

#ifndef __RFB_CMSGHANDLER_H__
#define __RFB_CMSGHANDLER_H__

#include <stdint.h>

#include <rfb/ServerParams.h>
#include <rfb/Rect.h>
#include <rfb/ScreenSet.h>

namespace rdr { class InStream; }

namespace rfb {

  class CMsgHandler {
  public:
    CMsgHandler();
    virtual ~CMsgHandler();

    // The following methods are called as corresponding messages are
    // read.  A derived class should override these methods as desired.
    // Note that for the setDesktopSize(), setExtendedDesktopSize(),
    // setName(), serverInit() and handleClipboardCaps() methods, a
    // derived class should call on to CMsgHandler's methods to set the
    // members of "server" appropriately.

    virtual void setDesktopSize(int w, int h);
    virtual void setExtendedDesktopSize(unsigned reason, unsigned result,
                                        int w, int h,
                                        const ScreenSet& layout);
    virtual void setCursor(int width, int height, const Point& hotspot,
                           const uint8_t* data) = 0;
    virtual void setCursorPos(const Point& pos) = 0;
    virtual void setName(const char* name);
    virtual void fence(uint32_t flags, unsigned len, const uint8_t data[]);
    virtual void endOfContinuousUpdates();
    virtual void supportsQEMUKeyEvent();
    virtual void serverInit(int width, int height,
                            const PixelFormat& pf,
                            const char* name) = 0;

    virtual bool readAndDecodeRect(const Rect& r, int encoding,
                                   ModifiablePixelBuffer* pb) = 0;

    virtual void framebufferUpdateStart();
    virtual void framebufferUpdateEnd();
    virtual bool dataRect(const Rect& r, int encoding) = 0;

    virtual void setColourMapEntries(int firstColour, int nColours,
				     uint16_t* rgbs) = 0;
    virtual void bell() = 0;
    virtual void serverCutText(const char* str) = 0;

    virtual void setLEDState(unsigned int state);

    virtual void handleClipboardCaps(uint32_t flags,
                                     const uint32_t* lengths);
    virtual void handleClipboardRequest(uint32_t flags);
    virtual void handleClipboardPeek();
    virtual void handleClipboardNotify(uint32_t flags);
    virtual void handleClipboardProvide(uint32_t flags,
                                        const size_t* lengths,
                                        const uint8_t* const* data);

    ServerParams server;
  };
}
#endif
