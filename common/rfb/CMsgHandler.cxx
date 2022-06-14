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

#include <stdio.h>

#include <rfb/Exception.h>
#include <rfb/LogWriter.h>
#include <rfb/CMsgHandler.h>
#include <rfb/clipboardTypes.h>
#include <rfb/screenTypes.h>

static rfb::LogWriter vlog("CMsgHandler");

using namespace rfb;

CMsgHandler::CMsgHandler()
{
}

CMsgHandler::~CMsgHandler()
{
}

void CMsgHandler::setDesktopSize(int width, int height)
{
  server.setDimensions(width, height);
}

void CMsgHandler::setExtendedDesktopSize(unsigned reason, unsigned result,
                                         int width, int height,
                                         const ScreenSet& layout)
{
  server.supportsSetDesktopSize = true;

  if ((reason == reasonClient) && (result != resultSuccess))
    return;

  server.setDimensions(width, height, layout);
}

void CMsgHandler::setPixelFormat(const PixelFormat& pf)
{
  server.setPF(pf);
}

void CMsgHandler::setName(const char* name)
{
  server.setName(name);
}

void CMsgHandler::fence(rdr::U32 flags, unsigned len, const char data[])
{
  server.supportsFence = true;
}

void CMsgHandler::endOfContinuousUpdates()
{
  server.supportsContinuousUpdates = true;
}

void CMsgHandler::supportsQEMUKeyEvent()
{
  server.supportsQEMUKeyEvent = true;
}

void CMsgHandler::serverInit(int width, int height,
                             const PixelFormat& pf,
                             const char* name)
{
  server.setDimensions(width, height);
  server.setPF(pf);
  server.setName(name);
}

void CMsgHandler::framebufferUpdateStart()
{
}

void CMsgHandler::framebufferUpdateEnd()
{
}

void CMsgHandler::setLEDState(unsigned int state)
{
  server.setLEDState(state);
}

void CMsgHandler::handleClipboardCaps(rdr::U32 flags, const rdr::U32* lengths)
{
  int i;

  vlog.debug("Got server clipboard capabilities:");
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
        char bytes[1024];

        iecPrefix(lengths[i], "B", bytes, sizeof(bytes));
        vlog.debug("    %s (automatically send up to %s)",
                   type, bytes);
      }
    }
  }

  server.setClipboardCaps(flags, lengths);
}

void CMsgHandler::handleClipboardRequest(rdr::U32 flags)
{
}

void CMsgHandler::handleClipboardPeek(rdr::U32 flags)
{
}

void CMsgHandler::handleClipboardNotify(rdr::U32 flags)
{
}

void CMsgHandler::handleClipboardProvide(rdr::U32 flags,
                                         const size_t* lengths,
                                         const rdr::U8* const* data)
{
}
