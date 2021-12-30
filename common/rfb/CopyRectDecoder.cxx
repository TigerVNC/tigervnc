/* Copyright 2014 Pierre Ossman <ossman@cendio.se> for Cendio AB
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

#include <rdr/MemInStream.h>
#include <rdr/OutStream.h>
#include <rfb/PixelBuffer.h>
#include <rfb/Region.h>
#include <rfb/CopyRectDecoder.h>

using namespace rfb;

CopyRectDecoder::CopyRectDecoder() : Decoder(DecoderPlain)
{
}

CopyRectDecoder::~CopyRectDecoder()
{
}

bool CopyRectDecoder::readRect(const Rect& r, rdr::InStream* is,
                               const ServerParams& server, rdr::OutStream* os)
{
  if (!is->hasData(4))
    return false;
  os->copyBytes(is, 4);
  return true;
}


void CopyRectDecoder::getAffectedRegion(const Rect& rect,
                                        const void* buffer,
                                        size_t buflen,
                                        const ServerParams& server,
                                        Region* region)
{
  rdr::MemInStream is(buffer, buflen);
  int srcX = is.readU16();
  int srcY = is.readU16();

  Decoder::getAffectedRegion(rect, buffer, buflen, server, region);

  region->assign_union(Region(rect.translate(Point(srcX-rect.tl.x,
                                                   srcY-rect.tl.y))));
}

void CopyRectDecoder::decodeRect(const Rect& r, const void* buffer,
                                 size_t buflen, const ServerParams& server,
                                 ModifiablePixelBuffer* pb)
{
  rdr::MemInStream is(buffer, buflen);
  int srcX = is.readU16();
  int srcY = is.readU16();
  pb->copyRect(r, Point(r.tl.x-srcX, r.tl.y-srcY));
}
