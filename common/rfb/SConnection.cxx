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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <rfb/Exception.h>
#include <rfb/Security.h>
#include <rfb/clipboardTypes.h>
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
    is(0), os(0), reader_(0), writer_(0), ssecurity(0),
    authFailureTimer(this, &SConnection::handleAuthFailureTimeout),
    state_(RFBSTATE_UNINITIALISED), preferredEncoding(encodingRaw),
    accessRights(0x0000), clientClipboard(NULL), hasLocalClipboard(false),
    unsolicitedClipboardAttempt(false)
{
  defaultMajorVersion = 3;
  defaultMinorVersion = 8;
  if (rfb::Server::protocol3_3)
    defaultMinorVersion = 3;

  client.setVersion(defaultMajorVersion, defaultMinorVersion);
}

SConnection::~SConnection()
{
  cleanup();
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

bool SConnection::processMsg()
{
  switch (state_) {
  case RFBSTATE_PROTOCOL_VERSION: return processVersionMsg();      break;
  case RFBSTATE_SECURITY_TYPE:    return processSecurityTypeMsg(); break;
  case RFBSTATE_SECURITY:         return processSecurityMsg();     break;
  case RFBSTATE_SECURITY_FAILURE: return processSecurityFailure(); break;
  case RFBSTATE_INITIALISATION:   return processInitMsg();         break;
  case RFBSTATE_NORMAL:           return reader_->readMsg();       break;
  case RFBSTATE_QUERYING:
    throw Exception("SConnection::processMsg: bogus data from client while "
                    "querying");
  case RFBSTATE_CLOSING:
    throw Exception("SConnection::processMsg: called while closing");
  case RFBSTATE_UNINITIALISED:
    throw Exception("SConnection::processMsg: not initialised yet?");
  default:
    throw Exception("SConnection::processMsg: invalid state");
  }
}

bool SConnection::processVersionMsg()
{
  char verStr[13];
  int majorVersion;
  int minorVersion;

  vlog.debug("reading protocol version");

  if (!is->hasData(12))
    return false;

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
    return true;
  }

  // list supported security types for >=3.7 clients

  if (secTypes.empty())
    throwConnFailedException("No supported security types");

  os->writeU8(secTypes.size());
  for (i=secTypes.begin(); i!=secTypes.end(); i++)
    os->writeU8(*i);
  os->flush();
  state_ = RFBSTATE_SECURITY_TYPE;

  return true;
}


bool SConnection::processSecurityTypeMsg()
{
  vlog.debug("processing security type message");

  if (!is->hasData(1))
    return false;

  int secType = is->readU8();

  processSecurityType(secType);

  return true;
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
}

bool SConnection::processSecurityMsg()
{
  vlog.debug("processing security message");
  try {
    if (!ssecurity->processMsg())
      return false;
  } catch (AuthFailureException& e) {
    vlog.error("AuthFailureException: %s", e.str());
    state_ = RFBSTATE_SECURITY_FAILURE;
    // Introduce a slight delay of the authentication failure response
    // to make it difficult to brute force a password
    authFailureMsg.replaceBuf(strDup(e.str()));
    authFailureTimer.start(100);
    return true;
  }

  state_ = RFBSTATE_QUERYING;
  setAccessRights(ssecurity->getAccessRights());
  queryConnection(ssecurity->getUserName());

  // If the connection got approved right away then we can continue
  if (state_ == RFBSTATE_INITIALISATION)
    return true;

  // Otherwise we need to wait for the result
  // (or give up if if was rejected)
  return false;
}

bool SConnection::processSecurityFailure()
{
  // Silently drop any data if we are currently delaying an
  // authentication failure response as otherwise we would close
  // the connection on unexpected data, and an attacker could use
  // that to detect our delayed state.

  if (!is->hasData(1))
    return false;

  is->skip(is->avail());

  return true;
}

bool SConnection::processInitMsg()
{
  vlog.debug("reading client initialisation");
  return reader_->readClientInit();
}

bool SConnection::handleAuthFailureTimeout(Timer* t)
{
  if (state_ != RFBSTATE_SECURITY_FAILURE) {
    close("SConnection::handleAuthFailureTimeout: invalid state");
    return false;
  }

  try {
    os->writeU32(secResultFailed);
    if (!client.beforeVersion(3,8)) { // 3.8 onwards have failure message
      const char* reason = authFailureMsg.buf;
      os->writeU32(strlen(reason));
      os->writeBytes(reason, strlen(reason));
    }
    os->flush();
  } catch (rdr::Exception& e) {
    close(e.str());
    return false;
  }

  close(authFailureMsg.buf);

  return false;
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
      os->writeU32(strlen(str));
      os->writeBytes(str, strlen(str));
      os->flush();
    } else {
      os->writeU8(0);
      os->writeU32(strlen(str));
      os->writeBytes(str, strlen(str));
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
  if (state_ < RFBSTATE_QUERYING)
    throw Exception("SConnection::accessCheck: invalid state");

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

  if (client.supportsEncoding(pseudoEncodingExtendedClipboard)) {
    rdr::U32 sizes[] = { 0 };
    writer()->writeClipboardCaps(rfb::clipboardUTF8 |
                                 rfb::clipboardRequest |
                                 rfb::clipboardPeek |
                                 rfb::clipboardNotify |
                                 rfb::clipboardProvide,
                                 sizes);
  }
}

void SConnection::clientCutText(const char* str)
{
  hasLocalClipboard = false;

  strFree(clientClipboard);
  clientClipboard = NULL;

  clientClipboard = latin1ToUTF8(str);

  handleClipboardAnnounce(true);
}

void SConnection::handleClipboardRequest(rdr::U32 flags)
{
  if (!(flags & rfb::clipboardUTF8)) {
    vlog.debug("Ignoring clipboard request for unsupported formats 0x%x", flags);
    return;
  }
  if (!hasLocalClipboard) {
    vlog.debug("Ignoring unexpected clipboard request");
    return;
  }
  handleClipboardRequest();
}

void SConnection::handleClipboardPeek(rdr::U32 flags)
{
  if (client.clipboardFlags() & rfb::clipboardNotify)
    writer()->writeClipboardNotify(hasLocalClipboard ? rfb::clipboardUTF8 : 0);
}

void SConnection::handleClipboardNotify(rdr::U32 flags)
{
  strFree(clientClipboard);
  clientClipboard = NULL;

  if (flags & rfb::clipboardUTF8) {
    hasLocalClipboard = false;
    handleClipboardAnnounce(true);
  } else {
    handleClipboardAnnounce(false);
  }
}

void SConnection::handleClipboardProvide(rdr::U32 flags,
                                         const size_t* lengths,
                                         const rdr::U8* const* data)
{
  if (!(flags & rfb::clipboardUTF8)) {
    vlog.debug("Ignoring clipboard provide with unsupported formats 0x%x", flags);
    return;
  }

  strFree(clientClipboard);
  clientClipboard = NULL;

  clientClipboard = convertLF((const char*)data[0], lengths[0]);

  // FIXME: Should probably verify that this data was actually requested
  handleClipboardData(clientClipboard);
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
        if (!reason)
          reason = "Authentication failure";
        os->writeU32(strlen(reason));
        os->writeBytes(reason, strlen(reason));
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
  cleanup();
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

void SConnection::handleClipboardRequest()
{
}

void SConnection::handleClipboardAnnounce(bool available)
{
}

void SConnection::handleClipboardData(const char* data)
{
}

void SConnection::requestClipboard()
{
  if (clientClipboard != NULL) {
    handleClipboardData(clientClipboard);
    return;
  }

  if (client.supportsEncoding(pseudoEncodingExtendedClipboard) &&
      (client.clipboardFlags() & rfb::clipboardRequest))
    writer()->writeClipboardRequest(rfb::clipboardUTF8);
}

void SConnection::announceClipboard(bool available)
{
  hasLocalClipboard = available;
  unsolicitedClipboardAttempt = false;

  if (client.supportsEncoding(pseudoEncodingExtendedClipboard)) {
    // Attempt an unsolicited transfer?
    if (available &&
        (client.clipboardSize(rfb::clipboardUTF8) > 0) &&
        (client.clipboardFlags() & rfb::clipboardProvide)) {
      vlog.debug("Attempting unsolicited clipboard transfer...");
      unsolicitedClipboardAttempt = true;
      handleClipboardRequest();
      return;
    }

    if (client.clipboardFlags() & rfb::clipboardNotify) {
      writer()->writeClipboardNotify(available ? rfb::clipboardUTF8 : 0);
      return;
    }
  }

  if (available)
    handleClipboardRequest();
}

void SConnection::sendClipboardData(const char* data)
{
  if (client.supportsEncoding(pseudoEncodingExtendedClipboard) &&
      (client.clipboardFlags() & rfb::clipboardProvide)) {
    CharArray filtered(convertCRLF(data));
    size_t sizes[1] = { strlen(filtered.buf) + 1 };
    const rdr::U8* data[1] = { (const rdr::U8*)filtered.buf };

    if (unsolicitedClipboardAttempt) {
      unsolicitedClipboardAttempt = false;
      if (sizes[0] > client.clipboardSize(rfb::clipboardUTF8)) {
        vlog.debug("Clipboard was too large for unsolicited clipboard transfer");
        if (client.clipboardFlags() & rfb::clipboardNotify)
          writer()->writeClipboardNotify(rfb::clipboardUTF8);
        return;
      }
    }

    writer()->writeClipboardProvide(rfb::clipboardUTF8, sizes, data);
  } else {
    CharArray latin1(utf8ToLatin1(data));

    writer()->writeServerCutText(latin1.buf);
  }
}

void SConnection::cleanup()
{
  delete ssecurity;
  ssecurity = NULL;
  delete reader_;
  reader_ = NULL;
  delete writer_;
  writer_ = NULL;
  strFree(clientClipboard);
  clientClipboard = NULL;
}

void SConnection::writeFakeColourMap(void)
{
  int i;
  rdr::U16 red[256], green[256], blue[256];

  for (i = 0;i < 256;i++)
    client.pf().rgbFromPixel(i, &red[i], &green[i], &blue[i]);

  writer()->writeSetColourMapEntries(0, 256, red, green, blue);
}
