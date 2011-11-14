/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2011 Pierre Ossman for Cendio AB
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
#include <rfb/PixelFormat.h>
#include <rfb/msgTypes.h>
#include <rfb/Exception.h>
#include <rdr/InStream.h>
#include <rfb/CMsgReaderV3.h>
#include <rfb/CMsgHandler.h>
#include <rfb/util.h>
#include <rfb/ScreenSet.h>
#include <stdio.h>

using namespace rfb;

CMsgReaderV3::CMsgReaderV3(CMsgHandler* handler, rdr::InStream* is)
  : CMsgReader(handler, is), nUpdateRectsLeft(0)
{
}

CMsgReaderV3::~CMsgReaderV3()
{
}

void CMsgReaderV3::readServerInit()
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

void CMsgReaderV3::readMsg()
{
  if (nUpdateRectsLeft == 0) {

    int type = is->readU8();
    switch (type) {
    case msgTypeFramebufferUpdate:   readFramebufferUpdate(); break;
    case msgTypeSetColourMapEntries: readSetColourMapEntries(); break;
    case msgTypeBell:                readBell(); break;
    case msgTypeServerCutText:       readServerCutText(); break;
    case msgTypeServerFence:         readFence(); break;

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
    case pseudoEncodingDesktopSize:
      handler->setDesktopSize(w, h);
      break;
    case pseudoEncodingExtendedDesktopSize:
      readExtendedDesktopSize(x, y, w, h);
      break;
    case pseudoEncodingDesktopName:
      readSetDesktopName(x, y, w, h);
      break;
    case pseudoEncodingCursor:
      readSetCursor(w, h, Point(x,y));
      break;
    case pseudoEncodingLastRect:
      nUpdateRectsLeft = 1;     // this rectangle is the last one
      break;
    default:
      readRect(Rect(x, y, x+w, y+h), encoding);
      break;
    };

    nUpdateRectsLeft--;
    if (nUpdateRectsLeft == 0) handler->framebufferUpdateEnd();
  }
}

void CMsgReaderV3::readFramebufferUpdate()
{
  is->skip(1);
  nUpdateRectsLeft = is->readU16();
  handler->framebufferUpdateStart();
}

void CMsgReaderV3::readSetDesktopName(int x, int y, int w, int h)
{
  char* name = is->readString();

  if (x || y || w || h) {
    fprintf(stderr, "Ignoring DesktopName rect with non-zero position/size\n");
  } else {
    handler->setName(name);
  }

  delete [] name;
}

void CMsgReaderV3::readExtendedDesktopSize(int x, int y, int w, int h)
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

void CMsgReaderV3::readFence()
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
