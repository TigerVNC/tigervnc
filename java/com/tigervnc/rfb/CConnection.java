/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2012 Brian P. Hinz
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

  public CConnection()
  {
    super();
    csecurity = null; is = null; os = null; reader_ = null; writer_ = null;
    shared = false;
    state_ = RFBSTATE_UNINITIALISED;
    framebuffer = null; decoder = new DecodeManager(this);
    security = new SecurityClient();
  }

  // Methods to initialise the connection

  // setServerName() is used to provide a unique(ish) name for the server to
  // which we are connected.  This might be the result of getPeerEndpoint on
  // a TcpSocket, for example, or a host specified by DNS name & port.
  // The serverName is used when verifying the Identity of a host (see RA2).
  public final void setServerName(String name) {
    serverName = name;
  }

  // setShared sets the value of the shared flag which will be sent to the
  // server upon initialisation.
  public final void setShared(boolean s) { shared = s; }

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

  // setFramebuffer configures the PixelBuffer that the CConnection
  // should render all pixel data in to. Note that the CConnection
  // takes ownership of the PixelBuffer and it must not be deleted by
  // anyone else. Call setFramebuffer again with NULL or a different
  // PixelBuffer to delete the previous one.
  public void setFramebuffer(ModifiablePixelBuffer fb)
  {
    decoder.flush();

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
    state_ = RFBSTATE_PROTOCOL_VERSION;
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
    vlog.debug("reading protocol version");
    AtomicBoolean done = new AtomicBoolean();
    if (!cp.readVersion(is, done)) {
      state_ = RFBSTATE_INVALID;
      throw new Exception("reading version failed: not an RFB server?");
    }
    if (!done.get()) return;

    vlog.info("Server supports RFB protocol version "
              +cp.majorVersion+"."+ cp.minorVersion);

    // The only official RFB protocol versions are currently 3.3, 3.7 and 3.8
    if (cp.beforeVersion(3,3)) {
      String msg = ("Server gave unsupported RFB protocol version "+
                    cp.majorVersion+"."+cp.minorVersion);
      vlog.error(msg);
      state_ = RFBSTATE_INVALID;
      throw new Exception(msg);
    } else if (cp.beforeVersion(3,7)) {
      cp.setVersion(3,3);
    } else if (cp.afterVersion(3,8)) {
      cp.setVersion(3,8);
    }

    cp.writeVersion(os);
    state_ = RFBSTATE_SECURITY_TYPES;

    vlog.info("Using RFB protocol version "+
              cp.majorVersion+"."+cp.minorVersion);
  }

  private void processSecurityTypesMsg()
  {
    vlog.debug("processing security types message");

    int secType = Security.secTypeInvalid;

    List<Integer> secTypes = new ArrayList<Integer>();
    secTypes = security.GetEnabledSecTypes();

    if (cp.isVersion(3,3)) {

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
      state_ = RFBSTATE_INVALID;
      vlog.error("No matching security types");
      throw new Exception("No matching security types");
    }

    state_ = RFBSTATE_SECURITY;
    csecurity = security.GetCSecurity(secType);
    processSecurityMsg();
  }

  private void processSecurityMsg() {
    vlog.debug("processing security message");
    if (csecurity.processMsg(this)) {
      state_ = RFBSTATE_SECURITY_RESULT;
      processSecurityResultMsg();
    }
  }

  private void processSecurityResultMsg() {
    vlog.debug("processing security result message");
    int result;
    if (cp.beforeVersion(3,8) && csecurity.getType() == Security.secTypeNone) {
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
    String reason;
    if (cp.beforeVersion(3,8))
      reason = "Authentication failure";
    else
      reason = is.readString();
    state_ = RFBSTATE_INVALID;
    throw new AuthFailureException(reason);
  }

  private void processInitMsg() {
    vlog.debug("reading server initialisation");
    reader_.readServerInit();
  }

  private void throwConnFailedException() {
    state_ = RFBSTATE_INVALID;
    String reason;
    reason = is.readString();
    throw new ConnFailedException(reason);
  }

  private void securityCompleted() {
    state_ = RFBSTATE_INITIALISATION;
    reader_ = new CMsgReader(this, is);
    writer_ = new CMsgWriter(cp, os);
    vlog.debug("Authentication success!");
    authSuccess();
    writer_.writeClientInit(shared);
  }

  // Methods to be overridden in a derived class

  // Note: These must be called by any deriving classes

  public void setDesktopSize(int w, int h) {
    decoder.flush();

    super.setDesktopSize(w,h);
  }

  public void setExtendedDesktopSize(int reason,
                                     int result, int w, int h,
                                     ScreenSet layout) {
    decoder.flush();

    super.setExtendedDesktopSize(reason, result, w, h, layout);
  }

  public void readAndDecodeRect(Rect r, int encoding,
                                ModifiablePixelBuffer pb)
  {
    decoder.decodeRect(r, encoding, pb);
    decoder.flush();
  }

  // getIdVerifier() returns the identity verifier associated with the connection.
  // Ownership of the IdentityVerifier is retained by the CConnection instance.
  //public IdentityVerifier getIdentityVerifier() { return 0; }

  public void framebufferUpdateStart()
  {
    super.framebufferUpdateStart();
  }

  public void framebufferUpdateEnd()
  {
    decoder.flush();

    super.framebufferUpdateEnd();
  }

  public void dataRect(Rect r, int encoding)
  {
    decoder.decodeRect(r, encoding, framebuffer);
  }

  // authSuccess() is called when authentication has succeeded.
  public void authSuccess()
  {
  }

  // serverInit() is called when the ServerInit message is received.  The
  // derived class must call on to CConnection::serverInit().
  public void serverInit()
  {
    state_ = RFBSTATE_NORMAL;
    vlog.debug("initialisation done");
  }

  // Other methods

  public CMsgReader reader() { return reader_; }
  public CMsgWriter writer() { return writer_; }

  public InStream getInStream() { return is; }
  public OutStream getOutStream() { return os; }

  // Access method used by SSecurity implementations that can verify servers'
  // Identities, to determine the unique(ish) name of the server.
  public String getServerName() { return serverName; }

  public boolean isSecure() { return csecurity != null ? csecurity.isSecure() : false; }

  public static final int RFBSTATE_UNINITIALISED = 0;
  public static final int RFBSTATE_PROTOCOL_VERSION = 1;
  public static final int RFBSTATE_SECURITY_TYPES = 2;
  public static final int RFBSTATE_SECURITY = 3;
  public static final int RFBSTATE_SECURITY_RESULT = 4;
  public static final int RFBSTATE_INITIALISATION = 5;
  public static final int RFBSTATE_NORMAL = 6;
  public static final int RFBSTATE_INVALID = 7;

  public int state() { return state_; }

  public int getServerPort() { return serverPort; }
  public void setServerPort(int port) {
    serverPort = port;
  }

  protected void setState(int s) { state_ = s; }

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

    synchronized(this) {
      writer().writeFence(flags, len, data);
    }
  }

  private void throwAuthFailureException() {
    String reason;
    vlog.debug("state="+state()+", ver="+cp.majorVersion+"."+cp.minorVersion);
    if (state() == RFBSTATE_SECURITY_RESULT && !cp.beforeVersion(3,8)) {
      reason = is.readString();
    } else {
      reason = "Authentication failure";
    }
    state_ = RFBSTATE_INVALID;
    vlog.error(reason);
    throw new AuthFailureException(reason);
  }

  private InStream is;
  private OutStream os;
  private CMsgReader reader_;
  private CMsgWriter writer_;
  private boolean deleteStreamsWhenDone;
  private boolean shared;
  private int state_ = RFBSTATE_UNINITIALISED;

  private String serverName;

  protected ModifiablePixelBuffer framebuffer;
  private DecodeManager decoder;

  public CSecurity csecurity;
  public SecurityClient security;
  public static final int maxSecTypes = 8;
  int nSecTypes;
  int[] secTypes;
  int serverPort;
  boolean clientSecTypeOrder;

  static LogWriter vlog = new LogWriter("CConnection");
}
