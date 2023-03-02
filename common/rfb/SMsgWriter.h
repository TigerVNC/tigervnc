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

#include <stdint.h>

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
    void writeServerInit(uint16_t width, uint16_t height,
                         const PixelFormat& pf, const char* name);

    // Methods to write normal protocol messages

    // writeSetColourMapEntries() writes a setColourMapEntries message, using
    // the given colour entries.
    void writeSetColourMapEntries(int firstColour, int nColours,
                                  const uint16_t red[],
                                  const uint16_t green[],
                                  const uint16_t blue[]);

    // writeBell() does the obvious thing.
    void writeBell();

    void writeServerCutText(const char* str);

    void writeClipboardCaps(uint32_t caps, const uint32_t* lengths);
    void writeClipboardRequest(uint32_t flags);
    void writeClipboardPeek(uint32_t flags);
    void writeClipboardNotify(uint32_t flags);
    void writeClipboardProvide(uint32_t flags, const size_t* lengths,
                               const uint8_t* const* data);

    // writeFence() sends a new fence request or response to the client.
    void writeFence(uint32_t flags, unsigned len, const uint8_t data[]);

    // writeEndOfContinuousUpdates() indicates that we have left continuous
    // updates mode.
    void writeEndOfContinuousUpdates();

    // writeDesktopSize() won't actually write immediately, but will
    // write the relevant pseudo-rectangle as part of the next update.
    void writeDesktopSize(uint16_t reason, uint16_t result=0);

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
    void writeExtendedDesktopSizeRect(uint16_t reason, uint16_t result,
                                      int fb_width, int fb_height,
                                      const ScreenSet& layout);
    void writeSetDesktopNameRect(const char *name);
    void writeSetCursorRect(int width, int height,
                            int hotspotX, int hotspotY,
                            const uint8_t* data, const uint8_t* mask);
    void writeSetXCursorRect(int width, int height,
                             int hotspotX, int hotspotY,
                             const uint8_t* data, const uint8_t* mask);
    void writeSetCursorWithAlphaRect(int width, int height,
                                     int hotspotX, int hotspotY,
                                     const uint8_t* data);
    void writeSetVMwareCursorRect(int width, int height,
                                  int hotspotX, int hotspotY,
                                  const uint8_t* data);
    void writeSetVMwareCursorPositionRect(int hotspotX, int hotspotY);
    void writeLEDStateRect(uint8_t state);
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
      uint16_t reason, result;
    } ExtendedDesktopSizeMsg;

    std::list<ExtendedDesktopSizeMsg> extendedDesktopSizeMsgs;
  };
}
#endif
