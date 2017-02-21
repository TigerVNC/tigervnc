/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2014 Pierre Ossman for Cendio AB
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
#include <rdr/OutStream.h>
#include <rfb/Exception.h>
#include <rfb/encodings.h>
#include <rfb/ConnParams.h>
#include <rfb/util.h>

using namespace rfb;

ConnParams::ConnParams()
  : majorVersion(0), minorVersion(0),
    width(0), height(0), useCopyRect(false),
    supportsLocalCursor(false), supportsLocalXCursor(false),
    supportsLocalCursorWithAlpha(false),
    supportsDesktopResize(false), supportsExtendedDesktopSize(false),
    supportsDesktopRename(false), supportsLastRect(false),
    supportsSetDesktopSize(false), supportsFence(false),
    supportsContinuousUpdates(false),
    compressLevel(2), qualityLevel(-1), fineQualityLevel(-1),
    subsampling(subsampleUndefined), name_(0), verStrPos(0)
{
  setName("");
  cursor_ = new Cursor(0, 0, Point(), NULL);
}

ConnParams::~ConnParams()
{
  delete [] name_;
  delete cursor_;
}

bool ConnParams::readVersion(rdr::InStream* is, bool* done)
{
  if (verStrPos >= 12) return false;
  while (is->checkNoWait(1) && verStrPos < 12) {
    verStr[verStrPos++] = is->readU8();
  }

  if (verStrPos < 12) {
    *done = false;
    return true;
  }
  *done = true;
  verStr[12] = 0;
  return (sscanf(verStr, "RFB %03d.%03d\n", &majorVersion,&minorVersion) == 2);
}

void ConnParams::writeVersion(rdr::OutStream* os)
{
  char str[13];
  sprintf(str, "RFB %03d.%03d\n", majorVersion, minorVersion);
  os->writeBytes(str, 12);
  os->flush();
}

void ConnParams::setPF(const PixelFormat& pf)
{
  pf_ = pf;

  if (pf.bpp != 8 && pf.bpp != 16 && pf.bpp != 32)
    throw Exception("setPF: not 8, 16 or 32 bpp?");
}

void ConnParams::setName(const char* name)
{
  delete [] name_;
  name_ = strDup(name);
}

void ConnParams::setCursor(const Cursor& other)
{
  delete cursor_;
  cursor_ = new Cursor(other);
}

bool ConnParams::supportsEncoding(rdr::S32 encoding) const
{
  return encodings_.count(encoding) != 0;
}

void ConnParams::setEncodings(int nEncodings, const rdr::S32* encodings)
{
  useCopyRect = false;
  supportsLocalCursor = false;
  supportsLocalCursorWithAlpha = false;
  supportsDesktopResize = false;
  supportsExtendedDesktopSize = false;
  supportsLocalXCursor = false;
  supportsLastRect = false;
  compressLevel = -1;
  qualityLevel = -1;
  fineQualityLevel = -1;
  subsampling = subsampleUndefined;

  encodings_.clear();
  encodings_.insert(encodingRaw);

  for (int i = nEncodings-1; i >= 0; i--) {
    switch (encodings[i]) {
    case encodingCopyRect:
      useCopyRect = true;
      break;
    case pseudoEncodingCursor:
      supportsLocalCursor = true;
      break;
    case pseudoEncodingXCursor:
      supportsLocalXCursor = true;
      break;
    case pseudoEncodingCursorWithAlpha:
      supportsLocalCursorWithAlpha = true;
      break;
    case pseudoEncodingDesktopSize:
      supportsDesktopResize = true;
      break;
    case pseudoEncodingExtendedDesktopSize:
      supportsExtendedDesktopSize = true;
      break;
    case pseudoEncodingDesktopName:
      supportsDesktopRename = true;
      break;
    case pseudoEncodingLastRect:
      supportsLastRect = true;
      break;
    case pseudoEncodingFence:
      supportsFence = true;
      break;
    case pseudoEncodingContinuousUpdates:
      supportsContinuousUpdates = true;
      break;
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

    if (encodings[i] > 0)
      encodings_.insert(encodings[i]);
  }
}
