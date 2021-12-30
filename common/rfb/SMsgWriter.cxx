/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <rdr/OutStream.h>
#include <rdr/MemOutStream.h>
#include <rdr/ZlibOutStream.h>

#include <rfb/msgTypes.h>
#include <rfb/fenceTypes.h>
#include <rfb/clipboardTypes.h>
#include <rfb/Exception.h>
#include <rfb/ClientParams.h>
#include <rfb/UpdateTracker.h>
#include <rfb/Encoder.h>
#include <rfb/SMsgWriter.h>
#include <rfb/LogWriter.h>
#include <rfb/ledStates.h>

using namespace rfb;

static LogWriter vlog("SMsgWriter");

SMsgWriter::SMsgWriter(ClientParams* client_, rdr::OutStream* os_)
  : client(client_), os(os_),
    nRectsInUpdate(0), nRectsInHeader(0),
    needSetDesktopName(false), needCursor(false),
    needCursorPos(false), needLEDState(false),
    needQEMUKeyEvent(false)
{
}

SMsgWriter::~SMsgWriter()
{
}

void SMsgWriter::writeServerInit(rdr::U16 width, rdr::U16 height,
                                 const PixelFormat& pf, const char* name)
{
  os->writeU16(width);
  os->writeU16(height);
  pf.write(os);
  os->writeU32(strlen(name));
  os->writeBytes(name, strlen(name));
  endMsg();
}

void SMsgWriter::writeSetColourMapEntries(int firstColour, int nColours,
                                          const rdr::U16 red[],
                                          const rdr::U16 green[],
                                          const rdr::U16 blue[])
{
  startMsg(msgTypeSetColourMapEntries);
  os->pad(1);
  os->writeU16(firstColour);
  os->writeU16(nColours);
  for (int i = firstColour; i < firstColour+nColours; i++) {
    os->writeU16(red[i]);
    os->writeU16(green[i]);
    os->writeU16(blue[i]);
  }
  endMsg();
}

void SMsgWriter::writeBell()
{
  startMsg(msgTypeBell);
  endMsg();
}

void SMsgWriter::writeServerCutText(const char* str)
{
  size_t len;

  if (strchr(str, '\r') != NULL)
    throw Exception("Invalid carriage return in clipboard data");

  len = strlen(str);
  startMsg(msgTypeServerCutText);
  os->pad(3);
  os->writeU32(len);
  os->writeBytes(str, len);
  endMsg();
}

void SMsgWriter::writeClipboardCaps(rdr::U32 caps,
                                    const rdr::U32* lengths)
{
  size_t i, count;

  if (!client->supportsEncoding(pseudoEncodingExtendedClipboard))
    throw Exception("Client does not support extended clipboard");

  count = 0;
  for (i = 0;i < 16;i++) {
    if (caps & (1 << i))
      count++;
  }

  startMsg(msgTypeServerCutText);
  os->pad(3);
  os->writeS32(-(4 + 4 * count));

  os->writeU32(caps | clipboardCaps);

  count = 0;
  for (i = 0;i < 16;i++) {
    if (caps & (1 << i))
      os->writeU32(lengths[count++]);
  }

  endMsg();
}

void SMsgWriter::writeClipboardRequest(rdr::U32 flags)
{
  if (!client->supportsEncoding(pseudoEncodingExtendedClipboard))
    throw Exception("Client does not support extended clipboard");
  if (!(client->clipboardFlags() & clipboardRequest))
    throw Exception("Client does not support clipboard \"request\" action");

  startMsg(msgTypeServerCutText);
  os->pad(3);
  os->writeS32(-4);
  os->writeU32(flags | clipboardRequest);
  endMsg();
}

void SMsgWriter::writeClipboardPeek(rdr::U32 flags)
{
  if (!client->supportsEncoding(pseudoEncodingExtendedClipboard))
    throw Exception("Client does not support extended clipboard");
  if (!(client->clipboardFlags() & clipboardPeek))
    throw Exception("Client does not support clipboard \"peek\" action");

  startMsg(msgTypeServerCutText);
  os->pad(3);
  os->writeS32(-4);
  os->writeU32(flags | clipboardPeek);
  endMsg();
}

void SMsgWriter::writeClipboardNotify(rdr::U32 flags)
{
  if (!client->supportsEncoding(pseudoEncodingExtendedClipboard))
    throw Exception("Client does not support extended clipboard");
  if (!(client->clipboardFlags() & clipboardNotify))
    throw Exception("Client does not support clipboard \"notify\" action");

  startMsg(msgTypeServerCutText);
  os->pad(3);
  os->writeS32(-4);
  os->writeU32(flags | clipboardNotify);
  endMsg();
}

void SMsgWriter::writeClipboardProvide(rdr::U32 flags,
                                      const size_t* lengths,
                                      const rdr::U8* const* data)
{
  rdr::MemOutStream mos;
  rdr::ZlibOutStream zos;

  int i, count;

  if (!client->supportsEncoding(pseudoEncodingExtendedClipboard))
    throw Exception("Client does not support extended clipboard");
  if (!(client->clipboardFlags() & clipboardProvide))
    throw Exception("Client does not support clipboard \"provide\" action");

  zos.setUnderlying(&mos);

  count = 0;
  for (i = 0;i < 16;i++) {
    if (!(flags & (1 << i)))
      continue;
    zos.writeU32(lengths[count]);
    zos.writeBytes(data[count], lengths[count]);
    count++;
  }

  zos.flush();

  startMsg(msgTypeServerCutText);
  os->pad(3);
  os->writeS32(-(4 + mos.length()));
  os->writeU32(flags | clipboardProvide);
  os->writeBytes(mos.data(), mos.length());
  endMsg();
}

void SMsgWriter::writeFence(rdr::U32 flags, unsigned len, const char data[])
{
  if (!client->supportsEncoding(pseudoEncodingFence))
    throw Exception("Client does not support fences");
  if (len > 64)
    throw Exception("Too large fence payload");
  if ((flags & ~fenceFlagsSupported) != 0)
    throw Exception("Unknown fence flags");

  startMsg(msgTypeServerFence);
  os->pad(3);

  os->writeU32(flags);

  os->writeU8(len);

  if (len > 0)
    os->writeBytes(data, len);

  endMsg();
}

void SMsgWriter::writeEndOfContinuousUpdates()
{
  if (!client->supportsEncoding(pseudoEncodingContinuousUpdates))
    throw Exception("Client does not support continuous updates");

  startMsg(msgTypeEndOfContinuousUpdates);
  endMsg();
}

void SMsgWriter::writeDesktopSize(rdr::U16 reason, rdr::U16 result)
{
  ExtendedDesktopSizeMsg msg;

  if (!client->supportsEncoding(pseudoEncodingDesktopSize) &&
      !client->supportsEncoding(pseudoEncodingExtendedDesktopSize))
    throw Exception("Client does not support desktop size changes");

  msg.reason = reason;
  msg.result = result;

  extendedDesktopSizeMsgs.push_back(msg);
}

void SMsgWriter::writeSetDesktopName()
{
  if (!client->supportsEncoding(pseudoEncodingDesktopName))
    throw Exception("Client does not support desktop name changes");

  needSetDesktopName = true;
}

void SMsgWriter::writeCursor()
{
  if (!client->supportsEncoding(pseudoEncodingCursor) &&
      !client->supportsEncoding(pseudoEncodingXCursor) &&
      !client->supportsEncoding(pseudoEncodingCursorWithAlpha) &&
      !client->supportsEncoding(pseudoEncodingVMwareCursor))
    throw Exception("Client does not support local cursor");

  needCursor = true;
}

void SMsgWriter::writeCursorPos()
{
  if (!client->supportsEncoding(pseudoEncodingVMwareCursorPosition))
    throw Exception("Client does not support cursor position");

  needCursorPos = true;
}

void SMsgWriter::writeLEDState()
{
  if (!client->supportsEncoding(pseudoEncodingLEDState) &&
      !client->supportsEncoding(pseudoEncodingVMwareLEDState))
    throw Exception("Client does not support LED state");
  if (client->ledState() == ledUnknown)
    throw Exception("Server has not specified LED state");

  needLEDState = true;
}

void SMsgWriter::writeQEMUKeyEvent()
{
  if (!client->supportsEncoding(pseudoEncodingQEMUKeyEvent))
    throw Exception("Client does not support QEMU key events");

  needQEMUKeyEvent = true;
}

bool SMsgWriter::needFakeUpdate()
{
  if (needSetDesktopName)
    return true;
  if (needCursor)
    return true;
  if (needCursorPos)
    return true;
  if (needLEDState)
    return true;
  if (needQEMUKeyEvent)
    return true;
  if (needNoDataUpdate())
    return true;

  return false;
}

bool SMsgWriter::needNoDataUpdate()
{
  if (!extendedDesktopSizeMsgs.empty())
    return true;

  return false;
}

void SMsgWriter::writeNoDataUpdate()
{
  int nRects;

  nRects = 0;

  if (!extendedDesktopSizeMsgs.empty()) {
    if (client->supportsEncoding(pseudoEncodingExtendedDesktopSize))
      nRects += extendedDesktopSizeMsgs.size();
    else
      nRects++;
  }

  writeFramebufferUpdateStart(nRects);
  writeNoDataRects();
  writeFramebufferUpdateEnd();
}

void SMsgWriter::writeFramebufferUpdateStart(int nRects)
{
  startMsg(msgTypeFramebufferUpdate);
  os->pad(1);

  if (nRects != 0xFFFF) {
    if (needSetDesktopName)
      nRects++;
    if (needCursor)
      nRects++;
    if (needCursorPos)
      nRects++;
    if (needLEDState)
      nRects++;
    if (needQEMUKeyEvent)
      nRects++;
  }

  os->writeU16(nRects);

  nRectsInUpdate = 0;
  if (nRects == 0xFFFF)
    nRectsInHeader = 0;
  else
    nRectsInHeader = nRects;

  writePseudoRects();
}

void SMsgWriter::writeFramebufferUpdateEnd()
{
  if (nRectsInUpdate != nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeFramebufferUpdateEnd: "
                    "nRects out of sync");

  if (nRectsInHeader == 0) {
    // Send last rect. marker
    os->writeS16(0);
    os->writeS16(0);
    os->writeU16(0);
    os->writeU16(0);
    os->writeU32(pseudoEncodingLastRect);
  }

  endMsg();
}

void SMsgWriter::writeCopyRect(const Rect& r, int srcX, int srcY)
{
  startRect(r,encodingCopyRect);
  os->writeU16(srcX);
  os->writeU16(srcY);
  endRect();
}

void SMsgWriter::startRect(const Rect& r, int encoding)
{
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::startRect: nRects out of sync");

  os->writeS16(r.tl.x);
  os->writeS16(r.tl.y);
  os->writeU16(r.width());
  os->writeU16(r.height());
  os->writeU32(encoding);
}

void SMsgWriter::endRect()
{
  os->flush();
}

void SMsgWriter::startMsg(int type)
{
  os->writeU8(type);
}

void SMsgWriter::endMsg()
{
  os->flush();
}

void SMsgWriter::writePseudoRects()
{
  if (needCursor) {
    const Cursor& cursor = client->cursor();

    if (client->supportsEncoding(pseudoEncodingCursorWithAlpha)) {
      writeSetCursorWithAlphaRect(cursor.width(), cursor.height(),
                                  cursor.hotspot().x, cursor.hotspot().y,
                                  cursor.getBuffer());
    } else if (client->supportsEncoding(pseudoEncodingVMwareCursor)) {
      writeSetVMwareCursorRect(cursor.width(), cursor.height(),
                               cursor.hotspot().x, cursor.hotspot().y,
                               cursor.getBuffer());
    } else if (client->supportsEncoding(pseudoEncodingCursor)) {
      rdr::U8Array data(cursor.width()*cursor.height() * (client->pf().bpp/8));
      rdr::U8Array mask(cursor.getMask());

      const rdr::U8* in;
      rdr::U8* out;

      in = cursor.getBuffer();
      out = data.buf;
      for (int i = 0;i < cursor.width()*cursor.height();i++) {
        client->pf().bufferFromRGB(out, in, 1);
        in += 4;
        out += client->pf().bpp/8;
      }

      writeSetCursorRect(cursor.width(), cursor.height(),
                         cursor.hotspot().x, cursor.hotspot().y,
                         data.buf, mask.buf);
    } else if (client->supportsEncoding(pseudoEncodingXCursor)) {
      rdr::U8Array bitmap(cursor.getBitmap());
      rdr::U8Array mask(cursor.getMask());

      writeSetXCursorRect(cursor.width(), cursor.height(),
                          cursor.hotspot().x, cursor.hotspot().y,
                          bitmap.buf, mask.buf);
    } else {
      throw Exception("Client does not support local cursor");
    }

    needCursor = false;
  }

  if (needCursorPos) {
    const Point& cursorPos = client->cursorPos();

    if (client->supportsEncoding(pseudoEncodingVMwareCursorPosition)) {
      writeSetVMwareCursorPositionRect(cursorPos.x, cursorPos.y);
    } else {
      throw Exception("Client does not support cursor position");
    }

    needCursorPos = false;
  }

  if (needSetDesktopName) {
    writeSetDesktopNameRect(client->name());
    needSetDesktopName = false;
  }

  if (needLEDState) {
    writeLEDStateRect(client->ledState());
    needLEDState = false;
  }

  if (needQEMUKeyEvent) {
    writeQEMUKeyEventRect();
    needQEMUKeyEvent = false;
  }
}

void SMsgWriter::writeNoDataRects()
{
  if (!extendedDesktopSizeMsgs.empty()) {
    if (client->supportsEncoding(pseudoEncodingExtendedDesktopSize)) {
      std::list<ExtendedDesktopSizeMsg>::const_iterator ri;
      for (ri = extendedDesktopSizeMsgs.begin();ri != extendedDesktopSizeMsgs.end();++ri) {
        // FIXME: We can probably skip multiple reasonServer entries
        writeExtendedDesktopSizeRect(ri->reason, ri->result,
                                     client->width(), client->height(),
                                     client->screenLayout());
      }
    } else if (client->supportsEncoding(pseudoEncodingDesktopSize)) {
      // Some clients assume this is the last rectangle so don't send anything
      // more after this
      writeSetDesktopSizeRect(client->width(), client->height());
    } else {
      throw Exception("Client does not support desktop size changes");
    }

    extendedDesktopSizeMsgs.clear();
  }
}

void SMsgWriter::writeSetDesktopSizeRect(int width, int height)
{
  if (!client->supportsEncoding(pseudoEncodingDesktopSize))
    throw Exception("Client does not support desktop resize");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeSetDesktopSizeRect: nRects out of sync");

  os->writeS16(0);
  os->writeS16(0);
  os->writeU16(width);
  os->writeU16(height);
  os->writeU32(pseudoEncodingDesktopSize);
}

void SMsgWriter::writeExtendedDesktopSizeRect(rdr::U16 reason,
                                              rdr::U16 result,
                                              int fb_width,
                                              int fb_height,
                                              const ScreenSet& layout)
{
  ScreenSet::const_iterator si;

  if (!client->supportsEncoding(pseudoEncodingExtendedDesktopSize))
    throw Exception("Client does not support extended desktop resize");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeExtendedDesktopSizeRect: nRects out of sync");

  os->writeU16(reason);
  os->writeU16(result);
  os->writeU16(fb_width);
  os->writeU16(fb_height);
  os->writeU32(pseudoEncodingExtendedDesktopSize);

  os->writeU8(layout.num_screens());
  os->pad(3);

  for (si = layout.begin();si != layout.end();++si) {
    os->writeU32(si->id);
    os->writeU16(si->dimensions.tl.x);
    os->writeU16(si->dimensions.tl.y);
    os->writeU16(si->dimensions.width());
    os->writeU16(si->dimensions.height());
    os->writeU32(si->flags);
  }
}

void SMsgWriter::writeSetDesktopNameRect(const char *name)
{
  if (!client->supportsEncoding(pseudoEncodingDesktopName))
    throw Exception("Client does not support desktop rename");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeSetDesktopNameRect: nRects out of sync");

  os->writeS16(0);
  os->writeS16(0);
  os->writeU16(0);
  os->writeU16(0);
  os->writeU32(pseudoEncodingDesktopName);
  os->writeU32(strlen(name));
  os->writeBytes(name, strlen(name));
}

void SMsgWriter::writeSetCursorRect(int width, int height,
                                    int hotspotX, int hotspotY,
                                    const void* data, const void* mask)
{
  if (!client->supportsEncoding(pseudoEncodingCursor))
    throw Exception("Client does not support local cursors");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeSetCursorRect: nRects out of sync");

  os->writeS16(hotspotX);
  os->writeS16(hotspotY);
  os->writeU16(width);
  os->writeU16(height);
  os->writeU32(pseudoEncodingCursor);
  os->writeBytes(data, width * height * (client->pf().bpp/8));
  os->writeBytes(mask, (width+7)/8 * height);
}

void SMsgWriter::writeSetXCursorRect(int width, int height,
                                     int hotspotX, int hotspotY,
                                     const void* data, const void* mask)
{
  if (!client->supportsEncoding(pseudoEncodingXCursor))
    throw Exception("Client does not support local cursors");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeSetXCursorRect: nRects out of sync");

  os->writeS16(hotspotX);
  os->writeS16(hotspotY);
  os->writeU16(width);
  os->writeU16(height);
  os->writeU32(pseudoEncodingXCursor);
  if (width * height > 0) {
    os->writeU8(255);
    os->writeU8(255);
    os->writeU8(255);
    os->writeU8(0);
    os->writeU8(0);
    os->writeU8(0);
    os->writeBytes(data, (width+7)/8 * height);
    os->writeBytes(mask, (width+7)/8 * height);
  }
}

void SMsgWriter::writeSetCursorWithAlphaRect(int width, int height,
                                             int hotspotX, int hotspotY,
                                             const rdr::U8* data)
{
  if (!client->supportsEncoding(pseudoEncodingCursorWithAlpha))
    throw Exception("Client does not support local cursors");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeSetCursorWithAlphaRect: nRects out of sync");

  os->writeS16(hotspotX);
  os->writeS16(hotspotY);
  os->writeU16(width);
  os->writeU16(height);
  os->writeU32(pseudoEncodingCursorWithAlpha);

  // FIXME: Use an encoder with compression?
  os->writeU32(encodingRaw);

  // Alpha needs to be pre-multiplied
  for (int i = 0;i < width*height;i++) {
    os->writeU8((unsigned)data[0] * data[3] / 255);
    os->writeU8((unsigned)data[1] * data[3] / 255);
    os->writeU8((unsigned)data[2] * data[3] / 255);
    os->writeU8(data[3]);
    data += 4;
  }
}

void SMsgWriter::writeSetVMwareCursorRect(int width, int height,
                                          int hotspotX, int hotspotY,
                                          const rdr::U8* data)
{
  if (!client->supportsEncoding(pseudoEncodingVMwareCursor))
    throw Exception("Client does not support local cursors");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeSetVMwareCursorRect: nRects out of sync");

  os->writeS16(hotspotX);
  os->writeS16(hotspotY);
  os->writeU16(width);
  os->writeU16(height);
  os->writeU32(pseudoEncodingVMwareCursor);

  os->writeU8(1); // Alpha cursor
  os->pad(1);

  // FIXME: Should alpha be premultiplied?
  os->writeBytes(data, width*height*4);
}

void SMsgWriter::writeSetVMwareCursorPositionRect(int hotspotX, int hotspotY)
{
  if (!client->supportsEncoding(pseudoEncodingVMwareCursorPosition))
    throw Exception("Client does not support cursor position");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeSetVMwareCursorRect: nRects out of sync");

  os->writeS16(hotspotX);
  os->writeS16(hotspotY);
  os->writeU16(0);
  os->writeU16(0);
  os->writeU32(pseudoEncodingVMwareCursorPosition);
}

void SMsgWriter::writeLEDStateRect(rdr::U8 state)
{
  if (!client->supportsEncoding(pseudoEncodingLEDState) &&
      !client->supportsEncoding(pseudoEncodingVMwareLEDState))
    throw Exception("Client does not support LED state updates");
  if (client->ledState() == ledUnknown)
    throw Exception("Server does not support LED state updates");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeLEDStateRect: nRects out of sync");

  os->writeS16(0);
  os->writeS16(0);
  os->writeU16(0);
  os->writeU16(0);
  if (client->supportsEncoding(pseudoEncodingLEDState)) {
    os->writeU32(pseudoEncodingLEDState);
    os->writeU8(state);
  } else {
    os->writeU32(pseudoEncodingVMwareLEDState);
    os->writeU32(state);
  }
}

void SMsgWriter::writeQEMUKeyEventRect()
{
  if (!client->supportsEncoding(pseudoEncodingQEMUKeyEvent))
    throw Exception("Client does not support QEMU extended key events");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeQEMUKeyEventRect: nRects out of sync");

  os->writeS16(0);
  os->writeS16(0);
  os->writeU16(0);
  os->writeU16(0);
  os->writeU32(pseudoEncodingQEMUKeyEvent);
}
