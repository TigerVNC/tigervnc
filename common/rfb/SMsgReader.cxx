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
#include <stdio.h>

#include <rdr/InStream.h>
#include <rdr/ZlibInStream.h>

#include <rfb/msgTypes.h>
#include <rfb/qemuTypes.h>
#include <rfb/clipboardTypes.h>
#include <rfb/Exception.h>
#include <rfb/util.h>
#include <rfb/SMsgHandler.h>
#include <rfb/SMsgReader.h>
#include <rfb/Configuration.h>
#include <rfb/LogWriter.h>

using namespace rfb;

static LogWriter vlog("SMsgReader");

static IntParameter maxCutText("MaxCutText", "Maximum permitted length of an incoming clipboard update", 256*1024);

SMsgReader::SMsgReader(SMsgHandler* handler_, rdr::InStream* is_)
  : handler(handler_), is(is_)
{
}

SMsgReader::~SMsgReader()
{
}

void SMsgReader::readClientInit()
{
  bool shared = is->readU8();
  handler->clientInit(shared);
}

void SMsgReader::readMsg()
{
  int msgType = is->readU8();
  switch (msgType) {
  case msgTypeSetPixelFormat:
    readSetPixelFormat();
    break;
  case msgTypeSetEncodings:
    readSetEncodings();
    break;
  case msgTypeSetDesktopSize:
    readSetDesktopSize();
    break;
  case msgTypeFramebufferUpdateRequest:
    readFramebufferUpdateRequest();
    break;
  case msgTypeEnableContinuousUpdates:
    readEnableContinuousUpdates();
    break;
  case msgTypeClientFence:
    readFence();
    break;
  case msgTypeKeyEvent:
    readKeyEvent();
    break;
  case msgTypePointerEvent:
    readPointerEvent();
    break;
  case msgTypeClientCutText:
    readClientCutText();
    break;
  case msgTypeQEMUClientMessage:
    readQEMUMessage();
    break;
  default:
    vlog.error("unknown message type %d", msgType);
    throw Exception("unknown message type");
  }
}

void SMsgReader::readSetPixelFormat()
{
  is->skip(3);
  PixelFormat pf;
  pf.read(is);
  handler->setPixelFormat(pf);
}

void SMsgReader::readSetEncodings()
{
  is->skip(1);
  int nEncodings = is->readU16();
  rdr::S32Array encodings(nEncodings);
  for (int i = 0; i < nEncodings; i++)
    encodings.buf[i] = is->readU32();
  handler->setEncodings(nEncodings, encodings.buf);
}

void SMsgReader::readSetDesktopSize()
{
  int width, height;
  int screens, i;
  rdr::U32 id, flags;
  int sx, sy, sw, sh;
  ScreenSet layout;

  is->skip(1);

  width = is->readU16();
  height = is->readU16();

  screens = is->readU8();
  is->skip(1);

  for (i = 0;i < screens;i++) {
    id = is->readU32();
    sx = is->readU16();
    sy = is->readU16();
    sw = is->readU16();
    sh = is->readU16();
    flags = is->readU32();

    layout.add_screen(Screen(id, sx, sy, sw, sh, flags));
  }

  handler->setDesktopSize(width, height, layout);
}

void SMsgReader::readFramebufferUpdateRequest()
{
  bool inc = is->readU8();
  int x = is->readU16();
  int y = is->readU16();
  int w = is->readU16();
  int h = is->readU16();
  handler->framebufferUpdateRequest(Rect(x, y, x+w, y+h), inc);
}

void SMsgReader::readEnableContinuousUpdates()
{
  bool enable;
  int x, y, w, h;

  enable = is->readU8();

  x = is->readU16();
  y = is->readU16();
  w = is->readU16();
  h = is->readU16();

  handler->enableContinuousUpdates(enable, x, y, w, h);
}

void SMsgReader::readFence()
{
  rdr::U32 flags;
  rdr::U8 len;
  char data[64];

  is->skip(3);

  flags = is->readU32();

  len = is->readU8();
  if (len > sizeof(data)) {
    vlog.error("Ignoring fence with too large payload");
    is->skip(len);
    return;
  }

  is->readBytes(data, len);
  
  handler->fence(flags, len, data);
}

void SMsgReader::readKeyEvent()
{
  bool down = is->readU8();
  is->skip(2);
  rdr::U32 key = is->readU32();
  handler->keyEvent(key, 0, down);
}

void SMsgReader::readPointerEvent()
{
  int mask = is->readU8();
  int x = is->readU16();
  int y = is->readU16();
  handler->pointerEvent(Point(x, y), mask);
}


void SMsgReader::readClientCutText()
{
  is->skip(3);
  rdr::U32 len = is->readU32();

  if (len & 0x80000000) {
    rdr::S32 slen = len;
    slen = -slen;
    readExtendedClipboard(slen);
    return;
  }

  if (len > (size_t)maxCutText) {
    is->skip(len);
    vlog.error("Cut text too long (%d bytes) - ignoring", len);
    return;
  }
  CharArray ca(len);
  is->readBytes(ca.buf, len);
  CharArray filtered(convertLF(ca.buf, len));
  handler->clientCutText(filtered.buf);
}

void SMsgReader::readExtendedClipboard(rdr::S32 len)
{
  rdr::U32 flags;
  rdr::U32 action;

  if (len < 4)
    throw Exception("Invalid extended clipboard message");
  if (len > maxCutText) {
    vlog.error("Extended clipboard message too long (%d bytes) - ignoring", len);
    is->skip(len);
    return;
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

      lengths[num] = zis.readU32();
      if (lengths[num] > (size_t)maxCutText) {
        vlog.error("Extended clipboard data too long (%d bytes) - ignoring",
                   (unsigned)lengths[num]);
        zis.skip(lengths[num]);
        flags &= ~(1 << i);
        continue;
      }

      buffers[num] = new rdr::U8[lengths[num]];
      zis.readBytes(buffers[num], lengths[num]);
      num++;
    }

    zis.removeUnderlying();

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
}

void SMsgReader::readQEMUMessage()
{
  int subType = is->readU8();
  switch (subType) {
  case qemuExtendedKeyEvent:
    readQEMUKeyEvent();
    break;
  default:
    throw Exception("unknown QEMU submessage type %d", subType);
  }
}

void SMsgReader::readQEMUKeyEvent()
{
  bool down = is->readU16();
  rdr::U32 keysym = is->readU32();
  rdr::U32 keycode = is->readU32();
  if (!keycode) {
    vlog.error("Key event without keycode - ignoring");
    return;
  }
  handler->keyEvent(keysym, keycode, down);
}
