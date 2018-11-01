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
#include <string.h>

#include "scrnintstr.h"
#include "randrstr.h"

#include "RandrGlue.h"
#include "XorgGlue.h"

static int scrIdx;

void vncSetGlueContext(int screenIndex);

void vncSetGlueContext(int screenIndex)
{
  scrIdx = screenIndex;
}

int vncGetScreenWidth(void)
{
  return screenInfo.screens[scrIdx]->width;
}

int vncGetScreenHeight(void)
{
  return screenInfo.screens[scrIdx]->height;
}

int vncRandRIsValidScreenSize(int width, int height)
{
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);

  if (width < rp->minWidth || rp->maxWidth < width)
    return 0;
  if (height < rp->minHeight || rp->maxHeight < height)
    return 0;

  return 1;
}

int vncRandRResizeScreen(int width, int height)
{
  ScreenPtr pScreen = screenInfo.screens[scrIdx];
  int dpi, mwidth, mheight;

  /* Keep the DPI specified at start */
  dpi = monitorResolution ? monitorResolution : 96;
  mwidth = width * 254 / dpi / 10;
  mheight = height * 254 / dpi / 10;

  return RRScreenSizeSet(pScreen, width, height, mwidth, mheight);
}

void vncRandRUpdateSetTime(void)
{
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  rp->lastSetTime = currentTime;
}

int vncRandRHasOutputClones(void)
{
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  for (int i = 0;i < rp->numCrtcs;i++) {
    if (rp->crtcs[i]->numOutputs > 1)
      return 1;
  }
  return 0;
}

int vncRandRGetOutputCount(void)
{
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  return rp->numOutputs;
}

int vncRandRGetAvailableOutputs(void)
{
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
}

char *vncRandRGetOutputName(int outputIdx)
{
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  return strdup(rp->outputs[outputIdx]->name);
}

int vncRandRIsOutputEnabled(int outputIdx)
{
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);

  if (rp->outputs[outputIdx]->crtc == NULL)
    return 0;
  if (rp->outputs[outputIdx]->crtc->mode == NULL)
    return 0;

  return 1;
}

int vncRandRIsOutputUsable(int outputIdx)
{
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

  return 0;
}

int vncRandRIsOutputConnected(int outputIdx)
{
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);

  RROutputPtr output;

  output = rp->outputs[outputIdx];
  return (output->connection == RR_Connected);
}

static RRModePtr vncRandRGetMatchingMode(int outputIdx, int width, int height)
{
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);

  RROutputPtr output;

  output = rp->outputs[outputIdx];

  if (output->crtc != NULL) {
    unsigned int swap;
    switch (output->crtc->rotation) {
    case RR_Rotate_90:
    case RR_Rotate_270:
      swap = width;
      width = height;
      height = swap;
      break;
    }
  }

  for (int i = 0; i < output->numModes; i++) {
    if ((output->modes[i]->mode.width == width) &&
        (output->modes[i]->mode.height == height))
      return output->modes[i];
  }

  return NULL;
}

int vncRandRCheckOutputMode(int outputIdx, int width, int height)
{
  if (vncRandRGetMatchingMode(outputIdx, width, height) != NULL)
    return 1;
  if (vncRandRCanCreateModes())
    return 1;
  return 0;
}

int vncRandRDisableOutput(int outputIdx)
{
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  RRCrtcPtr crtc;
  int i;
  RROutputPtr *outputs;
  int numOutputs = 0;
  RRModePtr mode;
  int ret;

  crtc = rp->outputs[outputIdx]->crtc;
  if (crtc == NULL)
    return 1;

  /* Remove this output from the CRTC configuration */
  outputs = malloc(crtc->numOutputs * sizeof(RROutputPtr));
  if (!outputs) {
    return 0;
  }

  for (i = 0; i < crtc->numOutputs; i++) {
    if (rp->outputs[outputIdx] != crtc->outputs[i]) {
      outputs[numOutputs++] = crtc->outputs[i];
    }
  }

  if (numOutputs == 0) {
    mode = NULL;
  } else {
    mode = crtc->mode;
  }

  ret = RRCrtcSet(crtc, mode, crtc->x, crtc->y, crtc->rotation, numOutputs, outputs);
  free(outputs);
  return ret;
}

unsigned int vncRandRGetOutputId(int outputIdx)
{
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  return rp->outputs[outputIdx]->id;
}

int vncRandRGetOutputDimensions(int outputIdx,
                            int *x, int *y, int *width, int *height)
{
  rrScrPrivPtr rp = rrGetScrPriv(screenInfo.screens[scrIdx]);
  RRCrtcPtr crtc;
  int swap;
  *x = *y = *width = *height = 0;

  crtc = rp->outputs[outputIdx]->crtc;
  if (crtc == NULL || !crtc->mode)
    return 1;

  *x = crtc->x;
  *y = crtc->y;
  *width = crtc->mode->mode.width;
  *height = crtc->mode->mode.height;

  switch (crtc->rotation & 0xf) {
  case RR_Rotate_90:
  case RR_Rotate_270:
    swap = *width;
    *width = *height;
    *height = swap;
    break;
  }
  return 0;
}

int vncRandRReconfigureOutput(int outputIdx, int x, int y,
                              int width, int height)
{
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
      return 0;
  }

  /* Make sure we have the mode we want */
  mode = vncRandRGetMatchingMode(outputIdx, width, height);
  if (mode == NULL) {
    mode = vncRandRCreateMode(output, width, height);
    if (mode == NULL)
      return 0;
  }
  mode = vncRandRSetPreferredMode(output, mode);
  if (mode == NULL)
    return 0;

  /* Reconfigure new mode and position */
  return RRCrtcSet(crtc, mode, x, y, crtc->rotation, 1, &output);
}

int vncRandRCanCreateOutputs(int extraOutputs)
{
  return vncRandRCanCreateScreenOutputs(scrIdx, extraOutputs);
}

int vncRandRCreateOutputs(int extraOutputs)
{
  return vncRandRCreateScreenOutputs(scrIdx, extraOutputs);
}
