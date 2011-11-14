/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2011 Pierre Ossman for Cendio AB
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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
#include <rdr/OutStream.h>
#include <rdr/MemOutStream.h>
#include <rfb/msgTypes.h>
#include <rfb/screenTypes.h>
#include <rfb/fenceTypes.h>
#include <rfb/Exception.h>
#include <rfb/ConnParams.h>
#include <rfb/SMsgWriterV3.h>

using namespace rfb;

SMsgWriterV3::SMsgWriterV3(ConnParams* cp, rdr::OutStream* os)
  : SMsgWriter(cp, os), updateOS(0), realOS(os), nRectsInUpdate(0),
    nRectsInHeader(0), wsccb(0), needSetDesktopSize(false),
    needExtendedDesktopSize(false), needSetDesktopName(false)
{
}

SMsgWriterV3::~SMsgWriterV3()
{
  delete updateOS;
}

void SMsgWriterV3::writeServerInit()
{
  os->writeU16(cp->width);
  os->writeU16(cp->height);
  cp->pf().write(os);
  os->writeString(cp->name());
  endMsg();
}

void SMsgWriterV3::startMsg(int type)
{
  if (os != realOS)
    throw Exception("startMsg called while writing an update?");

  os->writeU8(type);
}

void SMsgWriterV3::endMsg()
{
  os->flush();
}

void SMsgWriterV3::writeFence(rdr::U32 flags, unsigned len, const char data[])
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

void SMsgWriterV3::writeEndOfContinuousUpdates()
{
  if (!cp->supportsContinuousUpdates)
    throw Exception("Client does not support continuous updates");

  startMsg(msgTypeEndOfContinuousUpdates);
  endMsg();
}

bool SMsgWriterV3::writeSetDesktopSize() {
  if (!cp->supportsDesktopResize) return false;
  needSetDesktopSize = true;
  return true;
}

bool SMsgWriterV3::writeExtendedDesktopSize() {
  if (!cp->supportsExtendedDesktopSize) return false;
  needExtendedDesktopSize = true;
  return true;
}

bool SMsgWriterV3::writeExtendedDesktopSize(rdr::U16 reason, rdr::U16 result,
                                            int fb_width, int fb_height,
                                            const ScreenSet& layout) {
  ExtendedDesktopSizeMsg msg;

  if (!cp->supportsExtendedDesktopSize) return false;

  msg.reason = reason;
  msg.result = result;
  msg.fb_width = fb_width;
  msg.fb_height = fb_height;
  msg.layout = layout;

  extendedDesktopSizeMsgs.push_back(msg);

  return true;
}

bool SMsgWriterV3::writeSetDesktopName() {
  if (!cp->supportsDesktopRename) return false;
  needSetDesktopName = true;
  return true;
}

void SMsgWriterV3::cursorChange(WriteSetCursorCallback* cb)
{
  wsccb = cb;
}

void SMsgWriterV3::writeSetCursor(int width, int height, const Point& hotspot,
                                  void* data, void* mask)
{
  if (!wsccb) return;
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriterV3::writeSetCursor: nRects out of sync");
  os->writeS16(hotspot.x);
  os->writeS16(hotspot.y);
  os->writeU16(width);
  os->writeU16(height);
  os->writeU32(pseudoEncodingCursor);
  os->writeBytes(data, width * height * (cp->pf().bpp/8));
  os->writeBytes(mask, (width+7)/8 * height);
}

void SMsgWriterV3::writeSetXCursor(int width, int height, int hotspotX,
				   int hotspotY, void* data, void* mask)
{
  if (!wsccb) return;
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriterV3::writeSetXCursor: nRects out of sync");
  os->writeS16(hotspotX);
  os->writeS16(hotspotY);
  os->writeU16(width);
  os->writeU16(height);
  os->writeU32(pseudoEncodingXCursor);
  // FIXME: We only support black and white cursors, currently. We
  // could pass the correct color by using the pix0/pix1 values
  // returned from getBitmap, in writeSetCursorCallback. However, we
  // would then need to undo the conversion from rgb to Pixel that is
  // done by FakeAllocColor. 
  if (width * height) {
    os->writeU8(0);
    os->writeU8(0);
    os->writeU8(0);
    os->writeU8(255);
    os->writeU8(255);
    os->writeU8(255);
    os->writeBytes(data, (width+7)/8 * height);
    os->writeBytes(mask, (width+7)/8 * height);
  }
}

bool SMsgWriterV3::needFakeUpdate()
{
  return wsccb || needSetDesktopName || needNoDataUpdate();
}

bool SMsgWriterV3::needNoDataUpdate()
{
  return needSetDesktopSize || needExtendedDesktopSize ||
         !extendedDesktopSizeMsgs.empty();
}

void SMsgWriterV3::writeNoDataUpdate()
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

void SMsgWriterV3::writeFramebufferUpdateStart(int nRects)
{
  startMsg(msgTypeFramebufferUpdate);
  os->pad(1);

  if (nRects != 0xFFFF) {
    if (wsccb)
      nRects++;
    if (needSetDesktopName)
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

void SMsgWriterV3::writeFramebufferUpdateStart()
{
  nRectsInUpdate = nRectsInHeader = 0;

  if (!updateOS)
    updateOS = new rdr::MemOutStream;
  os = updateOS;

  writePseudoRects();
}

void SMsgWriterV3::writeFramebufferUpdateEnd()
{
  if (nRectsInUpdate != nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriterV3::writeFramebufferUpdateEnd: "
                    "nRects out of sync");

  if (nRectsInHeader == 0) {
    // Send last rect. marker
    os->writeS16(0);
    os->writeS16(0);
    os->writeU16(0);
    os->writeU16(0);
    os->writeU32(pseudoEncodingLastRect);
  }

  if (os == updateOS) {
    os = realOS;
    startMsg(msgTypeFramebufferUpdate);
    os->pad(1);
    os->writeU16(nRectsInUpdate);
    os->writeBytes(updateOS->data(), updateOS->length());
    updateOS->clear();
  }

  updatesSent++;
  endMsg();
}

void SMsgWriterV3::startRect(const Rect& r, int encoding)
{
  if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
    throw Exception("SMsgWriterV3::startRect: nRects out of sync");

  currentEncoding = encoding;
  lenBeforeRect = os->length();
  if (encoding != encodingCopyRect)
    rawBytesEquivalent += 12 + r.width() * r.height() * (bpp()/8);

  os->writeS16(r.tl.x);
  os->writeS16(r.tl.y);
  os->writeU16(r.width());
  os->writeU16(r.height());
  os->writeU32(encoding);
}

void SMsgWriterV3::endRect()
{
  if (currentEncoding <= encodingMax) {
    bytesSent[currentEncoding] += os->length() - lenBeforeRect;
    rectsSent[currentEncoding]++;
  }
}

void SMsgWriterV3::writePseudoRects()
{
  if (wsccb) {
    wsccb->writeSetCursorCallback();
    wsccb = 0;
  }

  if (needSetDesktopName) {
    if (!cp->supportsDesktopRename)
      throw Exception("Client does not support desktop rename");
    if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
      throw Exception("SMsgWriterV3 setDesktopName: nRects out of sync");

    os->writeS16(0);
    os->writeS16(0);
    os->writeU16(0);
    os->writeU16(0);
    os->writeU32(pseudoEncodingDesktopName);
    os->writeString(cp->name());

    needSetDesktopName = false;
  }
}

void SMsgWriterV3::writeNoDataRects()
{
  // Start with specific ExtendedDesktopSize messages
  if (!extendedDesktopSizeMsgs.empty()) {
    std::list<ExtendedDesktopSizeMsg>::const_iterator ri;
    ScreenSet::const_iterator si;

    if (!cp->supportsExtendedDesktopSize)
      throw Exception("Client does not support extended desktop resize");
    if ((nRectsInUpdate += extendedDesktopSizeMsgs.size()) > nRectsInHeader && nRectsInHeader)
      throw Exception("SMsgWriterV3 SetDesktopSize reply: nRects out of sync");

    for (ri = extendedDesktopSizeMsgs.begin();ri != extendedDesktopSizeMsgs.end();++ri) {
      os->writeU16(ri->reason);
      os->writeU16(ri->result);
      os->writeU16(ri->fb_width);
      os->writeU16(ri->fb_height);
      os->writeU32(pseudoEncodingExtendedDesktopSize);

      os->writeU8(ri->layout.num_screens());
      os->pad(3);

      for (si = ri->layout.begin();si != ri->layout.end();++si) {
        os->writeU32(si->id);
        os->writeU16(si->dimensions.tl.x);
        os->writeU16(si->dimensions.tl.y);
        os->writeU16(si->dimensions.width());
        os->writeU16(si->dimensions.height());
        os->writeU32(si->flags);
      }
    }

    extendedDesktopSizeMsgs.clear();
  }

  // Send this before SetDesktopSize to make life easier on the clients
  if (needExtendedDesktopSize) {
    if (!cp->supportsExtendedDesktopSize)
      throw Exception("Client does not support extended desktop resize");
    if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
      throw Exception("SMsgWriterV3 setExtendedDesktopSize: nRects out of sync");

    os->writeU16(0);
    os->writeU16(0);
    os->writeU16(cp->width);
    os->writeU16(cp->height);
    os->writeU32(pseudoEncodingExtendedDesktopSize);

    os->writeU8(cp->screenLayout.num_screens());
    os->pad(3);

    ScreenSet::const_iterator iter;
    for (iter = cp->screenLayout.begin();iter != cp->screenLayout.end();++iter) {
      os->writeU32(iter->id);
      os->writeU16(iter->dimensions.tl.x);
      os->writeU16(iter->dimensions.tl.y);
      os->writeU16(iter->dimensions.width());
      os->writeU16(iter->dimensions.height());
      os->writeU32(iter->flags);
    }

    needExtendedDesktopSize = false;
  }

  // Some clients assume this is the last rectangle so don't send anything
  // more after this
  if (needSetDesktopSize) {
    if (!cp->supportsDesktopResize)
      throw Exception("Client does not support desktop resize");
    if (++nRectsInUpdate > nRectsInHeader && nRectsInHeader)
      throw Exception("SMsgWriterV3 setDesktopSize: nRects out of sync");

    os->writeS16(0);
    os->writeS16(0);
    os->writeU16(cp->width);
    os->writeU16(cp->height);
    os->writeU32(pseudoEncodingDesktopSize);

    needSetDesktopSize = false;
  }
}

