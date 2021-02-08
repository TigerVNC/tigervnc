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

// -=- VNCServerST.h

// Single-threaded VNCServer implementation

#ifndef __RFB_VNCSERVERST_H__
#define __RFB_VNCSERVERST_H__

#include <sys/time.h>

#include <rfb/SDesktop.h>
#include <rfb/VNCServer.h>
#include <rfb/Blacklist.h>
#include <rfb/Cursor.h>
#include <rfb/Timer.h>
#include <rfb/ScreenSet.h>

namespace rfb {

  class VNCSConnectionST;
  class ComparingUpdateTracker;
  class ListConnInfo;
  class PixelBuffer;
  class KeyRemapper;

  class VNCServerST : public VNCServer,
                      public Timer::Callback {
  public:
    // -=- Constructors

    //   Create a server exporting the supplied desktop.
    VNCServerST(const char* name_, SDesktop* desktop_);
    virtual ~VNCServerST();


    // Methods overridden from SocketServer

    // addSocket
    //   Causes the server to allocate an RFB-protocol management
    //   structure for the socket & initialise it.
    virtual void addSocket(network::Socket* sock, bool outgoing=false);

    // removeSocket
    //   Clean up any resources associated with the Socket
    virtual void removeSocket(network::Socket* sock);

    // getSockets() gets a list of sockets.  This can be used to generate an
    // fd_set for calling select().
    virtual void getSockets(std::list<network::Socket*>* sockets);

    // processSocketReadEvent
    //   Read more RFB data from the Socket.  If an error occurs during
    //   processing then shutdown() is called on the Socket, causing
    //   removeSocket() to be called by the caller at a later time.
    virtual void processSocketReadEvent(network::Socket* sock);

    // processSocketWriteEvent
    //   Flush pending data from the Socket on to the network.
    virtual void processSocketWriteEvent(network::Socket* sock);


    // Methods overridden from VNCServer

    virtual void blockUpdates();
    virtual void unblockUpdates();
    virtual void setPixelBuffer(PixelBuffer* pb, const ScreenSet& layout);
    virtual void setPixelBuffer(PixelBuffer* pb);
    virtual void setScreenLayout(const ScreenSet& layout);
    virtual const PixelBuffer* getPixelBuffer() const { return pb; }

    virtual void requestClipboard();
    virtual void announceClipboard(bool available);
    virtual void sendClipboardData(const char* data);

    virtual void approveConnection(network::Socket* sock, bool accept,
                                   const char* reason);
    virtual void closeClients(const char* reason) {closeClients(reason, 0);}
    virtual SConnection* getConnection(network::Socket* sock);

    virtual void add_changed(const Region &region);
    virtual void add_copied(const Region &dest, const Point &delta);
    virtual void setCursor(int width, int height, const Point& hotspot,
                           const rdr::U8* data);
    virtual void setCursorPos(const Point& p, bool warped);
    virtual void setName(const char* name_);
    virtual void setLEDState(unsigned state);

    virtual void bell();

    // VNCServerST-only methods

    // Methods to get the currently set server state

    const ScreenSet& getScreenLayout() const { return screenLayout; }
    const Cursor* getCursor() const { return cursor; }
    const Point& getCursorPos() const { return cursorPos; }
    const char* getName() const { return name.buf; }
    unsigned getLEDState() const { return ledState; }

    // Event handlers
    void keyEvent(rdr::U32 keysym, rdr::U32 keycode, bool down);
    void pointerEvent(VNCSConnectionST* client, const Point& pos, int buttonMask);

    void handleClipboardRequest(VNCSConnectionST* client);
    void handleClipboardAnnounce(VNCSConnectionST* client, bool available);
    void handleClipboardData(VNCSConnectionST* client, const char* data);

    unsigned int setDesktopSize(VNCSConnectionST* requester,
                                int fb_width, int fb_height,
                                const ScreenSet& layout);

    // closeClients() closes all RFB sessions, except the specified one (if
    // any), and logs the specified reason for closure.
    void closeClients(const char* reason, network::Socket* sock);

    // queryConnection() does some basic checks and then passes on the
    // request to the desktop.
    void queryConnection(VNCSConnectionST* client, const char* userName);

    // clientReady() is called by a VNCSConnectionST instance when the
    // client has completed the handshake and is ready for normal
    // communication.
    void clientReady(VNCSConnectionST* client, bool shared);

    // Estimated time until the next time new updates will be pushed
    // to clients
    int msToNextUpdate();

    // Part of the framebuffer that has been modified but is not yet
    // ready to be sent to clients
    Region getPendingRegion();

    // getRenderedCursor() returns an up to date version of the server
    // side rendered cursor buffer
    const RenderedCursor* getRenderedCursor();

  protected:

    // Timer callbacks
    virtual bool handleTimeout(Timer* t);

    // - Internal methods

    void startDesktop();
    void stopDesktop();

    // - Check how many of the clients are authenticated.
    int authClientCount();

    bool needRenderedCursor();
    void startFrameClock();
    void stopFrameClock();
    void writeUpdate();

    bool getComparerState();

  protected:
    Blacklist blacklist;
    Blacklist* blHosts;

    SDesktop* desktop;
    bool desktopStarted;
    int blockCounter;
    PixelBuffer* pb;
    ScreenSet screenLayout;
    unsigned int ledState;

    CharArray name;

    std::list<VNCSConnectionST*> clients;
    VNCSConnectionST* pointerClient;
    VNCSConnectionST* clipboardClient;
    std::list<VNCSConnectionST*> clipboardRequestors;
    std::list<network::Socket*> closingSockets;

    ComparingUpdateTracker* comparer;

    Point cursorPos;
    Cursor* cursor;
    RenderedCursor renderedCursor;
    bool renderedCursorInvalid;

    KeyRemapper* keyRemapper;

    Timer idleTimer;
    Timer disconnectTimer;
    Timer connectTimer;

    Timer frameTimer;
  };

};

#endif

