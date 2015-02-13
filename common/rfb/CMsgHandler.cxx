/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2011 Pierre Ossman for Cendio AB
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

#include <rfb/Exception.h>
#include <rfb/CMsgHandler.h>
#include <rfb/screenTypes.h>

using namespace rfb;

CMsgHandler::CMsgHandler()
{
}

CMsgHandler::~CMsgHandler()
{
}

void CMsgHandler::setDesktopSize(int width, int height)
{
  cp.width = width;
  cp.height = height;
}

void CMsgHandler::setExtendedDesktopSize(int reason, int result,
                                         int width, int height,
                                         const ScreenSet& layout)
{
  cp.supportsSetDesktopSize = true;

  if ((reason == (signed)reasonClient) && (result != (signed)resultSuccess))
    return;

  if (!layout.validate(width, height))
    fprintf(stderr, "Server sent us an invalid screen layout\n");

  cp.width = width;
  cp.height = height;
  cp.screenLayout = layout;
}

void CMsgHandler::setPixelFormat(const PixelFormat& pf)
{
  cp.setPF(pf);
}

void CMsgHandler::setName(const char* name)
{
  cp.setName(name);
}

void CMsgHandler::fence(rdr::U32 flags, unsigned len, const char data[])
{
  cp.supportsFence = true;
}

void CMsgHandler::endOfContinuousUpdates()
{
  cp.supportsContinuousUpdates = true;
}

void CMsgHandler::framebufferUpdateStart()
{
}

void CMsgHandler::framebufferUpdateEnd()
{
}
