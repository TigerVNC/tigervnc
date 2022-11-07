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

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <rfb/Exception.h>
#include <rfb/clipboardTypes.h>
#include <rfb/fenceTypes.h>
#include <rfb/CMsgReader.h>
#include <rfb/CMsgWriter.h>
#include <rfb/CSecurity.h>
#include <rfb/Decoder.h>
#include <rfb/Security.h>
#include <rfb/SecurityClient.h>
#include <rfb/CConnection.h>
#include <rfb/util.h>

#include <rfb/LogWriter.h>

#include <rdr/InStream.h>
#include <rdr/OutStream.h>

using namespace rfb;

static LogWriter vlog("CConnection");

CConnection::CConnection()
  : csecurity(0),
    supportsLocalCursor(false), supportsCursorPosition(false),
    supportsDesktopResize(false), supportsLEDState(false),
    is(0), os(0), reader_(0), writer_(0),
    shared(false),
    state_(RFBSTATE_UNINITIALISED), serverName(strDup("")),
    pendingPFChange(false), preferredEncoding(encodingTight),
    compressLevel(2), qualityLevel(-1),
    formatChange(false), encodingChange(false),
    firstUpdate(true), pendingUpdate(false), continuousUpdates(false),
    forceNonincremental(true),
    framebuffer(NULL), decoder(this),
    serverClipboard(NULL), hasLocalClipboard(false)
{
}

CConnection::~CConnection()
{
  close();
}

void CConnection::setServerName(const char* name_)
{
  if (name_ == NULL)
    name_ = "";
  serverName.replaceBuf(strDup(name_));
}

void CConnection::setStreams(rdr::InStream* is_, rdr::OutStream* os_)
{
  is = is_;
  os = os_;
}

void CConnection::setFramebuffer(ModifiablePixelBuffer* fb)
{
  decoder.flush();

  if (fb) {
    assert(fb->width() == server.width());
    assert(fb->height() == server.height());
  }

  if ((framebuffer != NULL) && (fb != NULL)) {
    Rect rect;

    const rdr::U8* data;
    int stride;

    const rdr::U8 black[4] = { 0, 0, 0, 0 };

    // Copy still valid area

    rect.setXYWH(0, 0,
                 __rfbmin(fb->width(), framebuffer->width()),
                 __rfbmin(fb->height(), framebuffer->height()));
    data = framebuffer->getBuffer(framebuffer->getRect(), &stride);
    fb->imageRect(rect, data, stride);

    // Black out any new areas

    if (fb->width() > framebuffer->width()) {
      rect.setXYWH(framebuffer->width(), 0,
                   fb->width() - framebuffer->width(),
                   fb->height());
      fb->fillRect(rect, black);
    }

    if (fb->height() > framebuffer->height()) {
      rect.setXYWH(0, framebuffer->height(),
                   fb->width(),
                   fb->height() - framebuffer->height());
      fb->fillRect(rect, black);
    }
  }

  delete framebuffer;
  framebuffer = fb;
}

void CConnection::initialiseProtocol()
{
  state_ = RFBSTATE_PROTOCOL_VERSION;
}

bool CConnection::processMsg()
{
  switch (state_) {

  case RFBSTATE_PROTOCOL_VERSION: return processVersionMsg();        break;
  case RFBSTATE_SECURITY_TYPES:   return processSecurityTypesMsg();  break;
  case RFBSTATE_SECURITY:         return processSecurityMsg();       break;
  case RFBSTATE_SECURITY_RESULT:  return processSecurityResultMsg(); break;
  case RFBSTATE_SECURITY_REASON:  return processSecurityReasonMsg(); break;
  case RFBSTATE_INITIALISATION:   return processInitMsg();           break;
  case RFBSTATE_NORMAL:           return reader_->readMsg();         break;
  case RFBSTATE_CLOSING:
    throw Exception("CConnection::processMsg: called while closing");
  case RFBSTATE_UNINITIALISED:
    throw Exception("CConnection::processMsg: not initialised yet?");
  default:
    throw Exception("CConnection::processMsg: invalid state");
  }
}

bool CConnection::processVersionMsg()
{
  char verStr[27]; // FIXME: gcc has some bug in format-overflow
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
    throw Exception("reading version failed: not an RFB server?");
  }

  server.setVersion(majorVersion, minorVersion);

  vlog.info("Server supports RFB protocol version %d.%d",
            server.majorVersion, server.minorVersion);

  // The only official RFB protocol versions are currently 3.3, 3.7 and 3.8
  if (server.beforeVersion(3,3)) {
    vlog.error("Server gave unsupported RFB protocol version %d.%d",
               server.majorVersion, server.minorVersion);
    state_ = RFBSTATE_INVALID;
    throw Exception("Server gave unsupported RFB protocol version %d.%d",
                    server.majorVersion, server.minorVersion);
  } else if (server.beforeVersion(3,7)) {
    server.setVersion(3,3);
  } else if (server.afterVersion(3,8)) {
    server.setVersion(3,8);
  }

  sprintf(verStr, "RFB %03d.%03d\n",
          server.majorVersion, server.minorVersion);
  os->writeBytes(verStr, 12);
  os->flush();

  state_ = RFBSTATE_SECURITY_TYPES;

  vlog.info("Using RFB protocol version %d.%d",
            server.majorVersion, server.minorVersion);

  return true;
}


bool CConnection::processSecurityTypesMsg()
{
  vlog.debug("processing security types message");

  int secType = secTypeInvalid;

  std::list<rdr::U8> secTypes;
  secTypes = security.GetEnabledSecTypes();

  if (server.isVersion(3,3)) {

    // legacy 3.3 server may only offer "vnc authentication" or "none"

    if (!is->hasData(4))
      return false;

    secType = is->readU32();
    if (secType == secTypeInvalid) {
      state_ = RFBSTATE_SECURITY_REASON;
      return true;
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

    if (!is->hasData(1))
      return false;

    is->setRestorePoint();

    int nServerSecTypes = is->readU8();

    if (!is->hasDataOrRestore(nServerSecTypes))
      return false;
    is->clearRestorePoint();

    if (nServerSecTypes == 0) {
      state_ = RFBSTATE_SECURITY_REASON;
      return true;
    }

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
      vlog.info("Choosing security type %s(%d)",secTypeName(secType),secType);
    }
  }

  if (secType == secTypeInvalid) {
    state_ = RFBSTATE_INVALID;
    vlog.error("No matching security types");
    throw Exception("No matching security types");
  }

  state_ = RFBSTATE_SECURITY;
  csecurity = security.GetCSecurity(this, secType);

  return true;
}

bool CConnection::processSecurityMsg()
{
  vlog.debug("processing security message");
  if (!csecurity->processMsg())
    return false;

  state_ = RFBSTATE_SECURITY_RESULT;

  return true;
}

bool CConnection::processSecurityResultMsg()
{
  vlog.debug("processing security result message");
  int result;

  if (server.beforeVersion(3,8) && csecurity->getType() == secTypeNone) {
    result = secResultOK;
  } else {
    if (!is->hasData(4))
      return false;
    result = is->readU32();
  }

  switch (result) {
  case secResultOK:
    securityCompleted();
    return true;
  case secResultFailed:
    vlog.debug("auth failed");
    break;
  case secResultTooMany:
    vlog.debug("auth failed - too many tries");
    break;
  default:
    throw Exception("Unknown security result from server");
  }

  if (server.beforeVersion(3,8)) {
    state_ = RFBSTATE_INVALID;
    throw AuthFailureException();
  }

  state_ = RFBSTATE_SECURITY_REASON;
  return true;
}

bool CConnection::processSecurityReasonMsg()
{
  vlog.debug("processing security reason message");

  if (!is->hasData(4))
    return false;

  is->setRestorePoint();

  rdr::U32 len = is->readU32();
  if (!is->hasDataOrRestore(len))
    return false;
  is->clearRestorePoint();

  CharArray reason(len + 1);
  is->readBytes(reason.buf, len);
  reason.buf[len] = '\0';

  state_ = RFBSTATE_INVALID;
  throw AuthFailureException(reason.buf);
}

bool CConnection::processInitMsg()
{
  vlog.debug("reading server initialisation");
  return reader_->readServerInit();
}

void CConnection::securityCompleted()
{
  state_ = RFBSTATE_INITIALISATION;
  reader_ = new CMsgReader(this, is);
  writer_ = new CMsgWriter(&server, os);
  vlog.debug("Authentication success!");
  authSuccess();
  writer_->writeClientInit(shared);
}

void CConnection::close()
{
  state_ = RFBSTATE_CLOSING;

  /*
   * We're already shutting down, so just log any pending decoder
   * problems
   */
  try {
    decoder.flush();
  } catch (rdr::Exception& e) {
    vlog.error("%s", e.str());
  }

  setFramebuffer(NULL);
  delete csecurity;
  csecurity = NULL;
  delete reader_;
  reader_ = NULL;
  delete writer_;
  writer_ = NULL;
  strFree(serverClipboard);
  serverClipboard = NULL;
}

void CConnection::setDesktopSize(int w, int h)
{
  decoder.flush();

  CMsgHandler::setDesktopSize(w,h);

  if (continuousUpdates)
    writer()->writeEnableContinuousUpdates(true, 0, 0,
                                           server.width(),
                                           server.height());

  resizeFramebuffer();
  assert(framebuffer != NULL);
  assert(framebuffer->width() == server.width());
  assert(framebuffer->height() == server.height());
}

void CConnection::setExtendedDesktopSize(unsigned reason,
                                         unsigned result,
                                         int w, int h,
                                         const ScreenSet& layout)
{
  decoder.flush();

  CMsgHandler::setExtendedDesktopSize(reason, result, w, h, layout);

  if (continuousUpdates)
    writer()->writeEnableContinuousUpdates(true, 0, 0,
                                           server.width(),
                                           server.height());

  resizeFramebuffer();
  assert(framebuffer != NULL);
  assert(framebuffer->width() == server.width());
  assert(framebuffer->height() == server.height());
}

void CConnection::endOfContinuousUpdates()
{
  CMsgHandler::endOfContinuousUpdates();

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

void CConnection::serverInit(int width, int height,
                             const PixelFormat& pf,
                             const char* name)
{
  CMsgHandler::serverInit(width, height, pf, name);

  state_ = RFBSTATE_NORMAL;
  vlog.debug("initialisation done");

  initDone();
  assert(framebuffer != NULL);
  assert(framebuffer->width() == server.width());
  assert(framebuffer->height() == server.height());

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

bool CConnection::readAndDecodeRect(const Rect& r, int encoding,
                                    ModifiablePixelBuffer* pb)
{
  if (!decoder.decodeRect(r, encoding, pb))
    return false;
  decoder.flush();
  return true;
}

void CConnection::framebufferUpdateStart()
{
  CMsgHandler::framebufferUpdateStart();

  assert(framebuffer != NULL);

  // Note: This might not be true if continuous updates are supported
  pendingUpdate = false;

  requestNewUpdate();
}

void CConnection::framebufferUpdateEnd()
{
  decoder.flush();

  CMsgHandler::framebufferUpdateEnd();

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
      writer()->writeEnableContinuousUpdates(true, 0, 0,
                                             server.width(),
                                             server.height());
    }

    firstUpdate = false;
  }
}

bool CConnection::dataRect(const Rect& r, int encoding)
{
  return decoder.decodeRect(r, encoding, framebuffer);
}

void CConnection::serverCutText(const char* str)
{
  hasLocalClipboard = false;

  strFree(serverClipboard);
  serverClipboard = NULL;

  serverClipboard = latin1ToUTF8(str);

  handleClipboardAnnounce(true);
}

void CConnection::handleClipboardCaps(rdr::U32 flags,
                                      const rdr::U32* lengths)
{
  rdr::U32 sizes[] = { 0 };

  CMsgHandler::handleClipboardCaps(flags, lengths);

  writer()->writeClipboardCaps(rfb::clipboardUTF8 |
                               rfb::clipboardRequest |
                               rfb::clipboardPeek |
                               rfb::clipboardNotify |
                               rfb::clipboardProvide,
                               sizes);
}

void CConnection::handleClipboardRequest(rdr::U32 flags)
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

void CConnection::handleClipboardPeek(rdr::U32 flags)
{
  if (server.clipboardFlags() & rfb::clipboardNotify)
    writer()->writeClipboardNotify(hasLocalClipboard ? rfb::clipboardUTF8 : 0);
}

void CConnection::handleClipboardNotify(rdr::U32 flags)
{
  strFree(serverClipboard);
  serverClipboard = NULL;

  if (flags & rfb::clipboardUTF8) {
    hasLocalClipboard = false;
    handleClipboardAnnounce(true);
  } else {
    handleClipboardAnnounce(false);
  }
}

void CConnection::handleClipboardProvide(rdr::U32 flags,
                                         const size_t* lengths,
                                         const rdr::U8* const* data)
{
  if (!(flags & rfb::clipboardUTF8)) {
    vlog.debug("Ignoring clipboard provide with unsupported formats 0x%x", flags);
    return;
  }

  strFree(serverClipboard);
  serverClipboard = NULL;

  serverClipboard = convertLF((const char*)data[0], lengths[0]);

  // FIXME: Should probably verify that this data was actually requested
  handleClipboardData(serverClipboard);
}

void CConnection::authSuccess()
{
}

void CConnection::initDone()
{
}

void CConnection::resizeFramebuffer()
{
  assert(false);
}

void CConnection::handleClipboardRequest()
{
}

void CConnection::handleClipboardAnnounce(bool available)
{
}

void CConnection::handleClipboardData(const char* data)
{
}

void CConnection::requestClipboard()
{
  if (serverClipboard != NULL) {
    handleClipboardData(serverClipboard);
    return;
  }

  if (server.clipboardFlags() & rfb::clipboardRequest)
    writer()->writeClipboardRequest(rfb::clipboardUTF8);
}

void CConnection::announceClipboard(bool available)
{
  hasLocalClipboard = available;
  unsolicitedClipboardAttempt = false;

  // Attempt an unsolicited transfer?
  if (available &&
      (server.clipboardSize(rfb::clipboardUTF8) > 0) &&
      (server.clipboardFlags() & rfb::clipboardProvide)) {
    vlog.debug("Attempting unsolicited clipboard transfer...");
    unsolicitedClipboardAttempt = true;
    handleClipboardRequest();
    return;
  }

  if (server.clipboardFlags() & rfb::clipboardNotify) {
    writer()->writeClipboardNotify(available ? rfb::clipboardUTF8 : 0);
    return;
  }

  if (available)
    handleClipboardRequest();
}

void CConnection::sendClipboardData(const char* data)
{
  if (server.clipboardFlags() & rfb::clipboardProvide) {
    CharArray filtered(convertCRLF(data));
    size_t sizes[1] = { strlen(filtered.buf) + 1 };
    const rdr::U8* data[1] = { (const rdr::U8*)filtered.buf };

    if (unsolicitedClipboardAttempt) {
      unsolicitedClipboardAttempt = false;
      if (sizes[0] > server.clipboardSize(rfb::clipboardUTF8)) {
        vlog.debug("Clipboard was too large for unsolicited clipboard transfer");
        if (server.clipboardFlags() & rfb::clipboardNotify)
          writer()->writeClipboardNotify(rfb::clipboardUTF8);
        return;
      }
    }

    writer()->writeClipboardProvide(rfb::clipboardUTF8, sizes, data);
  } else {
    CharArray latin1(utf8ToLatin1(data));

    writer()->writeClientCutText(latin1.buf);
  }
}

void CConnection::refreshFramebuffer()
{
  forceNonincremental = true;

  // Without continuous updates we have to make sure we only have a
  // single update in flight, so we'll have to wait to do the refresh
  if (continuousUpdates)
    requestNewUpdate();
}

void CConnection::setPreferredEncoding(int encoding)
{
  if (preferredEncoding == encoding)
    return;

  preferredEncoding = encoding;
  encodingChange = true;
}

int CConnection::getPreferredEncoding()
{
  return preferredEncoding;
}

void CConnection::setCompressLevel(int level)
{
  if (compressLevel == level)
    return;

  compressLevel = level;
  encodingChange = true;
}

void CConnection::setQualityLevel(int level)
{
  if (qualityLevel == level)
    return;

  qualityLevel = level;
  encodingChange = true;
}

void CConnection::setPF(const PixelFormat& pf)
{
  if (server.pf().equal(pf) && !formatChange)
    return;

  nextPF = pf;
  formatChange = true;
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

// requestNewUpdate() requests an update from the server, having set the
// format and encoding appropriately.
void CConnection::requestNewUpdate()
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
      writer()->writeEnableContinuousUpdates(false, 0, 0, 0, 0);

    writer()->writeSetPixelFormat(pendingPF);

    if (continuousUpdates)
      writer()->writeEnableContinuousUpdates(true, 0, 0,
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
    writer()->writeFramebufferUpdateRequest(Rect(0, 0,
                                                 server.width(),
                                                 server.height()),
                                            !forceNonincremental);
  }

  forceNonincremental = false;
}

// Ask for encodings based on which decoders are supported.  Assumes higher
// encoding numbers are more desirable.

void CConnection::updateEncodings()
{
  std::list<rdr::U32> encodings;

  if (supportsLocalCursor) {
    encodings.push_back(pseudoEncodingCursorWithAlpha);
    encodings.push_back(pseudoEncodingVMwareCursor);
    encodings.push_back(pseudoEncodingCursor);
    encodings.push_back(pseudoEncodingXCursor);
  }
  if (supportsCursorPosition) {
    encodings.push_back(pseudoEncodingVMwareCursorPosition);
  }
  if (supportsDesktopResize) {
    encodings.push_back(pseudoEncodingDesktopSize);
    encodings.push_back(pseudoEncodingExtendedDesktopSize);
  }
  if (supportsLEDState) {
    encodings.push_back(pseudoEncodingLEDState);
    encodings.push_back(pseudoEncodingVMwareLEDState);
  }

  encodings.push_back(pseudoEncodingDesktopName);
  encodings.push_back(pseudoEncodingLastRect);
  encodings.push_back(pseudoEncodingExtendedClipboard);
  encodings.push_back(pseudoEncodingContinuousUpdates);
  encodings.push_back(pseudoEncodingFence);
  encodings.push_back(pseudoEncodingQEMUKeyEvent);

  if (Decoder::supported(preferredEncoding)) {
    encodings.push_back(preferredEncoding);
  }

  encodings.push_back(encodingCopyRect);

  for (int i = encodingMax; i >= 0; i--) {
    if ((i != preferredEncoding) && Decoder::supported(i))
      encodings.push_back(i);
  }

  if (compressLevel >= 0 && compressLevel <= 9)
      encodings.push_back(pseudoEncodingCompressLevel0 + compressLevel);
  if (qualityLevel >= 0 && qualityLevel <= 9)
      encodings.push_back(pseudoEncodingQualityLevel0 + qualityLevel);

  writer()->writeSetEncodings(encodings);
}
