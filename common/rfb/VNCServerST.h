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


    // Methods overridden from VNCServer

    // addSocket
    //   Causes the server to allocate an RFB-protocol management
    //   structure for the socket & initialise it.
    void addSocket(network::Socket* sock, bool outgoing=false,
                   AccessRights ar=AccessDefault) override;

    // removeSocket
    //   Clean up any resources associated with the Socket
    void removeSocket(network::Socket* sock) override;

    // getSockets() gets a list of sockets.  This can be used to generate an
    // fd_set for calling select().
    void getSockets(std::list<network::Socket*>* sockets) override;

    // processSocketReadEvent
    //   Read more RFB data from the Socket.  If an error occurs during
    //   processing then shutdown() is called on the Socket, causing
    //   removeSocket() to be called by the caller at a later time.
    void processSocketReadEvent(network::Socket* sock) override;

    // processSocketWriteEvent
    //   Flush pending data from the Socket on to the network.
    void processSocketWriteEvent(network::Socket* sock) override;

    void blockUpdates() override;
    void unblockUpdates() override;
    uint64_t getMsc() override;
    void queueMsc(uint64_t target) override;
    void setPixelBuffer(PixelBuffer* pb, const ScreenSet& layout) override;
    void setPixelBuffer(PixelBuffer* pb) override;
    void setScreenLayout(const ScreenSet& layout) override;
    const PixelBuffer* getPixelBuffer() const override { return pb; }

    void requestClipboard() override;
    void announceClipboard(bool available) override;
    void sendClipboardData(const char* data) override;

    void approveConnection(network::Socket* sock, bool accept,
                           const char* reason) override;
    void closeClients(const char* reason) override {closeClients(reason, nullptr);}
    SConnection* getConnection(network::Socket* sock) override;

    void add_changed(const Region &region) override;
    void add_copied(const Region &dest, const Point &delta) override;
    void setCursor(int width, int height, const Point& hotspot,
                   const uint8_t* data) override;
    void setCursorPos(const Point& p, bool warped) override;
    void setName(const char* name_) override;
    void setLEDState(unsigned state) override;

    void bell() override;

    // VNCServerST-only methods

    // Methods to get the currently set server state

    const ScreenSet& getScreenLayout() const { return screenLayout; }
    const Cursor* getCursor() const { return cursor; }
    const Point& getCursorPos() const { return cursorPos; }
    const char* getName() const { return name.c_str(); }
    unsigned getLEDState() const { return ledState; }

    // Event handlers
    void keyEvent(uint32_t keysym, uint32_t keycode, bool down);
    void pointerEvent(VNCSConnectionST* client, const Point& pos, uint8_t buttonMask);

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
    void handleTimeout(Timer* t) override;

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

    std::string name;

    std::list<VNCSConnectionST*> clients;
    VNCSConnectionST* pointerClient;
    VNCSConnectionST* clipboardClient;
    std::list<VNCSConnectionST*> clipboardRequestors;
    std::list<network::Socket*> closingSockets;

    time_t pointerClientTime;

    ComparingUpdateTracker* comparer;

    Point cursorPos;
    Cursor* cursor;
    RenderedCursor renderedCursor;
    bool renderedCursorInvalid;

    KeyRemapper* keyRemapper;

    Timer idleTimer;
    Timer disconnectTimer;
    Timer connectTimer;

    uint64_t msc, queuedMsc;
    Timer frameTimer;
  };

};

#endif

