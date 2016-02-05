/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2019 Pierre Ossman for Cendio AB
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
#include <rfb/Exception.h>
#include <rfb/SMsgHandler.h>
#include <rfb/ScreenSet.h>
#include <rfb/encodings.h>

using namespace rfb;

SMsgHandler::SMsgHandler()
{
}

SMsgHandler::~SMsgHandler()
{
}

void SMsgHandler::clientInit(bool shared)
{
}

void SMsgHandler::setPixelFormat(const PixelFormat& pf)
{
  client.setPF(pf);
}

void SMsgHandler::setEncodings(int nEncodings, const rdr::S32* encodings)
{
  bool firstFence, firstContinuousUpdates, firstLEDState,
       firstQEMUKeyEvent;

  firstFence = !client.supportsFence();
  firstContinuousUpdates = !client.supportsContinuousUpdates();
  firstLEDState = !client.supportsLEDState();
  firstQEMUKeyEvent = !client.supportsEncoding(pseudoEncodingQEMUKeyEvent);

  client.setEncodings(nEncodings, encodings);

  supportsLocalCursor();

  if (client.supportsFence() && firstFence)
    supportsFence();
  if (client.supportsContinuousUpdates() && firstContinuousUpdates)
    supportsContinuousUpdates();
  if (client.supportsLEDState() && firstLEDState)
    supportsLEDState();
  if (client.supportsEncoding(pseudoEncodingQEMUKeyEvent) && firstQEMUKeyEvent)
    supportsQEMUKeyEvent();
}

void SMsgHandler::handleClipboardCaps(rdr::U32 flags, const rdr::U32* lengths)
{
  client.setClipboardCaps(flags, lengths);
}

void SMsgHandler::handleClipboardRequest(rdr::U32 flags)
{
}

void SMsgHandler::handleClipboardPeek(rdr::U32 flags)
{
}

void SMsgHandler::handleClipboardNotify(rdr::U32 flags)
{
}

void SMsgHandler::handleClipboardProvide(rdr::U32 flags,
                                         const size_t* lengths,
                                         const rdr::U8* const* data)
{
}

void SMsgHandler::supportsLocalCursor()
{
}

void SMsgHandler::supportsFence()
{
}

void SMsgHandler::supportsContinuousUpdates()
{
}

void SMsgHandler::supportsLEDState()
{
}

void SMsgHandler::supportsQEMUKeyEvent()
{
}
