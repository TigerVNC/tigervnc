/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2004-2005 Cendio AB.
 * Copyright 2009-2025 Pierre Ossman for Cendio AB
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
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

#include <core/string.h>

#include <rdr/InStream.h>
#include <rdr/OutStream.h>

#include <rfb/ServerParams.h>
#include <rfb/Exception.h>
#include <rfb/PixelBuffer.h>
#include <rfb/JPEGDecoder.h>

using namespace rfb;

JPEGDecoder::JPEGDecoder() : Decoder(DecoderPlain), state(IDLE)
{
}

JPEGDecoder::~JPEGDecoder()
{
}

bool JPEGDecoder::readRect(const core::Rect& /*r*/, rdr::InStream* is,
                           const ServerParams& /*server*/,
                           rdr::OutStream* os)
{
  if (state == IDLE)
    state = SEGMENT;

  while (state != IDLE) {
    if (state == SEGMENT) {
      if (!readSegment(is, os))
        return false;
    }

    if (state == STREAM) {
      if (!readStream(is, os))
        return false;
    }
  }

  return true;
}

bool JPEGDecoder::readSegment(rdr::InStream* is, rdr::OutStream* os)
{
  uint8_t marker, type;
  uint16_t len;

  if (!is->hasData(2))
    return false;

  is->setRestorePoint();

  marker = is->readU8();
  type = is->readU8();

  if (marker != 0xff)
    throw protocol_error(
      core::format("Invalid JPEG segment marker: 0x%02x", marker));

  // Markers without data
  if ((type == 0x01) || ((type >= 0xd0) && type <= 0xd9)) {
    os->writeU8(marker);
    os->writeU8(type);

    is->clearRestorePoint();

    // End of image?
    if (type == 0xd9)
      state = IDLE;

    return true;
  }

  if (!is->hasDataOrRestore(2))
    return false;

  len = is->readU16();

  if (!is->hasDataOrRestore(len-2))
    return false;

  if (type == 0xc0) {
    // start of frame, reset our state
    seenHuffman = false;
    seenQuant = false;
  }

  if (type == 0xc4) {
    const uint8_t *in;
    uint8_t *out;

    if (!seenHuffman)
      lastHuffmanTables.clear();
    seenHuffman = true;

    lastHuffmanTables.resize(lastHuffmanTables.size() + 1);
    lastHuffmanTables.back().resize(2 + len);

    in = is->getptr(len-2);
    out = lastHuffmanTables.back().data();

    out[0] = marker;
    out[1] = type;
    out[2] = (len >> 8) & 0xff;
    out[3] = (len >> 0) & 0xff;
    memcpy(out + 4, in, len-2);

    // getptr() resets the amount of assured data
    is->hasData(len-2);
  }

  if (type == 0xdb) {
    const uint8_t *in;
    uint8_t *out;

    if (!seenQuant)
      lastQuantTables.clear();
    seenQuant = true;

    lastQuantTables.resize(lastQuantTables.size() + 1);
    lastQuantTables.back().resize(2 + len);

    in = is->getptr(len-2);
    out = lastQuantTables.back().data();

    out[0] = marker;
    out[1] = type;
    out[2] = (len >> 8) & 0xff;
    out[3] = (len >> 0) & 0xff;
    memcpy(out + 4, in, len-2);

    // getptr() resets the amount of assured data
    is->hasData(len-2);
  }

  if (type == 0xda) {
    // start of image data, time to inject tables if they are missing

    if (!seenHuffman) {
      std::vector< std::vector<uint8_t> >::const_iterator iter;

      for (iter = lastHuffmanTables.begin();
            iter != lastHuffmanTables.end(); ++iter)
        os->writeBytes(iter->data(), iter->size());
    }

    if (!seenQuant) {
      std::vector< std::vector<uint8_t> >::const_iterator iter;

      for (iter = lastQuantTables.begin();
            iter != lastQuantTables.end(); ++iter)
        os->writeBytes(iter->data(), iter->size());
    }
  }

  os->writeU8(marker);
  os->writeU8(type);
  os->writeU16(len);
  os->copyBytes(is, len-2);

  is->clearRestorePoint();

  // Start of stream?
  if (type == 0xda)
    state = STREAM;

  return true;
}

bool JPEGDecoder::readStream(rdr::InStream* is, rdr::OutStream* os)
{
  // Peek instream until we see a marker that isn't a restart marker
  size_t avail;
  const uint8_t* ptr;
  const uint8_t* in;

  if (!is->hasData(2))
    return false;

  avail = is->avail();
  ptr = in = is->getptr(avail);
  while (in < (ptr + avail)) {
    in = (const uint8_t*)memchr(in, 0xff, (avail - (in - ptr))-1);

    if (in == nullptr)
      break;

    if ((in[1] != 0x00) && !((in[1] >= 0xd0) && (in[1] <= 0xd7))) {
      is->hasData(in - ptr);
      os->copyBytes(is, in - ptr);

      state = SEGMENT;
      return true;
    }

    in += 2;
  }

  is->hasData(avail-1);
  os->copyBytes(is, avail-1);

  return false;
}

void JPEGDecoder::decodeRect(const core::Rect& r, const uint8_t* buffer,
                             size_t buflen,
                             const ServerParams& /*server*/,
                             ModifiablePixelBuffer* pb)
{
  int stride;
  uint8_t *buf;

  JpegDecompressor jd;

  buf = pb->getBufferRW(r, &stride);
  jd.decompress(buffer, buflen, buf, stride, r, pb->getPF());
  pb->commitBufferRW(r);
}
