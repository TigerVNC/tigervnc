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
#include <rfb/Exception.h>
#include <rfb/SMsgHandler.h>
#include <rfb/ScreenSet.h>

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
  cp.setPF(pf);
}

void SMsgHandler::setEncodings(int nEncodings, const rdr::S32* encodings)
{
  bool firstFence, firstContinuousUpdates;

  firstFence = !cp.supportsFence;
  firstContinuousUpdates = !cp.supportsContinuousUpdates;

  cp.setEncodings(nEncodings, encodings);

  supportsLocalCursor();

  if (cp.supportsFence && firstFence)
    supportsFence();
  if (cp.supportsContinuousUpdates && firstContinuousUpdates)
    supportsContinuousUpdates();
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

void SMsgHandler::setDesktopSize(int fb_width, int fb_height,
                                 const ScreenSet& layout)
{
  cp.width = fb_width;
  cp.height = fb_height;
  cp.screenLayout = layout;
}

