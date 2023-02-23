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

#include <algorithm>

#include <core/LogWriter.h>
#include <core/string.h>

#include <rdr/OutStream.h>

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

using namespace rfb;

static core::LogWriter vlog("SConnection");

SConnection::SConnection(AccessRights accessRights_)
  : readyForSetColourMapEntries(false), is(nullptr), os(nullptr),
    reader_(nullptr), writer_(nullptr), ssecurity(nullptr),
    authFailureTimer(this, &SConnection::handleAuthFailureTimeout),
    state_(RFBSTATE_UNINITIALISED), preferredEncoding(encodingRaw),
    accessRights(accessRights_), hasRemoteClipboard(false),
    hasLocalClipboard(false),
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
  os->writeBytes((const uint8_t*)str, 12);
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
    throw std::logic_error("SConnection::processMsg: Bogus data from "
                           "client while querying");
  case RFBSTATE_CLOSING:
    throw std::logic_error("SConnection::processMsg: Called while closing");
  case RFBSTATE_UNINITIALISED:
    throw std::logic_error("SConnection::processMsg: Not initialised yet?");
  default:
    throw std::logic_error("SConnection::processMsg: Invalid state");
  }
}

bool SConnection::processVersionMsg()
{
  char verStr[13];
  int majorVersion;
  int minorVersion;

  vlog.debug("Reading protocol version");

  if (!is->hasData(12))
    return false;

  is->readBytes((uint8_t*)verStr, 12);
  verStr[12] = '\0';

  if (sscanf(verStr, "RFB %03d.%03d\n",
             &majorVersion, &minorVersion) != 2) {
    state_ = RFBSTATE_INVALID;
    throw protocol_error("Reading version failed, not an RFB client?");
  }

  client.setVersion(majorVersion, minorVersion);

  vlog.info("Client needs protocol version %d.%d",
            client.majorVersion, client.minorVersion);

  if (client.majorVersion != 3) {
    // unknown protocol version
    failConnection(core::format(
      "Client needs protocol version %d.%d, server has %d.%d",
      client.majorVersion, client.minorVersion,
      defaultMajorVersion, defaultMinorVersion));
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

  std::list<uint8_t> secTypes;
  std::list<uint8_t>::iterator i;
  secTypes = security.GetEnabledSecTypes();

  if (client.isVersion(3,3)) {

    // cope with legacy 3.3 client only if "no authentication" or "vnc
    // authentication" is supported.
    for (i=secTypes.begin(); i!=secTypes.end(); i++) {
      if (*i == secTypeNone || *i == secTypeVncAuth) break;
    }
    if (i == secTypes.end()) {
      failConnection(
        core::format("No supported security type for %d.%d client",
                     client.majorVersion, client.minorVersion));
    }

    os->writeU32(*i);
    if (*i == secTypeNone) os->flush();
    state_ = RFBSTATE_SECURITY;
    ssecurity = security.GetSSecurity(this, *i);
    return true;
  }

  // list supported security types for >=3.7 clients

  if (secTypes.empty())
    failConnection("No supported security types");

  os->writeU8(secTypes.size());
  for (i=secTypes.begin(); i!=secTypes.end(); i++)
    os->writeU8(*i);
  os->flush();
  state_ = RFBSTATE_SECURITY_TYPE;

  return true;
}


bool SConnection::processSecurityTypeMsg()
{
  vlog.debug("Processing security type message");

  if (!is->hasData(1))
    return false;

  int secType = is->readU8();

  processSecurityType(secType);

  return true;
}

void SConnection::processSecurityType(int secType)
{
  // Verify that the requested security type should be offered
  std::list<uint8_t> secTypes;

  secTypes = security.GetEnabledSecTypes();
  if (std::find(secTypes.begin(), secTypes.end(),
                secType) == secTypes.end())
    throw protocol_error("Requested security type not available");

  vlog.info("Client requests security type %s(%d)",
            secTypeName(secType),secType);

  try {
    state_ = RFBSTATE_SECURITY;
    ssecurity = security.GetSSecurity(this, secType);
  } catch (std::exception& e) {
    failConnection(e.what());
  }
}

bool SConnection::processSecurityMsg()
{
  vlog.debug("Processing security message");
  try {
    if (!ssecurity->processMsg())
      return false;
  } catch (auth_error& e) {
    vlog.error("Authentication error: %s", e.what());
    state_ = RFBSTATE_SECURITY_FAILURE;
    // Introduce a slight delay of the authentication failure response
    // to make it difficult to brute force a password
    authFailureMsg = e.what();
    authFailureTimer.start(100);
    return true;
  }

  state_ = RFBSTATE_QUERYING;
  setAccessRights(accessRights & ssecurity->getAccessRights());
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
  vlog.debug("Reading client initialisation");
  return reader_->readClientInit();
}

void SConnection::handleAuthFailureTimeout(core::Timer* /*t*/)
{
  if (state_ != RFBSTATE_SECURITY_FAILURE) {
    close("SConnection::handleAuthFailureTimeout: Invalid state");
    return;
  }

  try {
    os->writeU32(secResultFailed);
    if (!client.beforeVersion(3,8)) { // 3.8 onwards have failure message
      os->writeU32(authFailureMsg.size());
      os->writeBytes((const uint8_t*)authFailureMsg.data(),
                     authFailureMsg.size());
    }
    os->flush();
  } catch (std::exception& e) {
    close(e.what());
    return;
  }

  close(authFailureMsg.c_str());
}

void SConnection::failConnection(const char* message)
{
  vlog.info("Connection failed: %s", message);

  if (state_ == RFBSTATE_PROTOCOL_VERSION) {
    if (client.majorVersion == 3 && client.minorVersion == 3) {
      os->writeU32(0);
      os->writeU32(strlen(message));
      os->writeBytes((const uint8_t*)message, strlen(message));
      os->flush();
    } else {
      os->writeU8(0);
      os->writeU32(strlen(message));
      os->writeBytes((const uint8_t*)message, strlen(message));
      os->flush();
    }
  }

  state_ = RFBSTATE_INVALID;
  throw protocol_error(message);
}

void SConnection::failConnection(const std::string& message)
{
  failConnection(message.c_str());
}

void SConnection::setAccessRights(AccessRights ar)
{
  accessRights = ar;
}

bool SConnection::accessCheck(AccessRights ar) const
{
  if (state_ < RFBSTATE_QUERYING)
    throw std::logic_error("SConnection::accessCheck: Invalid state");

  return (accessRights & ar) == ar;
}

void SConnection::setEncodings(int nEncodings, const int32_t* encodings)
{
  int i;
  bool firstFence, firstContinuousUpdates, firstLEDState,
       firstQEMUKeyEvent, firstExtMouseButtonsEvent;

  preferredEncoding = encodingRaw;
  for (i = 0;i < nEncodings;i++) {
    if (EncodeManager::supported(encodings[i])) {
      preferredEncoding = encodings[i];
      break;
    }
  }

  firstFence = !client.supportsFence();
  firstContinuousUpdates = !client.supportsContinuousUpdates();
  firstLEDState = !client.supportsLEDState();
  firstQEMUKeyEvent = !client.supportsEncoding(pseudoEncodingQEMUKeyEvent);
  firstExtMouseButtonsEvent = !client.supportsEncoding(pseudoEncodingExtendedMouseButtons);

  client.setEncodings(nEncodings, encodings);

  supportsLocalCursor();

  if (client.supportsFence() && firstFence)
    supportsFence();
  if (client.supportsContinuousUpdates() && firstContinuousUpdates)
    supportsContinuousUpdates();
  if (client.supportsLEDState() && firstLEDState)
    supportsLEDState();
  if (client.supportsEncoding(pseudoEncodingQEMUKeyEvent) && firstQEMUKeyEvent)
    writer()->writeQEMUKeyEvent();
  if (client.supportsEncoding(pseudoEncodingExtendedMouseButtons) && firstExtMouseButtonsEvent)
    writer()->writeExtendedMouseButtonsSupport();

  if (client.supportsEncoding(pseudoEncodingExtendedClipboard)) {
    uint32_t sizes[] = { 0 };
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

  clientClipboard = str;
  hasRemoteClipboard = true;

  if (!accessCheck(AccessCutText))
    return;

  handleClipboardAnnounce(true);
}

void SConnection::handleClipboardCaps(uint32_t flags, const uint32_t* lengths)
{
  int i;

  vlog.debug("Got client clipboard capabilities:");
  for (i = 0;i < 16;i++) {
    if (flags & (1 << i)) {
      const char *type;

      switch (1 << i) {
        case clipboardUTF8:
          type = "Plain text";
          break;
        case clipboardRTF:
          type = "Rich text";
          break;
        case clipboardHTML:
          type = "HTML";
          break;
        case clipboardDIB:
          type = "Images";
          break;
        case clipboardFiles:
          type = "Files";
          break;
        default:
          vlog.debug("    Unknown format 0x%x", 1 << i);
          continue;
      }

      if (lengths[i] == 0)
        vlog.debug("    %s (only notify)", type);
      else {
        vlog.debug("    %s (automatically send up to %s)",
                   type, core::iecPrefix(lengths[i], "B").c_str());
      }
    }
  }

  client.setClipboardCaps(flags, lengths);
}

void SConnection::handleClipboardRequest(uint32_t flags)
{
  if (!(flags & rfb::clipboardUTF8)) {
    vlog.debug("Ignoring clipboard request for unsupported formats 0x%x", flags);
    return;
  }
  if (!hasLocalClipboard) {
    vlog.debug("Ignoring unexpected clipboard request");
    return;
  }
  if (!accessCheck(AccessCutText))
    return;
  handleClipboardRequest();
}

void SConnection::handleClipboardPeek()
{
  if (client.clipboardFlags() & rfb::clipboardNotify)
    writer()->writeClipboardNotify(hasLocalClipboard ? rfb::clipboardUTF8 : 0);
}

void SConnection::handleClipboardNotify(uint32_t flags)
{
  hasRemoteClipboard = false;

  if (flags & rfb::clipboardUTF8) {
    hasLocalClipboard = false;
    if (!accessCheck(AccessCutText))
      return;
    handleClipboardAnnounce(true);
  } else {
    if (!accessCheck(AccessCutText))
      return;
    handleClipboardAnnounce(false);
  }

}

void SConnection::handleClipboardProvide(uint32_t flags,
                                         const size_t* lengths,
                                         const uint8_t* const* data)
{
  if (!(flags & rfb::clipboardUTF8)) {
    vlog.debug("Ignoring clipboard provide with unsupported formats 0x%x", flags);
    return;
  }

  // FIXME: This conversion magic should be in SMsgReader
  if (!core::isValidUTF8((const char*)data[0], lengths[0])) {
    vlog.error("Invalid UTF-8 sequence in clipboard - ignoring");
    return;
  }
  clientClipboard = core::convertLF((const char*)data[0], lengths[0]);
  hasRemoteClipboard = true;

  if (!accessCheck(AccessCutText))
    return;

  // FIXME: Should probably verify that this data was actually requested
  handleClipboardData(clientClipboard.c_str());
}

void SConnection::supportsLocalCursor()
{
}

void SConnection::supportsFence()
{
}

void SConnection::supportsContinuousUpdates()
{
}

void SConnection::supportsLEDState()
{
}

void SConnection::authSuccess()
{
}

void SConnection::queryConnection(const char* /*userName*/)
{
  approveConnection(true);
}

void SConnection::approveConnection(bool accept, const char* reason)
{
  if (state_ != RFBSTATE_QUERYING)
    throw std::logic_error("SConnection::approveConnection: Invalid state");

  if (!client.beforeVersion(3,8) || ssecurity->getType() != secTypeNone) {
    if (accept) {
      os->writeU32(secResultOK);
    } else {
      os->writeU32(secResultFailed);
      if (!client.beforeVersion(3,8)) { // 3.8 onwards have failure message
        if (!reason)
          reason = "Connection rejected";
        os->writeU32(strlen(reason));
        os->writeBytes((const uint8_t*)reason, strlen(reason));
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
      throw auth_error(reason);
    else
      throw auth_error("Connection rejected");
  }
}

void SConnection::clientInit(bool /*shared*/)
{
  writer_->writeServerInit(client.width(), client.height(),
                           client.pf(), client.name());
  state_ = RFBSTATE_NORMAL;
}

void SConnection::close(const char* /*reason*/)
{
  state_ = RFBSTATE_CLOSING;
  cleanup();
}

void SConnection::setPixelFormat(const PixelFormat& pf)
{
  client.setPF(pf);
  readyForSetColourMapEntries = true;
  if (!pf.trueColour)
    writeFakeColourMap();
}

void SConnection::framebufferUpdateRequest(const core::Rect& /*r*/,
                                           bool /*incremental*/)
{
  if (!readyForSetColourMapEntries) {
    readyForSetColourMapEntries = true;
    if (!client.pf().trueColour) {
      writeFakeColourMap();
    }
  }
}

void SConnection::fence(uint32_t flags, unsigned len,
                        const uint8_t data[])
{
  if (!(flags & fenceFlagRequest))
    return;

  // We cannot guarantee any synchronisation at this level
  flags = 0;

  writer()->writeFence(flags, len, data);
}

void SConnection::enableContinuousUpdates(bool /*enable*/,
                                          int /*x*/, int /*y*/,
                                          int /*w*/, int /*h*/)
{
}

void SConnection::handleClipboardRequest()
{
}

void SConnection::handleClipboardAnnounce(bool /*available*/)
{
}

void SConnection::handleClipboardData(const char* /*data*/)
{
}

void SConnection::requestClipboard()
{
  if (!accessCheck(AccessCutText))
    return;

  if (hasRemoteClipboard) {
    handleClipboardData(clientClipboard.c_str());
    return;
  }

  if (client.supportsEncoding(pseudoEncodingExtendedClipboard) &&
      (client.clipboardFlags() & rfb::clipboardRequest))
    writer()->writeClipboardRequest(rfb::clipboardUTF8);
}

void SConnection::announceClipboard(bool available)
{
  if (!accessCheck(AccessCutText))
    return;

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
  if (!accessCheck(AccessCutText))
    return;

  if (client.supportsEncoding(pseudoEncodingExtendedClipboard) &&
      (client.clipboardFlags() & rfb::clipboardProvide)) {
    // FIXME: This conversion magic should be in SMsgWriter
    std::string filtered(core::convertCRLF(data));
    size_t sizes[1] = { filtered.size() + 1 };
    const uint8_t* datas[1] = { (const uint8_t*)filtered.c_str() };

    if (unsolicitedClipboardAttempt) {
      unsolicitedClipboardAttempt = false;
      if (sizes[0] > client.clipboardSize(rfb::clipboardUTF8)) {
        vlog.debug("Clipboard was too large for unsolicited clipboard transfer");
        if (client.clipboardFlags() & rfb::clipboardNotify)
          writer()->writeClipboardNotify(rfb::clipboardUTF8);
        return;
      }
    }

    writer()->writeClipboardProvide(rfb::clipboardUTF8, sizes, datas);
  } else {
    writer()->writeServerCutText(data);
  }
}

void SConnection::cleanup()
{
  delete ssecurity;
  ssecurity = nullptr;
  delete reader_;
  reader_ = nullptr;
  delete writer_;
  writer_ = nullptr;
}

void SConnection::writeFakeColourMap(void)
{
  int i;
  uint16_t red[256], green[256], blue[256];

  for (i = 0;i < 256;i++)
    client.pf().rgbFromPixel(i, &red[i], &green[i], &blue[i]);

  writer()->writeSetColourMapEntries(0, 256, red, green, blue);
}
