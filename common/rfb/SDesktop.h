/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2024 Pierre Ossman for Cendio AB
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

/////////////////////////////////////////////////////////////////////////////

// SDesktop is an interface implemented by back-ends, on which callbacks are
// made by the VNCServer as appropriate for pointer and keyboard events, etc.
// SDesktop objects are always created before the VNCServer - the SDesktop
// will be passed a pointer to the VNCServer in the start() call.  If a more
// implementation-specific pointer to the VNCServer is required then this
// can be provided to the SDesktop via an implementation-specific method.
//
// An SDesktop usually has an associated PixelBuffer which it tells the
// VNCServer via the VNCServer's setPixelBuffer() method.  It can do this at
// any time, but the PixelBuffer MUST be valid by the time the call to start()
// returns.  The PixelBuffer may be set to null again if desired when stop() is
// called.  Note that start() and stop() are guaranteed to be called
// alternately; there should never be two calls to start() without an
// intervening stop() and vice-versa.
//

#ifndef __RFB_SDESKTOP_H__
#define __RFB_SDESKTOP_H__

#include <rfb/PixelBuffer.h>
#include <rfb/VNCServer.h>
#include <rfb/screenTypes.h>

namespace network { class Socket; }

namespace rfb {

  class SDesktop {
  public:
    // init() is called immediately when the VNCServer gets a reference
    // to the SDesktop, so that a reverse reference can be set up.
    virtual void init(rfb::VNCServer* vs) = 0;

    // start() is called by the server when the first client authenticates
    // successfully, and can be used to begin any expensive tasks which are not
    // needed when there are no clients.  A valid PixelBuffer must have been
    // set via the VNCServer's setPixelBuffer() method by the time this call
    // returns.
    virtual void start() {}

    // stop() is called by the server when there are no longer any
    // authenticated clients, and therefore the desktop can cease any
    // expensive tasks.
    virtual void stop() {}

    // queryConnection() is called when a connection has been
    // successfully authenticated.  The sock and userName arguments
    // identify the socket and the name of the authenticated user, if
    // any. At some point later VNCServer::approveConnection() should
    // be called to either accept or reject the client.
    virtual void queryConnection(network::Socket* sock,
                                 const char* userName) = 0;

    // terminate() is called by the server when it wishes to terminate
    // itself, e.g. because it was configured to terminate when no one is
    // using it.

    virtual void terminate() = 0;

    // setScreenLayout() requests to reconfigure the framebuffer and/or
    // the layout of screens.
    virtual unsigned int setScreenLayout(int /*fb_width*/,
                                         int /*fb_height*/,
                                         const ScreenSet& /*layout*/) {
      return resultProhibited;
    }

    // frameTick() is called whenever a frame update has been processed,
    // signalling that a good time to render new data
    virtual void frameTick(uint64_t msc) { (void)msc; }

    // keyEvent() is called whenever a client sends an event that a
    // key was pressed or released.
    virtual void keyEvent(uint32_t /*keysym*/, uint32_t /*keycode*/,
                          bool /*down*/) {};

    // pointerEvent() is called whenever a client sends an event that
    // the pointer moved, or a button was pressed or released.
    virtual void pointerEvent(const Point& /*pos*/,
                              uint8_t /*buttonMask*/) {};

    // handleClipboardRequest() is called whenever a client requests
    // the server to send over its clipboard data. It will only be
    // called after the server has first announced a clipboard change
    // via VNCServer::announceClipboard().
    virtual void handleClipboardRequest() {}

    // handleClipboardAnnounce() is called to indicate a change in the
    // clipboard on a client. Call VNCServer::requestClipboard() to
    // access the actual data.
    virtual void handleClipboardAnnounce(bool /*available*/) {}

    // handleClipboardData() is called when a client has sent over
    // the clipboard data as a result of a previous call to
    // VNCServer::requestClipboard(). Note that this function might
    // never be called if the clipboard data was no longer available
    // when the client received the request.
    virtual void handleClipboardData(const char* /*data*/) {}

  protected:
    virtual ~SDesktop() {}
  };

  // -=- SStaticDesktop
  //     Trivial implementation of the SDesktop interface, which provides
  //     dummy input handlers and event processing routine, and exports
  //     a plain black desktop of the specified format.
  class SStaticDesktop : public SDesktop {
  public:
    SStaticDesktop(const Point& size)
      : server(nullptr), buffer(nullptr)
    {
      PixelFormat pf;
      const uint8_t black[4] = { 0, 0, 0, 0 };
      buffer = new ManagedPixelBuffer(pf, size.x, size.y);
      if (buffer)
        buffer->fillRect(buffer->getRect(), black);
    }
    SStaticDesktop(const Point& size, const PixelFormat& pf)
      : buffer(nullptr)
    {
      const uint8_t black[4] = { 0, 0, 0, 0 };
      buffer = new ManagedPixelBuffer(pf, size.x, size.y);
      if (buffer)
        buffer->fillRect(buffer->getRect(), black);
    }
    virtual ~SStaticDesktop() {
      if (buffer) delete buffer;
    }

    void init(VNCServer* vs) override {
      server = vs;
      server->setPixelBuffer(buffer);
    }
    void queryConnection(network::Socket* sock,
                         const char* /*userName*/) override {
      server->approveConnection(sock, true, nullptr);
    }

  protected:
    VNCServer* server;
    ManagedPixelBuffer* buffer;
  };

};

#endif // __RFB_SDESKTOP_H__
