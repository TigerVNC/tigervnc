/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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
  rdr::U32 encodings[encodingMax+2];
  if (cp->supportsLocalCursor)
    encodings[nEncodings++] = pseudoEncodingCursor;
  if (cp->supportsDesktopResize)
    encodings[nEncodings++] = pseudoEncodingDesktopSize;
  if (Decoder::supported(preferredEncoding)) {
    encodings[nEncodings++] = preferredEncoding;
  }
  if (useCopyRect) {
    encodings[nEncodings++] = encodingCopyRect;
  }
  for (int i = encodingMax; i >= 0; i--) {
    if (i != preferredEncoding && Decoder::supported(i)) {
      encodings[nEncodings++] = i;
    }
  }
  writeSetEncodings(nEncodings, encodings);
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


void CMsgWriter::writeKeyEvent(rdr::U32 key, bool down)
{
  startMsg(msgTypeKeyEvent);
  os->writeU8(down);
  os->pad(2);
  os->writeU32(key);
  endMsg();
}


void CMsgWriter::writePointerEvent(int x, int y, int buttonMask)
{
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  if (x >= cp->width) x = cp->width - 1;
  if (y >= cp->height) y = cp->height - 1;

  startMsg(msgTypePointerEvent);
  os->writeU8(buttonMask);
  os->writeU16(x);
  os->writeU16(y);
  endMsg();
}


void CMsgWriter::writeClientCutText(const char* str, int len)
{
  startMsg(msgTypeClientCutText);
  os->pad(3);
  os->writeU32(len);
  os->writeBytes(str, len);
  endMsg();
}

void CMsgWriter::setOutStream(rdr::OutStream* os_)
{
  os = os_;
}
