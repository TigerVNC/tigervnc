/* Copyright 2018 Peter Astrand <astrand@cendio.se> for Cendio AB
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

#ifdef HAVE_XRANDR
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <stdlib.h>
#include <string.h>

#include "RandrGlue.h"

typedef struct _vncGlueContext {
  Display *dpy;
  XRRScreenResources *res;
} vncGlueContext;

static vncGlueContext randrGlueContext;

void vncSetGlueContext(Display *dpy, void *res)
{
  randrGlueContext.dpy = dpy;
  randrGlueContext.res = (XRRScreenResources *)res;
}

static RRMode vncRandRGetMatchingMode(XRROutputInfo *output,
                                       unsigned int width, unsigned int height)
{
  vncGlueContext *ctx = &randrGlueContext;

  /*
   * We're not going to change which modes are preferred, but let's
   * see if we can at least find a mode with matching dimensions.
   */

  if (output->crtc) {
    XRRCrtcInfo *crtc;
    unsigned int swap;

    crtc = XRRGetCrtcInfo(ctx->dpy, ctx->res, output->crtc);
    if (!crtc)
      return None;

    switch (crtc->rotation) {
    case RR_Rotate_90:
    case RR_Rotate_270:
      swap = width;
      width = height;
      height = swap;
      break;
    }

    XRRFreeCrtcInfo(crtc);
  }

  for (int i = 0; i < ctx->res->nmode; i++) {
    for (int j = 0; j < output->nmode; j++) {
      if ((output->modes[j] == ctx->res->modes[i].id) &&
          (ctx->res->modes[i].width == width) &&
          (ctx->res->modes[i].height == height)) {
        return ctx->res->modes[i].id;
      }
    }
  }

  return None;
}

int vncGetScreenWidth(void)
{
  vncGlueContext *ctx = &randrGlueContext;
  return DisplayWidth(ctx->dpy, DefaultScreen(ctx->dpy));
}

int vncGetScreenHeight(void)
{
  vncGlueContext *ctx = &randrGlueContext;
  return DisplayHeight(ctx->dpy, DefaultScreen(ctx->dpy));
}

int vncRandRIsValidScreenSize(int width, int height)
{
  vncGlueContext *ctx = &randrGlueContext;
  /* Assert size ranges */
  int minwidth, minheight, maxwidth, maxheight;
  int ret = XRRGetScreenSizeRange(ctx->dpy, DefaultRootWindow(ctx->dpy),
                                  &minwidth, &minheight,
                                  &maxwidth, &maxheight);
  if (!ret) {
    return 0;
  }
  if (width < minwidth || maxwidth < width) {
    return 0;
  }
  if (height < minheight || maxheight < height) {
    return 0;
  }

  return 1;
}

int vncRandRResizeScreen(int width, int height)
{
  vncGlueContext *ctx = &randrGlueContext;

  int mwidth, mheight;

  // Always calculate a DPI of 96.
  // It's what mutter does, so good enough for us.
  mwidth = width * 254 / 96 / 10;
  mheight = height * 254 / 96 / 10;

  XRRSetScreenSize(ctx->dpy, DefaultRootWindow(ctx->dpy),
                   width, height, mwidth, mheight);

  return 1;
}

void vncRandRUpdateSetTime(void)
{

}

int vncRandRHasOutputClones(void)
{
  vncGlueContext *ctx = &randrGlueContext;
  for (int i = 0; i < ctx->res->ncrtc; i++) {
    XRRCrtcInfo *crtc = XRRGetCrtcInfo(ctx->dpy, ctx->res, ctx->res->crtcs[i]);
    if (!crtc) {
      return 0;
    }
    if (crtc->noutput > 1) {
      XRRFreeCrtcInfo (crtc);
      return 1;
    }
    XRRFreeCrtcInfo (crtc);
  }
  return 0;
}

int vncRandRGetOutputCount(void)
{
  vncGlueContext *ctx = &randrGlueContext;
  return ctx->res->noutput;
}

int vncRandRGetAvailableOutputs(void)
{
  vncGlueContext *ctx = &randrGlueContext;

  int availableOutputs;
  RRCrtc *usedCrtcs;
  int numUsed;

  int i, j, k;

  usedCrtcs = (RRCrtc*)malloc(sizeof(RRCrtc) * ctx->res->ncrtc);
  if (usedCrtcs == NULL)
    return 0;

  /*
   * This gets slightly complicated because we might need to hook a CRTC
   * up to the output, but also check that we don't try to use the same
   * CRTC for multiple outputs.
   */
  availableOutputs = 0;
  numUsed = 0;
  for (i = 0;i < ctx->res->noutput; i++) {
    XRROutputInfo *output;

    output = XRRGetOutputInfo(ctx->dpy, ctx->res, ctx->res->outputs[i]);
    if (!output) {
      continue;
    }

    if (output->crtc != None)
      availableOutputs++;
    else {
      for (j = 0;j < output->ncrtc;j++) {
        XRRCrtcInfo *crtc = XRRGetCrtcInfo(ctx->dpy, ctx->res, output->crtcs[j]);
        if (!crtc) {
          continue;
        }
        if (crtc->noutput != 0) {
          XRRFreeCrtcInfo(crtc);
          continue;
        }
        XRRFreeCrtcInfo(crtc);

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
    XRRFreeOutputInfo(output);
  }

  free(usedCrtcs);

  return availableOutputs;
}

char *vncRandRGetOutputName(int outputIdx)
{
  vncGlueContext *ctx = &randrGlueContext;
  XRROutputInfo *output = XRRGetOutputInfo(ctx->dpy, ctx->res, ctx->res->outputs[outputIdx]);
  if (!output) {
    return strdup("");
  }
  char *ret = strdup(output->name);
  XRRFreeOutputInfo(output);
  return ret;
}

int vncRandRIsOutputEnabled(int outputIdx)
{
  vncGlueContext *ctx = &randrGlueContext;
  XRROutputInfo *output = XRRGetOutputInfo(ctx->dpy, ctx->res, ctx->res->outputs[outputIdx]);
  if (!output) {
    return 0;
  }

  if (output->crtc == None) {
    XRRFreeOutputInfo(output);
    return 0;
  }
  XRRCrtcInfo *crtc = XRRGetCrtcInfo(ctx->dpy, ctx->res, output->crtc);
  XRRFreeOutputInfo(output);
  if (!crtc) {
    return 0;
  }
  if (crtc->mode == None) {
    XRRFreeCrtcInfo(crtc);
    return 0;
  }
  XRRFreeCrtcInfo(crtc);
  return 1;
}

int vncRandRIsOutputUsable(int outputIdx)
{
  vncGlueContext *ctx = &randrGlueContext;

  XRROutputInfo *output;
  int i;

  output = XRRGetOutputInfo(ctx->dpy, ctx->res, ctx->res->outputs[outputIdx]);
  if (!output) {
    return 0;
  }

  if (output->crtc != None) {
    XRRFreeOutputInfo(output);
    return 1;
  }

  /* Any unused CRTCs? */
  for (i = 0;i < output->ncrtc;i++) {
    XRRCrtcInfo *crtc = XRRGetCrtcInfo(ctx->dpy, ctx->res, output->crtcs[i]);
    if (crtc->noutput == 0) {
      XRRFreeOutputInfo(output);
      XRRFreeCrtcInfo(crtc);
      return 1;
    }
    XRRFreeCrtcInfo(crtc);
  }

  XRRFreeOutputInfo(output);
  return 0;
}

int vncRandRIsOutputConnected(int outputIdx)
{
  vncGlueContext *ctx = &randrGlueContext;
  XRROutputInfo *output;

  output = XRRGetOutputInfo(ctx->dpy, ctx->res, ctx->res->outputs[outputIdx]);
  if (!output) {
    return 0;
  }

  int ret = (output->connection == RR_Connected);
  XRRFreeOutputInfo(output);
  return ret;
}

int vncRandRCheckOutputMode(int outputIdx, int width, int height)
{
  vncGlueContext *ctx = &randrGlueContext;
  XRROutputInfo *output;
  RRMode mode;

  output = XRRGetOutputInfo(ctx->dpy, ctx->res, ctx->res->outputs[outputIdx]);
  if (!output)
    return 0;

  /* Make sure we have the mode we want */
  mode = vncRandRGetMatchingMode(output, width, height);
  XRRFreeOutputInfo(output);

  if (mode == None)
    return 0;

  return 1;
}

int vncRandRDisableOutput(int outputIdx)
{
  vncGlueContext *ctx = &randrGlueContext;
  RRCrtc crtcid;
  int i;
  int move = 0;

  XRROutputInfo *output = XRRGetOutputInfo(ctx->dpy, ctx->res, ctx->res->outputs[outputIdx]);
  if (!output) {
    return 0;
  }

  crtcid = output->crtc;
  if (crtcid == 0) {
    XRRFreeOutputInfo(output);
    return 1;
  }

  XRRCrtcInfo *crtc = XRRGetCrtcInfo(ctx->dpy, ctx->res, output->crtc);
  XRRFreeOutputInfo(output);
  if (!crtc) {
    return 0;
  }

  /* Remove this output from the CRTC configuration */
  for (i = 0; i < crtc->noutput; i++) {
    if (ctx->res->outputs[outputIdx] == crtc->outputs[i]) {
      crtc->noutput -= 1;
      move = 1;
    }
    if (move && i < crtc->noutput) {
      crtc->outputs[i] = crtc->outputs[i+1];
    }
  }
  if (crtc->noutput == 0) {
    crtc->mode = None;
    crtc->outputs = NULL;
  }

  int ret = XRRSetCrtcConfig(ctx->dpy,
                             ctx->res,
                             crtcid,
                             CurrentTime,
                             crtc->x, crtc->y,
                             crtc->mode, crtc->rotation,
                             crtc->outputs, crtc->noutput);

  XRRFreeCrtcInfo(crtc);

  return (ret == RRSetConfigSuccess);
}

unsigned int vncRandRGetOutputId(int outputIdx)
{
  vncGlueContext *ctx = &randrGlueContext;
  return ctx->res->outputs[outputIdx];
}

int vncRandRGetOutputDimensions(int outputIdx,
                                 int *x, int *y, int *width, int *height)
{
  vncGlueContext *ctx = &randrGlueContext;
  int swap;
  *x = *y = *width = *height = 0;

  XRROutputInfo *output = XRRGetOutputInfo(ctx->dpy, ctx->res, ctx->res->outputs[outputIdx]);
  if (!output) {
    return 1;
  }

  if (!output->crtc) {
    XRRFreeOutputInfo(output);
    return 1;
  }

  XRRCrtcInfo *crtc = XRRGetCrtcInfo(ctx->dpy, ctx->res, output->crtc);
  XRRFreeOutputInfo(output);
  if (!crtc) {
    return 1;
  }
  if (crtc->mode == None) {
    XRRFreeCrtcInfo(crtc);
    return 1;
  }

  *x = crtc->x;
  *y = crtc->y;
  for (int m = 0; m < ctx->res->nmode; m++) {
    if (crtc->mode == ctx->res->modes[m].id) {
      *width = ctx->res->modes[m].width;
      *height = ctx->res->modes[m].height;
    }
  }

  switch (crtc->rotation) {
  case RR_Rotate_90:
  case RR_Rotate_270:
    swap = *width;
    *width = *height;
    *height = swap;
    break;
  }

  XRRFreeCrtcInfo(crtc);
  return 0;
}

int vncRandRReconfigureOutput(int outputIdx, int x, int y,
                              int width, int height)
{
  vncGlueContext *ctx = &randrGlueContext;

  XRROutputInfo *output;
  RRCrtc crtcid;
  RRMode mode;
  XRRCrtcInfo *crtc = NULL;

  int i, ret;

  output = XRRGetOutputInfo(ctx->dpy, ctx->res, ctx->res->outputs[outputIdx]);
  if (!output) {
    return 0;
  }

  crtcid = output->crtc;

  /* Need a CRTC? */
  if (crtcid == None) {
    for (i = 0;i < output->ncrtc;i++) {
      crtc = XRRGetCrtcInfo(ctx->dpy, ctx->res, output->crtcs[i]);
      if (!crtc) {
        continue;
      }

      if (crtc->noutput != 0) {
        XRRFreeCrtcInfo(crtc);
        continue;
      }

      crtcid = output->crtcs[i];
      crtc->rotation = RR_Rotate_0;
      break;
    }
  } else {
    crtc = XRRGetCrtcInfo(ctx->dpy, ctx->res, crtcid);
  }

  /* Couldn't find one... */
  if (crtc == NULL) {
    XRRFreeOutputInfo(output);
    return 0;
  }

  /* Make sure we have the mode we want */
  mode = vncRandRGetMatchingMode(output, width, height);
  if (mode == None) {
    XRRFreeCrtcInfo(crtc);
    XRRFreeOutputInfo(output);
    return 0;
  }

  /* Reconfigure new mode and position */
  ret = XRRSetCrtcConfig (ctx->dpy, ctx->res, crtcid, CurrentTime, x, y,
                    mode, crtc->rotation, ctx->res->outputs+outputIdx, 1);

  XRRFreeCrtcInfo(crtc);
  XRRFreeOutputInfo(output);

  return (ret == RRSetConfigSuccess);
}

int vncRandRCanCreateOutputs(int extraOutputs)
{
  return 0;
}

int vncRandRCreateOutputs(int extraOutputs)
{
  return 0;
}

#endif /* HAVE_XRANDR */
