/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2011 Pierre Ossman for Cendio AB
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

#include <rdr/types.h>
#include <rfb/PixelFormat.h>
#include <rfb/ConnParams.h>
#include <rfb/InputHandler.h>
#include <rfb/ScreenSet.h>

namespace rdr { class InStream; }

namespace rfb {

  class SMsgHandler : public InputHandler {
  public:
    SMsgHandler();
    virtual ~SMsgHandler();

    // The following methods are called as corresponding messages are read.  A
    // derived class should override these methods as desired.  Note that for
    // the setPixelFormat(), setEncodings() and setDesktopSize() methods, a
    // derived class must call on to SMsgHandler's methods.

    virtual void clientInit(bool shared);

    virtual void setPixelFormat(const PixelFormat& pf);
    virtual void setEncodings(int nEncodings, const rdr::S32* encodings);
    virtual void framebufferUpdateRequest(const Rect& r, bool incremental) = 0;
    virtual void setDesktopSize(int fb_width, int fb_height,
                                const ScreenSet& layout) = 0;
    virtual void fence(rdr::U32 flags, unsigned len, const char data[]) = 0;
    virtual void enableContinuousUpdates(bool enable,
                                         int x, int y, int w, int h) = 0;

    // InputHandler interface
    // The InputHandler methods will be called for the corresponding messages.

    // supportsLocalCursor() is called whenever the status of
    // cp.supportsLocalCursor has changed.  At the moment this happens on a
    // setEncodings message, but in the future this may be due to a message
    // specially for this purpose.
    virtual void supportsLocalCursor();

    // supportsFence() is called the first time we detect support for fences
    // in the client. A fence message should be sent at this point to notify
    // the client of server support.
    virtual void supportsFence();

    // supportsContinuousUpdates() is called the first time we detect that
    // the client wants the continuous updates extension. A
    // EndOfContinuousUpdates message should be sent back to the client at
    // this point if it is supported.
    virtual void supportsContinuousUpdates();

    ConnParams cp;
  };
}
#endif
