/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2019 Pierre Ossman for Cendio AB
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
//
// SMsgReader - class for reading RFB messages on the server side
// (i.e. messages from client to server).
//

#ifndef __RFB_SMSGREADER_H__
#define __RFB_SMSGREADER_H__

namespace rdr { class InStream; }

namespace rfb {
  class SMsgHandler;

  class SMsgReader {
  public:
    SMsgReader(SMsgHandler* handler, rdr::InStream* is);
    virtual ~SMsgReader();

    bool readClientInit();

    // readMsg() reads a message, calling the handler as appropriate.
    bool readMsg();

    rdr::InStream* getInStream() { return is; }

  protected:
    bool readSetPixelFormat();
    bool readSetEncodings();
    bool readSetDesktopSize();

    bool readFramebufferUpdateRequest();
    bool readEnableContinuousUpdates();

    bool readFence();

    bool readKeyEvent();
    bool readPointerEvent();
    bool readClientCutText();
    bool readExtendedClipboard(rdr::S32 len);

    bool readQEMUMessage();
    bool readQEMUKeyEvent();

  private:
    SMsgHandler* handler;
    rdr::InStream* is;

    enum stateEnum {
      MSGSTATE_IDLE,
      MSGSTATE_MESSAGE,
    };

    stateEnum state;

    rdr::U8 currentMsgType;
  };
}
#endif
