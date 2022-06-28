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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <stdio.h>

#include <rdr/InStream.h>
#include <rdr/ZlibInStream.h>

#include <rfb/msgTypes.h>
#include <rfb/clipboardTypes.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>
#include <rfb/util.h>
#include <rfb/CMsgHandler.h>
#include <rfb/CMsgReader.h>

static rfb::LogWriter vlog("CMsgReader");

static rfb::IntParameter maxCutText("MaxCutText", "Maximum permitted length of an incoming clipboard update", 256*1024);

using namespace rfb;

CMsgReader::CMsgReader(CMsgHandler* handler_, rdr::InStream* is_)
  : imageBufIdealSize(0), handler(handler_), is(is_),
    state(MSGSTATE_IDLE), cursorEncoding(-1)
{
}

CMsgReader::~CMsgReader()
{
}

bool CMsgReader::readServerInit()
{
  int width, height;
  rdr::U32 len;

  if (!is->hasData(2 + 2 + 16 + 4))
    return false;

  is->setRestorePoint();

  width = is->readU16();
  height = is->readU16();

  PixelFormat pf;
  pf.read(is);

  len = is->readU32();
  if (!is->hasDataOrRestore(len))
    return false;
  is->clearRestorePoint();
  CharArray name(len + 1);
  is->readBytes(name.buf, len);
  name.buf[len] = '\0';
  handler->serverInit(width, height, pf, name.buf);

  return true;
}

bool CMsgReader::readMsg()
{
  if (state == MSGSTATE_IDLE) {
    if (!is->hasData(1))
      return false;

    currentMsgType = is->readU8();
    state = MSGSTATE_MESSAGE;
  }

  if (currentMsgType != msgTypeFramebufferUpdate) {
    bool ret;

    switch (currentMsgType) {
    case msgTypeSetColourMapEntries:
      ret = readSetColourMapEntries();
      break;
    case msgTypeBell:
      ret = readBell();
      break;
    case msgTypeServerCutText:
      ret = readServerCutText();
      break;
    case msgTypeFramebufferUpdate:
      ret = readFramebufferUpdate();
      break;
    case msgTypeServerFence:
      ret = readFence();
      break;
    case msgTypeEndOfContinuousUpdates:
      ret = readEndOfContinuousUpdates();
      break;
    default:
      throw Exception("Unknown message type %d", currentMsgType);
    }

    if (ret)
      state = MSGSTATE_IDLE;

    return ret;
  } else {
    if (state == MSGSTATE_MESSAGE) {
      if (!readFramebufferUpdate())
        return false;

      // Empty update?
      if (nUpdateRectsLeft == 0) {
        state = MSGSTATE_IDLE;
        handler->framebufferUpdateEnd();
        return true;
      }

      state = MSGSTATE_RECT_HEADER;
    }

    if (state == MSGSTATE_RECT_HEADER) {
      if (!is->hasData(2 + 2 + 2 + 2 + 4))
        return false;

      int x = is->readU16();
      int y = is->readU16();
      int w = is->readU16();
      int h = is->readU16();

      dataRect.setXYWH(x, y, w, h);

      rectEncoding = is->readS32();

      state = MSGSTATE_RECT_DATA;
    }

    bool ret;

    switch (rectEncoding) {
    case pseudoEncodingLastRect:
      nUpdateRectsLeft = 1;     // this rectangle is the last one
      ret = true;
      break;
    case pseudoEncodingXCursor:
      ret = readSetXCursor(dataRect.width(), dataRect.height(), dataRect.tl);
      break;
    case pseudoEncodingCursor:
      ret = readSetCursor(dataRect.width(), dataRect.height(), dataRect.tl);
      break;
    case pseudoEncodingCursorWithAlpha:
      ret = readSetCursorWithAlpha(dataRect.width(), dataRect.height(), dataRect.tl);
      break;
    case pseudoEncodingVMwareCursor:
      ret = readSetVMwareCursor(dataRect.width(), dataRect.height(), dataRect.tl);
      break;
    case pseudoEncodingVMwareCursorPosition:
      handler->setCursorPos(dataRect.tl);
      ret = true;
      break;
    case pseudoEncodingDesktopName:
      ret = readSetDesktopName(dataRect.tl.x, dataRect.tl.y,
                               dataRect.width(), dataRect.height());
      break;
    case pseudoEncodingDesktopSize:
      handler->setDesktopSize(dataRect.width(), dataRect.height());
      ret = true;
      break;
    case pseudoEncodingExtendedDesktopSize:
      ret = readExtendedDesktopSize(dataRect.tl.x, dataRect.tl.y,
                                    dataRect.width(), dataRect.height());
      break;
    case pseudoEncodingLEDState:
      ret = readLEDState();
      break;
    case pseudoEncodingVMwareLEDState:
      ret = readVMwareLEDState();
      break;
    case pseudoEncodingQEMUKeyEvent:
      handler->supportsQEMUKeyEvent();
      ret = true;
      break;
    default:
      ret = readRect(dataRect, rectEncoding);
      break;
    };

    if (ret) {
      state = MSGSTATE_RECT_HEADER;
      nUpdateRectsLeft--;
      if (nUpdateRectsLeft == 0) {
        state = MSGSTATE_IDLE;
        handler->framebufferUpdateEnd();
      }
    }

    return ret;
  }
}

bool CMsgReader::readSetColourMapEntries()
{
  if (!is->hasData(1 + 2 + 2))
    return false;

  is->setRestorePoint();

  is->skip(1);
  int firstColour = is->readU16();
  int nColours = is->readU16();

  if (!is->hasDataOrRestore(nColours * 3 * 2))
    return false;
  is->clearRestorePoint();

  rdr::U16Array rgbs(nColours * 3);
  for (int i = 0; i < nColours * 3; i++)
    rgbs.buf[i] = is->readU16();
  handler->setColourMapEntries(firstColour, nColours, rgbs.buf);

  return true;
}

bool CMsgReader::readBell()
{
  handler->bell();
  return true;
}

bool CMsgReader::readServerCutText()
{
  if (!is->hasData(3 + 4))
    return false;

  is->setRestorePoint();

  is->skip(3);
  rdr::U32 len = is->readU32();

  if (len & 0x80000000) {
    rdr::S32 slen = len;
    slen = -slen;
    if (readExtendedClipboard(slen)) {
      is->clearRestorePoint();
      return true;
    } else {
      is->gotoRestorePoint();
      return false;
    }
  }

  if (!is->hasDataOrRestore(len))
    return false;
  is->clearRestorePoint();

  if (len > (size_t)maxCutText) {
    is->skip(len);
    vlog.error("cut text too long (%d bytes) - ignoring",len);
    return true;
  }
  CharArray ca(len);
  is->readBytes(ca.buf, len);
  CharArray filtered(convertLF(ca.buf, len));
  handler->serverCutText(filtered.buf);

  return true;
}

bool CMsgReader::readExtendedClipboard(rdr::S32 len)
{
  rdr::U32 flags;
  rdr::U32 action;

  if (!is->hasData(len))
    return false;

  if (len < 4)
    throw Exception("Invalid extended clipboard message");
  if (len > maxCutText) {
    vlog.error("Extended clipboard message too long (%d bytes) - ignoring", len);
    is->skip(len);
    return true;
  }

  flags = is->readU32();
  action = flags & clipboardActionMask;

  if (action & clipboardCaps) {
    int i;
    size_t num;
    rdr::U32 lengths[16];

    num = 0;
    for (i = 0;i < 16;i++) {
      if (flags & (1 << i))
        num++;
    }

    if (len < (rdr::S32)(4 + 4*num))
      throw Exception("Invalid extended clipboard message");

    num = 0;
    for (i = 0;i < 16;i++) {
      if (flags & (1 << i))
        lengths[num++] = is->readU32();
    }

    handler->handleClipboardCaps(flags, lengths);
  } else if (action == clipboardProvide) {
    rdr::ZlibInStream zis;

    int i;
    size_t num;
    size_t lengths[16];
    rdr::U8* buffers[16];

    zis.setUnderlying(is, len - 4);

    num = 0;
    for (i = 0;i < 16;i++) {
      if (!(flags & 1 << i))
        continue;

      if (!zis.hasData(4))
        throw Exception("Extended clipboard decode error");

      lengths[num] = zis.readU32();

      if (lengths[num] > (size_t)maxCutText) {
        vlog.error("Extended clipboard data too long (%d bytes) - ignoring",
                   (unsigned)lengths[num]);

        // Slowly (safely) drain away the data
        while (lengths[num] > 0) {
          size_t chunk;

          if (!zis.hasData(1))
            throw Exception("Extended clipboard decode error");

          chunk = zis.avail();
          if (chunk > lengths[num])
            chunk = lengths[num];

          zis.skip(chunk);
          lengths[num] -= chunk;
        }

        flags &= ~(1 << i);

        continue;
      }

      if (!zis.hasData(lengths[num]))
        throw Exception("Extended clipboard decode error");

      buffers[num] = new rdr::U8[lengths[num]];
      zis.readBytes(buffers[num], lengths[num]);
      num++;
    }

    zis.flushUnderlying();
    zis.setUnderlying(NULL, 0);

    handler->handleClipboardProvide(flags, lengths, buffers);

    num = 0;
    for (i = 0;i < 16;i++) {
      if (!(flags & 1 << i))
        continue;
      delete [] buffers[num++];
    }
  } else {
    switch (action) {
    case clipboardRequest:
      handler->handleClipboardRequest(flags);
      break;
    case clipboardPeek:
      handler->handleClipboardPeek(flags);
      break;
    case clipboardNotify:
      handler->handleClipboardNotify(flags);
      break;
    default:
      throw Exception("Invalid extended clipboard action");
    }
  }

  return true;
}

bool CMsgReader::readFence()
{
  rdr::U32 flags;
  rdr::U8 len;
  char data[64];

  if (!is->hasData(3 + 4 + 1))
    return false;

  is->setRestorePoint();

  is->skip(3);

  flags = is->readU32();

  len = is->readU8();

  if (!is->hasDataOrRestore(len))
    return false;
  is->clearRestorePoint();

  if (len > sizeof(data)) {
    vlog.error("Ignoring fence with too large payload");
    is->skip(len);
    return true;
  }

  is->readBytes(data, len);

  handler->fence(flags, len, data);

  return true;
}

bool CMsgReader::readEndOfContinuousUpdates()
{
  handler->endOfContinuousUpdates();
  return true;
}

bool CMsgReader::readFramebufferUpdate()
{
  if (!is->hasData(1 + 2))
    return false;

  is->skip(1);
  nUpdateRectsLeft = is->readU16();
  handler->framebufferUpdateStart();

  return true;
}

bool CMsgReader::readRect(const Rect& r, int encoding)
{
  if ((r.br.x > handler->server.width()) ||
      (r.br.y > handler->server.height())) {
    vlog.error("Rect too big: %dx%d at %d,%d exceeds %dx%d",
	    r.width(), r.height(), r.tl.x, r.tl.y,
            handler->server.width(), handler->server.height());
    throw Exception("Rect too big");
  }

  if (r.is_empty())
    vlog.error("zero size rect");

  return handler->dataRect(r, encoding);
}

bool CMsgReader::readSetXCursor(int width, int height, const Point& hotspot)
{
  if (width > maxCursorSize || height > maxCursorSize)
    throw Exception("Too big cursor");

  rdr::U8Array rgba(width*height*4);

  if (width * height > 0) {
    rdr::U8 pr, pg, pb;
    rdr::U8 sr, sg, sb;
    int data_len = ((width+7)/8) * height;
    int mask_len = ((width+7)/8) * height;
    rdr::U8Array data(data_len);
    rdr::U8Array mask(mask_len);

    int x, y;
    rdr::U8* out;

    if (!is->hasData(3 + 3 + data_len + mask_len))
      return false;

    pr = is->readU8();
    pg = is->readU8();
    pb = is->readU8();

    sr = is->readU8();
    sg = is->readU8();
    sb = is->readU8();

    is->readBytes(data.buf, data_len);
    is->readBytes(mask.buf, mask_len);

    int maskBytesPerRow = (width+7)/8;
    out = rgba.buf;
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
  }

  handler->setCursor(width, height, hotspot, rgba.buf);

  return true;
}

bool CMsgReader::readSetCursor(int width, int height, const Point& hotspot)
{
  if (width > maxCursorSize || height > maxCursorSize)
    throw Exception("Too big cursor");

  int data_len = width * height * (handler->server.pf().bpp/8);
  int mask_len = ((width+7)/8) * height;
  rdr::U8Array data(data_len);
  rdr::U8Array mask(mask_len);

  int x, y;
  rdr::U8Array rgba(width*height*4);
  rdr::U8* in;
  rdr::U8* out;

  if (!is->hasData(data_len + mask_len))
    return false;

  is->readBytes(data.buf, data_len);
  is->readBytes(mask.buf, mask_len);

  int maskBytesPerRow = (width+7)/8;
  in = data.buf;
  out = rgba.buf;
  for (y = 0;y < height;y++) {
    for (x = 0;x < width;x++) {
      int byte = y * maskBytesPerRow + x / 8;
      int bit = 7 - x % 8;

      handler->server.pf().rgbFromBuffer(out, in, 1);

      if (mask.buf[byte] & (1 << bit))
        out[3] = 255;
      else
        out[3] = 0;

      in += handler->server.pf().bpp/8;
      out += 4;
    }
  }

  handler->setCursor(width, height, hotspot, rgba.buf);

  return true;
}

bool CMsgReader::readSetCursorWithAlpha(int width, int height, const Point& hotspot)
{
  if (width > maxCursorSize || height > maxCursorSize)
    throw Exception("Too big cursor");

  const PixelFormat rgbaPF(32, 32, false, true, 255, 255, 255, 16, 8, 0);
  ManagedPixelBuffer pb(rgbaPF, width, height);
  PixelFormat origPF;

  bool ret;

  rdr::U8* buf;
  int stride;

  // We can't use restore points as the decoder likely wants to as well, so
  // we need to keep track of the read encoding

  if (cursorEncoding == -1) {
    if (!is->hasData(4))
      return false;

    cursorEncoding = is->readS32();
  }

  origPF = handler->server.pf();
  handler->server.setPF(rgbaPF);
  ret = handler->readAndDecodeRect(pb.getRect(), cursorEncoding, &pb);
  handler->server.setPF(origPF);

  if (!ret)
    return false;

  cursorEncoding = -1;

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

    buf += 4;
  }

  pb.commitBufferRW(pb.getRect());

  handler->setCursor(width, height, hotspot,
                     pb.getBuffer(pb.getRect(), &stride));

  return true;
}

bool CMsgReader::readSetVMwareCursor(int width, int height, const Point& hotspot)
{
  if (width > maxCursorSize || height > maxCursorSize)
    throw Exception("Too big cursor");

  rdr::U8 type;

  if (!is->hasData(1 + 1))
    return false;

  is->setRestorePoint();

  type = is->readU8();
  is->skip(1);

  if (type == 0) {
    int len = width * height * (handler->server.pf().bpp/8);
    rdr::U8Array andMask(len);
    rdr::U8Array xorMask(len);

    rdr::U8Array data(width*height*4);

    rdr::U8* andIn;
    rdr::U8* xorIn;
    rdr::U8* out;
    int Bpp;

    if (!is->hasDataOrRestore(len + len))
      return false;
    is->clearRestorePoint();

    is->readBytes(andMask.buf, len);
    is->readBytes(xorMask.buf, len);

    andIn = andMask.buf;
    xorIn = xorMask.buf;
    out = data.buf;
    Bpp = handler->server.pf().bpp/8;
    for (int y = 0;y < height;y++) {
      for (int x = 0;x < width;x++) {
        Pixel andPixel, xorPixel;

        andPixel = handler->server.pf().pixelFromBuffer(andIn);
        xorPixel = handler->server.pf().pixelFromBuffer(xorIn);
        andIn += Bpp;
        xorIn += Bpp;

        if (andPixel == 0) {
          rdr::U8 r, g, b;

          // Opaque pixel

          handler->server.pf().rgbFromPixel(xorPixel, &r, &g, &b);
          *out++ = r;
          *out++ = g;
          *out++ = b;
          *out++ = 0xff;
        } else if (xorPixel == 0) {
          // Fully transparent pixel
          *out++ = 0;
          *out++ = 0;
          *out++ = 0;
          *out++ = 0;
        } else if (andPixel == xorPixel) {
          // Inverted pixel

          // We don't really support this, so just turn the pixel black
          // FIXME: Do an outline like WinVNC does?
          *out++ = 0;
          *out++ = 0;
          *out++ = 0;
          *out++ = 0xff;
        } else {
          // Partially transparent/inverted pixel

          // We _really_ can't handle this, just make it black
          *out++ = 0;
          *out++ = 0;
          *out++ = 0;
          *out++ = 0xff;
        }
      }
    }

    handler->setCursor(width, height, hotspot, data.buf);
  } else if (type == 1) {
    rdr::U8Array data(width*height*4);

    if (!is->hasDataOrRestore(width*height*4))
      return false;
    is->clearRestorePoint();

    // FIXME: Is alpha premultiplied?
    is->readBytes(data.buf, width*height*4);

    handler->setCursor(width, height, hotspot, data.buf);
  } else {
    throw Exception("Unknown cursor type");
  }

  return true;
}

bool CMsgReader::readSetDesktopName(int x, int y, int w, int h)
{
  rdr::U32 len;

  if (!is->hasData(4))
    return false;

  is->setRestorePoint();

  len = is->readU32();

  if (!is->hasDataOrRestore(len))
    return false;
  is->clearRestorePoint();

  CharArray name(len + 1);
  is->readBytes(name.buf, len);
  name.buf[len] = '\0';

  if (x || y || w || h) {
    vlog.error("Ignoring DesktopName rect with non-zero position/size");
  } else {
    handler->setName(name.buf);
  }

  return true;
}

bool CMsgReader::readExtendedDesktopSize(int x, int y, int w, int h)
{
  unsigned int screens, i;
  rdr::U32 id, flags;
  int sx, sy, sw, sh;
  ScreenSet layout;

  if (!is->hasData(1 + 3))
    return false;

  is->setRestorePoint();

  screens = is->readU8();
  is->skip(3);

  if (!is->hasDataOrRestore(16 * screens))
    return false;
  is->clearRestorePoint();

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

  return true;
}

bool CMsgReader::readLEDState()
{
  rdr::U8 state;

  if (!is->hasData(1))
    return false;

  state = is->readU8();

  handler->setLEDState(state);

  return true;
}

bool CMsgReader::readVMwareLEDState()
{
  rdr::U32 state;

  if (!is->hasData(4))
    return false;

  state = is->readU32();

  // As luck has it, this extension uses the same bit definitions,
  // so no conversion required

  handler->setLEDState(state);

  return true;
}
