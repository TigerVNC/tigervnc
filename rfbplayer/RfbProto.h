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

// -=- RfbProto.h

#include <rfb/CMsgReaderV3.h>
#include <rfb/CMsgHandler.h>
#include <rfb/CSecurity.h>
#include <rfb/secTypes.h>

#include <rfbplayer/FbsInputStream.h>

using namespace rfb;

class RfbProto : public CMsgHandler {
  public:

    RfbProto(char *fileName);
    ~RfbProto();

    void newSession(char *filename);
    void initialiseProtocol();
    void interruptFrameDelay() { is->interruptFrameDelay(); };
    const rdr::InStream* getInStream() { return is; }

    virtual void processMsg();

    // serverInit() is called when the ServerInit message is received.  The
    // derived class must call on to CMsgHandler::serverInit().
    void serverInit();

    enum stateEnum {
      RFBSTATE_PROTOCOL_VERSION,
      RFBSTATE_SECURITY,
      RFBSTATE_INITIALISATION,
      RFBSTATE_NORMAL,
      RFBSTATE_INVALID
    };

    stateEnum state() { return state_; }

  protected:
    void setState(stateEnum s) { state_ = s; }
    virtual void framebufferUpdateEnd() {};

    FbsInputStream* is;
    CMsgReaderV3* reader;
    stateEnum state_;

  private:
    void processVersionMsg();
    void processSecurityMsg();
    void processInitMsg();
};
