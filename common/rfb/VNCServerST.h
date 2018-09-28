/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2016 Pierre Ossman for Cendio AB
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
#include <rfb/LogWriter.h>
#include <rfb/Blacklist.h>
#include <rfb/Cursor.h>
#include <rfb/Timer.h>
#include <network/Socket.h>
#include <rfb/ScreenSet.h>

namespace rfb {

  class VNCSConnectionST;
  class ComparingUpdateTracker;
  class ListConnInfo;
  class PixelBuffer;
  class KeyRemapper;

  class VNCServerST : public VNCServer,
                      public Timer::Callback,
                      public network::SocketServer {
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

    // checkTimeouts
    //   Returns the number of milliseconds left until the next idle timeout
    //   expires.  If any have already expired, the corresponding connections
    //   are closed.  Zero is returned if there is no idle timeout.
    virtual int checkTimeouts();


    // Methods overridden from VNCServer

    virtual void blockUpdates();
    virtual void unblockUpdates();
    virtual void setPixelBuffer(PixelBuffer* pb, const ScreenSet& layout);
    virtual void setPixelBuffer(PixelBuffer* pb);
    virtual void setScreenLayout(const ScreenSet& layout);
    virtual PixelBuffer* getPixelBuffer() const { return pb; }
    virtual void serverCutText(const char* str, int len);
    virtual void add_changed(const Region &region);
    virtual void add_copied(const Region &dest, const Point &delta);
    virtual void setCursor(int width, int height, const Point& hotspot,
                           const rdr::U8* data);
    virtual void setCursorPos(const Point& p);
    virtual void setLEDState(unsigned state);

    virtual void bell();

    // - Close all currently-connected clients, by calling
    //   their close() method with the supplied reason.
    virtual void closeClients(const char* reason) {closeClients(reason, 0);}

    // VNCServerST-only methods

    // closeClients() closes all RFB sessions, except the specified one (if
    // any), and logs the specified reason for closure.
    void closeClients(const char* reason, network::Socket* sock);

    // getSConnection() gets the SConnection for a particular Socket.  If
    // the Socket is not recognised then null is returned.

    SConnection* getSConnection(network::Socket* sock);

    // getName() returns the name of this VNC Server.  NB: The value returned
    // is the server's internal buffer which may change after any other methods
    // are called - take a copy if necessary.
    const char* getName() const {return name.buf;}

    // setName() specifies the desktop name that the server should provide to
    // clients
    virtual void setName(const char* name_);

    // A QueryConnectionHandler, if supplied, is passed details of incoming
    // connections to approve, reject, or query the user about.
    //
    // queryConnection() is called when a connection has been
    // successfully authenticated.  The sock and userName arguments identify
    // the socket and the name of the authenticated user, if any.  It should
    // return ACCEPT if the connection should be accepted, REJECT if it should
    // be rejected, or PENDING if a decision cannot yet be reached.  If REJECT
    // is returned, *reason can be set to a string describing the reason - this
    // will be delete[]ed when it is finished with.  If PENDING is returned,
    // approveConnection() must be called some time later to accept or reject
    // the connection.
    enum queryResult { ACCEPT, REJECT, PENDING };
    struct QueryConnectionHandler {
      virtual ~QueryConnectionHandler() {}
      virtual queryResult queryConnection(network::Socket* sock,
                                          const char* userName,
                                          char** reason) = 0;
    };
    void setQueryConnectionHandler(QueryConnectionHandler* qch) {
      queryConnectionHandler = qch;
    }

    // queryConnection is called as described above, and either passes the
    // request on to the registered handler, or accepts the connection if
    // no handler has been specified.
    virtual queryResult queryConnection(network::Socket* sock,
                                        const char* userName,
                                        char** reason) {
      return queryConnectionHandler
        ? queryConnectionHandler->queryConnection(sock, userName, reason)
        : ACCEPT;
    }

    // approveConnection() is called by the active QueryConnectionHandler,
    // some time after queryConnection() has returned with PENDING, to accept
    // or reject the connection.  The accept argument should be true for
    // acceptance, or false for rejection, in which case a string reason may
    // also be given.
    void approveConnection(network::Socket* sock, bool accept,
                           const char* reason);

    // setBlacklist() is called to replace the VNCServerST's internal
    // Blacklist instance with another instance.  This allows a single
    // Blacklist to be shared by multiple VNCServerST instances.
    void setBlacklist(Blacklist* bl) {blHosts = bl ? bl : &blacklist;}

    // setKeyRemapper() replaces the VNCServerST's default key remapper.
    // NB: A null pointer is valid here.
    void setKeyRemapper(KeyRemapper* kr) { keyRemapper = kr; }

    void getConnInfo(ListConnInfo * listConn);
    void setConnStatus(ListConnInfo* listConn);

    bool getDisable() { return disableclients;};
    void setDisable(bool disable) { disableclients = disable;};

  protected:

    friend class VNCSConnectionST;

    // Timer callbacks
    virtual bool handleTimeout(Timer* t);

    // - Internal methods

    void startDesktop();
    void stopDesktop();

    static LogWriter connectionsLog;
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
    std::list<network::Socket*> closingSockets;

    ComparingUpdateTracker* comparer;

    Point cursorPos;
    Cursor* cursor;
    RenderedCursor renderedCursor;
    bool renderedCursorInvalid;

    // - Check how many of the clients are authenticated.
    int authClientCount();

    bool needRenderedCursor();
    void startFrameClock();
    void stopFrameClock();
    int msToNextUpdate();
    void writeUpdate();
    Region getPendingRegion();
    const RenderedCursor* getRenderedCursor();

    void notifyScreenLayoutChange(VNCSConnectionST *requester);

    bool getComparerState();

    QueryConnectionHandler* queryConnectionHandler;
    KeyRemapper* keyRemapper;

    time_t lastUserInputTime;
    time_t lastDisconnectTime;
    time_t lastConnectionTime;

    bool disableclients;

    Timer frameTimer;
  };

};

#endif

