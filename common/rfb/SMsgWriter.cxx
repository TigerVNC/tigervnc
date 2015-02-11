/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
#include <stdio.h>
#include <rdr/OutStream.h>
#include <rfb/msgTypes.h>
#include <rfb/fenceTypes.h>
#include <rfb/Exception.h>
#include <rfb/ConnParams.h>
#include <rfb/UpdateTracker.h>
#include <rfb/Encoder.h>
#include <rfb/SMsgWriter.h>
#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("SMsgWriter");

SMsgWriter::SMsgWriter(ConnParams* cp_, rdr::OutStream* os_)
  : cp(cp_), os(os_),
    nRectsInUpdate(0), nRectsInHeader(0),
    needSetDesktopSize(false), needExtendedDesktopSize(false),
    needSetDesktopName(false), needSetCursor(false), needSetXCursor(false)
{
}

SMsgWriter::~SMsgWriter()
{
}

void SMsgWriter::writeServerInit()
{
  os->writeU16(cp->width);
  os->writeU16(cp->height);
  cp->pf().write(os);
  os->writeString(cp->name());
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

void SMsgWriter::writeServerCutText(const char* str, int len)
{
  startMsg(msgTypeServerCutText);
  os->pad(3);
  os->writeU32(len);
  os->writeBytes(str, len);
  endMsg();
}

void SMsgWriter::writeFence(rdr::U32 flags, unsigned len, const char data[])
{
  if (!cp->supportsFence)
    throw Exception("Client does not support fences");
  if (len > 64)
    throw Exception("Too large fence payload");
  if ((flags & ~fenceFlagsSupported) != 0)
    throw Exception("Unknown fence flags");

  startMsg(msgTypeServerFence);
  os->pad(3);

  os->writeU32(flags);

  os->writeU8(len);
  os->writeBytes(data, len);

  endMsg();
}

void SMsgWriter::writeEndOfContinuousUpdates()
{
  if (!cp->supportsContinuousUpdates)
    throw Exception("Client does not support continuous updates");

  startMsg(msgTypeEndOfContinuousUpdates);
  endMsg();
}

bool SMsgWriter::writeSetDesktopSize() {
  if (!cp->supportsDesktopResize)
    return false;

  needSetDesktopSize = true;

  return true;
}

bool SMsgWriter::writeExtendedDesktopSize() {
  if (!cp->supportsExtendedDesktopSize)
    return false;

  needExtendedDesktopSize = true;

  return true;
}

bool SMsgWriter::writeExtendedDesktopSize(rdr::U16 reason, rdr::U16 result,
                                          int fb_width, int fb_height,
                                          const ScreenSet& layout) {
  ExtendedDesktopSizeMsg msg;

  if (!cp->supportsExtendedDesktopSize)
    return false;

  msg.reason = reason;
  msg.result = result;
  msg.fb_width = fb_width;
  msg.fb_height = fb_height;
  msg.layout = layout;

  extendedDesktopSizeMsgs.push_back(msg);

  return true;
}

bool SMsgWriter::writeSetDesktopName() {
  if (!cp->supportsDesktopRename)
    return false;

  needSetDesktopName = true;

  return true;
}

bool SMsgWriter::writeSetCursor()
{
  if (!cp->supportsLocalCursor)
    return false;

  needSetCursor = true;

  return true;
}

bool SMsgWriter::writeSetXCursor()
{
  if (!cp->supportsLocalXCursor)
    return false;

  needSetXCursor = true;

  return true;
}

bool SMsgWriter::needFakeUpdate()
{
  if (needSetDesktopName)
    return true;
  if (needSetCursor || needSetXCursor)
    return true;
  if (needNoDataUpdate())
    return true;

  return false;
}

bool SMsgWriter::needNoDataUpdate()
{
  if (needSetDesktopSize)
    return true;
  if (needExtendedDesktopSize || !extendedDesktopSizeMsgs.empty())
    return true;

  return false;
}

void SMsgWriter::writeNoDataUpdate()
{
  int nRects;

  nRects = 0;

  if (needSetDesktopSize)
    nRects++;
  if (needExtendedDesktopSize)
    nRects++;
  if (!extendedDesktopSizeMsgs.empty())
    nRects += extendedDesktopSizeMsgs.size();

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
    if (needSetCursor)
      nRects++;
    if (needSetXCursor)
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
  if (needSetCursor) {
    rdr::U8* data;
    int stride;

    const Cursor& cursor = cp->cursor();

    data = new rdr::U8[cursor.area() * cp->pf().bpp/8];
    cursor.getImage(cp->pf(), data, cursor.getRect());

    writeSetCursorRect(cursor.width(), cursor.height(),
                       cursor.hotspot.x, cursor.hotspot.y,
                       data, cursor.mask.buf);
    needSetCursor = false;

    delete [] data;
  }

  if (needSetXCursor) {
    const Cursor& cursor = cp->cursor();
    Pixel pix0, pix1;
    rdr::U8 rgb0[3], rgb1[3];
    rdr::U8Array bitmap(cursor.getBitmap(&pix0, &pix1));

    if (!bitmap.buf) {
      // FIXME: We could reduce to two colors.
      throw Exception("SMsgWriter::writePseudoRects: Unable to send multicolor cursor: RichCursor not supported by client");
    }

    cp->pf().rgbFromPixel(pix0, &rgb0[0], &rgb0[1], &rgb0[2]);
    cp->pf().rgbFromPixel(pix1, &rgb1[0], &rgb1[1], &rgb1[2]);

    writeSetXCursorRect(cursor.width(), cursor.height(),
                        cursor.hotspot.x, cursor.hotspot.y,
                        rgb0, rgb1, bitmap.buf, cursor.mask.buf);
    needSetXCursor = false;
  }

  if (needSetDesktopName) {
    writeSetDesktopNameRect(cp->name());
    needSetDesktopName = false;
  }
}

void SMsgWriter::writeNoDataRects()
{
  // Start with specific ExtendedDesktopSize messages
  if (!extendedDesktopSizeMsgs.empty()) {
    std::list<ExtendedDesktopSizeMsg>::const_iterator ri;

    for (ri = extendedDesktopSizeMsgs.begin();ri != extendedDesktopSizeMsgs.end();++ri) {
      writeExtendedDesktopSizeRect(ri->reason, ri->result,
                                   ri->fb_width, ri->fb_height, ri->layout);
    }

    extendedDesktopSizeMsgs.clear();
  }

  // Send this before SetDesktopSize to make life easier on the clients
  if (needExtendedDesktopSize) {
    writeExtendedDesktopSizeRect(0, 0, cp->width, cp->height,
                                 cp->screenLayout);
    needExtendedDesktopSize = false;
  }

  // Some clients assume this is the last rectangle so don't send anything
  // more after this
  if (needSetDesktopSize) {
    writeSetDesktopSizeRect(cp->width, cp->height);
    needSetDesktopSize = false;
  }
}

void SMsgWriter::writeSetDesktopSizeRect(int width, int height)
{
  if (!cp->supportsDesktopResize)
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

  if (!cp->supportsExtendedDesktopSize)
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
  if (!cp->supportsDesktopRename)
    throw Exception("Client does not support desktop rename");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeSetDesktopNameRect: nRects out of sync");

  os->writeS16(0);
  os->writeS16(0);
  os->writeU16(0);
  os->writeU16(0);
  os->writeU32(pseudoEncodingDesktopName);
  os->writeString(name);
}

void SMsgWriter::writeSetCursorRect(int width, int height,
                                    int hotspotX, int hotspotY,
                                    const void* data, const void* mask)
{
  if (!cp->supportsLocalCursor)
    throw Exception("Client does not support local cursors");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeSetCursorRect: nRects out of sync");

  os->writeS16(hotspotX);
  os->writeS16(hotspotY);
  os->writeU16(width);
  os->writeU16(height);
  os->writeU32(pseudoEncodingCursor);
  os->writeBytes(data, width * height * (cp->pf().bpp/8));
  os->writeBytes(mask, (width+7)/8 * height);
}

void SMsgWriter::writeSetXCursorRect(int width, int height,
                                     int hotspotX, int hotspotY,
                                     const rdr::U8 pix0[],
                                     const rdr::U8 pix1[],
                                     const void* data, const void* mask)
{
  if (!cp->supportsLocalXCursor)
    throw Exception("Client does not support local cursors");
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriter::writeSetXCursorRect: nRects out of sync");

  os->writeS16(hotspotX);
  os->writeS16(hotspotY);
  os->writeU16(width);
  os->writeU16(height);
  os->writeU32(pseudoEncodingXCursor);
  if (width * height) {
    os->writeU8(pix0[0]);
    os->writeU8(pix0[1]);
    os->writeU8(pix0[2]);
    os->writeU8(pix1[0]);
    os->writeU8(pix1[1]);
    os->writeU8(pix1[2]);
    os->writeBytes(data, (width+7)/8 * height);
    os->writeBytes(mask, (width+7)/8 * height);
  }
}
