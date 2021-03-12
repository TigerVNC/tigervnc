/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2019 Brian P. Hinz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

package com.tigervnc.rfb;

import java.awt.color.*;
import java.awt.image.*;
import java.nio.*;
import java.util.*;
import java.util.concurrent.atomic.AtomicBoolean;

import com.tigervnc.network.*;
import com.tigervnc.rdr.*;

abstract public class CConnection extends CMsgHandler {

  static LogWriter vlog = new LogWriter("CConnection");

  private static final String osName = 
    System.getProperty("os.name").toLowerCase(Locale.ENGLISH);

  public CConnection()
  {
    super();
    csecurity = null;
    supportsLocalCursor = false; supportsDesktopResize = false;
    is = null; os = null; reader_ = null; writer_ = null;
    shared = false;
    state_ = stateEnum.RFBSTATE_UNINITIALISED;
    pendingPFChange = false; preferredEncoding = Encodings.encodingTight;
    compressLevel = 2; qualityLevel = -1;
    formatChange = false; encodingChange = false;
    firstUpdate = true; pendingUpdate = false; continuousUpdates = false;
    forceNonincremental = true;
    framebuffer = null; decoder = new DecodeManager(this);
    security = new SecurityClient();
  }

  // Methods to initialise the connection

  // setServerName() is used to provide a unique(ish) name for the server to
  // which we are connected.  This might be the result of getPeerEndpoint on
  // a TcpSocket, for example, or a host specified by DNS name & port.
  // The serverName is used when verifying the Identity of a host (see RA2).
  public void setServerName(String name_) { serverName = name_; }

  public void setServerPort(int port_) { serverPort = port_; }

  // setStreams() sets the streams to be used for the connection.  These must
  // be set before initialiseProtocol() and processMsg() are called.  The
  // CSecurity object may call setStreams() again to provide alternative
  // streams over which the RFB protocol is sent (i.e. encrypting/decrypting
  // streams).  Ownership of the streams remains with the caller
  // (i.e. SConnection will not delete them).
  public final void setStreams(InStream is_, OutStream os_)
  {
    is = is_;
    os = os_;
  }

  // setShared sets the value of the shared flag which will be sent to the
  // server upon initialisation.
  public final void setShared(boolean s) { shared = s; }

  // setFramebuffer configures the PixelBuffer that the CConnection
  // should render all pixel data in to. Note that the CConnection
  // takes ownership of the PixelBuffer and it must not be deleted by
  // anyone else. Call setFramebuffer again with NULL or a different
  // PixelBuffer to delete the previous one.
  public void setFramebuffer(ModifiablePixelBuffer fb)
  {
    decoder.flush();

    if (fb != null) {
      assert(fb.width() == server.width());
      assert(fb.height() == server.height());
    }

    if ((framebuffer != null) && (fb != null)) {
      Rect rect = new Rect();

      Raster data;

      byte[] black = new byte[4];

      // Copy still valid area

      rect.setXYWH(0, 0,
                   Math.min(fb.width(), framebuffer.width()),
                   Math.min(fb.height(), framebuffer.height()));
      data = framebuffer.getBuffer(rect);
      fb.imageRect(framebuffer.getPF(), rect, data);

      // Black out any new areas

      if (fb.width() > framebuffer.width()) {
        rect.setXYWH(framebuffer.width(), 0,
                     fb.width() - framebuffer.width(),
                     fb.height());
        fb.fillRect(rect, black);
      }

      if (fb.height() > framebuffer.height()) {
        rect.setXYWH(0, framebuffer.height(),
                     fb.width(),
                     fb.height() - framebuffer.height());
        fb.fillRect(rect, black);
      }
    }

    framebuffer = fb;
  }

  // initialiseProtocol() should be called once the streams and security
  // types are set.  Subsequently, processMsg() should be called whenever
  // there is data to read on the InStream.
  public final void initialiseProtocol()
  {
    state_ = stateEnum.RFBSTATE_PROTOCOL_VERSION;
  }

  // processMsg() should be called whenever there is data to read on the
  // InStream.  You must have called initialiseProtocol() first.
  public void processMsg()
  {
    switch (state_) {

    case RFBSTATE_PROTOCOL_VERSION: processVersionMsg();        break;
    case RFBSTATE_SECURITY_TYPES:   processSecurityTypesMsg();  break;
    case RFBSTATE_SECURITY:         processSecurityMsg();       break;
    case RFBSTATE_SECURITY_RESULT:  processSecurityResultMsg(); break;
    case RFBSTATE_INITIALISATION:   processInitMsg();           break;
    case RFBSTATE_NORMAL:           reader_.readMsg();          break;
    case RFBSTATE_UNINITIALISED:
      throw new Exception("CConnection.processMsg: not initialised yet?");
    default:
      throw new Exception("CConnection.processMsg: invalid state");
    }
  }

  private void processVersionMsg()
  {
    ByteBuffer verStr = ByteBuffer.allocate(12);
    int majorVersion;
    int minorVersion;

    vlog.debug("reading protocol version");

    if (!is.checkNoWait(12))
      return;

    is.readBytes(verStr, 12);

    if ((new String(verStr.array())).matches("RFB \\d{3}\\.\\d{3}\\n")) {
      majorVersion =
        Integer.parseInt((new String(verStr.array())).substring(4,7));
      minorVersion =
        Integer.parseInt((new String(verStr.array())).substring(8,11));
    } else {
      state_ = stateEnum.RFBSTATE_INVALID;
      throw new Exception("reading version failed: not an RFB server?");
    }

    server.setVersion(majorVersion, minorVersion);

    vlog.info("Server supports RFB protocol version "
              +server.majorVersion+"."+ server.minorVersion);

    // The only official RFB protocol versions are currently 3.3, 3.7 and 3.8
    if (server.beforeVersion(3,3)) {
      String msg = ("Server gave unsupported RFB protocol version "+
                    server.majorVersion+"."+server.minorVersion);
      vlog.error(msg);
      state_ = stateEnum.RFBSTATE_INVALID;
      throw new Exception(msg);
    } else if (server.beforeVersion(3,7)) {
      server.setVersion(3,3);
    } else if (server.afterVersion(3,8)) {
      server.setVersion(3,8);
    }

    verStr.clear();
    verStr.put(String.format("RFB %03d.%03d\n",
               majorVersion, minorVersion).getBytes()).flip();
    os.writeBytes(verStr.array(), 0, 12);
    os.flush();

    state_ = stateEnum.RFBSTATE_SECURITY_TYPES;

    vlog.info("Using RFB protocol version "+
              server.majorVersion+"."+server.minorVersion);
  }

  private void processSecurityTypesMsg()
  {
    vlog.debug("processing security types message");

    int secType = Security.secTypeInvalid;

    List<Integer> secTypes = new ArrayList<Integer>();
    secTypes = security.GetEnabledSecTypes();

    if (server.isVersion(3,3)) {

      // legacy 3.3 server may only offer "vnc authentication" or "none"

      secType = is.readU32();
      if (secType == Security.secTypeInvalid) {
        throwConnFailedException();

      } else if (secType == Security.secTypeNone || secType == Security.secTypeVncAuth) {
        Iterator<Integer> i;
        for (i = secTypes.iterator(); i.hasNext(); ) {
          int refType = (Integer)i.next();
          if (refType == secType) {
            secType = refType;
            break;
          }
        }

        if (!secTypes.contains(secType))
          secType = Security.secTypeInvalid;
      } else {
        vlog.error("Unknown 3.3 security type "+secType);
        throw new Exception("Unknown 3.3 security type");
      }

    } else {

      // 3.7 server will offer us a list

      int nServerSecTypes = is.readU8();
      if (nServerSecTypes == 0)
        throwConnFailedException();

      Iterator<Integer> j;

      for (int i = 0; i < nServerSecTypes; i++) {
        int serverSecType = is.readU8();
        vlog.debug("Server offers security type "+
                   Security.secTypeName(serverSecType)+"("+serverSecType+")");

        /*
        * Use the first type sent by server which matches client's type.
        * It means server's order specifies priority.
        */
        if (secType == Security.secTypeInvalid) {
          for (j = secTypes.iterator(); j.hasNext(); ) {
            int refType = (Integer)j.next();
            if (refType == serverSecType) {
              secType = refType;
              break;
            }
          }
        }
      }

      // Inform the server of our decision
      if (secType != Security.secTypeInvalid) {
        os.writeU8(secType);
        os.flush();
        vlog.debug("Choosing security type "+Security.secTypeName(secType)+
                   "("+secType+")");
      }
    }

    if (secType == Security.secTypeInvalid) {
      state_ = stateEnum.RFBSTATE_INVALID;
      vlog.error("No matching security types");
      throw new Exception("No matching security types");
    }

    state_ = stateEnum.RFBSTATE_SECURITY;
    csecurity = security.GetCSecurity(secType);
    processSecurityMsg();
  }

  private void processSecurityMsg() {
    vlog.debug("processing security message");
    if (csecurity.processMsg(this)) {
      state_ = stateEnum.RFBSTATE_SECURITY_RESULT;
      processSecurityResultMsg();
    }
  }

  private void processSecurityResultMsg() {
    vlog.debug("processing security result message");
    int result;
    if (server.beforeVersion(3,8) && csecurity.getType() == Security.secTypeNone) {
      result = Security.secResultOK;
    } else {
      if (!is.checkNoWait(1)) return;
      result = is.readU32();
    }
    switch (result) {
    case Security.secResultOK:
      securityCompleted();
      return;
    case Security.secResultFailed:
      vlog.debug("auth failed");
      break;
    case Security.secResultTooMany:
      vlog.debug("auth failed - too many tries");
      break;
    default:
      throw new Exception("Unknown security result from server");
    }
    state_ = stateEnum.RFBSTATE_INVALID;
    if (server.beforeVersion(3,8))
      throw new AuthFailureException();
    String reason = is.readString();
    throw new AuthFailureException(reason);
  }

  private void processInitMsg() {
    vlog.debug("reading server initialisation");
    reader_.readServerInit();
  }

  private void throwConnFailedException() {
    state_ = stateEnum.RFBSTATE_INVALID;
    String reason;
    reason = is.readString();
    throw new ConnFailedException(reason);
  }

  private void securityCompleted() {
    state_ = stateEnum.RFBSTATE_INITIALISATION;
    reader_ = new CMsgReader(this, is);
    writer_ = new CMsgWriter(server, os);
    vlog.debug("Authentication success!");
    authSuccess();
    writer_.writeClientInit(shared);
  }

  // Methods overridden from CMsgHandler

  // Note: These must be called by any deriving classes

  public void setDesktopSize(int w, int h) {
    decoder.flush();

    super.setDesktopSize(w,h);

    if (continuousUpdates)
      writer().writeEnableContinuousUpdates(true, 0, 0,
                                            server.width(),
                                            server.height());

    resizeFramebuffer();
    assert(framebuffer != null);
    assert(framebuffer.width() == server.width());
    assert(framebuffer.height() == server.height());
  }

  public void setExtendedDesktopSize(int reason,
                                     int result,
                                     int w, int h,
                                     ScreenSet layout) {
    decoder.flush();

    super.setExtendedDesktopSize(reason, result, w, h, layout);

    if (continuousUpdates)
      writer().writeEnableContinuousUpdates(true, 0, 0,
                                            server.width(),
                                            server.height());

    resizeFramebuffer();
    assert(framebuffer != null);
    assert(framebuffer.width() == server.width());
    assert(framebuffer.height() == server.height());
  }

  public void endOfContinuousUpdates()
  {
    super.endOfContinuousUpdates();

    // We've gotten the marker for a format change, so make the pending
    // one active
    if (pendingPFChange) {
      server.setPF(pendingPF);
      pendingPFChange = false;

      // We might have another change pending
      if (formatChange)
        requestNewUpdate();
    }
  }
  // serverInit() is called when the ServerInit message is received.  The
  // derived class must call on to CConnection::serverInit().
  public void serverInit(int width, int height,
                         PixelFormat pf, String name)
  {
    super.serverInit(width, height, pf, name);
    
    state_ = stateEnum.RFBSTATE_NORMAL;
    vlog.debug("initialisation done");

    initDone();
    assert(framebuffer != null);
    // FIXME: even if the client is scaling?
    assert(framebuffer.width() == server.width());
    assert(framebuffer.height() == server.height());

    // We want to make sure we call SetEncodings at least once
    encodingChange = true;

    requestNewUpdate();

    // This initial update request is a bit of a corner case, so we need
    // to help out setting the correct format here.
    if (pendingPFChange) {
      server.setPF(pendingPF);
      pendingPFChange = false;
    }
  }

  public void readAndDecodeRect(Rect r, int encoding,
                                ModifiablePixelBuffer pb)
  {
    decoder.decodeRect(r, encoding, pb);
    decoder.flush();
  }

  public void framebufferUpdateStart()
  {
    super.framebufferUpdateStart();

    assert(framebuffer != null);

    // Note: This might not be true if continuous updates are supported
    pendingUpdate = false;

    requestNewUpdate();
  }

  public void framebufferUpdateEnd()
  {
    decoder.flush();

    super.framebufferUpdateEnd();

    // A format change has been scheduled and we are now past the update
    // with the old format. Time to active the new one.
    if (pendingPFChange && !continuousUpdates) {
      server.setPF(pendingPF);
      pendingPFChange = false;
    }

    if (firstUpdate) {
      if (server.supportsContinuousUpdates) {
        vlog.info("Enabling continuous updates");
        continuousUpdates = true;
        writer().writeEnableContinuousUpdates(true, 0, 0,
                                              server.width(),
                                              server.height());
      }

      firstUpdate = false;
    }
  }

  public void dataRect(Rect r, int encoding)
  {
    decoder.decodeRect(r, encoding, framebuffer);
  }

  // Methods to be overridden in a derived class

  // authSuccess() is called when authentication has succeeded.
  public void authSuccess() { }

  // initDone() is called when the connection is fully established
  // and standard messages can be sent. This is called before the
  // initial FramebufferUpdateRequest giving a derived class the
  // chance to modify pixel format and settings. The derived class
  // must also make sure it has provided a valid framebuffer before
  // returning.
  public void initDone() { }

  // resizeFramebuffer() is called whenever the framebuffer
  // dimensions or the screen layout changes. A subclass must make
  // sure the pixel buffer has been updated once this call returns.
  public void resizeFramebuffer()
  {
    assert(false);
  }

  // refreshFramebuffer() forces a complete refresh of the entire
  // framebuffer
  public void refreshFramebuffer()
  {
    forceNonincremental = true;

    // Without fences, we cannot safely trigger an update request directly
    // but must wait for the next update to arrive.
    if (continuousUpdates)
      requestNewUpdate();
  }

  // setPreferredEncoding()/getPreferredEncoding() adjusts which
  // encoding is listed first as a hint to the server that it is the
  // preferred one
  public void setPreferredEncoding(int encoding)
  {
    if (preferredEncoding == encoding)
      return;
  
    preferredEncoding = encoding;
    encodingChange = true;
  }
  
  public int getPreferredEncoding()
  {
    return preferredEncoding;
  }
  
  // setCompressLevel()/setQualityLevel() controls the encoding hints
  // sent to the server
  public void setCompressLevel(int level)
  {
    if (compressLevel == level)
      return;
  
    compressLevel = level;
    encodingChange = true;
  }
  
  public void setQualityLevel(int level)
  {
    if (qualityLevel == level)
      return;
  
    qualityLevel = level;
    encodingChange = true;
  }

  // setPF() controls the pixel format requested from the server.
  // server.pf() will automatically be adjusted once the new format
  // is active.
  public void setPF(PixelFormat pf)
  {
    if (server.pf().equal(pf) && !formatChange)
      return;

    nextPF = pf;
    formatChange = true;
  }

  public CMsgReader reader() { return reader_; }
  public CMsgWriter writer() { return writer_; }

  public InStream getInStream() { return is; }
  public OutStream getOutStream() { return os; }

  // Access method used by SSecurity implementations that can verify servers'
  // Identities, to determine the unique(ish) name of the server.
  public String getServerName() { return serverName; }
  public int getServerPort() { return serverPort; }

  boolean isSecure() { return csecurity != null ? csecurity.isSecure() : false; }

  public enum stateEnum {
    RFBSTATE_UNINITIALISED,
    RFBSTATE_PROTOCOL_VERSION,
    RFBSTATE_SECURITY_TYPES,
    RFBSTATE_SECURITY,
    RFBSTATE_SECURITY_RESULT,
    RFBSTATE_INITIALISATION,
    RFBSTATE_NORMAL,
    RFBSTATE_INVALID
  };

  public stateEnum state() { return state_; }

  protected void setState(stateEnum s) { state_ = s; }

  protected void setReader(CMsgReader r) { reader_ = r; }
  protected void setWriter(CMsgWriter w) { writer_ = w; }

  protected ModifiablePixelBuffer getFramebuffer() { return framebuffer; }

  public void fence(int flags, int len, byte[] data)
  {
    super.fence(flags, len, data);

    if ((flags & fenceTypes.fenceFlagRequest) != 0)
      return;

    // We cannot guarantee any synchronisation at this level
    flags = 0;

    writer().writeFence(flags, len, data);
  }

  // requestNewUpdate() requests an update from the server, having set the
  // format and encoding appropriately.
  private void requestNewUpdate()
  {
    if (formatChange && !pendingPFChange) {
      /* Catch incorrect requestNewUpdate calls */
      assert(!pendingUpdate || continuousUpdates);

      // We have to make sure we switch the internal format at a safe
      // time. For continuous updates we temporarily disable updates and
      // look for a EndOfContinuousUpdates message to see when to switch.
      // For classical updates we just got a new update right before this
      // function was called, so we need to make sure we finish that
      // update before we can switch.

      pendingPFChange = true;
      pendingPF = nextPF;

      if (continuousUpdates)
        writer().writeEnableContinuousUpdates(false, 0, 0, 0, 0);

      writer().writeSetPixelFormat(pendingPF);

      if (continuousUpdates)
        writer().writeEnableContinuousUpdates(true, 0, 0,
                                              server.width(),
                                              server.height());
      formatChange = false;
    }

    if (encodingChange) {
      updateEncodings();
      encodingChange = false;
    }

    if (forceNonincremental || !continuousUpdates) {
      pendingUpdate = true;
      writer().writeFramebufferUpdateRequest(new Rect(0, 0,
                                                      server.width(),
                                                      server.height()),
                                             !forceNonincremental);
    }

    forceNonincremental = false;
  }

  // Ask for encodings based on which decoders are supported.  Assumes higher
  // encoding numbers are more desirable.

  private void updateEncodings()
  {
    List<Integer> encodings = new ArrayList<Integer>();

    if (server.supportsLocalCursor) {
      // JRE on Windows does not support cursors with alpha
      if (!osName.contains("windows")) {
        encodings.add(Encodings.pseudoEncodingCursorWithAlpha);
        encodings.add(Encodings.pseudoEncodingVMwareCursor);
      }
      encodings.add(Encodings.pseudoEncodingCursor);
      encodings.add(Encodings.pseudoEncodingXCursor);
    }
    if (server.supportsDesktopResize) {
      encodings.add(Encodings.pseudoEncodingDesktopSize);
      encodings.add(Encodings.pseudoEncodingExtendedDesktopSize);
    }
    if (server.supportsClientRedirect)
      encodings.add(Encodings.pseudoEncodingClientRedirect);

    encodings.add(Encodings.pseudoEncodingDesktopName);
    encodings.add(Encodings.pseudoEncodingLastRect);
    encodings.add(Encodings.pseudoEncodingContinuousUpdates);
    encodings.add(Encodings.pseudoEncodingFence);

    if (Decoder.supported(preferredEncoding)) {
      encodings.add(preferredEncoding);
    }

    encodings.add(Encodings.encodingCopyRect);

    for (int i = Encodings.encodingMax; i >= 0; i--) {
      if ((i != preferredEncoding) && Decoder.supported(i))
        encodings.add(i);
    }

    if (compressLevel >= 0 && compressLevel <= 9)
      encodings.add(Encodings.pseudoEncodingCompressLevel0 + compressLevel);
    if (qualityLevel >= 0 && qualityLevel <= 9)
      encodings.add(Encodings.pseudoEncodingQualityLevel0 + qualityLevel);

    writer().writeSetEncodings(encodings);
  }

  private void throwAuthFailureException() {
    String reason;
    vlog.debug("state="+state()+", ver="+server.majorVersion+"."+server.minorVersion);
    if (state() == stateEnum.RFBSTATE_SECURITY_RESULT && !server.beforeVersion(3,8)) {
      reason = is.readString();
    } else {
      reason = "Authentication failure";
    }
    state_ = stateEnum.RFBSTATE_INVALID;
    vlog.error(reason);
    throw new AuthFailureException(reason);
  }

  public CSecurity csecurity;
  public SecurityClient security;

  protected boolean supportsLocalCursor;
  protected boolean supportsDesktopResize;

  private InStream is;
  private OutStream os;
  private CMsgReader reader_;
  private CMsgWriter writer_;
  private boolean deleteStreamsWhenDone;
  private boolean shared;
  private stateEnum state_;

  private String serverName;
  private int serverPort;

  private boolean pendingPFChange;
  private PixelFormat pendingPF;

  private int preferredEncoding;
  private int compressLevel;
  private int qualityLevel;

  private boolean formatChange;
  private PixelFormat nextPF;
  private boolean encodingChange;

  private boolean firstUpdate;
  private boolean pendingUpdate;
  private boolean continuousUpdates;

  private boolean forceNonincremental;

  protected ModifiablePixelBuffer framebuffer;
  private DecodeManager decoder;
}
