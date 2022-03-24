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
    void setServerName(const char* name_);

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
    bool processMsg();

    // close() gracefully shuts down the connection to the server and
    // should be called before terminating the underlying network
    // connection
    void close();


    // Methods overridden from CMsgHandler

    // Note: These must be called by any deriving classes

    virtual void setDesktopSize(int w, int h);
    virtual void setExtendedDesktopSize(unsigned reason, unsigned result,
                                        int w, int h,
                                        const ScreenSet& layout);

    virtual void endOfContinuousUpdates();

    virtual void serverInit(int width, int height,
                            const PixelFormat& pf,
                            const char* name);

    virtual bool readAndDecodeRect(const Rect& r, int encoding,
                                   ModifiablePixelBuffer* pb);

    virtual void framebufferUpdateStart();
    virtual void framebufferUpdateEnd();
    virtual bool dataRect(const Rect& r, int encoding);

    virtual void serverCutText(const char* str);

    virtual void handleClipboardCaps(rdr::U32 flags,
                                     const rdr::U32* lengths);
    virtual void handleClipboardRequest(rdr::U32 flags);
    virtual void handleClipboardPeek(rdr::U32 flags);
    virtual void handleClipboardNotify(rdr::U32 flags);
    virtual void handleClipboardProvide(rdr::U32 flags,
                                        const size_t* lengths,
                                        const rdr::U8* const* data);


    // Methods to be overridden in a derived class

    // authSuccess() is called when authentication has succeeded.
    virtual void authSuccess();

    // initDone() is called when the connection is fully established
    // and standard messages can be sent. This is called before the
    // initial FramebufferUpdateRequest giving a derived class the
    // chance to modify pixel format and settings. The derived class
    // must also make sure it has provided a valid framebuffer before
    // returning.
    virtual void initDone() = 0;

    // resizeFramebuffer() is called whenever the framebuffer
    // dimensions or the screen layout changes. A subclass must make
    // sure the pixel buffer has been updated once this call returns.
    virtual void resizeFramebuffer();

    // handleClipboardRequest() is called whenever the server requests
    // the client to send over its clipboard data. It will only be
    // called after the client has first announced a clipboard change
    // via announceClipboard().
    virtual void handleClipboardRequest();

    // handleClipboardAnnounce() is called to indicate a change in the
    // clipboard on the server. Call requestClipboard() to access the
    // actual data.
    virtual void handleClipboardAnnounce(bool available);

    // handleClipboardData() is called when the server has sent over
    // the clipboard data as a result of a previous call to
    // requestClipboard(). Note that this function might never be
    // called if the clipboard data was no longer available when the
    // server received the request.
    virtual void handleClipboardData(const char* data);


    // Other methods

    // requestClipboard() will result in a request to the server to
    // transfer its clipboard data. A call to handleClipboardData()
    // will be made once the data is available.
    virtual void requestClipboard();

    // announceClipboard() informs the server of changes to the
    // clipboard on the client. The server may later request the
    // clipboard data via handleClipboardRequest().
    virtual void announceClipboard(bool available);

    // sendClipboardData() transfers the clipboard data to the server
    // and should be called whenever the server has requested the
    // clipboard via handleClipboardRequest().
    virtual void sendClipboardData(const char* data);

    // refreshFramebuffer() forces a complete refresh of the entire
    // framebuffer
    void refreshFramebuffer();

    // setPreferredEncoding()/getPreferredEncoding() adjusts which
    // encoding is listed first as a hint to the server that it is the
    // preferred one
    void setPreferredEncoding(int encoding);
    int getPreferredEncoding();
    // setCompressLevel()/setQualityLevel() controls the encoding hints
    // sent to the server
    void setCompressLevel(int level);
    void setQualityLevel(int level);
    // setPF() controls the pixel format requested from the server.
    // server.pf() will automatically be adjusted once the new format
    // is active.
    void setPF(const PixelFormat& pf);

    CMsgReader* reader() { return reader_; }
    CMsgWriter* writer() { return writer_; }

    rdr::InStream* getInStream() { return is; }
    rdr::OutStream* getOutStream() { return os; }

    // Access method used by SSecurity implementations that can verify servers'
    // Identities, to determine the unique(ish) name of the server.
    const char* getServerName() const { return serverName.buf; }

    bool isSecure() const { return csecurity ? csecurity->isSecure() : false; }

    enum stateEnum {
      RFBSTATE_UNINITIALISED,
      RFBSTATE_PROTOCOL_VERSION,
      RFBSTATE_SECURITY_TYPES,
      RFBSTATE_SECURITY,
      RFBSTATE_SECURITY_RESULT,
      RFBSTATE_SECURITY_REASON,
      RFBSTATE_INITIALISATION,
      RFBSTATE_NORMAL,
      RFBSTATE_CLOSING,
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

  protected:
    // Optional capabilities that a subclass is expected to set to true
    // if supported
    bool supportsLocalCursor;
    bool supportsCursorPosition;
    bool supportsDesktopResize;
    bool supportsLEDState;

  private:
    // This is a default implementation of fences that automatically
    // responds to requests, stating no support for synchronisation.
    // When overriding, call CMsgHandler::fence() directly in order to
    // state correct support for fence flags.
    virtual void fence(rdr::U32 flags, unsigned len, const char data[]);

  private:
    bool processVersionMsg();
    bool processSecurityTypesMsg();
    bool processSecurityMsg();
    bool processSecurityResultMsg();
    bool processSecurityReasonMsg();
    bool processInitMsg();
    void throwAuthFailureException();
    void securityCompleted();

    void requestNewUpdate();
    void updateEncodings();

    rdr::InStream* is;
    rdr::OutStream* os;
    CMsgReader* reader_;
    CMsgWriter* writer_;
    bool deleteStreamsWhenDone;
    bool shared;
    stateEnum state_;

    CharArray serverName;

    bool pendingPFChange;
    rfb::PixelFormat pendingPF;

    int preferredEncoding;
    int compressLevel;
    int qualityLevel;

    bool formatChange;
    rfb::PixelFormat nextPF;
    bool encodingChange;

    bool firstUpdate;
    bool pendingUpdate;
    bool continuousUpdates;

    bool forceNonincremental;

    ModifiablePixelBuffer* framebuffer;
    DecodeManager decoder;

    char* serverClipboard;
    bool hasLocalClipboard;
    bool unsolicitedClipboardAttempt;
  };
}
#endif
