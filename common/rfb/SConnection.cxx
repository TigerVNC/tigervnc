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
#include <stdio.h>
#include <string.h>
#include <rfb/Exception.h>
#include <rfb/Security.h>
#include <rfb/msgTypes.h>
#include <rfb/fenceTypes.h>
#include <rfb/SMsgReader.h>
#include <rfb/SMsgWriter.h>
#include <rfb/SConnection.h>
#include <rfb/ServerCore.h>
#include <rfb/encodings.h>
#include <rfb/EncodeManager.h>
#include <rfb/SSecurity.h>

#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("SConnection");

// AccessRights values
const SConnection::AccessRights SConnection::AccessView           = 0x0001;
const SConnection::AccessRights SConnection::AccessKeyEvents      = 0x0002;
const SConnection::AccessRights SConnection::AccessPtrEvents      = 0x0004;
const SConnection::AccessRights SConnection::AccessCutText        = 0x0008;
const SConnection::AccessRights SConnection::AccessSetDesktopSize = 0x0010;
const SConnection::AccessRights SConnection::AccessNonShared      = 0x0020;
const SConnection::AccessRights SConnection::AccessDefault        = 0x03ff;
const SConnection::AccessRights SConnection::AccessNoQuery        = 0x0400;
const SConnection::AccessRights SConnection::AccessFull           = 0xffff;


SConnection::SConnection()
  : readyForSetColourMapEntries(false),
    is(0), os(0), reader_(0), writer_(0),
    ssecurity(0), state_(RFBSTATE_UNINITIALISED),
    preferredEncoding(encodingRaw)
{
  defaultMajorVersion = 3;
  defaultMinorVersion = 8;
  if (rfb::Server::protocol3_3)
    defaultMinorVersion = 3;

  client.setVersion(defaultMajorVersion, defaultMinorVersion);
}

SConnection::~SConnection()
{
  if (ssecurity)
    delete ssecurity;
  delete reader_;
  reader_ = 0;
  delete writer_;
  writer_ = 0;
}

void SConnection::setStreams(rdr::InStream* is_, rdr::OutStream* os_)
{
  is = is_;
  os = os_;
}

void SConnection::initialiseProtocol()
{
  char str[13];

  sprintf(str, "RFB %03d.%03d\n", defaultMajorVersion, defaultMinorVersion);
  os->writeBytes(str, 12);
  os->flush();

  state_ = RFBSTATE_PROTOCOL_VERSION;
}

void SConnection::processMsg()
{
  switch (state_) {
  case RFBSTATE_PROTOCOL_VERSION: processVersionMsg();      break;
  case RFBSTATE_SECURITY_TYPE:    processSecurityTypeMsg(); break;
  case RFBSTATE_SECURITY:         processSecurityMsg();     break;
  case RFBSTATE_INITIALISATION:   processInitMsg();         break;
  case RFBSTATE_NORMAL:           reader_->readMsg();       break;
  case RFBSTATE_QUERYING:
    throw Exception("SConnection::processMsg: bogus data from client while "
                    "querying");
  case RFBSTATE_UNINITIALISED:
    throw Exception("SConnection::processMsg: not initialised yet?");
  default:
    throw Exception("SConnection::processMsg: invalid state");
  }
}

void SConnection::processVersionMsg()
{
  char verStr[13];
  int majorVersion;
  int minorVersion;

  vlog.debug("reading protocol version");

  if (!is->checkNoWait(12))
    return;

  is->readBytes(verStr, 12);
  verStr[12] = '\0';

  if (sscanf(verStr, "RFB %03d.%03d\n",
             &majorVersion, &minorVersion) != 2) {
    state_ = RFBSTATE_INVALID;
    throw Exception("reading version failed: not an RFB client?");
  }

  client.setVersion(majorVersion, minorVersion);

  vlog.info("Client needs protocol version %d.%d",
            client.majorVersion, client.minorVersion);

  if (client.majorVersion != 3) {
    // unknown protocol version
    throwConnFailedException("Client needs protocol version %d.%d, server has %d.%d",
                             client.majorVersion, client.minorVersion,
                             defaultMajorVersion, defaultMinorVersion);
  }

  if (client.minorVersion != 3 && client.minorVersion != 7 && client.minorVersion != 8) {
    vlog.error("Client uses unofficial protocol version %d.%d",
               client.majorVersion,client.minorVersion);
    if (client.minorVersion >= 8)
      client.minorVersion = 8;
    else if (client.minorVersion == 7)
      client.minorVersion = 7;
    else
      client.minorVersion = 3;
    vlog.error("Assuming compatibility with version %d.%d",
               client.majorVersion,client.minorVersion);
  }

  versionReceived();

  std::list<rdr::U8> secTypes;
  std::list<rdr::U8>::iterator i;
  secTypes = security.GetEnabledSecTypes();

  if (client.isVersion(3,3)) {

    // cope with legacy 3.3 client only if "no authentication" or "vnc
    // authentication" is supported.
    for (i=secTypes.begin(); i!=secTypes.end(); i++) {
      if (*i == secTypeNone || *i == secTypeVncAuth) break;
    }
    if (i == secTypes.end()) {
      throwConnFailedException("No supported security type for %d.%d client",
                               client.majorVersion, client.minorVersion);
    }

    os->writeU32(*i);
    if (*i == secTypeNone) os->flush();
    state_ = RFBSTATE_SECURITY;
    ssecurity = security.GetSSecurity(this, *i);
    processSecurityMsg();
    return;
  }

  // list supported security types for >=3.7 clients

  if (secTypes.empty())
    throwConnFailedException("No supported security types");

  os->writeU8(secTypes.size());
  for (i=secTypes.begin(); i!=secTypes.end(); i++)
    os->writeU8(*i);
  os->flush();
  state_ = RFBSTATE_SECURITY_TYPE;
}


void SConnection::processSecurityTypeMsg()
{
  vlog.debug("processing security type message");
  int secType = is->readU8();

  processSecurityType(secType);
}

void SConnection::processSecurityType(int secType)
{
  // Verify that the requested security type should be offered
  std::list<rdr::U8> secTypes;
  std::list<rdr::U8>::iterator i;

  secTypes = security.GetEnabledSecTypes();
  for (i=secTypes.begin(); i!=secTypes.end(); i++)
    if (*i == secType) break;
  if (i == secTypes.end())
    throw Exception("Requested security type not available");

  vlog.info("Client requests security type %s(%d)",
            secTypeName(secType),secType);

  try {
    state_ = RFBSTATE_SECURITY;
    ssecurity = security.GetSSecurity(this, secType);
  } catch (rdr::Exception& e) {
    throwConnFailedException("%s", e.str());
  }

  processSecurityMsg();
}

void SConnection::processSecurityMsg()
{
  vlog.debug("processing security message");
  try {
    bool done = ssecurity->processMsg();
    if (done) {
      state_ = RFBSTATE_QUERYING;
      setAccessRights(ssecurity->getAccessRights());
      queryConnection(ssecurity->getUserName());
    }
  } catch (AuthFailureException& e) {
    vlog.error("AuthFailureException: %s", e.str());
    state_ = RFBSTATE_SECURITY_FAILURE;
    authFailure(e.str());
  }
}

void SConnection::processInitMsg()
{
  vlog.debug("reading client initialisation");
  reader_->readClientInit();
}

void SConnection::throwConnFailedException(const char* format, ...)
{
	va_list ap;
	char str[256];

	va_start(ap, format);
	(void) vsnprintf(str, sizeof(str), format, ap);
	va_end(ap);

  vlog.info("Connection failed: %s", str);

  if (state_ == RFBSTATE_PROTOCOL_VERSION) {
    if (client.majorVersion == 3 && client.minorVersion == 3) {
      os->writeU32(0);
      os->writeString(str);
      os->flush();
    } else {
      os->writeU8(0);
      os->writeString(str);
      os->flush();
    }
  }

  state_ = RFBSTATE_INVALID;
  throw ConnFailedException(str);
}

void SConnection::setAccessRights(AccessRights ar)
{
  accessRights = ar;
}

bool SConnection::accessCheck(AccessRights ar) const
{
  return (accessRights & ar) == ar;
}

void SConnection::setEncodings(int nEncodings, const rdr::S32* encodings)
{
  int i;

  preferredEncoding = encodingRaw;
  for (i = 0;i < nEncodings;i++) {
    if (EncodeManager::supported(encodings[i])) {
      preferredEncoding = encodings[i];
      break;
    }
  }

  SMsgHandler::setEncodings(nEncodings, encodings);
}

void SConnection::supportsQEMUKeyEvent()
{
  writer()->writeQEMUKeyEvent();
}

void SConnection::versionReceived()
{
}

void SConnection::authSuccess()
{
}

void SConnection::authFailure(const char* reason)
{
  if (state_ != RFBSTATE_SECURITY_FAILURE)
    throw Exception("SConnection::authFailure: invalid state");

  os->writeU32(secResultFailed);
  if (!client.beforeVersion(3,8)) // 3.8 onwards have failure message
    os->writeString(reason);
  os->flush();

  throw AuthFailureException(reason);
}

void SConnection::queryConnection(const char* userName)
{
  approveConnection(true);
}

void SConnection::approveConnection(bool accept, const char* reason)
{
  if (state_ != RFBSTATE_QUERYING)
    throw Exception("SConnection::approveConnection: invalid state");

  if (!client.beforeVersion(3,8) || ssecurity->getType() != secTypeNone) {
    if (accept) {
      os->writeU32(secResultOK);
    } else {
      os->writeU32(secResultFailed);
      if (!client.beforeVersion(3,8)) { // 3.8 onwards have failure message
        if (reason)
          os->writeString(reason);
        else
          os->writeString("Authentication failure");
      }
    }
    os->flush();
  }

  if (accept) {
    state_ = RFBSTATE_INITIALISATION;
    reader_ = new SMsgReader(this, is);
    writer_ = new SMsgWriter(&client, os);
    authSuccess();
  } else {
    state_ = RFBSTATE_INVALID;
    if (reason)
      throw AuthFailureException(reason);
    else
      throw AuthFailureException();
  }
}

void SConnection::clientInit(bool shared)
{
  writer_->writeServerInit(client.width(), client.height(),
                           client.pf(), client.name());
  state_ = RFBSTATE_NORMAL;
}

void SConnection::close(const char* reason)
{
  state_ = RFBSTATE_CLOSING;
}

void SConnection::setPixelFormat(const PixelFormat& pf)
{
  SMsgHandler::setPixelFormat(pf);
  readyForSetColourMapEntries = true;
  if (!pf.trueColour)
    writeFakeColourMap();
}

void SConnection::framebufferUpdateRequest(const Rect& r, bool incremental)
{
  if (!readyForSetColourMapEntries) {
    readyForSetColourMapEntries = true;
    if (!client.pf().trueColour) {
      writeFakeColourMap();
    }
  }
}

void SConnection::fence(rdr::U32 flags, unsigned len, const char data[])
{
  if (!(flags & fenceFlagRequest))
    return;

  // We cannot guarantee any synchronisation at this level
  flags = 0;

  writer()->writeFence(flags, len, data);
}

void SConnection::enableContinuousUpdates(bool enable,
                                          int x, int y, int w, int h)
{
}

void SConnection::writeFakeColourMap(void)
{
  int i;
  rdr::U16 red[256], green[256], blue[256];

  for (i = 0;i < 256;i++)
    client.pf().rgbFromPixel(i, &red[i], &green[i], &blue[i]);

  writer()->writeSetColourMapEntries(0, 256, red, green, blue);
}
