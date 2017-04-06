/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2017 Pierre Ossman for Cendio AB
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

#include <assert.h>
#include <stdio.h>

#include <rfb/msgTypes.h>
#include <rdr/InStream.h>
#include <rfb/Exception.h>
#include <rfb/util.h>
#include <rfb/CMsgHandler.h>
#include <rfb/CMsgReader.h>

using namespace rfb;

CMsgReader::CMsgReader(CMsgHandler* handler_, rdr::InStream* is_)
  : imageBufIdealSize(0), handler(handler_), is(is_),
    nUpdateRectsLeft(0)
{
}

CMsgReader::~CMsgReader()
{
}

void CMsgReader::readServerInit()
{
  int width = is->readU16();
  int height = is->readU16();
  handler->setDesktopSize(width, height);
  PixelFormat pf;
  pf.read(is);
  handler->setPixelFormat(pf);
  CharArray name(is->readString());
  handler->setName(name.buf);
  handler->serverInit();
}

void CMsgReader::readMsg()
{
  if (nUpdateRectsLeft == 0) {
    int type = is->readU8();

    switch (type) {
    case msgTypeSetColourMapEntries:
      readSetColourMapEntries();
      break;
    case msgTypeBell:
      readBell();
      break;
    case msgTypeServerCutText:
      readServerCutText();
      break;
    case msgTypeFramebufferUpdate:
      readFramebufferUpdate();
      break;
    case msgTypeServerFence:
      readFence();
      break;
    case msgTypeEndOfContinuousUpdates:
      readEndOfContinuousUpdates();
      break;
    default:
      fprintf(stderr, "unknown message type %d\n", type);
      throw Exception("unknown message type");
    }
  } else {
    int x = is->readU16();
    int y = is->readU16();
    int w = is->readU16();
    int h = is->readU16();
    int encoding = is->readS32();

    switch (encoding) {
    case pseudoEncodingLastRect:
      nUpdateRectsLeft = 1;     // this rectangle is the last one
      break;
    case pseudoEncodingXCursor:
      readSetXCursor(w, h, Point(x,y));
      break;
    case pseudoEncodingCursor:
      readSetCursor(w, h, Point(x,y));
      break;
    case pseudoEncodingCursorWithAlpha:
      readSetCursorWithAlpha(w, h, Point(x,y));
      break;
    case pseudoEncodingDesktopName:
      readSetDesktopName(x, y, w, h);
      break;
    case pseudoEncodingDesktopSize:
      handler->setDesktopSize(w, h);
      break;
    case pseudoEncodingExtendedDesktopSize:
      readExtendedDesktopSize(x, y, w, h);
      break;
    default:
      readRect(Rect(x, y, x+w, y+h), encoding);
      break;
    };

    nUpdateRectsLeft--;
    if (nUpdateRectsLeft == 0)
      handler->framebufferUpdateEnd();
  }
}

void CMsgReader::readSetColourMapEntries()
{
  is->skip(1);
  int firstColour = is->readU16();
  int nColours = is->readU16();
  rdr::U16Array rgbs(nColours * 3);
  for (int i = 0; i < nColours * 3; i++)
    rgbs.buf[i] = is->readU16();
  handler->setColourMapEntries(firstColour, nColours, rgbs.buf);
}

void CMsgReader::readBell()
{
  handler->bell();
}

void CMsgReader::readServerCutText()
{
  is->skip(3);
  rdr::U32 len = is->readU32();
  if (len > 256*1024) {
    is->skip(len);
    fprintf(stderr,"cut text too long (%d bytes) - ignoring\n",len);
    return;
  }
  CharArray ca(len+1);
  ca.buf[len] = 0;
  is->readBytes(ca.buf, len);
  handler->serverCutText(ca.buf, len);
}

void CMsgReader::readFence()
{
  rdr::U32 flags;
  rdr::U8 len;
  char data[64];

  is->skip(3);

  flags = is->readU32();

  len = is->readU8();
  if (len > sizeof(data)) {
    fprintf(stderr, "Ignoring fence with too large payload\n");
    is->skip(len);
    return;
  }

  is->readBytes(data, len);

  handler->fence(flags, len, data);
}

void CMsgReader::readEndOfContinuousUpdates()
{
  handler->endOfContinuousUpdates();
}

void CMsgReader::readFramebufferUpdate()
{
  is->skip(1);
  nUpdateRectsLeft = is->readU16();
  handler->framebufferUpdateStart();
}

void CMsgReader::readRect(const Rect& r, int encoding)
{
  if ((r.br.x > handler->cp.width) || (r.br.y > handler->cp.height)) {
    fprintf(stderr, "Rect too big: %dx%d at %d,%d exceeds %dx%d\n",
	    r.width(), r.height(), r.tl.x, r.tl.y,
            handler->cp.width, handler->cp.height);
    throw Exception("Rect too big");
  }

  if (r.is_empty())
    fprintf(stderr, "Warning: zero size rect\n");

  handler->dataRect(r, encoding);
}

void CMsgReader::readSetXCursor(int width, int height, const Point& hotspot)
{
  if (width > maxCursorSize || height > maxCursorSize)
    throw Exception("Too big cursor");

  rdr::U8 pr, pg, pb;
  rdr::U8 sr, sg, sb;
  int data_len = ((width+7)/8) * height;
  int mask_len = ((width+7)/8) * height;
  rdr::U8Array data(data_len);
  rdr::U8Array mask(mask_len);

  int x, y;
  rdr::U8 buf[width*height*4];
  rdr::U8* out;

  if (width * height) {
    pr = is->readU8();
    pg = is->readU8();
    pb = is->readU8();

    sr = is->readU8();
    sg = is->readU8();
    sb = is->readU8();

    is->readBytes(data.buf, data_len);
    is->readBytes(mask.buf, mask_len);
  }

  int maskBytesPerRow = (width+7)/8;
  out = buf;
  for (y = 0;y < height;y++) {
    for (x = 0;x < width;x++) {
      int byte = y * maskBytesPerRow + x / 8;
      int bit = 7 - x % 8;

      if (data.buf[byte] & (1 << bit)) {
        out[0] = pr;
        out[1] = pg;
        out[2] = pb;
      } else {
        out[0] = sr;
        out[1] = sg;
        out[2] = sb;
      }

      if (mask.buf[byte] & (1 << bit))
        out[3] = 255;
      else
        out[3] = 0;

      out += 4;
    }
  }

  handler->setCursor(width, height, hotspot, buf);
}

void CMsgReader::readSetCursor(int width, int height, const Point& hotspot)
{
  if (width > maxCursorSize || height > maxCursorSize)
    throw Exception("Too big cursor");

  int data_len = width * height * (handler->cp.pf().bpp/8);
  int mask_len = ((width+7)/8) * height;
  rdr::U8Array data(data_len);
  rdr::U8Array mask(mask_len);

  int x, y;
  rdr::U8 buf[width*height*4];
  rdr::U8* in;
  rdr::U8* out;

  is->readBytes(data.buf, data_len);
  is->readBytes(mask.buf, mask_len);

  int maskBytesPerRow = (width+7)/8;
  in = data.buf;
  out = buf;
  for (y = 0;y < height;y++) {
    for (x = 0;x < width;x++) {
      int byte = y * maskBytesPerRow + x / 8;
      int bit = 7 - x % 8;

      handler->cp.pf().rgbFromBuffer(out, in, 1);

      if (mask.buf[byte] & (1 << bit))
        out[3] = 255;
      else
        out[3] = 0;

      in += handler->cp.pf().bpp/8;
      out += 4;
    }
  }

  handler->setCursor(width, height, hotspot, buf);
}

void CMsgReader::readSetCursorWithAlpha(int width, int height, const Point& hotspot)
{
  if (width > maxCursorSize || height > maxCursorSize)
    throw Exception("Too big cursor");

  int encoding;

  const PixelFormat rgbaPF(32, 32, false, true, 255, 255, 255, 16, 8, 0);
  ManagedPixelBuffer pb(rgbaPF, width, height);
  PixelFormat origPF;

  rdr::U8* buf;
  int stride;

  encoding = is->readS32();

  origPF = handler->cp.pf();
  handler->cp.setPF(rgbaPF);
  handler->readAndDecodeRect(pb.getRect(), encoding, &pb);
  handler->cp.setPF(origPF);

  // On-wire data has pre-multiplied alpha, but we store it
  // non-pre-multiplied
  buf = pb.getBufferRW(pb.getRect(), &stride);
  assert(stride == width);

  for (int i = 0;i < pb.area();i++) {
    rdr::U8 alpha;

    alpha = buf[3];
    if (alpha == 0)
      alpha = 1; // Avoid division by zero

    buf[0] = (unsigned)buf[0] * 255/alpha;
    buf[1] = (unsigned)buf[1] * 255/alpha;
    buf[2] = (unsigned)buf[2] * 255/alpha;
    buf[3] = alpha;

    buf += 4;
  }

  pb.commitBufferRW(pb.getRect());

  handler->setCursor(width, height, hotspot,
                     pb.getBuffer(pb.getRect(), &stride));
}

void CMsgReader::readSetDesktopName(int x, int y, int w, int h)
{
  char* name = is->readString();

  if (x || y || w || h) {
    fprintf(stderr, "Ignoring DesktopName rect with non-zero position/size\n");
  } else {
    handler->setName(name);
  }

  delete [] name;
}

void CMsgReader::readExtendedDesktopSize(int x, int y, int w, int h)
{
  unsigned int screens, i;
  rdr::U32 id, flags;
  int sx, sy, sw, sh;
  ScreenSet layout;

  screens = is->readU8();
  is->skip(3);

  for (i = 0;i < screens;i++) {
    id = is->readU32();
    sx = is->readU16();
    sy = is->readU16();
    sw = is->readU16();
    sh = is->readU16();
    flags = is->readU32();

    layout.add_screen(Screen(id, sx, sy, sw, sh, flags));
  }

  handler->setExtendedDesktopSize(x, y, w, h, layout);
}
