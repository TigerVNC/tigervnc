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
  : handler(handler_), is(is_), state(MSGSTATE_IDLE)
{
}

SMsgReader::~SMsgReader()
{
}

bool SMsgReader::readClientInit()
{
  if (!is->hasData(1))
    return false;
  bool shared = is->readU8();
  handler->clientInit(shared);
  return true;
}

bool SMsgReader::readMsg()
{
  bool ret;

  if (state == MSGSTATE_IDLE) {
    if (!is->hasData(1))
      return false;

    currentMsgType = is->readU8();
    state = MSGSTATE_MESSAGE;
  }

  switch (currentMsgType) {
  case msgTypeSetPixelFormat:
    ret = readSetPixelFormat();
    break;
  case msgTypeSetEncodings:
    ret = readSetEncodings();
    break;
  case msgTypeSetDesktopSize:
    ret = readSetDesktopSize();
    break;
  case msgTypeFramebufferUpdateRequest:
    ret = readFramebufferUpdateRequest();
    break;
  case msgTypeEnableContinuousUpdates:
    ret = readEnableContinuousUpdates();
    break;
  case msgTypeClientFence:
    ret = readFence();
    break;
  case msgTypeKeyEvent:
    ret = readKeyEvent();
    break;
  case msgTypePointerEvent:
    ret = readPointerEvent();
    break;
  case msgTypeClientCutText:
    ret = readClientCutText();
    break;
  case msgTypeQEMUClientMessage:
    ret = readQEMUMessage();
    break;
  default:
    vlog.error("unknown message type %d", currentMsgType);
    throw Exception("unknown message type");
  }

  if (ret)
    state = MSGSTATE_IDLE;

  return ret;
}

bool SMsgReader::readSetPixelFormat()
{
  if (!is->hasData(3 + 16))
    return false;
  is->skip(3);
  PixelFormat pf;
  pf.read(is);
  handler->setPixelFormat(pf);
  return true;
}

bool SMsgReader::readSetEncodings()
{
  if (!is->hasData(1 + 2))
    return false;

  is->setRestorePoint();

  is->skip(1);

  int nEncodings = is->readU16();

  if (!is->hasDataOrRestore(nEncodings * 4))
    return false;
  is->clearRestorePoint();

  rdr::S32Array encodings(nEncodings);
  for (int i = 0; i < nEncodings; i++)
    encodings.buf[i] = is->readU32();

  handler->setEncodings(nEncodings, encodings.buf);

  return true;
}

bool SMsgReader::readSetDesktopSize()
{
  int width, height;
  int screens, i;
  rdr::U32 id, flags;
  int sx, sy, sw, sh;
  ScreenSet layout;

  if (!is->hasData(1 + 2 + 2 + 1 + 1))
    return true;

  is->setRestorePoint();

  is->skip(1);

  width = is->readU16();
  height = is->readU16();

  screens = is->readU8();
  is->skip(1);

  if (!is->hasDataOrRestore(screens * (4 + 2 + 2 + 2 + 2 + 4)))
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

  handler->setDesktopSize(width, height, layout);

  return true;
}

bool SMsgReader::readFramebufferUpdateRequest()
{
  if (!is->hasData(1 + 2 + 2 + 2 + 2))
    return false;
  bool inc = is->readU8();
  int x = is->readU16();
  int y = is->readU16();
  int w = is->readU16();
  int h = is->readU16();
  handler->framebufferUpdateRequest(Rect(x, y, x+w, y+h), inc);
  return true;
}

bool SMsgReader::readEnableContinuousUpdates()
{
  bool enable;
  int x, y, w, h;

  if (!is->hasData(1 + 2 + 2 + 2 + 2))
    return false;

  enable = is->readU8();

  x = is->readU16();
  y = is->readU16();
  w = is->readU16();
  h = is->readU16();

  handler->enableContinuousUpdates(enable, x, y, w, h);

  return true;
}

bool SMsgReader::readFence()
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

bool SMsgReader::readKeyEvent()
{
  if (!is->hasData(1 + 2 + 4))
    return false;
  bool down = is->readU8();
  is->skip(2);
  rdr::U32 key = is->readU32();
  handler->keyEvent(key, 0, down);
  return true;
}

bool SMsgReader::readPointerEvent()
{
  if (!is->hasData(1 + 2 + 2))
    return false;
  int mask = is->readU8();
  int x = is->readU16();
  int y = is->readU16();
  handler->pointerEvent(Point(x, y), mask);
  return true;
}


bool SMsgReader::readClientCutText()
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
    vlog.error("Cut text too long (%d bytes) - ignoring", len);
    return true;
  }

  CharArray ca(len);
  is->readBytes(ca.buf, len);
  CharArray filtered(convertLF(ca.buf, len));
  handler->clientCutText(filtered.buf);

  return true;
}

bool SMsgReader::readExtendedClipboard(rdr::S32 len)
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

bool SMsgReader::readQEMUMessage()
{
  int subType;
  bool ret;

  if (!is->hasData(1))
    return false;

  is->setRestorePoint();

  subType = is->readU8();

  switch (subType) {
  case qemuExtendedKeyEvent:
    ret = readQEMUKeyEvent();
    break;
  default:
    throw Exception("unknown QEMU submessage type %d", subType);
  }

  if (!ret) {
    is->gotoRestorePoint();
    return false;
  } else {
    is->clearRestorePoint();
    return true;
  }
}

bool SMsgReader::readQEMUKeyEvent()
{
  if (!is->hasData(2 + 4 + 4))
    return false;
  bool down = is->readU16();
  rdr::U32 keysym = is->readU32();
  rdr::U32 keycode = is->readU32();
  if (!keycode) {
    vlog.error("Key event without keycode - ignoring");
    return true;
  }
  handler->keyEvent(keysym, keycode, down);
  return true;
}
