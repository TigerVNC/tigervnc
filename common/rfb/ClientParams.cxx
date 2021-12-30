/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2014-2019 Pierre Ossman for Cendio AB
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

#include <rfb/Exception.h>
#include <rfb/encodings.h>
#include <rfb/ledStates.h>
#include <rfb/clipboardTypes.h>
#include <rfb/ClientParams.h>

using namespace rfb;

ClientParams::ClientParams()
  : majorVersion(0), minorVersion(0),
    compressLevel(2), qualityLevel(-1), fineQualityLevel(-1),
    subsampling(subsampleUndefined),
    width_(0), height_(0), name_(0),
    cursorPos_(0, 0), ledState_(ledUnknown)
{
  setName("");

  cursor_ = new Cursor(0, 0, Point(), NULL);

  clipFlags = clipboardUTF8 | clipboardRTF | clipboardHTML |
              clipboardRequest | clipboardNotify | clipboardProvide;
  memset(clipSizes, 0, sizeof(clipSizes));
  clipSizes[0] = 20 * 1024 * 1024;
}

ClientParams::~ClientParams()
{
  delete [] name_;
  delete cursor_;
}

void ClientParams::setDimensions(int width, int height)
{
  ScreenSet layout;
  layout.add_screen(rfb::Screen(0, 0, 0, width, height, 0));
  setDimensions(width, height, layout);
}

void ClientParams::setDimensions(int width, int height, const ScreenSet& layout)
{
  if (!layout.validate(width, height))
    throw Exception("Attempted to configure an invalid screen layout");

  width_ = width;
  height_ = height;
  screenLayout_ = layout;
}

void ClientParams::setPF(const PixelFormat& pf)
{
  pf_ = pf;

  if (pf.bpp != 8 && pf.bpp != 16 && pf.bpp != 32)
    throw Exception("setPF: not 8, 16 or 32 bpp?");
}

void ClientParams::setName(const char* name)
{
  delete [] name_;
  name_ = strDup(name);
}

void ClientParams::setCursor(const Cursor& other)
{
  delete cursor_;
  cursor_ = new Cursor(other);
}

void ClientParams::setCursorPos(const Point& pos)
{
  cursorPos_ = pos;
}

bool ClientParams::supportsEncoding(rdr::S32 encoding) const
{
  return encodings_.count(encoding) != 0;
}

void ClientParams::setEncodings(int nEncodings, const rdr::S32* encodings)
{
  compressLevel = -1;
  qualityLevel = -1;
  fineQualityLevel = -1;
  subsampling = subsampleUndefined;

  encodings_.clear();
  encodings_.insert(encodingRaw);

  for (int i = nEncodings-1; i >= 0; i--) {
    switch (encodings[i]) {
    case pseudoEncodingSubsamp1X:
      subsampling = subsampleNone;
      break;
    case pseudoEncodingSubsampGray:
      subsampling = subsampleGray;
      break;
    case pseudoEncodingSubsamp2X:
      subsampling = subsample2X;
      break;
    case pseudoEncodingSubsamp4X:
      subsampling = subsample4X;
      break;
    case pseudoEncodingSubsamp8X:
      subsampling = subsample8X;
      break;
    case pseudoEncodingSubsamp16X:
      subsampling = subsample16X;
      break;
    }

    if (encodings[i] >= pseudoEncodingCompressLevel0 &&
        encodings[i] <= pseudoEncodingCompressLevel9)
      compressLevel = encodings[i] - pseudoEncodingCompressLevel0;

    if (encodings[i] >= pseudoEncodingQualityLevel0 &&
        encodings[i] <= pseudoEncodingQualityLevel9)
      qualityLevel = encodings[i] - pseudoEncodingQualityLevel0;

    if (encodings[i] >= pseudoEncodingFineQualityLevel0 &&
        encodings[i] <= pseudoEncodingFineQualityLevel100)
      fineQualityLevel = encodings[i] - pseudoEncodingFineQualityLevel0;

    encodings_.insert(encodings[i]);
  }
}

void ClientParams::setLEDState(unsigned int state)
{
  ledState_ = state;
}

rdr::U32 ClientParams::clipboardSize(unsigned int format) const
{
  int i;

  for (i = 0;i < 16;i++) {
    if (((unsigned)1 << i) == format)
      return clipSizes[i];
  }

  throw Exception("Invalid clipboard format 0x%x", format);
}

void ClientParams::setClipboardCaps(rdr::U32 flags, const rdr::U32* lengths)
{
  int i, num;

  clipFlags = flags;

  num = 0;
  for (i = 0;i < 16;i++) {
    if (!(flags & (1 << i)))
      continue;
    clipSizes[i] = lengths[num++];
  }
}

bool ClientParams::supportsLocalCursor() const
{
  if (supportsEncoding(pseudoEncodingCursorWithAlpha))
    return true;
  if (supportsEncoding(pseudoEncodingVMwareCursor))
    return true;
  if (supportsEncoding(pseudoEncodingCursor))
    return true;
  if (supportsEncoding(pseudoEncodingXCursor))
    return true;
  return false;
}

bool ClientParams::supportsCursorPosition() const
{
  if (supportsEncoding(pseudoEncodingVMwareCursorPosition))
    return true;
  return false;
}

bool ClientParams::supportsDesktopSize() const
{
  if (supportsEncoding(pseudoEncodingExtendedDesktopSize))
    return true;
  if (supportsEncoding(pseudoEncodingDesktopSize))
    return true;
  return false;
}

bool ClientParams::supportsLEDState() const
{
  if (supportsEncoding(pseudoEncodingLEDState))
    return true;
  if (supportsEncoding(pseudoEncodingVMwareLEDState))
    return true;
  return false;
}

bool ClientParams::supportsFence() const
{
  if (supportsEncoding(pseudoEncodingFence))
    return true;
  return false;
}

bool ClientParams::supportsContinuousUpdates() const
{
  if (supportsEncoding(pseudoEncodingContinuousUpdates))
    return true;
  return false;
}
