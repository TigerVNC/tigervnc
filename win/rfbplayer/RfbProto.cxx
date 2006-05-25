/* Copyright (C) 2004 TightVNC Team.  All Rights Reserved.
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

// -=- RFB Protocol

#include <rfb/Exception.h>
#include <rfb/LogWriter.h>

#include <rfbplayer/RfbProto.h>

using namespace rfb;

static LogWriter vlog("RfbProto");

//
// Constructor
//

RfbProto::RfbProto(char *fileName) {
  is = NULL;
  reader = NULL;
  newSession(fileName);
}

//
// Destructor
//

RfbProto::~RfbProto() {
  if (is) delete is;
  is = 0;
  if (reader) delete reader;
  reader = 0;
}

void RfbProto::newSession(char *fileName) {
  // Close the previous session
  if (is) {
    delete is;
    is = NULL;
    delete reader;
    reader = NULL;
  }

  // Begin the new session
  if (fileName != NULL) {
    is = new FbsInputStream(fileName);
    reader = new CMsgReaderV3(this, is);
    initialiseProtocol();
  }
}

void RfbProto::initialiseProtocol() {
  state_ = RFBSTATE_PROTOCOL_VERSION;
}

void RfbProto::processMsg()
{
  switch (state_) {

  case RFBSTATE_PROTOCOL_VERSION: processVersionMsg();       break;
  case RFBSTATE_SECURITY:         processSecurityMsg();      break;
  case RFBSTATE_INITIALISATION:   processInitMsg();          break;
  case RFBSTATE_NORMAL:           reader->readMsg();         break;
  default:
    throw rfb::Exception("RfbProto::processMsg: invalid state");
  }
}

void RfbProto::processVersionMsg()
{
  vlog.debug("reading protocol version");
  memset(&cp, 0, sizeof(cp));
  bool done;
  if (!cp.readVersion(is, &done)) {
    state_ = RFBSTATE_INVALID;
    throw rfb::Exception("reading version failed: wrong file format?");
  }
  if (!done) return;

  // The only official RFB protocol versions are currently 3.3, 3.7 and 3.8
  if (!cp.isVersion(3,3) && !cp.isVersion(3,7) && !cp.isVersion(3,8)) {
    char msg[256];
    sprintf(msg,"File have unsupported RFB protocol version %d.%d",
            cp.majorVersion, cp.minorVersion);
    state_ = RFBSTATE_INVALID;
    throw rfb::Exception(msg);
  }

  state_ = RFBSTATE_SECURITY;

  vlog.info("Using RFB protocol version %d.%d",
            cp.majorVersion, cp.minorVersion);
}

void RfbProto::processSecurityMsg()
{
  vlog.debug("processing security types message");

  int secType = secTypeInvalid;

  // legacy 3.3 server may only offer "vnc authentication" or "none"
  secType = is->readU32();
  if (secType == secTypeInvalid) {
    int reasonLen = is->readU32();
    char *reason = new char[reasonLen];
    is->readBytes(reason, reasonLen);
    throw rfb::Exception(reason); 
  }

  if (secType != secTypeNone) {
    throw rfb::Exception("Wrong authentication type in the session file");
  }

  state_ = RFBSTATE_INITIALISATION;
}

void RfbProto::processInitMsg() {
  vlog.debug("reading server initialisation");
  reader->readServerInit(); 
}

void RfbProto::serverInit()
{
  state_ = RFBSTATE_NORMAL;
  vlog.debug("initialisation done");
}
