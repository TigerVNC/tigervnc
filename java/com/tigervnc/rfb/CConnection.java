/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

package com.tigervnc.rfb;

import java.util.*;

import com.tigervnc.network.*;
import com.tigervnc.rdr.*;

abstract public class CConnection extends CMsgHandler {

  public CConnection() 
  {
    csecurity = null; is = null; os = null; reader_ = null; 
    writer_ = null; shared = false;
    state_ = RFBSTATE_UNINITIALISED; useProtocol3_3 = false;
    security = new SecurityClient();
  }

  // deleteReaderAndWriter() deletes the reader and writer associated with
  // this connection.  This may be useful if you want to delete the streams
  // before deleting the SConnection to make sure that no attempt by the
  // SConnection is made to read or write.
  // XXX Do we really need this at all???
  public void deleteReaderAndWriter()
  {
    reader_ = null;
    writer_ = null;
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
    if (!cp.readVersion(is)) {
      state_ = RFBSTATE_INVALID;
      throw new Exception("reading version failed: not an RFB server?");
    }
    if (!cp.done) return;

    vlog.info("Server supports RFB protocol version "
              +cp.majorVersion+"."+ cp.minorVersion);

    // The only official RFB protocol versions are currently 3.3, 3.7 and 3.8
    if (cp.beforeVersion(3,3)) {
      String msg = ("Server gave unsupported RFB protocol version "+
                    cp.majorVersion+"."+cp.minorVersion);
      vlog.error(msg);
      state_ = RFBSTATE_INVALID;
      throw new Exception(msg);
    } else if (useProtocol3_3 || cp.beforeVersion(3,7)) {
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
    secTypes = Security.GetEnabledSecTypes();

    if (cp.isVersion(3,3)) {

      // legacy 3.3 server may only offer "vnc authentication" or "none"

      secType = is.readU32();
      if (secType == Security.secTypeInvalid) {
        throwConnFailedException();

      } else if (secType == Security.secTypeNone || secType == Security.secTypeVncAuth) {
        Iterator i;
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

      Iterator j;

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
    reader_ = new CMsgReaderV3(this, is);
    writer_ = new CMsgWriterV3(cp, os);
    vlog.debug("Authentication success!");
    authSuccess();
    writer_.writeClientInit(shared);
  }

  // Methods to initialise the connection

  // setServerName() is used to provide a unique(ish) name for the server to
  // which we are connected.  This might be the result of getPeerEndpoint on
  // a TcpSocket, for example, or a host specified by DNS name & port.
  // The serverName is used when verifying the Identity of a host (see RA2).
  public final void setServerName(String name) {
    serverName = name;
  }

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

  // setProtocol3_3 configures whether or not the CConnection should
  // only ever support protocol version 3.3
  public final void setProtocol3_3(boolean s) { useProtocol3_3 = s; }

  public void setServerPort(int port) {
    serverPort = port;
  }
 
  public void initSecTypes() {
    nSecTypes = 0;
  }

  // Methods to be overridden in a derived class

  // getIdVerifier() returns the identity verifier associated with the connection.
  // Ownership of the IdentityVerifier is retained by the CConnection instance.
  //public IdentityVerifier getIdentityVerifier() { return 0; }

  // authSuccess() is called when authentication has succeeded.
  public void authSuccess() {}

  // serverInit() is called when the ServerInit message is received.  The
  // derived class must call on to CConnection::serverInit().
  public void serverInit()
  {
    state_ = RFBSTATE_NORMAL;
    vlog.debug("initialisation done");
  }

  // getCSecurity() gets the CSecurity object for the given type.  The type
  // is guaranteed to be one of the secTypes passed in to addSecType().  The
  // CSecurity object's destroy() method will be called by the CConnection
  // from its destructor.
  //abstract public CSecurity getCSecurity(int secType);

  // getCurrentCSecurity() gets the CSecurity instance used for this
  // connection.
  //public CSecurity getCurrentCSecurity() { return security; }
  
  // setClientSecTypeOrder() determines whether the client should obey the
  // server's security type preference, by picking the first server security
  // type that the client supports, or whether it should pick the first type
  // that the server supports, from the client-supported list of types.
  public void setClientSecTypeOrder( boolean csto ) {
    clientSecTypeOrder = csto;
  }

  // Other methods

  public CMsgReaderV3 reader() { return reader_; }
  public CMsgWriterV3 writer() { return writer_; }

  public InStream getInStream() { return is; }
  public OutStream getOutStream() { return os; }

  public String getServerName() { return serverName; }
  public int getServerPort() { return serverPort; }

  public static final int RFBSTATE_UNINITIALISED = 0;
  public static final int RFBSTATE_PROTOCOL_VERSION = 1;
  public static final int RFBSTATE_SECURITY_TYPES = 2;
  public static final int RFBSTATE_SECURITY = 3;
  public static final int RFBSTATE_SECURITY_RESULT = 4;
  public static final int RFBSTATE_INITIALISATION = 5;
  public static final int RFBSTATE_NORMAL = 6;
  public static final int RFBSTATE_INVALID = 7;

  public int state() { return state_; }

  protected final void setState(int s) { state_ = s; }
  
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

  InStream is;
  OutStream os;
  CMsgReaderV3 reader_;
  CMsgWriterV3 writer_;
  boolean shared;
  public CSecurity csecurity;
  public SecurityClient security;
  public static final int maxSecTypes = 8;
  int nSecTypes;
  int[] secTypes;
  int state_ = RFBSTATE_UNINITIALISED;
  String serverName;
  int serverPort;
  boolean useProtocol3_3;
  boolean clientSecTypeOrder;

  static LogWriter vlog = new LogWriter("CConnection");
}
