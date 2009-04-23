/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009 Pierre Ossman for Cendio AB
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
#ifndef __RFB_SMSGWRITERV3_H__
#define __RFB_SMSGWRITERV3_H__

#include <list>
#include <utility>

#include <rfb/SMsgWriter.h>

namespace rdr { class MemOutStream; }

namespace rfb {
  class SMsgWriterV3 : public SMsgWriter {
  public:
    SMsgWriterV3(ConnParams* cp, rdr::OutStream* os);
    virtual ~SMsgWriterV3();

    virtual void writeServerInit();
    virtual void startMsg(int type);
    virtual void endMsg();
    virtual bool writeSetDesktopSize();
    virtual bool writeExtendedDesktopSize();
    virtual bool writeExtendedDesktopSize(rdr::U16 reason, rdr::U16 result,
                                          int fb_width, int fb_height,
                                          const ScreenSet& layout);
    virtual bool writeSetDesktopName();
    virtual void cursorChange(WriteSetCursorCallback* cb);
    virtual void writeSetCursor(int width, int height, const Point& hotspot,
                                void* data, void* mask);
    virtual void writeSetXCursor(int width, int height, int hotspotX,
				 int hotspotY, void* data, void* mask);
    virtual bool needFakeUpdate();
    virtual bool needNoDataUpdate();
    virtual void writeNoDataUpdate();
    virtual void writeFramebufferUpdateStart(int nRects);
    virtual void writeFramebufferUpdateStart();
    virtual void writeFramebufferUpdateEnd();
    virtual void startRect(const Rect& r, unsigned int encoding);
    virtual void endRect();

  protected:
    virtual void writePseudoRects();
    virtual void writeNoDataRects();

  private:
    rdr::MemOutStream* updateOS;
    rdr::OutStream* realOS;
    int nRectsInUpdate;
    int nRectsInHeader;
    WriteSetCursorCallback* wsccb;
    bool needSetDesktopSize;
    bool needExtendedDesktopSize;
    bool needSetDesktopName;
    bool needLastRect;

    typedef struct {
      rdr::U16 reason, result;
      int fb_width, fb_height;
      ScreenSet layout;
    } ExtendedDesktopSizeMsg;
    std::list<ExtendedDesktopSizeMsg> extendedDesktopSizeMsgs;

  };
}
#endif
