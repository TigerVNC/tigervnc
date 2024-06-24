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
// VNCServer - abstract interface implemented by the RFB library.  The back-end
// code calls the relevant methods as appropriate.

#ifndef __RFB_VNCSERVER_H__
#define __RFB_VNCSERVER_H__

#include <rfb/UpdateTracker.h>
#include <rfb/SSecurity.h>
#include <rfb/ScreenSet.h>

namespace network { class Socket; }

namespace rfb {

  class VNCServer : public UpdateTracker {
  public:
    // addSocket() tells the server to serve the Socket.  The caller
    //   retains ownership of the Socket - the only way for the server
    //   to discard a Socket is by calling shutdown() on it.
    //   outgoing is set to true if the socket was created by connecting out
    //   to another host, or false if the socket was created by accept()ing
    //   an incoming connection.
    //   accessRights allows to set the access rights to the server.
    virtual void addSocket(network::Socket* sock, bool outgoing=false,
                           AccessRights accessRights = AccessDefault) = 0;

    // removeSocket() tells the server to stop serving the Socket.  The
    //   caller retains ownership of the Socket - the server must NOT
    //   delete the Socket!  This call is used mainly to cause per-Socket
    //   resources to be freed.
    virtual void removeSocket(network::Socket* sock) = 0;

    // getSockets() gets a list of sockets.  This can be used to generate an
    //   fd_set for calling select().
    virtual void getSockets(std::list<network::Socket*>* sockets) = 0;

    // processSocketReadEvent() tells the server there is a Socket read event.
    //   The implementation can indicate that the Socket is no longer active
    //   by calling shutdown() on it.  The caller will then call removeSocket()
    //   soon after processSocketEvent returns, to allow any pre-Socket
    //   resources to be tidied up.
    virtual void processSocketReadEvent(network::Socket* sock) = 0;

    // processSocketReadEvent() tells the server there is a Socket write event.
    //   This is only necessary if the Socket has been put in non-blocking
    //   mode and needs this callback to flush the buffer.
    virtual void processSocketWriteEvent(network::Socket* sock) = 0;

    // blockUpdates()/unblockUpdates() tells the server that the pixel buffer
    // is currently in flux and may not be accessed. The attributes of the
    // pixel buffer may still be accessed, but not the frame buffer itself.
    // Note that access must be unblocked the exact same number of times it
    // was blocked.
    virtual void blockUpdates() = 0;
    virtual void unblockUpdates() = 0;

    virtual uint64_t getMsc() = 0;
    virtual void queueMsc(uint64_t target) = 0;

    // setPixelBuffer() tells the server to use the given pixel buffer (and
    // optionally a modified screen layout).  If this differs in size from
    // the previous pixel buffer, this may result in protocol messages being
    // sent, or clients being disconnected.
    virtual void setPixelBuffer(PixelBuffer* pb, const ScreenSet& layout) = 0;
    virtual void setPixelBuffer(PixelBuffer* pb) = 0;

    // setScreenLayout() modifies the current screen layout without changing
    // the pixelbuffer. Clients will be notified of the new layout.
    virtual void setScreenLayout(const ScreenSet& layout) = 0;

    // getPixelBuffer() returns a pointer to the PixelBuffer object.
    virtual const PixelBuffer* getPixelBuffer() const = 0;

    // requestClipboard() will result in a request to a client to
    // transfer its clipboard data. A call to
    // SDesktop::handleClipboardData() will be made once the data is
    // available.
    virtual void requestClipboard() = 0;

    // announceClipboard() informs all clients of changes to the
    // clipboard on the server. A client may later request the
    // clipboard data via SDesktop::handleClipboardRequest().
    virtual void announceClipboard(bool available) = 0;

    // sendClipboardData() transfers the clipboard data to a client
    // and should be called whenever a client has requested the
    // clipboard via SDesktop::handleClipboardRequest().
    virtual void sendClipboardData(const char* data) = 0;

    // bell() tells the server that it should make all clients make a bell sound.
    virtual void bell() = 0;

    // approveConnection() is called some time after
    // SDesktop::queryConnection() has been called, to accept or reject
    // the connection.  The accept argument should be true for
    // acceptance, or false for rejection, in which case a string
    // reason may also be given.
    virtual void approveConnection(network::Socket* sock, bool accept,
                                   const char* reason = nullptr) = 0;

    // - Close all currently-connected clients, by calling
    //   their close() method with the supplied reason.
    virtual void closeClients(const char* reason) = 0;

    // getConnection() gets the SConnection for a particular Socket.  If
    // the Socket is not recognised then null is returned.
    virtual SConnection* getConnection(network::Socket* sock) = 0;

    // setCursor() tells the server that the cursor has changed.  The
    // cursorData argument contains width*height rgba quadruplets with
    // non-premultiplied alpha.
    virtual void setCursor(int width, int height, const Point& hotspot,
                           const uint8_t* cursorData) = 0;

    // setCursorPos() tells the server the current position of the cursor, and
    // whether the server initiated that change (e.g. through another X11
    // client calling XWarpPointer()).
    virtual void setCursorPos(const Point& p, bool warped) = 0;

    // setName() tells the server what desktop title to supply to clients
    virtual void setName(const char* name) = 0;

    // setLEDState() tells the server what the current lock keys LED
    // state is
    virtual void setLEDState(unsigned int state) = 0;
  };
}
#endif
