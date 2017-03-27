/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2017 Pierre Ossman for Cendio AB
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
// CConnection - class on the client side representing a connection to a
// server.  A derived class should override methods appropriately.
//

#ifndef __RFB_CCONNECTION_H__
#define __RFB_CCONNECTION_H__

#include <rfb/CMsgHandler.h>
#include <rfb/DecodeManager.h>
#include <rfb/SecurityClient.h>
#include <rfb/util.h>

namespace rfb {

  class CMsgReader;
  class CMsgWriter;
  class CSecurity;
  class IdentityVerifier;

  class CConnection : public CMsgHandler {
  public:

    CConnection();
    virtual ~CConnection();

    // Methods to initialise the connection

    // setServerName() is used to provide a unique(ish) name for the server to
    // which we are connected.  This might be the result of getPeerEndpoint on
    // a TcpSocket, for example, or a host specified by DNS name & port.
    // The serverName is used when verifying the Identity of a host (see RA2).
    void setServerName(const char* name_) { serverName.replaceBuf(strDup(name_)); }

    // setStreams() sets the streams to be used for the connection.  These must
    // be set before initialiseProtocol() and processMsg() are called.  The
    // CSecurity object may call setStreams() again to provide alternative
    // streams over which the RFB protocol is sent (i.e. encrypting/decrypting
    // streams).  Ownership of the streams remains with the caller
    // (i.e. SConnection will not delete them).
    void setStreams(rdr::InStream* is, rdr::OutStream* os);

    // setShared sets the value of the shared flag which will be sent to the
    // server upon initialisation.
    void setShared(bool s) { shared = s; }

    // setProtocol3_3 configures whether or not the CConnection should
    // only ever support protocol version 3.3
    void setProtocol3_3(bool s) {useProtocol3_3 = s;}

    // setFramebuffer configures the PixelBuffer that the CConnection
    // should render all pixel data in to. Note that the CConnection
    // takes ownership of the PixelBuffer and it must not be deleted by
    // anyone else. Call setFramebuffer again with NULL or a different
    // PixelBuffer to delete the previous one.
    void setFramebuffer(ModifiablePixelBuffer* fb);

    // initialiseProtocol() should be called once the streams and security
    // types are set.  Subsequently, processMsg() should be called whenever
    // there is data to read on the InStream.
    void initialiseProtocol();

    // processMsg() should be called whenever there is either:
    // - data available on the underlying network stream
    //   In this case, processMsg may return without processing an RFB message,
    //   if the available data does not result in an RFB message being ready
    //   to handle. e.g. if data is encrypted.
    // NB: This makes it safe to call processMsg() in response to select()
    // - data available on the CConnection's current InStream
    //   In this case, processMsg should always process the available RFB
    //   message before returning.
    // NB: In either case, you must have called initialiseProtocol() first.
    void processMsg();


    // Methods overridden from CMsgHandler

    // Note: These must be called by any deriving classes

    virtual void setDesktopSize(int w, int h);
    virtual void setExtendedDesktopSize(unsigned reason, unsigned result,
                                        int w, int h,
                                        const ScreenSet& layout);

    virtual void readAndDecodeRect(const Rect& r, int encoding,
                                   ModifiablePixelBuffer* pb);

    virtual void framebufferUpdateStart();
    virtual void framebufferUpdateEnd();
    virtual void dataRect(const Rect& r, int encoding);


    // Methods to be overridden in a derived class

    // getIdVerifier() returns the identity verifier associated with the connection.
    // Ownership of the IdentityVerifier is retained by the CConnection instance.
    virtual IdentityVerifier* getIdentityVerifier() {return 0;}

    // authSuccess() is called when authentication has succeeded.
    virtual void authSuccess();

    // serverInit() is called when the ServerInit message is received.  The
    // derived class must call on to CConnection::serverInit().
    virtual void serverInit();


    // Other methods

    CMsgReader* reader() { return reader_; }
    CMsgWriter* writer() { return writer_; }

    rdr::InStream* getInStream() { return is; }
    rdr::OutStream* getOutStream() { return os; }

    // Access method used by SSecurity implementations that can verify servers'
    // Identities, to determine the unique(ish) name of the server.
    const char* getServerName() const { return serverName.buf; }

    enum stateEnum {
      RFBSTATE_UNINITIALISED,
      RFBSTATE_PROTOCOL_VERSION,
      RFBSTATE_SECURITY_TYPES,
      RFBSTATE_SECURITY,
      RFBSTATE_SECURITY_RESULT,
      RFBSTATE_INITIALISATION,
      RFBSTATE_NORMAL,
      RFBSTATE_INVALID
    };

    stateEnum state() { return state_; }

    CSecurity *csecurity;
    SecurityClient security;
  protected:
    void setState(stateEnum s) { state_ = s; }

    void setReader(CMsgReader *r) { reader_ = r; }
    void setWriter(CMsgWriter *w) { writer_ = w; }

    ModifiablePixelBuffer* getFramebuffer() { return framebuffer; }

  private:
    // This is a default implementation of fences that automatically
    // responds to requests, stating no support for synchronisation.
    // When overriding, call CMsgHandler::fence() directly in order to
    // state correct support for fence flags.
    virtual void fence(rdr::U32 flags, unsigned len, const char data[]);

  private:
    void processVersionMsg();
    void processSecurityTypesMsg();
    void processSecurityMsg();
    void processSecurityResultMsg();
    void processInitMsg();
    void throwAuthFailureException();
    void throwConnFailedException();
    void securityCompleted();

    rdr::InStream* is;
    rdr::OutStream* os;
    CMsgReader* reader_;
    CMsgWriter* writer_;
    bool deleteStreamsWhenDone;
    bool shared;
    stateEnum state_;

    CharArray serverName;

    bool useProtocol3_3;

    ModifiablePixelBuffer* framebuffer;
    DecodeManager decoder;
  };
}
#endif
