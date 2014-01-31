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
#include <rdr/InStream.h>
#include <rfb/CMsgReader.h>
#include <rfb/CMsgHandler.h>
#include <rfb/PixelBuffer.h>
#include <rfb/CopyRectDecoder.h>

using namespace rfb;

CopyRectDecoder::CopyRectDecoder(CMsgReader* reader) : Decoder(reader)
{
}

CopyRectDecoder::~CopyRectDecoder()
{
}

void CopyRectDecoder::readRect(const Rect& r, CMsgHandler* handler)
{
  int srcX = reader->getInStream()->readU16();
  int srcY = reader->getInStream()->readU16();
  handler->copyRect(r, srcX, srcY);
}
