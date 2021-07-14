/* Copyright (C) 2021 Vladimir Sukhonosov <xornet@xornet.org>
 * Copyright (C) 2021 Martins Mozeiko <martins.mozeiko@gmail.com>
 * All Rights Reserved.
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

#define MAX_H264_INSTANCES 64

#include <deque>

#include <rdr/MemInStream.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rfb/LogWriter.h>
#include <rfb/H264Decoder.h>

using namespace rfb;

static LogWriter vlog("h264");

enum rectFlags {
  resetContext       = 0x1,
  resetAllContexts   = 0x2,
};

H264Decoder::H264Decoder() : Decoder(DecoderPlain)
{
}

H264Decoder::~H264Decoder()
{
  resetContexts();
}

void H264Decoder::resetContexts()
{
  os::AutoMutex lock(&mutex);
  for (std::deque<H264DecoderContext*>::iterator it = contexts.begin(); it != contexts.end(); it++)
    delete *it;
  contexts.clear();
}

H264DecoderContext* H264Decoder::findContext(const Rect& r)
{
  os::AutoMutex m(&mutex);
  for (std::deque<H264DecoderContext*>::iterator it = contexts.begin(); it != contexts.end(); it++)
    if ((*it)->isEqualRect(r))
      return *it;
  return NULL;
}

bool H264Decoder::readRect(const Rect& r, rdr::InStream* is,
                           const ServerParams& server, rdr::OutStream* os)
{
  rdr::U32 len;

  if (!is->hasData(8))
    return false;

  is->setRestorePoint();

  len = is->readU32();
  os->writeU32(len);
  rdr::U32 flags = is->readU32();

  if (flags & resetAllContexts)
  {
    resetContexts();
    if (!len)
      return false;
    flags &= ~(resetContext | resetAllContexts);
  }
  os->writeU32(flags);

  if (!is->hasDataOrRestore(len))
    return false;

  is->clearRestorePoint();

  os->copyBytes(is, len);

  if (!findContext(r))
  {
    os::AutoMutex lock(&mutex);
    if (contexts.size() >= MAX_H264_INSTANCES)
    {
      delete contexts.front();
      contexts.pop_front();
    }
    H264DecoderContext *h264_ctx = H264DecoderContext::createContext(r);
    if (!h264_ctx)
      return false;
    contexts.push_back(h264_ctx);
  }

  return true;
}

void H264Decoder::decodeRect(const Rect& r, const void* buffer,
                             size_t buflen, const ServerParams& server,
                             ModifiablePixelBuffer* pb)
{
  H264DecoderContext* ctx = findContext(r);
  if (!ctx)
  {
    vlog.error("Context not found for decoding rect");
    return;
  }
  if (!ctx->isReady())
    return;
  rdr::MemInStream is(buffer, buflen);
  rdr::U32 len = is.readU32();
  rdr::U32 flags = is.readU32();

  if (flags & resetContext)
    ctx->reset();

  if (!len)
    return;

  rdr::U8* h264_buffer = const_cast<rdr::U8*>(is.getptr(0));

  ctx->decode(h264_buffer, len, flags, pb);
}
