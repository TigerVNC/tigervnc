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
// SMsgWriter - class for writing RFB messages on the server side.
//

#ifndef __RFB_SMSGWRITER_H__
#define __RFB_SMSGWRITER_H__

#include <rdr/types.h>
#include <rfb/encodings.h>
#include <rfb/ScreenSet.h>

namespace rdr { class OutStream; }

namespace rfb {

  class ConnParams;
  class ScreenSet;

  class SMsgWriter {
  public:
    SMsgWriter(ConnParams* cp, rdr::OutStream* os);
    virtual ~SMsgWriter();

    // writeServerInit() must only be called at the appropriate time in the
    // protocol initialisation.
    void writeServerInit();

    // Methods to write normal protocol messages

    // writeSetColourMapEntries() writes a setColourMapEntries message, using
    // the given colour entries.
    void writeSetColourMapEntries(int firstColour, int nColours,
                                  const rdr::U16 red[],
                                  const rdr::U16 green[],
                                  const rdr::U16 blue[]);

    // writeBell() and writeServerCutText() do the obvious thing.
    void writeBell();
    void writeServerCutText(const char* str, int len);

    // writeFence() sends a new fence request or response to the client.
    void writeFence(rdr::U32 flags, unsigned len, const char data[]);

    // writeEndOfContinuousUpdates() indicates that we have left continuous
    // updates mode.
    void writeEndOfContinuousUpdates();

    // writeSetDesktopSize() won't actually write immediately, but will
    // write the relevant pseudo-rectangle as part of the next update.
    bool writeSetDesktopSize();
    // Same thing for the extended version. The first version queues up a
    // generic update of the current server state, but the second queues a
    // specific message.
    bool writeExtendedDesktopSize();
    bool writeExtendedDesktopSize(rdr::U16 reason, rdr::U16 result,
                                  int fb_width, int fb_height,
                                  const ScreenSet& layout);

    bool writeSetDesktopName();

    // Like setDesktopSize, we can't just write out a cursor message
    // immediately. 
    bool writeSetCursor();
    bool writeSetXCursor();

    // needFakeUpdate() returns true when an immediate update is needed in
    // order to flush out pseudo-rectangles to the client.
    bool needFakeUpdate();

    // needNoDataUpdate() returns true when an update without any
    // framebuffer changes need to be sent (using writeNoDataUpdate()).
    // Commonly this is an update that modifies the size of the framebuffer
    // or the screen layout.
    bool needNoDataUpdate();

    // writeNoDataUpdate() write a framebuffer update containing only
    // pseudo-rectangles.
    void writeNoDataUpdate();

    // writeFramebufferUpdateStart() initiates an update which you can fill
    // in using writeCopyRect() and encoders. Finishing the update by calling
    // writeFramebufferUpdateEnd().
    void writeFramebufferUpdateStart(int nRects);
    void writeFramebufferUpdateEnd();

    // There is no explicit encoder for CopyRect rects.
    void writeCopyRect(const Rect& r, int srcX, int srcY);

    // Encoders should call these to mark the start and stop of individual
    // rects.
    void startRect(const Rect& r, int enc);
    void endRect();

  protected:
    void startMsg(int type);
    void endMsg();

    void writePseudoRects();
    void writeNoDataRects();

    void writeSetDesktopSizeRect(int width, int height);
    void writeExtendedDesktopSizeRect(rdr::U16 reason, rdr::U16 result,
                                      int fb_width, int fb_height,
                                      const ScreenSet& layout);
    void writeSetDesktopNameRect(const char *name);
    void writeSetCursorRect(int width, int height,
                            int hotspotX, int hotspotY,
                            const void* data, const void* mask);
    void writeSetXCursorRect(int width, int height,
                             int hotspotX, int hotspotY,
                             const rdr::U8 pix0[], const rdr::U8 pix1[],
                             const void* data, const void* mask);

    ConnParams* cp;
    rdr::OutStream* os;

    int nRectsInUpdate;
    int nRectsInHeader;

    bool needSetDesktopSize;
    bool needExtendedDesktopSize;
    bool needSetDesktopName;
    bool needLastRect;
    bool needSetCursor;
    bool needSetXCursor;

    typedef struct {
      rdr::U16 reason, result;
      int fb_width, fb_height;
      ScreenSet layout;
    } ExtendedDesktopSizeMsg;

    std::list<ExtendedDesktopSizeMsg> extendedDesktopSizeMsgs;
  };
}
#endif
