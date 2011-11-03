/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2004-2005 Cendio AB.
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
#include <rfb/CMsgReader.h>
#include <rfb/CMsgHandler.h>
#include <rfb/TightDecoder.h>

using namespace rfb;

#define TIGHT_MAX_WIDTH 2048

#define FILL_RECT(r, p) handler->fillRect(r, p)
#define IMAGE_RECT(r, p) handler->imageRect(r, p)
#define BPP 8
#include <rfb/tightDecode.h>
#undef BPP
#define BPP 16
#include <rfb/tightDecode.h>
#undef BPP
#define BPP 32
#include <rfb/tightDecode.h>
#undef BPP

Decoder* TightDecoder::create(CMsgReader* reader)
{
  return new TightDecoder(reader);
}

TightDecoder::TightDecoder(CMsgReader* reader_) : reader(reader_)
{
}

TightDecoder::~TightDecoder()
{
}

void TightDecoder::readRect(const Rect& r, CMsgHandler* handler)
{
  is = reader->getInStream();
  this->handler = handler;
  clientpf = handler->getPreferredPF();
  serverpf = handler->cp.pf();

  if (clientpf.equal(serverpf)) {
    /* Decode directly into the framebuffer (fast path) */
    directDecode = true;
  } else {
    /* Decode into an intermediate buffer and use pixel translation */
    directDecode = false;
  }

  switch (serverpf.bpp) {
  case 8:
    tightDecode8 (r); break;
  case 16:
    tightDecode16(r); break;
  case 32:
    tightDecode32(r); break;
  }
}
