/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <rfb/encodings.h>
#include <rfb/Region.h>
#include <rfb/Decoder.h>
#include <rfb/RawDecoder.h>
#include <rfb/CopyRectDecoder.h>
#include <rfb/RREDecoder.h>
#include <rfb/HextileDecoder.h>
#include <rfb/ZRLEDecoder.h>
#include <rfb/TightDecoder.h>
#ifdef HAVE_H264
#include <rfb/H264Decoder.h>
#endif

using namespace rfb;

Decoder::Decoder(enum DecoderFlags flags) : flags(flags)
{
}

Decoder::~Decoder()
{
}

void Decoder::getAffectedRegion(const Rect& rect, const void* buffer,
                                size_t buflen, const ServerParams& server,
                                Region* region)
{
  region->reset(rect);
}

bool Decoder::doRectsConflict(const Rect& rectA, const void* bufferA,
                              size_t buflenA, const Rect& rectB,
                              const void* bufferB, size_t buflenB,
                              const ServerParams& server)
{
  return false;
}

bool Decoder::supported(int encoding)
{
  switch (encoding) {
  case encodingRaw:
  case encodingCopyRect:
  case encodingRRE:
  case encodingHextile:
  case encodingZRLE:
  case encodingTight:
#ifdef HAVE_H264
  case encodingH264:
#endif
    return true;
  default:
    return false;
  }
}

Decoder* Decoder::createDecoder(int encoding)
{
  switch (encoding) {
  case encodingRaw:
    return new RawDecoder();
  case encodingCopyRect:
    return new CopyRectDecoder();
  case encodingRRE:
    return new RREDecoder();
  case encodingHextile:
    return new HextileDecoder();
  case encodingZRLE:
    return new ZRLEDecoder();
  case encodingTight:
    return new TightDecoder();
#ifdef HAVE_H264
  case encodingH264:
    return new H264Decoder();
#endif
  default:
    return NULL;
  }
}
