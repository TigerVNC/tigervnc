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
#include <rfb/Security.h>
#include <rfb/msgTypes.h>
#include <rfb/CapsList.h>
#include <rfb/SMsgReaderV3.h>
#include <rfb/SMsgWriterV3.h>
#include <rfb/SConnection.h>
#include <rfb/ServerCore.h>

#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("SConnection");

// AccessRights values
const SConnection::AccessRights SConnection::AccessView       = 0x0001;
const SConnection::AccessRights SConnection::AccessKeyEvents  = 0x0002;
const SConnection::AccessRights SConnection::AccessPtrEvents  = 0x0004;
const SConnection::AccessRights SConnection::AccessCutText    = 0x0008;
const SConnection::AccessRights SConnection::AccessDefault    = 0x03ff;
const SConnection::AccessRights SConnection::AccessNoQuery    = 0x0400;
const SConnection::AccessRights SConnection::AccessFull       = 0xffff;


SConnection::SConnection(SSecurityFactory* secFact, bool reverseConnection_)
  : readyForSetColourMapEntries(false),
    is(0), os(0), reader_(0), writer_(0),
    security(0), securityFactory(secFact), state_(RFBSTATE_UNINITIALISED),
    reverseConnection(reverseConnection_)
{
  defaultMajorVersion = 3;
  defaultMinorVersion = 8;
  if (rfb::Server::protocol3_3)
    defaultMinorVersion = 3;

  cp.setVersion(defaultMajorVersion, defaultMinorVersion);
}

SConnection::~SConnection()
{
  if (security) security->destroy();
  deleteReaderAndWriter();
}

void SConnection::deleteReaderAndWriter()
{
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
  cp.writeVersion(os);
  state_ = RFBSTATE_PROTOCOL_VERSION;
}

void SConnection::processMsg()
{
  switch (state_) {
  case RFBSTATE_PROTOCOL_VERSION: processVersionMsg();      break;
  case RFBSTATE_SECURITY_TYPE:    processSecurityTypeMsg(); break;
  case RFBSTATE_TIGHT_TUNN_TYPE:  processTunnelTypeMsg();   break;
  case RFBSTATE_TIGHT_AUTH_TYPE:  processAuthTypeMsg();     break;
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
  vlog.debug("reading protocol version");
  bool done;
  if (!cp.readVersion(is, &done)) {
    state_ = RFBSTATE_INVALID;
    throw Exception("reading version failed: not an RFB client?");
  }
  if (!done) return;

  vlog.info("Client needs protocol version %d.%d",
            cp.majorVersion, cp.minorVersion);

  if (cp.majorVersion != 3) {
    // unknown protocol version
    char msg[256];
    sprintf(msg,"Error: client needs protocol version %d.%d, server has %d.%d",
            cp.majorVersion, cp.minorVersion,
            defaultMajorVersion, defaultMinorVersion);
    throwConnFailedException(msg);
  }

  if (cp.minorVersion != 3 && cp.minorVersion != 7 && cp.minorVersion != 8) {
    vlog.error("Client uses unofficial protocol version %d.%d",
               cp.majorVersion,cp.minorVersion);
    if (cp.minorVersion >= 8)
      cp.minorVersion = 8;
    else if (cp.minorVersion == 7)
      cp.minorVersion = 7;
    else
      cp.minorVersion = 3;
    vlog.error("Assuming compatibility with version %d.%d",
               cp.majorVersion,cp.minorVersion);
  }

  versionReceived();

  std::list<rdr::U8> secTypes;
  std::list<rdr::U8>::iterator i;
  securityFactory->getSecTypes(&secTypes, reverseConnection);

  if (cp.isVersion(3,3)) {

    // cope with legacy 3.3 client only if "no authentication" or "vnc
    // authentication" is supported.
    for (i=secTypes.begin(); i!=secTypes.end(); i++) {
      if (*i == secTypeNone || *i == secTypeVncAuth) break;
    }
    if (i == secTypes.end()) {
      char msg[256];
      sprintf(msg,"No supported security type for %d.%d client",
              cp.majorVersion, cp.minorVersion);
      throwConnFailedException(msg);
    }

    os->writeU32(*i);
    if (*i == secTypeNone) os->flush();
    state_ = RFBSTATE_SECURITY;
    security = securityFactory->getSSecurity(*i, reverseConnection);
    processSecurityMsg();
    return;
  }

  // Add a special security type to advertise TightVNC protocol extensions.
  secTypes.push_back(secTypeTight);

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

  if (secType == secTypeTight) {
    vlog.info("Enabling TightVNC protocol extensions");
    cp.tightExtensionsEnabled = true;
    offerTunneling();
  } else {
    processSecurityType(secType);
  }
}

//
// TightVNC-specific protocol initialization (tunneling, authentication)
//

void SConnection::offerTunneling()
{
  vlog.debug("offering list of tunneling methods");
  int nTypes = 0;

  // Advertise our tunneling capabilities (currently, nothing to advertise).
  os->writeU32(nTypes);
  os->flush();

  if (nTypes) {
    // NOTE: Never executed in current version.
    state_ = RFBSTATE_TIGHT_TUNN_TYPE;
  } else {
    offerAuthentication();
  }
}

// NOTE: This function is never called in current version.
void SConnection::processTunnelTypeMsg()
{
  vlog.debug("processing tunneling type message (TightVNC extension)");
  int tunnelType = is->readU32();
  vlog.error("unsupported tunneling type %d requested, ignoring", tunnelType);
  offerAuthentication();
}

void SConnection::offerAuthentication()
{
  vlog.debug("offering list of authentication methods");

  // See processVersionMsg(), the code below is similar.

  std::list<rdr::U8> secTypes;
  std::list<rdr::U8>::iterator i;

  // NOTE: In addition to standard security types, we might want to offer
  //       TightVNC-specific authentication types. But currently we support
  //       only the standard security types: secTypeNone and secTypeVncAuth.
  securityFactory->getSecTypes(&secTypes, reverseConnection);

  CapsList caps;
  for (i = secTypes.begin(); i != secTypes.end(); i++) {
    // FIXME: Capability info should be provided by SSecurity objects.
    switch (*i) {
    case secTypeNone:     caps.addStandard(*i, "NOAUTH__"); break;
    case secTypeVncAuth:  caps.addStandard(*i, "VNCAUTH_"); break;
    default:
      // This should not ever happen.
      vlog.error("not offering unknown security type %d", (int)*i);
    }
  }

  if (caps.getSize() < 1)
    throwConnFailedException("No supported security types");

  if (caps.includesOnly(secTypeNone)) {
    // Special case - if caps includes nothing else than secTypeNone, we send
    // an empty capability list and do not expect security type selection from
    // the client. Then, continue the protocol like if the client has selected
    // secTypeNone (starting at base protocol version 3.8, "security result"
    // will follow).
    os->writeU32(0);
    os->flush();
    processSecurityType(secTypeNone);
  } else {
    // Normal case - sending the list of authentication capabilities.
    os->writeU32(caps.getSize());
    caps.write(os);
    os->flush();
    state_ = RFBSTATE_TIGHT_AUTH_TYPE;
  }
}

void SConnection::processAuthTypeMsg()
{
  vlog.debug("processing authentication type message (TightVNC extension)");

  // NOTE: Currently, we support only the standard security types, so we
  //       just pass TightVNC authentication type for standard processing,
  //       just as it was usual RFB security type.
  int secType = is->readU32();
  processSecurityType(secType);
}

//
// End of TightVNC-specific code
//

void SConnection::processSecurityType(int secType)
{
  // Verify that the requested security type should be offered
  std::list<rdr::U8> secTypes;
  std::list<rdr::U8>::iterator i;
  securityFactory->getSecTypes(&secTypes, reverseConnection);
  for (i=secTypes.begin(); i!=secTypes.end(); i++)
    if (*i == secType) break;
  if (i == secTypes.end())
    throw Exception("Requested security type not available");

  vlog.info("Client requests security type %s(%d)",
            secTypeName(secType),secType);

  try {
    state_ = RFBSTATE_SECURITY;
    security = securityFactory->getSSecurity(secType, reverseConnection);
  } catch (rdr::Exception& e) {
    throwConnFailedException(e.str());
  }

  processSecurityMsg();
}

void SConnection::processSecurityMsg()
{
  vlog.debug("processing security message");
  try {
    bool done = security->processMsg(this);
    if (done) {
      state_ = RFBSTATE_QUERYING;
      queryConnection(security->getUserName());
    }
  } catch (AuthFailureException& e) {
    vlog.error("AuthFailureException: %s", e.str());
    os->writeU32(secResultFailed);
    if (!cp.beforeVersion(3,8)) // 3.8 onwards have failure message
      os->writeString(e.str());
    os->flush();
    throw;
  }
}

void SConnection::processInitMsg()
{
  vlog.debug("reading client initialisation");
  reader_->readClientInit();
}

void SConnection::throwConnFailedException(const char* msg)
{
  vlog.info(msg);
  if (state_ == RFBSTATE_PROTOCOL_VERSION) {
    if (cp.majorVersion == 3 && cp.minorVersion == 3) {
      os->writeU32(0);
      os->writeString(msg);
      os->flush();
    } else {
      os->writeU8(0);
      os->writeString(msg);
      os->flush();
    }
  }
  state_ = RFBSTATE_INVALID;
  throw ConnFailedException(msg);
}

void SConnection::writeConnFailedFromScratch(const char* msg,
                                             rdr::OutStream* os)
{
  os->writeBytes("RFB 003.003\n", 12);
  os->writeU32(0);
  os->writeString(msg);
  os->flush();
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

  if (!reason) reason = "Authentication failure";

  if (!cp.beforeVersion(3,8) || security->getType() != secTypeNone) {
    if (accept) {
      os->writeU32(secResultOK);
    } else {
      os->writeU32(secResultFailed);
      if (!cp.beforeVersion(3,8)) // 3.8 onwards have failure message
        os->writeString(reason);
    }
    os->flush();
  }

  if (accept) {
    state_ = RFBSTATE_INITIALISATION;
    reader_ = new SMsgReaderV3(this, is);
    writer_ = new SMsgWriterV3(&cp, os);
    authSuccess();
  } else {
    state_ = RFBSTATE_INVALID;
    throw AuthFailureException(reason);
  }
}

void SConnection::setInitialColourMap()
{
}

void SConnection::clientInit(bool shared)
{
  writer_->writeServerInit();

  // FIXME: Send interaction capabilities via writer_?
  if (cp.tightExtensionsEnabled)
    sendInteractionCaps();

  state_ = RFBSTATE_NORMAL;
}

// FIXME: Move sendInteractionCaps() to a class derived from SMsgWriterV3?
void SConnection::sendInteractionCaps()
{
  //
  // Advertise support for non-standard server-to-client messages
  //

  CapsList scaps;

  //
  // Advertise support for non-standard client-to-server messages
  //

  CapsList ccaps;

  //
  // Advertise all supported encoding types (except raw encoding).
  //

  CapsList ecaps;

  // First, add true encodings.
  for (int i = 1; i <= encodingMax; i++) {
    if (Encoder::supported(i)) {
      // FIXME: Capability info should be provided by Encoder objects.
      switch (i) {
      case encodingRRE:      ecaps.addStandard(i, "RRE_____"); break;
      case encodingCoRRE:    ecaps.addStandard(i, "CORRE___"); break;
      case encodingHextile:  ecaps.addStandard(i, "HEXTILE_"); break;
      case encodingZRLE:     ecaps.addStandard(i, "ZRLE____"); break;
      case encodingTight:    ecaps.addTightExt(i, "TIGHT___"); break;
      default:
        // This should not ever happen.
        vlog.error("not advertising unknown encoding type %d", (int)i);
      }
    }
  }

  // CopyRect is special - Encoder::supported() returns 0 for it,
  // that's why we add it here explicitly.
  ecaps.addStandard(encodingCopyRect,             "COPYRECT");

  // Add supported pseudo encodings as well.
  ecaps.addTightExt(pseudoEncodingCompressLevel0, "COMPRLVL");
  ecaps.addTightExt(pseudoEncodingQualityLevel0,  "JPEGQLVL");
  ecaps.addTightExt(pseudoEncodingXCursor,        "X11CURSR");
  ecaps.addTightExt(pseudoEncodingCursor,         "RCHCURSR");
  ecaps.addTightExt(pseudoEncodingLastRect,       "LASTRECT");
  ecaps.addStandard(pseudoEncodingDesktopSize,    "NEWFBSIZ");

  os->writeU16(scaps.getSize());
  os->writeU16(ccaps.getSize());
  os->writeU16(ecaps.getSize());
  os->writeU16(0);
  if (scaps.getSize())
    scaps.write(os);
  if (ccaps.getSize())
    ccaps.write(os);
  if (ecaps.getSize())
    ecaps.write(os);
  os->flush();
}

void SConnection::setPixelFormat(const PixelFormat& pf)
{
  SMsgHandler::setPixelFormat(pf);
  readyForSetColourMapEntries = true;
}

void SConnection::framebufferUpdateRequest(const Rect& r, bool incremental)
{
  if (!readyForSetColourMapEntries) {
    readyForSetColourMapEntries = true;
    if (!cp.pf().trueColour) {
      setInitialColourMap();
    }
  }
}
