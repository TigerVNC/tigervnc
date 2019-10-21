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

    void readClientInit();

    // readMsg() reads a message, calling the handler as appropriate.
    void readMsg();

    rdr::InStream* getInStream() { return is; }

  protected:
    void readSetPixelFormat();
    void readSetEncodings();
    void readSetDesktopSize();

    void readFramebufferUpdateRequest();
    void readEnableContinuousUpdates();

    void readFence();

    void readKeyEvent();
    void readPointerEvent();
    void readClientCutText();
    void readExtendedClipboard(rdr::S32 len);

    void readQEMUMessage();
    void readQEMUKeyEvent();

    SMsgHandler* handler;
    rdr::InStream* is;
  };
}
#endif
