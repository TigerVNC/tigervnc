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
// SMsgHandler - class to handle incoming messages on the server side.
//

#ifndef __RFB_SMSGHANDLER_H__
#define __RFB_SMSGHANDLER_H__

#include <stdint.h>

#include <rfb/ClientParams.h>

namespace rfb {

  class SMsgHandler {
  public:
    virtual ~SMsgHandler() {}

    // The following methods are called as corresponding messages are
    // read. A derived class must override these methods.

    virtual void clientInit(bool shared) = 0;

    virtual void setPixelFormat(const PixelFormat& pf) = 0;
    virtual void setEncodings(int nEncodings,
                              const int32_t* encodings) = 0;
    virtual void framebufferUpdateRequest(const core::Rect& r,
                                          bool incremental) = 0;
    virtual void setDesktopSize(int fb_width, int fb_height,
                                const ScreenSet& layout) = 0;
    virtual void fence(uint32_t flags, unsigned len,
                       const uint8_t data[]) = 0;
    virtual void enableContinuousUpdates(bool enable,
                                         int x, int y,
                                         int w, int h) = 0;

    virtual void keyEvent(uint32_t keysym, uint32_t keycode,
                          bool down) = 0;
    virtual void pointerEvent(const core::Point& pos,
                              uint16_t buttonMask) = 0;

    virtual void clientCutText(const char* str) = 0;

    virtual void handleClipboardCaps(uint32_t flags,
                                     const uint32_t* lengths) = 0;
    virtual void handleClipboardRequest(uint32_t flags) = 0;
    virtual void handleClipboardPeek() = 0;
    virtual void handleClipboardNotify(uint32_t flags) = 0;
    virtual void handleClipboardProvide(uint32_t flags,
                                        const size_t* lengths,
                                        const uint8_t* const* data) = 0;

    ClientParams client;
  };
}
#endif
