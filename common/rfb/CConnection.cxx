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
#include <stdio.h>
#include <string.h>
#include <rfb/Exception.h>
#include <rfb/fenceTypes.h>
#include <rfb/CMsgReaderV3.h>
#include <rfb/CMsgWriterV3.h>
#include <rfb/CSecurity.h>
#include <rfb/Security.h>
#include <rfb/CConnection.h>
#include <rfb/util.h>

#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("CConnection");

CConnection::CConnection()
  : csecurity(0), is(0), os(0), reader_(0), writer_(0),
    shared(false),
    state_(RFBSTATE_UNINITIALISED), useProtocol3_3(false)
{
  security = new SecurityClient();
}

CConnection::~CConnection()
{
  if (csecurity) csecurity->destroy();
  deleteReaderAndWriter();
}

void CConnection::deleteReaderAndWriter()
{
  delete reader_;
  reader_ = 0;
  delete writer_;
  writer_ = 0;
}

void CConnection::setStreams(rdr::InStream* is_, rdr::OutStream* os_)
{
  is = is_;
  os = os_;
}

void CConnection::initialiseProtocol()
{
  state_ = RFBSTATE_PROTOCOL_VERSION;
}

void CConnection::processMsg()
{
  switch (state_) {

  case RFBSTATE_PROTOCOL_VERSION: processVersionMsg();       break;
  case RFBSTATE_SECURITY_TYPES:   processSecurityTypesMsg(); break;
  case RFBSTATE_SECURITY:         processSecurityMsg();      break;
  case RFBSTATE_SECURITY_RESULT:  processSecurityResultMsg(); break;
  case RFBSTATE_INITIALISATION:   processInitMsg();          break;
  case RFBSTATE_NORMAL:           reader_->readMsg();        break;
  case RFBSTATE_UNINITIALISED:
    throw Exception("CConnection::processMsg: not initialised yet?");
  default:
    throw Exception("CConnection::processMsg: invalid state");
  }
}

void CConnection::processVersionMsg()
{
  vlog.debug("reading protocol version");
  bool done;
  if (!cp.readVersion(is, &done)) {
    state_ = RFBSTATE_INVALID;
    throw Exception("reading version failed: not an RFB server?");
  }
  if (!done) return;

  vlog.info("Server supports RFB protocol version %d.%d",
            cp.majorVersion, cp.minorVersion);

  // The only official RFB protocol versions are currently 3.3, 3.7 and 3.8
  if (cp.beforeVersion(3,3)) {
    char msg[256];
    sprintf(msg,"Server gave unsupported RFB protocol version %d.%d",
            cp.majorVersion, cp.minorVersion);
    vlog.error("%s", msg);
    state_ = RFBSTATE_INVALID;
    throw Exception(msg);
  } else if (useProtocol3_3 || cp.beforeVersion(3,7)) {
    cp.setVersion(3,3);
  } else if (cp.afterVersion(3,8)) {
    cp.setVersion(3,8);
  }

  cp.writeVersion(os);
  state_ = RFBSTATE_SECURITY_TYPES;

  vlog.info("Using RFB protocol version %d.%d",
            cp.majorVersion, cp.minorVersion);
}


void CConnection::processSecurityTypesMsg()
{
  vlog.debug("processing security types message");

  int secType = secTypeInvalid;

  std::list<rdr::U8> secTypes;
  secTypes = security->GetEnabledSecTypes();

  if (cp.isVersion(3,3)) {

    // legacy 3.3 server may only offer "vnc authentication" or "none"

    secType = is->readU32();
    if (secType == secTypeInvalid) {
      throwConnFailedException();

    } else if (secType == secTypeNone || secType == secTypeVncAuth) {
      std::list<rdr::U8>::iterator i;
      for (i = secTypes.begin(); i != secTypes.end(); i++)
        if (*i == secType) {
          secType = *i;
          break;
        }

      if (i == secTypes.end())
        secType = secTypeInvalid;
    } else {
      vlog.error("Unknown 3.3 security type %d", secType);
      throw Exception("Unknown 3.3 security type");
    }

  } else {

    // >=3.7 server will offer us a list

    int nServerSecTypes = is->readU8();
    if (nServerSecTypes == 0)
      throwConnFailedException();

    std::list<rdr::U8>::iterator j;

    for (int i = 0; i < nServerSecTypes; i++) {
      rdr::U8 serverSecType = is->readU8();
      vlog.debug("Server offers security type %s(%d)",
                 secTypeName(serverSecType), serverSecType);

      /*
       * Use the first type sent by server which matches client's type.
       * It means server's order specifies priority.
       */
      if (secType == secTypeInvalid) {
        for (j = secTypes.begin(); j != secTypes.end(); j++)
          if (*j == serverSecType) {
            secType = *j;
            break;
          }
       }
    }

    // Inform the server of our decision
    if (secType != secTypeInvalid) {
      os->writeU8(secType);
      os->flush();
      vlog.debug("Choosing security type %s(%d)",secTypeName(secType),secType);
    }
  }

  if (secType == secTypeInvalid) {
    state_ = RFBSTATE_INVALID;
    vlog.error("No matching security types");
    throw Exception("No matching security types");
  }

  state_ = RFBSTATE_SECURITY;
  csecurity = security->GetCSecurity(secType);
  processSecurityMsg();
}

void CConnection::processSecurityMsg()
{
  vlog.debug("processing security message");
  if (csecurity->processMsg(this)) {
    state_ = RFBSTATE_SECURITY_RESULT;
    processSecurityResultMsg();
  }
}

void CConnection::processSecurityResultMsg()
{
  vlog.debug("processing security result message");
  int result;
  if (cp.beforeVersion(3,8) && csecurity->getType() == secTypeNone) {
    result = secResultOK;
  } else {
    if (!is->checkNoWait(1)) return;
    result = is->readU32();
  }
  switch (result) {
  case secResultOK:
    securityCompleted();
    return;
  case secResultFailed:
    vlog.debug("auth failed");
    break;
  case secResultTooMany:
    vlog.debug("auth failed - too many tries");
    break;
  default:
    throw Exception("Unknown security result from server");
  }
  CharArray reason;
  if (cp.beforeVersion(3,8))
    reason.buf = strDup("Authentication failure");
  else
    reason.buf = is->readString();
  state_ = RFBSTATE_INVALID;
  throw AuthFailureException(reason.buf);
}

void CConnection::processInitMsg()
{
  vlog.debug("reading server initialisation");
  reader_->readServerInit();
}

void CConnection::throwConnFailedException()
{
  state_ = RFBSTATE_INVALID;
  CharArray reason;
  reason.buf = is->readString();
  throw ConnFailedException(reason.buf);
}

void CConnection::securityCompleted()
{
  state_ = RFBSTATE_INITIALISATION;
  reader_ = new CMsgReaderV3(this, is);
  writer_ = new CMsgWriterV3(&cp, os);
  vlog.debug("Authentication success!");
  authSuccess();
  writer_->writeClientInit(shared);
}

void CConnection::authSuccess()
{
}

void CConnection::serverInit()
{
  state_ = RFBSTATE_NORMAL;
  vlog.debug("initialisation done");
}

void CConnection::fence(rdr::U32 flags, unsigned len, const char data[])
{
  CMsgHandler::fence(flags, len, data);

  if (!(flags & fenceFlagRequest))
    return;

  // We cannot guarantee any synchronisation at this level
  flags = 0;

  writer()->writeFence(flags, len, data);
}
