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
// CMsgReader - class for reading RFB messages on the server side
// (i.e. messages from client to server).
//

#ifndef __RFB_CMSGREADER_H__
#define __RFB_CMSGREADER_H__

#include <rdr/types.h>

#include <rfb/Rect.h>
#include <rfb/encodings.h>

namespace rdr { class InStream; }

namespace rfb {
  class CMsgHandler;
  struct Rect;

  class CMsgReader {
  public:
    CMsgReader(CMsgHandler* handler, rdr::InStream* is);
    virtual ~CMsgReader();

    bool readServerInit();

    // readMsg() reads a message, calling the handler as appropriate.
    bool readMsg();

    rdr::InStream* getInStream() { return is; }

    int imageBufIdealSize;

  protected:
    bool readSetColourMapEntries();
    bool readBell();
    bool readServerCutText();
    bool readExtendedClipboard(rdr::S32 len);
    bool readFence();
    bool readEndOfContinuousUpdates();

    bool readFramebufferUpdate();

    bool readRect(const Rect& r, int encoding);

    bool readSetXCursor(int width, int height, const Point& hotspot);
    bool readSetCursor(int width, int height, const Point& hotspot);
    bool readSetCursorWithAlpha(int width, int height, const Point& hotspot);
    bool readSetVMwareCursor(int width, int height, const Point& hotspot);
    bool readSetDesktopName(int x, int y, int w, int h);
    bool readExtendedDesktopSize(int x, int y, int w, int h);
    bool readLEDState();
    bool readVMwareLEDState();

  private:
    CMsgHandler* handler;
    rdr::InStream* is;

    enum stateEnum {
      MSGSTATE_IDLE,
      MSGSTATE_MESSAGE,
      MSGSTATE_RECT_HEADER,
      MSGSTATE_RECT_DATA,
    };

    stateEnum state;

    rdr::U8 currentMsgType;
    int nUpdateRectsLeft;
    Rect dataRect;
    int rectEncoding;

    int cursorEncoding;

    static const int maxCursorSize = 256;
  };
}
#endif
