/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2011-2015 Pierre Ossman for Cendio AB
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <assert.h>

#include "scrnintstr.h"
#ifdef RANDR
#include "randrstr.h"
#endif

#include "XorgGlue.h"

const char *vncGetDisplay(void)
{
  return display;
}

unsigned long vncGetServerGeneration(void)
{
  return serverGeneration;
}

void vncFatalError(const char *format, ...)
{
  va_list args;
  char buffer[4096];

  va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);

  FatalError("%s", buffer);
}

int vncGetScreenCount(void)
{
  return screenInfo.numScreens;
}

void vncGetScreenFormat(int scrIdx, int *depth, int *bpp,
                        int *trueColour, int *bigEndian,
                        int *redMask, int *greenMask, int *blueMask)
{
  int i;
  VisualPtr vis = NULL;

  assert(depth);
  assert(bpp);
  assert(trueColour);
  assert(bigEndian);
  assert(redMask);
  assert(greenMask);
  assert(blueMask);

  *depth = screenInfo.screens[scrIdx]->rootDepth;

  for (i = 0; i < screenInfo.numPixmapFormats; i++) {
    if (screenInfo.formats[i].depth == *depth) {
      *bpp = screenInfo.formats[i].bitsPerPixel;
      break;
    }
  }

  if (i == screenInfo.numPixmapFormats)
    FatalError("No pixmap format for root depth\n");

  *bigEndian = (screenInfo.imageByteOrder == MSBFirst);

  for (i = 0; i < screenInfo.screens[scrIdx]->numVisuals; i++) {
    if (screenInfo.screens[scrIdx]->visuals[i].vid == 
        screenInfo.screens[scrIdx]->rootVisual) {
      vis = &screenInfo.screens[scrIdx]->visuals[i];
      break;
    }
  }

  if (i == screenInfo.screens[scrIdx]->numVisuals)
    FatalError("No visual record for root visual\n");

  *trueColour = (vis->class == TrueColor);

  *redMask = vis->redMask;
  *greenMask = vis->greenMask;
  *blueMask = vis->blueMask;
}

int vncGetScreenX(int scrIdx)
{
  return screenInfo.screens[scrIdx]->x;
}

int vncGetScreenY(int scrIdx)
{
  return screenInfo.screens[scrIdx]->y;
}

