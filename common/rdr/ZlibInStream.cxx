/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

#include <core/i18n.h>

#include <rdr/ZlibInStream.h>
#include <zlib.h>

using namespace rdr;

ZlibInStream::ZlibInStream()
  : underlying(nullptr), zs(nullptr), bytesIn(0)
{
  init();
}

ZlibInStream::~ZlibInStream()
{
  deinit();
}

void ZlibInStream::setUnderlying(InStream* is, size_t bytesIn_)
{
  underlying = is;
  bytesIn = bytesIn_;
  skip(avail());
}

void ZlibInStream::flushUnderlying()
{
  while (bytesIn > 0) {
    if (!hasData(1))
      throw std::runtime_error(
        _("Failed to flush remaining stream data"));
    skip(avail());
  }

  setUnderlying(nullptr, 0);
}

void ZlibInStream::reset()
{
  deinit();
  init();
}

void ZlibInStream::init()
{
  assert(zs == nullptr);

  zs = new z_stream;
  zs->zalloc    = nullptr;
  zs->zfree     = nullptr;
  zs->opaque    = nullptr;
  zs->next_in   = nullptr;
  zs->avail_in  = 0;
  if (inflateInit(zs) != Z_OK) {
    delete zs;
    zs = nullptr;
    throw std::runtime_error(_("Failed to initialize zlib"));
  }
}

void ZlibInStream::deinit()
{
  assert(zs != nullptr);
  setUnderlying(nullptr, 0);
  inflateEnd(zs);
  delete zs;
  zs = nullptr;
}

bool ZlibInStream::fillBuffer()
{
  if (!underlying)
    throw std::logic_error("ZlibInStream overrun: No underlying stream");

  zs->next_out = (uint8_t*)end;
  zs->avail_out = availSpace();

  if (!underlying->hasData(1))
    return false;
  size_t length = underlying->avail();
  if (length > bytesIn)
    length = bytesIn;
  zs->next_in = (uint8_t*)underlying->getptr(length);
  zs->avail_in = length;

  int rc = inflate(zs, Z_SYNC_FLUSH);
  if (rc < 0) {
    throw std::runtime_error(_("Failed to decompress data"));
  }

  bytesIn -= length - zs->avail_in;
  end = zs->next_out;
  underlying->setptr(length - zs->avail_in);
  return true;
}
