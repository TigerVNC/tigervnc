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

int vncGetScreenWidth(int scrIdx)
{
  return screenInfo.screens[scrIdx]->width;
}

int vncGetScreenHeight(int scrIdx)
{
  return screenInfo.screens[scrIdx]->height;
}

int vncRandRResizeScreen(int scrIdx, int width, int height)
{
#ifdef RANDR
  ScreenPtr pScreen = screenInfo.screens[scrIdx];
  /* Try to retain DPI when we resize */
  return RRScreenSizeSet(pScreen, width, height,
                         pScreen->mmWidth * width / pScreen->width,
                         pScreen->mmHeight * height / pScreen->height);
#else
  return -1;
#endif
}

void vncRandRUpdateSetTime(int scrIdx)
{
#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  rp->lastSetTime = currentTime;
#endif
}

int vncRandRHasOutputClones(int scrIdx)
{
#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  for (int i = 0;i < rp->numCrtcs;i++) {
    if (rp->crtcs[i]->numOutputs > 1)
      return 1;
  }
#endif
  return 0;
}

int vncRandRGetOutputCount(int scrIdx)
{
#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  return rp->numOutputs;
#else
  return 0;
#endif
}

int vncRandRGetAvailableOutputs(int scrIdx)
{
#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);

  int availableOutputs;
  RRCrtcPtr *usedCrtcs;
  int numUsed;

  int i, j, k;

  usedCrtcs = malloc(sizeof(RRCrtcPtr) * rp->numCrtcs);
  if (usedCrtcs == NULL)
    return 0;

  /*
   * This gets slightly complicated because we might need to hook a CRTC
   * up to the output, but also check that we don't try to use the same
   * CRTC for multiple outputs.
   */
  availableOutputs = 0;
  numUsed = 0;
  for (i = 0;i < rp->numOutputs;i++) {
    RROutputPtr output;

    output = rp->outputs[i];

    if (output->crtc != NULL)
      availableOutputs++;
    else {
      for (j = 0;j < output->numCrtcs;j++) {
        if (output->crtcs[j]->numOutputs != 0)
          continue;

        for (k = 0;k < numUsed;k++) {
          if (usedCrtcs[k] == output->crtcs[j])
            break;
        }
        if (k != numUsed)
          continue;

        availableOutputs++;

        usedCrtcs[numUsed] = output->crtcs[j];
        numUsed++;

        break;
      }
    }
  }

  free(usedCrtcs);

  return availableOutputs;
#else
  return 0;
#endif
}

const char *vncRandRGetOutputName(int scrIdx, int outputIdx)
{
#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  return rp->outputs[outputIdx]->name;
#else
  return "";
#endif
}

int vncRandRIsOutputEnabled(int scrIdx, int outputIdx)
{
#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);

  if (rp->outputs[outputIdx]->crtc == NULL)
    return 0;
  if (rp->outputs[outputIdx]->crtc->mode == NULL)
    return 0;

  return 1;
#else
  return 0;
#endif
}

int vncRandRIsOutputUsable(int scrIdx, int outputIdx)
{
#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);

  RROutputPtr output;
  int i;

  output = rp->outputs[outputIdx];
  if (output->crtc != NULL)
    return 1;

  /* Any unused CRTCs? */
  for (i = 0;i < output->numCrtcs;i++) {
    if (output->crtcs[i]->numOutputs == 0)
      return 1;
  }
#endif

  return 0;
}

int vncRandRDisableOutput(int scrIdx, int outputIdx)
{
#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  RRCrtcPtr crtc;

  crtc = rp->outputs[outputIdx]->crtc;
  if (crtc == NULL)
    return 0;

  return RRCrtcSet(crtc, NULL, crtc->x, crtc->y, crtc->rotation, 0, NULL);
#else
  return -1;
#endif
}

intptr_t vncRandRGetOutputId(int scrIdx, int outputIdx)
{
#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  return (intptr_t)rp->outputs[outputIdx];
#else
  return 0;
#endif
}

void vncRandRGetOutputDimensions(int scrIdx, int outputIdx,
                            int *x, int *y, int *width, int *height)
{
#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);

  *x = rp->outputs[outputIdx]->crtc->x;
  *y = rp->outputs[outputIdx]->crtc->y;
  *width = rp->outputs[outputIdx]->crtc->mode->mode.width;
  *height = rp->outputs[outputIdx]->crtc->mode->mode.height;
#endif
}

int vncRandRReconfigureOutput(int scrIdx, int outputIdx, int x, int y,
                              int width, int height)
{
#ifdef RANDR
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);

  RROutputPtr output;
  RRCrtcPtr crtc;
  RRModePtr mode;

  int i;

  output = rp->outputs[outputIdx];
  crtc = output->crtc;

  /* Need a CRTC? */
  if (crtc == NULL) {
    for (i = 0;i < output->numCrtcs;i++) {
      if (output->crtcs[i]->numOutputs != 0)
        continue;

      crtc = output->crtcs[i];
      break;
    }

    /* Couldn't find one... */
    if (crtc == NULL)
      return -1;
  }

  /* Make sure we have the mode we want */
  mode = vncRandRCreatePreferredMode(output, width, height);
  if (mode == NULL)
    return -1;

  /* Reconfigure new mode and position */
  return RRCrtcSet(crtc, mode, x, y, crtc->rotation, 1, &output);
#else
  return -1;
#endif
}
