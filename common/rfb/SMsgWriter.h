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
  class TransImageGetter;
  class ColourMap;
  class Region;
  class UpdateInfo;
  class Encoder;
  class ScreenSet;

  class WriteSetCursorCallback {
  public:
    virtual void writeSetCursorCallback() = 0;
  };

  class SMsgWriter {
  public:
    SMsgWriter(ConnParams* cp, rdr::OutStream* os);
    virtual ~SMsgWriter();

    // writeServerInit() must only be called at the appropriate time in the
    // protocol initialisation.
    void writeServerInit();

    // Methods to write normal protocol messages

    // writeSetColourMapEntries() writes a setColourMapEntries message, using
    // the given ColourMap object to lookup the RGB values of the given range
    // of colours.
    void writeSetColourMapEntries(int firstColour, int nColours,
                                  ColourMap* cm);

    // writeBell() and writeServerCutText() do the obvious thing.
    void writeBell();
    void writeServerCutText(const char* str, int len);

    // writeFence() sends a new fence request or response to the client.
    void writeFence(rdr::U32 flags, unsigned len, const char data[]);

    // writeEndOfContinuousUpdates() indicates that we have left continuous
    // updates mode.
    void writeEndOfContinuousUpdates();

    // setupCurrentEncoder() should be called before each framebuffer update,
    // prior to calling getNumRects() or writeFramebufferUpdateStart().
    void setupCurrentEncoder();

    // getNumRects() computes the number of sub-rectangles that will compose a
    // given rectangle, for current encoder.
    int getNumRects(const Rect &r);

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

    // Like setDesktopSize, we can't just write out a setCursor message
    // immediately. Instead of calling writeSetCursor() directly,
    // you must call cursorChange(), and then invoke writeSetCursor()
    // in response to the writeSetCursorCallback() callback. This will
    // happen when the next update is sent.
    void cursorChange(WriteSetCursorCallback* cb);
    void writeSetCursor(int width, int height, const Point& hotspot,
                        void* data, void* mask);
    void writeSetXCursor(int width, int height, int hotspotX, int hotspotY,
                         void* data, void* mask);

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

    // writeRects() accepts an UpdateInfo (changed & copied regions) and an
    // ImageGetter to fetch pixels from.  It then calls writeCopyRect() and
    // writeRect() as appropriate.  writeFramebufferUpdateStart() must be used
    // before the first writeRects() call and writeFrameBufferUpdateEnd() after
    // the last one.
    void writeRects(const UpdateInfo& update, TransImageGetter* ig);

    // To construct a framebuffer update you can call
    // writeFramebufferUpdateStart(), followed by a number of writeCopyRect()s
    // and writeRect()s, finishing with writeFramebufferUpdateEnd().
    void writeFramebufferUpdateStart(int nRects);
    void writeFramebufferUpdateEnd();

    // writeRect() writers the given rectangle using either the preferred
    // encoder, or the one explicitly given.
    void writeRect(const Rect& r, TransImageGetter* ig);
    void writeRect(const Rect& r, int encoding, TransImageGetter* ig);

    void writeCopyRect(const Rect& r, int srcX, int srcY);

    void startRect(const Rect& r, int enc);
    void endRect();

    ConnParams* getConnParams() { return cp; }
    rdr::OutStream* getOutStream() { return os; }
    rdr::U8* getImageBuf(int required, int requested=0, int* nPixels=0);
    int bpp();

    int getUpdatesSent()           { return updatesSent; }
    int getRectsSent(int encoding) { return rectsSent[encoding]; }
    int getBytesSent(int encoding) { return bytesSent[encoding]; }
    rdr::U64 getRawBytesEquivalent()    { return rawBytesEquivalent; }

    int imageBufIdealSize;

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

    ConnParams* cp;
    rdr::OutStream* os;

    Encoder* encoders[encodingMax+1];
    int currentEncoding;

    int nRectsInUpdate;
    int nRectsInHeader;

    WriteSetCursorCallback* wsccb;

    bool needSetDesktopSize;
    bool needExtendedDesktopSize;
    bool needSetDesktopName;
    bool needLastRect;

    int lenBeforeRect;
    int updatesSent;
    int bytesSent[encodingMax+1];
    int rectsSent[encodingMax+1];
    rdr::U64 rawBytesEquivalent;

    rdr::U8* imageBuf;
    int imageBufSize;

    typedef struct {
      rdr::U16 reason, result;
      int fb_width, fb_height;
      ScreenSet layout;
    } ExtendedDesktopSizeMsg;

    std::list<ExtendedDesktopSizeMsg> extendedDesktopSizeMsgs;
  };
}
#endif
