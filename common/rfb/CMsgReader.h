/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2014 Pierre Ossman for Cendio AB
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

    void readServerInit();

    // readMsg() reads a message, calling the handler as appropriate.
    void readMsg();

    rdr::InStream* getInStream() { return is; }

    int imageBufIdealSize;

  protected:
    void readSetColourMapEntries();
    void readBell();
    void readServerCutText();
    void readFence();
    void readEndOfContinuousUpdates();

    void readFramebufferUpdate();

    void readRect(const Rect& r, int encoding);

    void readSetCursor(int width, int height, const Point& hotspot);
    void readSetDesktopName(int x, int y, int w, int h);
    void readExtendedDesktopSize(int x, int y, int w, int h);

    CMsgHandler* handler;
    rdr::InStream* is;
    int nUpdateRectsLeft;
  };
}
#endif
