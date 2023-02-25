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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/Exception.h>
#include <rfb/LogWriter.h>
#include <rfb/SMsgHandler.h>
#include <rfb/ScreenSet.h>
#include <rfb/clipboardTypes.h>
#include <rfb/encodings.h>
#include <rfb/util.h>

using namespace rfb;

static LogWriter vlog("SMsgHandler");

SMsgHandler::SMsgHandler()
{
}

SMsgHandler::~SMsgHandler()
{
}

void SMsgHandler::clientInit(bool /*shared*/)
{
}

void SMsgHandler::setPixelFormat(const PixelFormat& pf)
{
  client.setPF(pf);
}

void SMsgHandler::setEncodings(int nEncodings, const int32_t* encodings)
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

void SMsgHandler::keyEvent(uint32_t /*keysym*/, uint32_t /*keycode*/,
                           bool /*down*/)
{
}

void SMsgHandler::pointerEvent(const Point& /*pos*/,
                               uint8_t /*buttonMask*/)
{
}

void SMsgHandler::clientCutText(const char* /*str*/)
{
}

void SMsgHandler::handleClipboardCaps(uint32_t flags, const uint32_t* lengths)
{
  int i;

  vlog.debug("Got client clipboard capabilities:");
  for (i = 0;i < 16;i++) {
    if (flags & (1 << i)) {
      const char *type;

      switch (1 << i) {
        case clipboardUTF8:
          type = "Plain text";
          break;
        case clipboardRTF:
          type = "Rich text";
          break;
        case clipboardHTML:
          type = "HTML";
          break;
        case clipboardDIB:
          type = "Images";
          break;
        case clipboardFiles:
          type = "Files";
          break;
        default:
          vlog.debug("    Unknown format 0x%x", 1 << i);
          continue;
      }

      if (lengths[i] == 0)
        vlog.debug("    %s (only notify)", type);
      else {
        vlog.debug("    %s (automatically send up to %s)",
                   type, iecPrefix(lengths[i], "B").c_str());
      }
    }
  }

  client.setClipboardCaps(flags, lengths);
}

void SMsgHandler::handleClipboardRequest(uint32_t /*flags*/)
{
}

void SMsgHandler::handleClipboardPeek()
{
}

void SMsgHandler::handleClipboardNotify(uint32_t /*flags*/)
{
}

void SMsgHandler::handleClipboardProvide(uint32_t /*flags*/,
                                         const size_t* /*lengths*/,
                                         const uint8_t* const* /*data*/)
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
