/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011 Pierre Ossman for Cendio AB
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

#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rfb/SMsgHandler.h>
#include <rfb/SecurityServer.h>

namespace rfb {

  class SMsgReader;
  class SMsgWriter;
  class SSecurity;

  class SConnection : public SMsgHandler {
  public:

    SConnection();
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

    // processMsg() should be called whenever there is data to read on the
    // InStream.  You must have called initialiseProtocol() first.
    void processMsg();

    // approveConnection() is called to either accept or reject the connection.
    // If accept is false, the reason string gives the reason for the
    // rejection.  It can either be called directly from queryConnection() or
    // later, after queryConnection() has returned.  It can only be called when
    // in state RFBSTATE_QUERYING.  On rejection, an AuthFailureException is
    // thrown, so this must be handled appropriately by the caller.
    void approveConnection(bool accept, const char* reason=0);


    // Overridden from SMsgHandler

    virtual void setEncodings(int nEncodings, rdr::S32* encodings);


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
    virtual void clientInit(bool shared);

    // setPixelFormat() is called when a SetPixelFormat message is received.
    // The derived class must call on to SConnection::setPixelFormat().
    virtual void setPixelFormat(const PixelFormat& pf);

    // framebufferUpdateRequest() is called when a FramebufferUpdateRequest
    // message is received.  The derived class must call on to
    // SConnection::framebufferUpdateRequest().
    virtual void framebufferUpdateRequest(const Rect& r, bool incremental);

    // fence() is called when we get a fence request or response. By default
    // it responds directly to requests (stating it doesn't support any
    // synchronisation) and drops responses. Override to implement more proper
    // support.
    virtual void fence(rdr::U32 flags, unsigned len, const char data[]);

    // enableContinuousUpdates() is called when the client wants to enable
    // or disable continuous updates, or change the active area.
    virtual void enableContinuousUpdates(bool enable,
                                         int x, int y, int w, int h);

    // setAccessRights() allows a security package to limit the access rights
    // of a VNCSConnectionST to the server.  How the access rights are treated
    // is up to the derived class.

    typedef rdr::U16 AccessRights;
    static const AccessRights AccessView;           // View display contents
    static const AccessRights AccessKeyEvents;      // Send key events
    static const AccessRights AccessPtrEvents;      // Send pointer events
    static const AccessRights AccessCutText;        // Send/receive clipboard events
    static const AccessRights AccessSetDesktopSize; // Change desktop size
    static const AccessRights AccessNonShared;      // Exclusive access to the server
    static const AccessRights AccessDefault;        // The default rights, INCLUDING FUTURE ONES
    static const AccessRights AccessNoQuery;        // Connect without local user accepting
    static const AccessRights AccessFull;           // All of the available AND FUTURE rights
    virtual void setAccessRights(AccessRights ar) = 0;

    // Other methods

    // authenticated() returns true if the client has authenticated
    // successfully.
    bool authenticated() { return (state_ == RFBSTATE_INITIALISATION ||
                                   state_ == RFBSTATE_NORMAL); }

    // deleteReaderAndWriter() deletes the reader and writer associated with
    // this connection.  This may be useful if you want to delete the streams
    // before deleting the SConnection to make sure that no attempt by the
    // SConnection is made to read or write.
    // XXX Do we really need this at all???
    void deleteReaderAndWriter();

    // throwConnFailedException() prints a message to the log, sends a conn
    // failed message to the client (if possible) and throws a
    // ConnFailedException.
    void throwConnFailedException(const char* msg);

    // writeConnFailedFromScratch() sends a conn failed message to an OutStream
    // without the need to negotiate the protocol version first.  It actually
    // does this by assuming that the client will understand version 3.3 of the
    // protocol.
    static void writeConnFailedFromScratch(const char* msg,
                                           rdr::OutStream* os);

    SMsgReader* reader() { return reader_; }
    SMsgWriter* writer() { return writer_; }

    rdr::InStream* getInStream() { return is; }
    rdr::OutStream* getOutStream() { return os; }

    enum stateEnum {
      RFBSTATE_UNINITIALISED,
      RFBSTATE_PROTOCOL_VERSION,
      RFBSTATE_SECURITY_TYPE,
      RFBSTATE_SECURITY,
      RFBSTATE_QUERYING,
      RFBSTATE_INITIALISATION,
      RFBSTATE_NORMAL,
      RFBSTATE_CLOSING,
      RFBSTATE_INVALID
    };

    stateEnum state() { return state_; }

    rdr::S32 getPreferredEncoding() { return preferredEncoding; }

  protected:
    void setState(stateEnum s) { state_ = s; }

    void setReader(SMsgReader *r) { reader_ = r; }
    void setWriter(SMsgWriter *w) { writer_ = w; }

  private:
    void writeFakeColourMap(void);

    bool readyForSetColourMapEntries;

    void processVersionMsg();
    void processSecurityTypeMsg();
    void processSecurityType(int secType);
    void processSecurityMsg();
    void processInitMsg();

    int defaultMajorVersion, defaultMinorVersion;
    rdr::InStream* is;
    rdr::OutStream* os;
    SMsgReader* reader_;
    SMsgWriter* writer_;
    SecurityServer *security;
    SSecurity* ssecurity;
    stateEnum state_;
    rdr::S32 preferredEncoding;
  };
}
#endif
