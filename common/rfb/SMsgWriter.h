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
// SMsgWriter - class for writing RFB messages on the server side.
//

#ifndef __RFB_SMSGWRITER_H__
#define __RFB_SMSGWRITER_H__

#include <rdr/types.h>
#include <rfb/encodings.h>
#include <rfb/ScreenSet.h>

namespace rdr { class OutStream; }

namespace rfb {

  class ClientParams;
  class PixelFormat;
  struct ScreenSet;

  class SMsgWriter {
  public:
    SMsgWriter(ClientParams* client, rdr::OutStream* os);
    virtual ~SMsgWriter();

    // writeServerInit() must only be called at the appropriate time in the
    // protocol initialisation.
    void writeServerInit(rdr::U16 width, rdr::U16 height,
                         const PixelFormat& pf, const char* name);

    // Methods to write normal protocol messages

    // writeSetColourMapEntries() writes a setColourMapEntries message, using
    // the given colour entries.
    void writeSetColourMapEntries(int firstColour, int nColours,
                                  const rdr::U16 red[],
                                  const rdr::U16 green[],
                                  const rdr::U16 blue[]);

    // writeBell() does the obvious thing.
    void writeBell();

    void writeServerCutText(const char* str);

    void writeClipboardCaps(rdr::U32 caps, const rdr::U32* lengths);
    void writeClipboardRequest(rdr::U32 flags);
    void writeClipboardPeek(rdr::U32 flags);
    void writeClipboardNotify(rdr::U32 flags);
    void writeClipboardProvide(rdr::U32 flags, const size_t* lengths,
                               const rdr::U8* const* data);

    // writeFence() sends a new fence request or response to the client.
    void writeFence(rdr::U32 flags, unsigned len, const char data[]);

    // writeEndOfContinuousUpdates() indicates that we have left continuous
    // updates mode.
    void writeEndOfContinuousUpdates();

    // writeDesktopSize() won't actually write immediately, but will
    // write the relevant pseudo-rectangle as part of the next update.
    void writeDesktopSize(rdr::U16 reason, rdr::U16 result=0);

    void writeSetDesktopName();

    // Like setDesktopSize, we can't just write out a cursor message
    // immediately. 
    void writeCursor();

    // Notifies the client that the cursor pointer was moved by the server.
    void writeCursorPos();

    // Same for LED state message
    void writeLEDState();

    // And QEMU keyboard event handshake
    void writeQEMUKeyEvent();

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
                             const void* data, const void* mask);
    void writeSetCursorWithAlphaRect(int width, int height,
                                     int hotspotX, int hotspotY,
                                     const rdr::U8* data);
    void writeSetVMwareCursorRect(int width, int height,
                                  int hotspotX, int hotspotY,
                                  const rdr::U8* data);
    void writeSetVMwareCursorPositionRect(int hotspotX, int hotspotY);
    void writeLEDStateRect(rdr::U8 state);
    void writeQEMUKeyEventRect();

    ClientParams* client;
    rdr::OutStream* os;

    int nRectsInUpdate;
    int nRectsInHeader;

    bool needSetDesktopName;
    bool needCursor;
    bool needCursorPos;
    bool needLEDState;
    bool needQEMUKeyEvent;

    typedef struct {
      rdr::U16 reason, result;
    } ExtendedDesktopSizeMsg;

    std::list<ExtendedDesktopSizeMsg> extendedDesktopSizeMsgs;
  };
}
#endif
