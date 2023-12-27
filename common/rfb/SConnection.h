/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2019 Pierre Ossman for Cendio AB
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
// SConnection - class on the server side representing a connection to a
// client.  A derived class should override methods appropriately.
//

#ifndef __RFB_SCONNECTION_H__
#define __RFB_SCONNECTION_H__

#include <string>

#include <rdr/InStream.h>
#include <rdr/OutStream.h>

#include <rfb/AccessRights.h>
#include <rfb/SMsgHandler.h>
#include <rfb/SecurityServer.h>
#include <rfb/Timer.h>

namespace rfb {

  class SMsgReader;
  class SMsgWriter;
  class SSecurity;

  class SConnection : public SMsgHandler {
  public:

    SConnection(AccessRights accessRights);
    virtual ~SConnection();

    // Methods to initialise the connection

    // setStreams() sets the streams to be used for the connection.  These must
    // be set before initialiseProtocol() and processMsg() are called.  The
    // SSecurity object may call setStreams() again to provide alternative
    // streams over which the RFB protocol is sent (i.e. encrypting/decrypting
    // streams).  Ownership of the streams remains with the caller
    // (i.e. SConnection will not delete them).
    void setStreams(rdr::InStream* is, rdr::OutStream* os);

    // initialiseProtocol() should be called once the streams and security
    // types are set.  Subsequently, processMsg() should be called whenever
    // there is data to read on the InStream.
    void initialiseProtocol();

    // processMsg() should be called whenever there is data available on
    // the CConnection's current InStream. It will process at most one
    // RFB message before returning. If there was insufficient data,
    // then it will return false and should be called again once more
    // data is available.
    bool processMsg();

    // approveConnection() is called to either accept or reject the connection.
    // If accept is false, the reason string gives the reason for the
    // rejection.  It can either be called directly from queryConnection() or
    // later, after queryConnection() has returned.  It can only be called when
    // in state RFBSTATE_QUERYING.  On rejection, an AuthFailureException is
    // thrown, so this must be handled appropriately by the caller.
    void approveConnection(bool accept, const char* reason=nullptr);


    // Methods to terminate the connection

    // close() shuts down the connection to the client and awaits
    // cleanup of the SConnection object by the server
    virtual void close(const char* reason);


    // Overridden from SMsgHandler

    void setEncodings(int nEncodings, const int32_t* encodings) override;

    void clientCutText(const char* str) override;

    void handleClipboardRequest(uint32_t flags) override;
    void handleClipboardPeek() override;
    void handleClipboardNotify(uint32_t flags) override;
    void handleClipboardProvide(uint32_t flags, const size_t* lengths,
                                const uint8_t* const* data) override;

    void supportsQEMUKeyEvent() override;


    // Methods to be overridden in a derived class

    // versionReceived() indicates that the version number has just been read
    // from the client.  The version will already have been "cooked"
    // to deal with unknown/bogus viewer protocol numbers.
    virtual void versionReceived();

    // authSuccess() is called when authentication has succeeded.
    virtual void authSuccess();

    // queryConnection() is called when authentication has succeeded, but
    // before informing the client.  It can be overridden to query a local user
    // to accept the incoming connection, for example.  The userName argument
    // is the name of the user making the connection, or null (note that the
    // storage for userName is owned by the caller).  The connection must be
    // accepted or rejected by calling approveConnection(), either directly
    // from queryConnection() or some time later.
    virtual void queryConnection(const char* userName);

    // clientInit() is called when the ClientInit message is received.  The
    // derived class must call on to SConnection::clientInit().
    void clientInit(bool shared) override;

    // setPixelFormat() is called when a SetPixelFormat message is received.
    // The derived class must call on to SConnection::setPixelFormat().
    void setPixelFormat(const PixelFormat& pf) override;

    // framebufferUpdateRequest() is called when a FramebufferUpdateRequest
    // message is received.  The derived class must call on to
    // SConnection::framebufferUpdateRequest().
    void framebufferUpdateRequest(const Rect& r, bool incremental) override;

    // fence() is called when we get a fence request or response. By default
    // it responds directly to requests (stating it doesn't support any
    // synchronisation) and drops responses. Override to implement more proper
    // support.
    void fence(uint32_t flags, unsigned len, const uint8_t data[]) override;

    // enableContinuousUpdates() is called when the client wants to enable
    // or disable continuous updates, or change the active area.
    void enableContinuousUpdates(bool enable,
                                 int x, int y, int w, int h) override;

    // handleClipboardRequest() is called whenever the client requests
    // the server to send over its clipboard data. It will only be
    // called after the server has first announced a clipboard change
    // via announceClipboard().
    virtual void handleClipboardRequest();

    // handleClipboardAnnounce() is called to indicate a change in the
    // clipboard on the client. Call requestClipboard() to access the
    // actual data.
    virtual void handleClipboardAnnounce(bool available);

    // handleClipboardData() is called when the client has sent over
    // the clipboard data as a result of a previous call to
    // requestClipboard(). Note that this function might never be
    // called if the clipboard data was no longer available when the
    // client received the request.
    virtual void handleClipboardData(const char* data);


    // Other methods

    // requestClipboard() will result in a request to the client to
    // transfer its clipboard data. A call to handleClipboardData()
    // will be made once the data is available.
    virtual void requestClipboard();

    // announceClipboard() informs the client of changes to the
    // clipboard on the server. The client may later request the
    // clipboard data via handleClipboardRequest().
    virtual void announceClipboard(bool available);

    // sendClipboardData() transfers the clipboard data to the client
    // and should be called whenever the client has requested the
    // clipboard via handleClipboardRequest().
    virtual void sendClipboardData(const char* data);

    // getAccessRights() returns the access rights of a SConnection to the server.
    AccessRights getAccessRights() { return accessRights; }

    // setAccessRights() allows a security package to limit the access rights
    // of a SConnection to the server.  How the access rights are treated
    // is up to the derived class.
    virtual void setAccessRights(AccessRights ar);
    virtual bool accessCheck(AccessRights ar) const;

    // authenticated() returns true if the client has authenticated
    // successfully.
    bool authenticated() { return (state_ == RFBSTATE_INITIALISATION ||
                                   state_ == RFBSTATE_NORMAL); }

    SMsgReader* reader() { return reader_; }
    SMsgWriter* writer() { return writer_; }

    rdr::InStream* getInStream() { return is; }
    rdr::OutStream* getOutStream() { return os; }

    enum stateEnum {
      RFBSTATE_UNINITIALISED,
      RFBSTATE_PROTOCOL_VERSION,
      RFBSTATE_SECURITY_TYPE,
      RFBSTATE_SECURITY,
      RFBSTATE_SECURITY_FAILURE,
      RFBSTATE_QUERYING,
      RFBSTATE_INITIALISATION,
      RFBSTATE_NORMAL,
      RFBSTATE_CLOSING,
      RFBSTATE_INVALID
    };

    stateEnum state() { return state_; }

    int32_t getPreferredEncoding() { return preferredEncoding; }

  protected:
    // throwConnFailedException() prints a message to the log, sends a conn
    // failed message to the client (if possible) and throws a
    // ConnFailedException.
    void throwConnFailedException(const char* format, ...)
      __attribute__((__format__ (__printf__, 2, 3)));

    void setState(stateEnum s) { state_ = s; }

    void setReader(SMsgReader *r) { reader_ = r; }
    void setWriter(SMsgWriter *w) { writer_ = w; }

  private:
    void cleanup();
    void writeFakeColourMap(void);

    bool readyForSetColourMapEntries;

    bool processVersionMsg();
    bool processSecurityTypeMsg();
    void processSecurityType(int secType);
    bool processSecurityMsg();
    bool processSecurityFailure();
    bool processInitMsg();

    void handleAuthFailureTimeout(Timer* t);

    int defaultMajorVersion, defaultMinorVersion;

    rdr::InStream* is;
    rdr::OutStream* os;

    SMsgReader* reader_;
    SMsgWriter* writer_;

    SecurityServer security;
    SSecurity* ssecurity;

    MethodTimer<SConnection> authFailureTimer;
    std::string authFailureMsg;

    stateEnum state_;
    int32_t preferredEncoding;
    AccessRights accessRights;

    std::string clientClipboard;
    bool hasRemoteClipboard;
    bool hasLocalClipboard;
    bool unsolicitedClipboardAttempt;
  };
}
#endif
