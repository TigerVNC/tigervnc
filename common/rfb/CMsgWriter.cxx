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
#include <stdio.h>
#include <rdr/OutStream.h>
#include <rfb/msgTypes.h>
#include <rfb/fenceTypes.h>
#include <rfb/qemuTypes.h>
#include <rfb/Exception.h>
#include <rfb/PixelFormat.h>
#include <rfb/Rect.h>
#include <rfb/ServerParams.h>
#include <rfb/CMsgWriter.h>

using namespace rfb;

CMsgWriter::CMsgWriter(ServerParams* server_, rdr::OutStream* os_)
  : server(server_), os(os_)
{
}

CMsgWriter::~CMsgWriter()
{
}

void CMsgWriter::writeClientInit(bool shared)
{
  os->writeU8(shared);
  endMsg();
}

void CMsgWriter::writeSetPixelFormat(const PixelFormat& pf)
{
  startMsg(msgTypeSetPixelFormat);                                 
  os->pad(3);
  pf.write(os);
  endMsg();
}

void CMsgWriter::writeSetEncodings(const std::list<rdr::U32> encodings)
{
  std::list<rdr::U32>::const_iterator iter;
  startMsg(msgTypeSetEncodings);
  os->skip(1);
  os->writeU16(encodings.size());
  for (iter = encodings.begin(); iter != encodings.end(); ++iter)
    os->writeU32(*iter);
  endMsg();
}

void CMsgWriter::writeSetDesktopSize(int width, int height,
                                     const ScreenSet& layout)
{
  if (!server->supportsSetDesktopSize)
    throw Exception("Server does not support SetDesktopSize");

  startMsg(msgTypeSetDesktopSize);
  os->pad(1);

  os->writeU16(width);
  os->writeU16(height);

  os->writeU8(layout.num_screens());
  os->pad(1);

  ScreenSet::const_iterator iter;
  for (iter = layout.begin();iter != layout.end();++iter) {
    os->writeU32(iter->id);
    os->writeU16(iter->dimensions.tl.x);
    os->writeU16(iter->dimensions.tl.y);
    os->writeU16(iter->dimensions.width());
    os->writeU16(iter->dimensions.height());
    os->writeU32(iter->flags);
  }

  endMsg();
}

void CMsgWriter::writeFramebufferUpdateRequest(const Rect& r, bool incremental)
{
  startMsg(msgTypeFramebufferUpdateRequest);
  os->writeU8(incremental);
  os->writeU16(r.tl.x);
  os->writeU16(r.tl.y);
  os->writeU16(r.width());
  os->writeU16(r.height());
  endMsg();
}

void CMsgWriter::writeEnableContinuousUpdates(bool enable,
                                              int x, int y, int w, int h)
{
  if (!server->supportsContinuousUpdates)
    throw Exception("Server does not support continuous updates");

  startMsg(msgTypeEnableContinuousUpdates);

  os->writeU8(!!enable);

  os->writeU16(x);
  os->writeU16(y);
  os->writeU16(w);
  os->writeU16(h);

  endMsg();
}

void CMsgWriter::writeFence(rdr::U32 flags, unsigned len, const char data[])
{
  if (!server->supportsFence)
    throw Exception("Server does not support fences");
  if (len > 64)
    throw Exception("Too large fence payload");
  if ((flags & ~fenceFlagsSupported) != 0)
    throw Exception("Unknown fence flags");

  startMsg(msgTypeClientFence);
  os->pad(3);

  os->writeU32(flags);

  os->writeU8(len);
  os->writeBytes(data, len);

  endMsg();
}

void CMsgWriter::writeKeyEvent(rdr::U32 keysym, rdr::U32 keycode, bool down)
{
  if (!server->supportsQEMUKeyEvent || !keycode) {
    /* This event isn't meaningful without a valid keysym */
    if (!keysym)
      return;

    startMsg(msgTypeKeyEvent);
    os->writeU8(down);
    os->pad(2);
    os->writeU32(keysym);
    endMsg();
  } else {
    startMsg(msgTypeQEMUClientMessage);
    os->writeU8(qemuExtendedKeyEvent);
    os->writeU16(down);
    os->writeU32(keysym);
    os->writeU32(keycode);
    endMsg();
  }
}


void CMsgWriter::writePointerEvent(const Point& pos, int buttonMask)
{
  Point p(pos);
  if (p.x < 0) p.x = 0;
  if (p.y < 0) p.y = 0;
  if (p.x >= server->width()) p.x = server->width() - 1;
  if (p.y >= server->height()) p.y = server->height() - 1;

  startMsg(msgTypePointerEvent);
  os->writeU8(buttonMask);
  os->writeU16(p.x);
  os->writeU16(p.y);
  endMsg();
}


void CMsgWriter::writeClientCutText(const char* str, rdr::U32 len)
{
  startMsg(msgTypeClientCutText);
  os->pad(3);
  os->writeU32(len);
  os->writeBytes(str, len);
  endMsg();
}

void CMsgWriter::startMsg(int type)
{
  os->writeU8(type);
}

void CMsgWriter::endMsg()
{
  os->flush();
}
