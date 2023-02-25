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
// VNCSConnectionST is our derived class of SConnection for VNCServerST - there
// is one for each connected client.  We think of VNCSConnectionST as part of
// the VNCServerST implementation, so its methods are allowed full access to
// members of VNCServerST.
//

#ifndef __RFB_VNCSCONNECTIONST_H__
#define __RFB_VNCSCONNECTIONST_H__

#include <map>

#include <rfb/Congestion.h>
#include <rfb/EncodeManager.h>
#include <rfb/SConnection.h>
#include <rfb/Timer.h>

namespace rfb {
  class VNCServerST;

  class VNCSConnectionST : private SConnection,
                           public Timer::Callback {
  public:
    VNCSConnectionST(VNCServerST* server_, network::Socket* s, bool reverse,
                     AccessRights ar);
    virtual ~VNCSConnectionST();

    // SConnection methods

    bool accessCheck(AccessRights ar) const override;
    void close(const char* reason) override;

    using SConnection::authenticated;

    // Methods called from VNCServerST.  None of these methods ever knowingly
    // throw an exception.

    // init() must be called to initialise the protocol.  If it fails it
    // returns false, and close() will have been called.
    bool init();

    // processMessages() processes incoming messages from the client, invoking
    // various callbacks as a result.  It continues to process messages until
    // reading might block.  shutdown() will be called on the connection's
    // Socket if an error occurs, via the close() call.
    void processMessages();

    // flushSocket() pushes any unwritten data on to the network.
    void flushSocket();

    // Called when the underlying pixelbuffer is resized or replaced.
    void pixelBufferChange();

    // Wrappers to make these methods "safe" for VNCServerST.
    void writeFramebufferUpdateOrClose();
    void screenLayoutChangeOrClose(uint16_t reason);
    void setCursorOrClose();
    void bellOrClose();
    void setDesktopNameOrClose(const char *name);
    void setLEDStateOrClose(unsigned int state);
    void approveConnectionOrClose(bool accept, const char* reason);
    void requestClipboardOrClose();
    void announceClipboardOrClose(bool available);
    void sendClipboardDataOrClose(const char* data);

    // The following methods never throw exceptions

    // getComparerState() returns if this client would like the framebuffer
    // comparer to be enabled.
    bool getComparerState();

    // renderedCursorChange() is called whenever the server-side rendered
    // cursor changes shape or position.  It ensures that the next update will
    // clean up the old rendered cursor and if necessary draw the new rendered
    // cursor.
    void renderedCursorChange();

    // cursorPositionChange() is called whenever the cursor has changed position by
    // the server.  If the client supports being informed about these changes then
    // it will arrange for the new cursor position to be sent to the client.
    void cursorPositionChange();

    // needRenderedCursor() returns true if this client needs the server-side
    // rendered cursor.  This may be because it does not support local cursor
    // or because the current cursor position has not been set by this client.
    bool needRenderedCursor();

    network::Socket* getSock() { return sock; }

    // Change tracking

    void add_changed(const Region& region) { updates.add_changed(region); }
    void add_copied(const Region& dest, const Point& delta) {
      updates.add_copied(dest, delta);
    }

    const char* getPeerEndpoint() const {return peerEndpoint.c_str();}

  private:
    // SConnection callbacks

    // These methods are invoked as callbacks from processMsg(
    void authSuccess() override;
    void queryConnection(const char* userName) override;
    void clientInit(bool shared) override;
    void setPixelFormat(const PixelFormat& pf) override;
    void pointerEvent(const Point& pos, uint8_t buttonMask) override;
    void keyEvent(uint32_t keysym, uint32_t keycode,
                  bool down) override;
    void framebufferUpdateRequest(const Rect& r,
                                  bool incremental) override;
    void setDesktopSize(int fb_width, int fb_height,
                        const ScreenSet& layout) override;
    void fence(uint32_t flags, unsigned len,
               const uint8_t data[]) override;
    void enableContinuousUpdates(bool enable,
                                 int x, int y, int w, int h) override;
    void handleClipboardRequest() override;
    void handleClipboardAnnounce(bool available) override;
    void handleClipboardData(const char* data) override;
    void supportsLocalCursor() override;
    void supportsFence() override;
    void supportsContinuousUpdates() override;
    void supportsLEDState() override;

    // Timer callbacks
    void handleTimeout(Timer* t) override;

    // Internal methods

    bool isShiftPressed();

    // Congestion control
    void writeRTTPing();
    bool isCongested();

    // writeFramebufferUpdate() attempts to write a framebuffer update to the
    // client.

    void writeFramebufferUpdate();
    void writeNoDataUpdate();
    void writeDataUpdate();
    void writeLosslessRefresh();

    void screenLayoutChange(uint16_t reason);
    void setCursor();
    void setCursorPos();
    void setDesktopName(const char *name);
    void setLEDState(unsigned int state);

  private:
    network::Socket* sock;
    std::string peerEndpoint;
    bool reverseConnection;

    bool inProcessMessages;

    bool pendingSyncFence, syncFence;
    uint32_t fenceFlags;
    unsigned fenceDataLen;
    uint8_t *fenceData;

    Congestion congestion;
    Timer congestionTimer;
    Timer losslessTimer;

    VNCServerST* server;
    SimpleUpdateTracker updates;
    Region requested;
    bool updateRenderedCursor, removeRenderedCursor;
    Region damagedCursorRegion;
    bool continuousUpdates;
    Region cuRegion;
    EncodeManager encodeManager;

    std::map<uint32_t, uint32_t> pressedKeys;

    Timer idleTimer;

    time_t pointerEventTime;
    Point pointerEventPos;
    bool clientHasCursor;

    std::string closeReason;
  };
}
#endif
