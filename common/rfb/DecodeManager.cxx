/* Copyright 2015 Pierre Ossman for Cendio AB
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

#include <assert.h>
#include <string.h>

#include <rfb/CConnection.h>
#include <rfb/DecodeManager.h>
#include <rfb/Decoder.h>

#include <rfb/LogWriter.h>

#include <rdr/Exception.h>

using namespace rfb;

static LogWriter vlog("DecodeManager");

DecodeManager::DecodeManager(CConnection *conn) :
  conn(conn)
{
  memset(decoders, 0, sizeof(decoders));
}

DecodeManager::~DecodeManager()
{
  for (size_t i = 0; i < sizeof(decoders)/sizeof(decoders[0]); i++)
    delete decoders[i];
}

void DecodeManager::decodeRect(const Rect& r, int encoding,
                               ModifiablePixelBuffer* pb)
{
  assert(pb != NULL);

  if (!Decoder::supported(encoding)) {
    vlog.error("Unknown encoding %d", encoding);
    throw rdr::Exception("Unknown encoding");
  }

  if (!decoders[encoding]) {
    decoders[encoding] = Decoder::createDecoder(encoding);
    if (!decoders[encoding]) {
      vlog.error("Unknown encoding %d", encoding);
      throw rdr::Exception("Unknown encoding");
    }
  }
  decoders[encoding]->readRect(r, conn->getInStream(), conn->cp, pb);
}
