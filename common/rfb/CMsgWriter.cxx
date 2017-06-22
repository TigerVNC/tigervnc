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
#include <rfb/encodings.h>
#include <rfb/Exception.h>
#include <rfb/PixelFormat.h>
#include <rfb/Rect.h>
#include <rfb/ConnParams.h>
#include <rfb/Decoder.h>
#include <rfb/CMsgWriter.h>

using namespace rfb;

CMsgWriter::CMsgWriter(ConnParams* cp_, rdr::OutStream* os_)
  : cp(cp_), os(os_)
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

void CMsgWriter::writeSetEncodings(int nEncodings, rdr::U32* encodings)
{
  startMsg(msgTypeSetEncodings);
  os->skip(1);
  os->writeU16(nEncodings);
  for (int i = 0; i < nEncodings; i++)
    os->writeU32(encodings[i]);
  endMsg();
}

// Ask for encodings based on which decoders are supported.  Assumes higher
// encoding numbers are more desirable.

void CMsgWriter::writeSetEncodings(int preferredEncoding, bool useCopyRect)
{
  int nEncodings = 0;
  rdr::U32 encodings[encodingMax+3];

  if (cp->supportsLocalCursor) {
    encodings[nEncodings++] = pseudoEncodingXCursor;
    encodings[nEncodings++] = pseudoEncodingCursor;
    encodings[nEncodings++] = pseudoEncodingCursorWithAlpha;
  }
  if (cp->supportsDesktopResize)
    encodings[nEncodings++] = pseudoEncodingDesktopSize;
  if (cp->supportsExtendedDesktopSize)
    encodings[nEncodings++] = pseudoEncodingExtendedDesktopSize;
  if (cp->supportsDesktopRename)
    encodings[nEncodings++] = pseudoEncodingDesktopName;

  encodings[nEncodings++] = pseudoEncodingLastRect;
  encodings[nEncodings++] = pseudoEncodingContinuousUpdates;
  encodings[nEncodings++] = pseudoEncodingFence;

  if (Decoder::supported(preferredEncoding)) {
    encodings[nEncodings++] = preferredEncoding;
  }

  if (useCopyRect) {
    encodings[nEncodings++] = encodingCopyRect;
  }

  /*
   * Prefer encodings in this order:
   *
   *   Tight, ZRLE, Hextile, *
   */

  if ((preferredEncoding != encodingTight) &&
      Decoder::supported(encodingTight))
    encodings[nEncodings++] = encodingTight;

  if ((preferredEncoding != encodingZRLE) &&
      Decoder::supported(encodingZRLE))
    encodings[nEncodings++] = encodingZRLE;

  if ((preferredEncoding != encodingHextile) &&
      Decoder::supported(encodingHextile))
    encodings[nEncodings++] = encodingHextile;

  // Remaining encodings
  for (int i = encodingMax; i >= 0; i--) {
    switch (i) {
    case encodingCopyRect:
    case encodingTight:
    case encodingZRLE:
    case encodingHextile:
      /* These have already been sent earlier */
      break;
    default:
      if ((i != preferredEncoding) && Decoder::supported(i))
        encodings[nEncodings++] = i;
    }
  }

  if (cp->compressLevel >= 0 && cp->compressLevel <= 9)
      encodings[nEncodings++] = pseudoEncodingCompressLevel0 + cp->compressLevel;
  if (cp->qualityLevel >= 0 && cp->qualityLevel <= 9)
      encodings[nEncodings++] = pseudoEncodingQualityLevel0 + cp->qualityLevel;

  writeSetEncodings(nEncodings, encodings);
}

void CMsgWriter::writeSetDesktopSize(int width, int height,
                                     const ScreenSet& layout)
{
  if (!cp->supportsSetDesktopSize)
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
  if (!cp->supportsContinuousUpdates)
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
  if (!cp->supportsFence)
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

void CMsgWriter::keyEvent(rdr::U32 key, bool down)
{
  startMsg(msgTypeKeyEvent);
  os->writeU8(down);
  os->pad(2);
  os->writeU32(key);
  endMsg();
}


void CMsgWriter::pointerEvent(const Point& pos, int buttonMask)
{
  Point p(pos);
  if (p.x < 0) p.x = 0;
  if (p.y < 0) p.y = 0;
  if (p.x >= cp->width) p.x = cp->width - 1;
  if (p.y >= cp->height) p.y = cp->height - 1;

  startMsg(msgTypePointerEvent);
  os->writeU8(buttonMask);
  os->writeU16(p.x);
  os->writeU16(p.y);
  endMsg();
}


void CMsgWriter::clientCutText(const char* str, int len)
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
