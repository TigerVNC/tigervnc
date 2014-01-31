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
#include <stdio.h>
#include <rfb/encodings.h>
#include <rfb/Exception.h>
#include <rfb/Encoder.h>
#include <rfb/RawEncoder.h>
#include <rfb/RREEncoder.h>
#include <rfb/HextileEncoder.h>
#include <rfb/ZRLEEncoder.h>
#include <rfb/TightEncoder.h>
#include <rfb/SConnection.h>

using namespace rfb;

Encoder::Encoder(SConnection *conn_) : conn(conn_)
{
}

Encoder::~Encoder()
{
}

bool Encoder::supported(int encoding)
{
  switch (encoding) {
  case encodingRaw:
  case encodingRRE:
  case encodingHextile:
  case encodingZRLE:
  case encodingTight:
    return true;
  default:
    return false;
  }
}

Encoder* Encoder::createEncoder(int encoding, SConnection* conn)
{
  switch (encoding) {
  case encodingRaw:
    return new RawEncoder(conn);
  case encodingRRE:
    return new RREEncoder(conn);
  case encodingHextile:
    return new HextileEncoder(conn);
  case encodingZRLE:
    return new ZRLEEncoder(conn);
  case encodingTight:
    return new TightEncoder(conn);
  default:
    return NULL;
  }
}
