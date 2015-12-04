/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2015 Pierre Ossman for Cendio AB
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

#include <stdio.h>

#include "vncHooks.h"
#include "vncExtInit.h"

#include "xorg-version.h"

#include "scrnintstr.h"
#include "windowstr.h"
#include "cursorstr.h"
#include "gcstruct.h"
#include "regionstr.h"
#include "dixfontstr.h"
#include "colormapst.h"
#ifdef RENDER
#include "picturestr.h"
#endif
#ifdef RANDR
#include "randrstr.h"
#endif

#define DBGPRINT(x) //(fprintf x)

// MAX_RECTS_PER_OP is the maximum number of rectangles we generate from
// operations like Polylines and PolySegment.  If the operation is more complex
// than this, we simply use the bounding box.  Ideally it would be a
// command-line option, but that would involve an extra malloc each time, so we
// fix it here.
#define MAX_RECTS_PER_OP 5

// vncHooksScreenRec and vncHooksGCRec contain pointers to the original
// functions which we "wrap" in order to hook the screen changes.  The screen
// functions are each wrapped individually, while the GC "funcs" and "ops" are
// wrapped as a unit.

typedef struct _vncHooksScreenRec {
  int                          ignoreHooks;

  CloseScreenProcPtr           CloseScreen;
  CreateGCProcPtr              CreateGC;
  CopyWindowProcPtr            CopyWindow;
  ClearToBackgroundProcPtr     ClearToBackground;
#if XORG < 110
  RestoreAreasProcPtr          RestoreAreas;
#endif
  DisplayCursorProcPtr         DisplayCursor;
  ScreenBlockHandlerProcPtr    BlockHandler;
#ifdef RENDER
  CompositeProcPtr             Composite;
  GlyphsProcPtr                Glyphs;
#endif
#ifdef RANDR
  RRSetConfigProcPtr           RandRSetConfig;
  RRScreenSetSizeProcPtr       RandRScreenSetSize;
  RRCrtcSetProcPtr             RandRCrtcSet;
#endif
} vncHooksScreenRec, *vncHooksScreenPtr;

typedef struct _vncHooksGCRec {
#if XORG >= 116
    const GCFuncs *wrappedFuncs;
    const GCOps *wrappedOps;
#else
    GCFuncs *wrappedFuncs;
    GCOps *wrappedOps;
#endif
} vncHooksGCRec, *vncHooksGCPtr;

#if XORG < 19
static int vncHooksScreenPrivateKeyIndex;
static int vncHooksGCPrivateKeyIndex;
static DevPrivateKey vncHooksScreenPrivateKey = &vncHooksScreenPrivateKeyIndex;
static DevPrivateKey vncHooksGCPrivateKey = &vncHooksGCPrivateKeyIndex;
#else
static DevPrivateKeyRec vncHooksScreenKeyRec;
static DevPrivateKeyRec vncHooksGCKeyRec;
#define vncHooksScreenPrivateKey (&vncHooksScreenKeyRec)
#define vncHooksGCPrivateKey (&vncHooksGCKeyRec)
#endif

#define vncHooksScreenPrivate(pScreen) \
        (vncHooksScreenPtr) dixLookupPrivate(&(pScreen)->devPrivates, \
                                             vncHooksScreenPrivateKey)
#define vncHooksGCPrivate(pGC) \
        (vncHooksGCPtr) dixLookupPrivate(&(pGC)->devPrivates, \
                                         vncHooksGCPrivateKey)

// screen functions

#if XORG <= 112
static Bool vncHooksCloseScreen(int i, ScreenPtr pScreen);
#else
static Bool vncHooksCloseScreen(ScreenPtr pScreen);
#endif
static Bool vncHooksCreateGC(GCPtr pGC);
static void vncHooksCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg,
                               RegionPtr pOldRegion);
static void vncHooksClearToBackground(WindowPtr pWin, int x, int y, int w,
                                      int h, Bool generateExposures);
#if XORG < 110
static RegionPtr vncHooksRestoreAreas(WindowPtr pWin, RegionPtr prgnExposed);
#endif
static Bool vncHooksDisplayCursor(DeviceIntPtr pDev,
                                  ScreenPtr pScreen, CursorPtr cursor);
#if XORG <= 112
static void vncHooksBlockHandler(int i, pointer blockData, pointer pTimeout,
                                 pointer pReadmask);
#else
static void vncHooksBlockHandler(ScreenPtr pScreen, void * pTimeout,
                                 void * pReadmask);
#endif
#ifdef RENDER
static void vncHooksComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask, 
			      PicturePtr pDst, INT16 xSrc, INT16 ySrc, INT16 xMask, 
			      INT16 yMask, INT16 xDst, INT16 yDst, CARD16 width, CARD16 height);
static void vncHooksGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst, 
			      PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc, int nlists, 
			      GlyphListPtr lists, GlyphPtr * glyphs);
#endif
#ifdef RANDR
static Bool vncHooksRandRSetConfig(ScreenPtr pScreen, Rotation rotation,
                                   int rate, RRScreenSizePtr pSize);
static Bool vncHooksRandRScreenSetSize(ScreenPtr pScreen,
                                       CARD16 width, CARD16 height,
                                       CARD32 mmWidth, CARD32 mmHeight);
static Bool vncHooksRandRCrtcSet(ScreenPtr pScreen, RRCrtcPtr crtc,
                                 RRModePtr mode, int x, int y,
                                 Rotation rotation, int numOutputs,
                                 RROutputPtr *outputs);
#endif

// GC "funcs"

static void vncHooksValidateGC(GCPtr pGC, unsigned long changes,
                               DrawablePtr pDrawable);
static void vncHooksChangeGC(GCPtr pGC, unsigned long mask);
static void vncHooksCopyGC(GCPtr src, unsigned long mask, GCPtr dst);
static void vncHooksDestroyGC(GCPtr pGC);
static void vncHooksChangeClip(GCPtr pGC, int type, void * pValue,int nrects);
static void vncHooksDestroyClip(GCPtr pGC);
static void vncHooksCopyClip(GCPtr dst, GCPtr src);

static GCFuncs vncHooksGCFuncs = {
  vncHooksValidateGC, vncHooksChangeGC, vncHooksCopyGC, vncHooksDestroyGC,
  vncHooksChangeClip, vncHooksDestroyClip, vncHooksCopyClip,
};

// GC "ops"

static void vncHooksFillSpans(DrawablePtr pDrawable, GCPtr pGC, int nInit,
                              DDXPointPtr pptInit, int *pwidthInit,
                              int fSorted);
static void vncHooksSetSpans(DrawablePtr pDrawable, GCPtr pGC, char *psrc,
                             DDXPointPtr ppt, int *pwidth, int nspans,
                             int fSorted);
static void vncHooksPutImage(DrawablePtr pDrawable, GCPtr pGC, int depth,
                             int x, int y, int w, int h, int leftPad,
                             int format, char *pBits);
static RegionPtr vncHooksCopyArea(DrawablePtr pSrc, DrawablePtr pDst,
                                  GCPtr pGC, int srcx, int srcy, int w, int h,
                                  int dstx, int dsty);
static RegionPtr vncHooksCopyPlane(DrawablePtr pSrc, DrawablePtr pDst,
                                   GCPtr pGC, int srcx, int srcy, int w, int h,
                                   int dstx, int dsty, unsigned long plane);
static void vncHooksPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode,
                              int npt, xPoint *pts);
static void vncHooksPolylines(DrawablePtr pDrawable, GCPtr pGC, int mode,
                              int npt, DDXPointPtr ppts);
static void vncHooksPolySegment(DrawablePtr pDrawable, GCPtr pGC, int nseg,
                                xSegment *segs);
static void vncHooksPolyRectangle(DrawablePtr pDrawable, GCPtr pGC, int nrects,
                                  xRectangle *rects);
static void vncHooksPolyArc(DrawablePtr pDrawable, GCPtr pGC, int narcs,
                            xArc *arcs);
static void vncHooksFillPolygon(DrawablePtr pDrawable, GCPtr pGC, int shape,
                                int mode, int count, DDXPointPtr pts);
static void vncHooksPolyFillRect(DrawablePtr pDrawable, GCPtr pGC, int nrects,
                                 xRectangle *rects);
static void vncHooksPolyFillArc(DrawablePtr pDrawable, GCPtr pGC, int narcs,
                                xArc *arcs);
static int vncHooksPolyText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                             int count, char *chars);
static int vncHooksPolyText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                              int count, unsigned short *chars);
static void vncHooksImageText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                               int count, char *chars);
static void vncHooksImageText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                                int count, unsigned short *chars);
static void vncHooksImageGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x,
                                  int y, unsigned int nglyph,
                                  CharInfoPtr *ppci, void * pglyphBase);
static void vncHooksPolyGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x,
                                 int y, unsigned int nglyph,
                                 CharInfoPtr *ppci, void * pglyphBase);
static void vncHooksPushPixels(GCPtr pGC, PixmapPtr pBitMap,
                               DrawablePtr pDrawable, int w, int h, int x,
                               int y);

static GCOps vncHooksGCOps = {
  vncHooksFillSpans, vncHooksSetSpans, vncHooksPutImage, vncHooksCopyArea,
  vncHooksCopyPlane, vncHooksPolyPoint, vncHooksPolylines, vncHooksPolySegment,
  vncHooksPolyRectangle, vncHooksPolyArc, vncHooksFillPolygon,
  vncHooksPolyFillRect, vncHooksPolyFillArc, vncHooksPolyText8,
  vncHooksPolyText16, vncHooksImageText8, vncHooksImageText16,
  vncHooksImageGlyphBlt, vncHooksPolyGlyphBlt, vncHooksPushPixels
};



/////////////////////////////////////////////////////////////////////////////
// vncHooksInit() is called at initialisation time and every time the server
// resets.  It is called once for each screen, but the indexes are only
// allocated once for each server generation.

int vncHooksInit(int scrIdx)
{
  ScreenPtr pScreen;
  vncHooksScreenPtr vncHooksScreen;

#ifdef RENDER
  PictureScreenPtr ps;
#endif
#ifdef RANDR
  rrScrPrivPtr rp;
#endif

  pScreen = screenInfo.screens[scrIdx];

  if (sizeof(BoxRec) != sizeof(struct UpdateRect)) {
    ErrorF("vncHooksInit: Incompatible BoxRec size\n");
    return FALSE;
  }

#if XORG < 19
  if (!dixRequestPrivate(vncHooksScreenPrivateKey, sizeof(vncHooksScreenRec))) {
    ErrorF("vncHooksInit: Allocation of vncHooksScreen failed\n");
    return FALSE;
  }
  if (!dixRequestPrivate(vncHooksGCPrivateKey, sizeof(vncHooksGCRec))) {
    ErrorF("vncHooksInit: Allocation of vncHooksGCRec failed\n");
    return FALSE;
  }

#else
  if (!dixRegisterPrivateKey(&vncHooksScreenKeyRec, PRIVATE_SCREEN,
      sizeof(vncHooksScreenRec))) {
    ErrorF("vncHooksInit: Allocation of vncHooksScreen failed\n");
    return FALSE;
  }
  if (!dixRegisterPrivateKey(&vncHooksGCKeyRec, PRIVATE_GC,
      sizeof(vncHooksGCRec))) {
    ErrorF("vncHooksInit: Allocation of vncHooksGCRec failed\n");
    return FALSE;
  }

#endif

  vncHooksScreen = vncHooksScreenPrivate(pScreen);

  vncHooksScreen->ignoreHooks = 0;

  vncHooksScreen->CloseScreen = pScreen->CloseScreen;
  vncHooksScreen->CreateGC = pScreen->CreateGC;
  vncHooksScreen->CopyWindow = pScreen->CopyWindow;
  vncHooksScreen->ClearToBackground = pScreen->ClearToBackground;
#if XORG < 110
  vncHooksScreen->RestoreAreas = pScreen->RestoreAreas;
#endif
  vncHooksScreen->DisplayCursor = pScreen->DisplayCursor;
  vncHooksScreen->BlockHandler = pScreen->BlockHandler;
#ifdef RENDER
  ps = GetPictureScreenIfSet(pScreen);
  if (ps) {
    vncHooksScreen->Composite = ps->Composite;
    vncHooksScreen->Glyphs = ps->Glyphs;
  }
#endif
#ifdef RANDR
  rp = rrGetScrPriv(pScreen);
  if (rp) {
    vncHooksScreen->RandRSetConfig = rp->rrSetConfig;
    vncHooksScreen->RandRScreenSetSize = rp->rrScreenSetSize;
    vncHooksScreen->RandRCrtcSet = rp->rrCrtcSet;
  }
#endif

  pScreen->CloseScreen = vncHooksCloseScreen;
  pScreen->CreateGC = vncHooksCreateGC;
  pScreen->CopyWindow = vncHooksCopyWindow;
  pScreen->ClearToBackground = vncHooksClearToBackground;
#if XORG < 110
  pScreen->RestoreAreas = vncHooksRestoreAreas;
#endif
  pScreen->DisplayCursor = vncHooksDisplayCursor;
  pScreen->BlockHandler = vncHooksBlockHandler;
#ifdef RENDER
  if (ps) {
    ps->Composite = vncHooksComposite;
    ps->Glyphs = vncHooksGlyphs;
  }
#endif
#ifdef RANDR
  if (rp) {
    /* Some RandR callbacks are optional */
    if (rp->rrSetConfig)
      rp->rrSetConfig = vncHooksRandRSetConfig;
    if (rp->rrScreenSetSize)
      rp->rrScreenSetSize = vncHooksRandRScreenSetSize;
    if (rp->rrCrtcSet)
      rp->rrCrtcSet = vncHooksRandRCrtcSet;
  }
#endif

  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// vncGetScreenImage() grabs a chunk of data from the main screen into the
// provided buffer. It lives here rather than in XorgGlue.c because it
// temporarily pauses the hooks.

void vncGetScreenImage(int scrIdx, int x, int y, int width, int height,
                       char *buffer, int strideBytes)
{
  ScreenPtr pScreen = screenInfo.screens[scrIdx];
  vncHooksScreenPtr vncHooksScreen = vncHooksScreenPrivate(pScreen);

  int i;

  vncHooksScreen->ignoreHooks++;

  // We do one line at a time since GetImage() cannot handle stride
  for (i = y; i < y + height; i++) {
    DrawablePtr pDrawable;
#if XORG < 19
    pDrawable = (DrawablePtr) WindowTable[scrIdx];
#else
    pDrawable = (DrawablePtr) pScreen->root;
#endif

    (*pScreen->GetImage) (pDrawable, x, i, width, 1,
                          ZPixmap, (unsigned long)~0L, buffer);

    buffer += strideBytes;
  }

  vncHooksScreen->ignoreHooks--;
}

/////////////////////////////////////////////////////////////////////////////
//
// Helper functions for adding changes and copies
//

static inline void add_changed(ScreenPtr pScreen, RegionPtr reg)
{
  vncHooksScreenPtr vncHooksScreen = vncHooksScreenPrivate(pScreen);
  if (vncHooksScreen->ignoreHooks)
    return;
  vncAddChanged(pScreen->myNum,
                (const struct UpdateRect*)REGION_EXTENTS(pScreen, reg),
                REGION_NUM_RECTS(reg),
                (const struct UpdateRect*)REGION_RECTS(reg));
}

static inline void add_copied(ScreenPtr pScreen, RegionPtr dst,
                              int dx, int dy)
{
  vncHooksScreenPtr vncHooksScreen = vncHooksScreenPrivate(pScreen);
  if (vncHooksScreen->ignoreHooks)
    return;
  vncAddCopied(pScreen->myNum,
               (const struct UpdateRect*)REGION_EXTENTS(pScreen, dst),
               REGION_NUM_RECTS(dst),
               (const struct UpdateRect*)REGION_RECTS(dst), dx, dy);
}


/////////////////////////////////////////////////////////////////////////////
//
// screen functions
//

// SCREEN_UNWRAP and SCREEN_REWRAP unwrap and rewrap the given screen function.

#define SCREEN_UNWRAP(scrn,field)                                         \
  ScreenPtr pScreen = scrn;                                               \
  vncHooksScreenPtr vncHooksScreen = vncHooksScreenPrivate(pScreen);      \
  pScreen->field = vncHooksScreen->field;                                 \
  DBGPRINT((stderr,"vncHooks" #field " called\n"));

#define SCREEN_REWRAP(field) pScreen->field = vncHooks##field;


// CloseScreen - unwrap the screen functions and call the original CloseScreen
// function

#if XORG <= 112
static Bool vncHooksCloseScreen(int i, ScreenPtr pScreen_)
#else
static Bool vncHooksCloseScreen(ScreenPtr pScreen_)
#endif
{
#ifdef RENDER
  PictureScreenPtr ps;
#endif
#ifdef RANDR
  rrScrPrivPtr rp;
#endif

  SCREEN_UNWRAP(pScreen_, CloseScreen);

  pScreen->CreateGC = vncHooksScreen->CreateGC;
  pScreen->CopyWindow = vncHooksScreen->CopyWindow;
  pScreen->ClearToBackground = vncHooksScreen->ClearToBackground;
#if XORG < 110
  pScreen->RestoreAreas = vncHooksScreen->RestoreAreas;
#endif
  pScreen->DisplayCursor = vncHooksScreen->DisplayCursor;
  pScreen->BlockHandler = vncHooksScreen->BlockHandler;
#ifdef RENDER
  ps = GetPictureScreenIfSet(pScreen);
  if (ps) {
    ps->Composite = vncHooksScreen->Composite;
    ps->Glyphs = vncHooksScreen->Glyphs;
  }
#endif
#ifdef RANDR
  rp = rrGetScrPriv(pScreen);
  if (rp) {
    rp->rrSetConfig = vncHooksScreen->RandRSetConfig;
    rp->rrScreenSetSize = vncHooksScreen->RandRScreenSetSize;
    rp->rrCrtcSet = vncHooksScreen->RandRCrtcSet;
  }
#endif

  DBGPRINT((stderr,"vncHooksCloseScreen: unwrapped screen functions\n"));

#if XORG <= 112
  return (*pScreen->CloseScreen)(i, pScreen);
#else
  return (*pScreen->CloseScreen)(pScreen);
#endif
}

// CreateGC - wrap the "GC funcs"

static Bool vncHooksCreateGC(GCPtr pGC)
{
  vncHooksGCPtr vncHooksGC = vncHooksGCPrivate(pGC);
  Bool ret;

  SCREEN_UNWRAP(pGC->pScreen, CreateGC);

  ret = (*pScreen->CreateGC) (pGC);

  vncHooksGC->wrappedOps = 0;
  vncHooksGC->wrappedFuncs = pGC->funcs;
  pGC->funcs = &vncHooksGCFuncs;

  SCREEN_REWRAP(CreateGC);

  return ret;
}

// CopyWindow - destination of the copy is the old region, clipped by
// borderClip, translated by the delta.  This call only does the copy - it
// doesn't affect any other bits.

static void vncHooksCopyWindow(WindowPtr pWin, DDXPointRec ptOldOrg,
                               RegionPtr pOldRegion)
{
  int dx, dy;
  BoxRec screen_box;
  RegionRec copied, screen_rgn;

  SCREEN_UNWRAP(pWin->drawable.pScreen, CopyWindow);

  REGION_NULL(pScreen, &copied);
  REGION_COPY(pScreen, &copied, pOldRegion);

  screen_box.x1 = 0;
  screen_box.y1 = 0;
  screen_box.x2 = pScreen->width;
  screen_box.y2 = pScreen->height;

  REGION_INIT(pScreen, &screen_rgn, &screen_box, 1);

  dx = pWin->drawable.x - ptOldOrg.x;
  dy = pWin->drawable.y - ptOldOrg.y;

  // RFB tracks copies in terms of destination rectangle, not source.
  // We also need to copy with changes to the Window's clipping region.
  // Finally, make sure we don't get copies to or from regions outside
  // the framebuffer.
  REGION_INTERSECT(pScreen, &copied, &copied, &screen_rgn);
  REGION_TRANSLATE(pScreen, &copied, dx, dy);
  REGION_INTERSECT(pScreen, &copied, &copied, &screen_rgn);
  REGION_INTERSECT(pScreen, &copied, &copied, &pWin->borderClip);

  (*pScreen->CopyWindow) (pWin, ptOldOrg, pOldRegion);

  if (REGION_NOTEMPTY(pScreen, &copied))
    add_copied(pScreen, &copied, dx, dy);

  REGION_UNINIT(pScreen, &copied);
  REGION_UNINIT(pScreen, &screen_rgn);

  SCREEN_REWRAP(CopyWindow);
}

// ClearToBackground - changed region is the given rectangle, clipped by
// clipList, but only if generateExposures is false.

static void vncHooksClearToBackground(WindowPtr pWin, int x, int y, int w,
                                      int h, Bool generateExposures)
{
  BoxRec box;
  RegionRec reg;

  SCREEN_UNWRAP(pWin->drawable.pScreen, ClearToBackground);

  box.x1 = x + pWin->drawable.x;
  box.y1 = y + pWin->drawable.y;
  box.x2 = w ? (box.x1 + w) : (pWin->drawable.x + pWin->drawable.width);
  box.y2 = h ? (box.y1 + h) : (pWin->drawable.y + pWin->drawable.height);

  REGION_INIT(pScreen, &reg, &box, 0);
  REGION_INTERSECT(pScreen, &reg, &reg, &pWin->clipList);

  (*pScreen->ClearToBackground) (pWin, x, y, w, h, generateExposures);

  if (!generateExposures) {
    add_changed(pScreen, &reg);
  }

  REGION_UNINIT(pScreen, &reg);

  SCREEN_REWRAP(ClearToBackground);
}

#if XORG < 110
// RestoreAreas - changed region is the given region

static RegionPtr vncHooksRestoreAreas(WindowPtr pWin, RegionPtr pRegion)
{
  RegionRec reg;

  SCREEN_UNWRAP(pWin->drawable.pScreen, RestoreAreas);

  REGION_NULL(pScreen, &reg);
  REGION_COPY(pScreen, &reg, pRegion);

  RegionPtr result = (*pScreen->RestoreAreas) (pWin, pRegion);

  add_changed(pScreen, &reg);

  REGION_UNINIT(pScreen, &reg);

  SCREEN_REWRAP(RestoreAreas);

  return result;
}
#endif

// DisplayCursor - get the cursor shape

static Bool vncHooksDisplayCursor(DeviceIntPtr pDev,
                                  ScreenPtr pScreen_, CursorPtr cursor)
{
  Bool ret;

  SCREEN_UNWRAP(pScreen_, DisplayCursor);

  ret = (*pScreen->DisplayCursor) (pDev, pScreen, cursor);

  /*
   * XXX DIX calls this function with NULL argument to remove cursor sprite from
   * screen. Should we handle this in setCursor as well?
   */
  if (cursor != NullCursor) {
    int width, height;
    int hotX, hotY;

    unsigned char *rgbaData;

    width = cursor->bits->width;
    height = cursor->bits->height;

    hotX = cursor->bits->xhot;
    hotY = cursor->bits->yhot;

    rgbaData = malloc(width * height * 4);
    if (rgbaData == NULL)
      goto out;

#ifdef ARGB_CURSOR
    if (cursor->bits->argb) {
      unsigned char *out;
      CARD32 *in;
      int i;

      in = cursor->bits->argb;
      out = rgbaData;
      for (i = 0; i < width*height; i++) {
        out[0] = (*in >> 16) & 0xff;
        out[1] = (*in >>  8) & 0xff;
        out[2] = (*in >>  0) & 0xff;
        out[3] = (*in >> 24) & 0xff;
        out += 4;
        in++;
      }
    } else {
#endif
      unsigned char *out;
      int xMaskBytesPerRow;

      xMaskBytesPerRow = BitmapBytePad(width);

      out = rgbaData;
      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
          int byte = y * xMaskBytesPerRow + x / 8;
#if (BITMAP_BIT_ORDER == MSBFirst)
          int bit = 7 - x % 8;
#else
          int bit = x % 8;
#endif

          if (cursor->bits->source[byte] & (1 << bit)) {
            out[0] = cursor->foreRed;
            out[1] = cursor->foreGreen;
            out[2] = cursor->foreBlue;
          } else {
            out[0] = cursor->backRed;
            out[1] = cursor->backGreen;
            out[2] = cursor->backBlue;
          }

          if (cursor->bits->mask[byte] & (1 << bit))
            out[3] = 0xff;
          else
            out[3] = 0x00;

          out += 4;
        }
      }
#ifdef ARGB_CURSOR
    }
#endif

    vncSetCursor(pScreen->myNum, width, height, hotX, hotY, rgbaData);

    free(rgbaData);
  }

out:
  SCREEN_REWRAP(DisplayCursor);

  return ret;
}

// BlockHandler - ignore any changes during the block handler - it's likely
// these are just drawing the cursor.

#if XORG <= 112
static void vncHooksBlockHandler(int i, pointer blockData, pointer pTimeout,
                                 pointer pReadmask)
#else
static void vncHooksBlockHandler(ScreenPtr pScreen_, void * pTimeout,
                                 void * pReadmask)
#endif
{
#if XORG <= 112
  SCREEN_UNWRAP(screenInfo.screens[i], BlockHandler);
#else
  SCREEN_UNWRAP(pScreen_, BlockHandler);
#endif

  vncHooksScreen->ignoreHooks++;

#if XORG <= 112
  (*pScreen->BlockHandler) (i, blockData, pTimeout, pReadmask);
#else
  (*pScreen->BlockHandler) (pScreen, pTimeout, pReadmask);
#endif

  vncHooksScreen->ignoreHooks--;

  SCREEN_REWRAP(BlockHandler);
}

#ifdef RENDER

// Composite - The core of XRENDER

static void vncHooksComposite(CARD8 op, PicturePtr pSrc, PicturePtr pMask,
		       PicturePtr pDst, INT16 xSrc, INT16 ySrc, INT16 xMask, 
		       INT16 yMask, INT16 xDst, INT16 yDst, CARD16 width, CARD16 height)
{
  ScreenPtr pScreen = pDst->pDrawable->pScreen;
  vncHooksScreenPtr vncHooksScreen = vncHooksScreenPrivate(pScreen);
  PictureScreenPtr ps = GetPictureScreen(pScreen);

  RegionRec changed;

  if (pDst->pDrawable->type == DRAWABLE_WINDOW &&
      ((WindowPtr) pDst->pDrawable)->viewable) {
    BoxRec box;
    RegionRec fbreg;

    box.x1 = max(pDst->pDrawable->x + xDst, 0);
    box.y1 = max(pDst->pDrawable->y + yDst, 0);
    box.x2 = box.x1 + width;
    box.y2 = box.y1 + height;
    REGION_INIT(pScreen, &changed, &box, 0);

    box.x1 = 0;
    box.y1 = 0;
    box.x2 = pScreen->width;
    box.y2 = pScreen->height;
    REGION_INIT(pScreen, &fbreg, &box, 0);

    REGION_INTERSECT(pScreen, &changed, &changed, &fbreg);

    REGION_UNINIT(pScreen, &fbreg);
  } else {
    REGION_NULL(pScreen, &changed);
  }


  ps->Composite = vncHooksScreen->Composite;
  (*ps->Composite)(op, pSrc, pMask, pDst, xSrc, ySrc,
		   xMask, yMask, xDst, yDst, width, height);
  ps->Composite = vncHooksComposite;

  if (REGION_NOTEMPTY(pScreen, &changed))
    add_changed(pScreen, &changed);

  REGION_UNINIT(pScreen, &changed);
}

static int
GlyphCount(int nlist, GlyphListPtr list, GlyphPtr * glyphs)
{
  int count;

  count = 0;
  while (nlist--) {
    count += list->len;
    list++;
  }

  return count;
}

static RegionPtr
GlyphsToRegion(ScreenPtr pScreen, int nlist, GlyphListPtr list, GlyphPtr *glyphs)
{
  int n;
  GlyphPtr glyph;
  int x, y;

  int nrects = GlyphCount(nlist, list, glyphs);
  xRectangle rects[nrects];
  xRectanglePtr rect;

  x = 0;
  y = 0;
  rect = &rects[0];
  while (nlist--) {
    x += list->xOff;
    y += list->yOff;
    n = list->len;
    list++;
    while (n--) {
      glyph = *glyphs++;
      rect->x = x - glyph->info.x;
      rect->y = y - glyph->info.y;
      rect->width = glyph->info.width;
      rect->height = glyph->info.height;
      x += glyph->info.xOff;
      y += glyph->info.yOff;
      rect++;
    }
  }

  return RECTS_TO_REGION(pScreen, nrects, rects, CT_NONE);
}

// Glyphs - Glyph specific version of Composite (caches and whatnot)

static void vncHooksGlyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
           PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc, int nlists, 
           GlyphListPtr lists, GlyphPtr * glyphs)
{
  ScreenPtr pScreen = pDst->pDrawable->pScreen;
  vncHooksScreenPtr vncHooksScreen = vncHooksScreenPrivate(pScreen);
  PictureScreenPtr ps = GetPictureScreen(pScreen);

  RegionPtr changed;

  if (pDst->pDrawable->type == DRAWABLE_WINDOW &&
      ((WindowPtr) pDst->pDrawable)->viewable) {
    BoxRec fbbox;
    RegionRec fbreg;

    changed = GlyphsToRegion(pScreen, nlists, lists, glyphs);
    REGION_TRANSLATE(pScreen, changed,
                     pDst->pDrawable->x, pDst->pDrawable->y);

    fbbox.x1 = 0;
    fbbox.y1 = 0;
    fbbox.x2 = pScreen->width;
    fbbox.y2 = pScreen->height;
    REGION_INIT(pScreen, &fbreg, &fbbox, 0);

    REGION_INTERSECT(pScreen, changed, changed, &fbreg);

    REGION_UNINIT(pScreen, &fbreg);
  } else {
    changed = REGION_CREATE(pScreen, NullBox, 0);
  }

  ps->Glyphs = vncHooksScreen->Glyphs;
  (*ps->Glyphs)(op, pSrc, pDst, maskFormat, xSrc, ySrc, nlists, lists, glyphs);
  ps->Glyphs = vncHooksGlyphs;

  if (REGION_NOTEMPTY(pScreen, changed))
    add_changed(pScreen, changed);

  REGION_DESTROY(pScreen, changed);
}

#endif /* RENDER */

// RandRSetConfig - follow any framebuffer changes

#ifdef RANDR

static Bool vncHooksRandRSetConfig(ScreenPtr pScreen, Rotation rotation,
                                   int rate, RRScreenSizePtr pSize)
{
  vncHooksScreenPtr vncHooksScreen = vncHooksScreenPrivate(pScreen);
  rrScrPrivPtr rp = rrGetScrPriv(pScreen);
  Bool ret;

  vncPreScreenResize(pScreen->myNum);

  rp->rrSetConfig = vncHooksScreen->RandRSetConfig;
  ret = (*rp->rrSetConfig)(pScreen, rotation, rate, pSize);
  rp->rrSetConfig = vncHooksRandRSetConfig;

  vncPostScreenResize(pScreen->myNum, ret, pScreen->width, pScreen->height);

  if (!ret)
    return FALSE;

  return TRUE;
}

static Bool vncHooksRandRScreenSetSize(ScreenPtr pScreen,
                                       CARD16 width, CARD16 height,
                                       CARD32 mmWidth, CARD32 mmHeight)
{
  vncHooksScreenPtr vncHooksScreen = vncHooksScreenPrivate(pScreen);
  rrScrPrivPtr rp = rrGetScrPriv(pScreen);
  Bool ret;

  vncPreScreenResize(pScreen->myNum);

  rp->rrScreenSetSize = vncHooksScreen->RandRScreenSetSize;
  ret = (*rp->rrScreenSetSize)(pScreen, width, height, mmWidth, mmHeight);
  rp->rrScreenSetSize = vncHooksRandRScreenSetSize;

  vncPostScreenResize(pScreen->myNum, ret, pScreen->width, pScreen->height);

  if (!ret)
    return FALSE;

  return TRUE;
}

static Bool vncHooksRandRCrtcSet(ScreenPtr pScreen, RRCrtcPtr crtc,
                                 RRModePtr mode, int x, int y,
                                 Rotation rotation, int num_outputs,
                                 RROutputPtr *outputs)
{
  vncHooksScreenPtr vncHooksScreen = vncHooksScreenPrivate(pScreen);
  rrScrPrivPtr rp = rrGetScrPriv(pScreen);
  Bool ret;

  rp->rrCrtcSet = vncHooksScreen->RandRCrtcSet;
  ret = (*rp->rrCrtcSet)(pScreen, crtc, mode, x, y, rotation,
                         num_outputs, outputs);
  rp->rrCrtcSet = vncHooksRandRCrtcSet;

  if (!ret)
    return FALSE;

  vncRefreshScreenLayout(pScreen->myNum);

  return TRUE;
}

#endif /* RANDR */

/////////////////////////////////////////////////////////////////////////////
//
// GC "funcs"
//

// Unwrap and rewrap helpers

#define GC_FUNC_PROLOGUE(pGC, name)\
    vncHooksGCPtr pGCPriv = vncHooksGCPrivate(pGC);\
    (pGC)->funcs = pGCPriv->wrappedFuncs;\
    if(pGCPriv->wrappedOps)\
        (pGC)->ops = pGCPriv->wrappedOps; \
    DBGPRINT((stderr,"vncHooks" #name " called\n"))

#define GC_FUNC_EPILOGUE(pGC)\
    pGCPriv->wrappedFuncs = (pGC)->funcs;\
    (pGC)->funcs = &vncHooksGCFuncs;\
    if(pGCPriv->wrappedOps) {\
        pGCPriv->wrappedOps = (pGC)->ops;\
        (pGC)->ops = &vncHooksGCOps;\
    }

// ValidateGC - wrap the "ops" if a viewable window OR the screen pixmap

static void vncHooksValidateGC(GCPtr pGC, unsigned long changes,
                               DrawablePtr pDrawable)
{
  GC_FUNC_PROLOGUE(pGC, ValidateGC);
  (*pGC->funcs->ValidateGC) (pGC, changes, pDrawable);
  if ((pDrawable->type == DRAWABLE_WINDOW &&
       ((WindowPtr) pDrawable)->viewable) ||
      (pDrawable == &pGC->pScreen->GetScreenPixmap(pGC->pScreen)->drawable)) {
    pGCPriv->wrappedOps = pGC->ops;
    DBGPRINT((stderr,"vncHooksValidateGC: wrapped GC ops\n"));
  } else {
    pGCPriv->wrappedOps = NULL;
  }
  GC_FUNC_EPILOGUE(pGC);
}

// Other GC funcs - just unwrap and call on

static void vncHooksChangeGC(GCPtr pGC, unsigned long mask) {
  GC_FUNC_PROLOGUE(pGC, ChangeGC);
  (*pGC->funcs->ChangeGC) (pGC, mask);
  GC_FUNC_EPILOGUE(pGC);
}
static void vncHooksCopyGC(GCPtr src, unsigned long mask, GCPtr dst) {
  GC_FUNC_PROLOGUE(dst, CopyGC);
  (*dst->funcs->CopyGC) (src, mask, dst);
  GC_FUNC_EPILOGUE(dst);
}
static void vncHooksDestroyGC(GCPtr pGC) {
  GC_FUNC_PROLOGUE(pGC, DestroyGC);
  (*pGC->funcs->DestroyGC) (pGC);
  GC_FUNC_EPILOGUE(pGC);
}
static void vncHooksChangeClip(GCPtr pGC, int type, void * pValue, int nrects)
{
  GC_FUNC_PROLOGUE(pGC, ChangeClip);
  (*pGC->funcs->ChangeClip) (pGC, type, pValue, nrects);
  GC_FUNC_EPILOGUE(pGC);
}
static void vncHooksDestroyClip(GCPtr pGC) {
  GC_FUNC_PROLOGUE(pGC, DestroyClip);
  (*pGC->funcs->DestroyClip) (pGC);
  GC_FUNC_EPILOGUE(pGC);
}
static void vncHooksCopyClip(GCPtr dst, GCPtr src) {
  GC_FUNC_PROLOGUE(dst, CopyClip);
  (*dst->funcs->CopyClip) (dst, src);
  GC_FUNC_EPILOGUE(dst);
}


/////////////////////////////////////////////////////////////////////////////
//
// GC "ops"
//

// Unwrap and rewrap helpers

#define GC_OP_PROLOGUE(pGC, name)\
    vncHooksGCPtr pGCPriv = vncHooksGCPrivate(pGC);\
    GCFuncs *oldFuncs = pGC->funcs;\
    pGC->funcs = pGCPriv->wrappedFuncs;\
    pGC->ops = pGCPriv->wrappedOps; \
    DBGPRINT((stderr,"vncHooks" #name " called\n"))

#define GC_OP_EPILOGUE(pGC)\
    pGCPriv->wrappedOps = pGC->ops;\
    pGC->funcs = oldFuncs;\
    pGC->ops   = &vncHooksGCOps

// FillSpans - assume the entire clip region is damaged. This is pessimistic,
// but I believe this function is rarely used so it doesn't matter.

static void vncHooksFillSpans(DrawablePtr pDrawable, GCPtr pGC, int nInit,
                              DDXPointPtr pptInit, int *pwidthInit,
                              int fSorted)
{
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, FillSpans);

  REGION_NULL(pGC->pScreen, &reg);
  REGION_COPY(pGC->pScreen, &reg, pGC->pCompositeClip);

  if (pDrawable->type == DRAWABLE_WINDOW)
    REGION_INTERSECT(pScreen, &reg, &reg, &((WindowPtr)pDrawable)->borderClip);

  (*pGC->ops->FillSpans) (pDrawable, pGC, nInit, pptInit, pwidthInit, fSorted);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

  GC_OP_EPILOGUE(pGC);
}

// SetSpans - assume the entire clip region is damaged.  This is pessimistic,
// but I believe this function is rarely used so it doesn't matter.

static void vncHooksSetSpans(DrawablePtr pDrawable, GCPtr pGC, char *psrc,
                             DDXPointPtr ppt, int *pwidth, int nspans,
                             int fSorted)
{
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, SetSpans);

  REGION_NULL(pGC->pScreen, &reg);
  REGION_COPY(pGC->pScreen, &reg, pGC->pCompositeClip);

  if (pDrawable->type == DRAWABLE_WINDOW)
    REGION_INTERSECT(pScreen, &reg, &reg, &((WindowPtr)pDrawable)->borderClip);

  (*pGC->ops->SetSpans) (pDrawable, pGC, psrc, ppt, pwidth, nspans, fSorted);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

  GC_OP_EPILOGUE(pGC);
}

// PutImage - changed region is the given rectangle, clipped by pCompositeClip

static void vncHooksPutImage(DrawablePtr pDrawable, GCPtr pGC, int depth,
                             int x, int y, int w, int h, int leftPad,
                             int format, char *pBits)
{
  BoxRec box;
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, PutImage);

  box.x1 = x + pDrawable->x;
  box.y1 = y + pDrawable->y;
  box.x2 = box.x1 + w;
  box.y2 = box.y1 + h;

  REGION_INIT(pGC->pScreen, &reg, &box, 0);
  REGION_INTERSECT(pGC->pScreen, &reg, &reg, pGC->pCompositeClip);

  (*pGC->ops->PutImage) (pDrawable, pGC, depth, x, y, w, h, leftPad, format,
                         pBits);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

  GC_OP_EPILOGUE(pGC);
}

// CopyArea - destination of the copy is the dest rectangle, clipped by
// pCompositeClip.  Any parts of the destination which cannot be copied from
// the source (could be all of it) go into the changed region.

static RegionPtr vncHooksCopyArea(DrawablePtr pSrc, DrawablePtr pDst,
                                  GCPtr pGC, int srcx, int srcy, int w, int h,
                                  int dstx, int dsty)
{
  RegionRec dst, src, changed;

  RegionPtr ret;

  GC_OP_PROLOGUE(pGC, CopyArea);

  // Apparently this happens now and then...
  if ((w == 0) || (h == 0))
    REGION_NULL(pGC->pScreen, &dst);
  else {
    BoxRec box;

    box.x1 = dstx + pDst->x;
    box.y1 = dsty + pDst->y;
    box.x2 = box.x1 + w;
    box.y2 = box.y1 + h;

    REGION_INIT(pGC->pScreen, &dst, &box, 0);
  }

  REGION_INTERSECT(pGC->pScreen, &dst, &dst, pGC->pCompositeClip);

  // The source of the data has to be something that's on screen.
  // This means either a window, or the screen pixmap.
  if ((pSrc->pScreen == pGC->pScreen) &&
      ((pSrc->type == DRAWABLE_WINDOW) ||
       (pSrc == &pGC->pScreen->GetScreenPixmap(pGC->pScreen)->drawable))) {
    BoxRec box;

    box.x1 = srcx + pSrc->x;
    box.y1 = srcy + pSrc->y;
    box.x2 = box.x1 + w;
    box.y2 = box.y1 + h;

    REGION_INIT(pGC->pScreen, &src, &box, 0);

    if ((pSrc->type == DRAWABLE_WINDOW) &&
        REGION_NOTEMPTY(pScreen, &((WindowPtr)pSrc)->clipList)) {
      REGION_INTERSECT(pScreen, &src, &src, &((WindowPtr)pSrc)->clipList);
    }

    REGION_TRANSLATE(pScreen, &src,
                     dstx + pDst->x - srcx - pSrc->x,
                     dsty + pDst->y - srcy - pSrc->y);
  } else {
    REGION_NULL(pGC->pScreen, &src);
  }

  REGION_NULL(pGC->pScreen, &changed);

  REGION_SUBTRACT(pScreen, &changed, &dst, &src);
  REGION_INTERSECT(pScreen, &dst, &dst, &src);

  ret = (*pGC->ops->CopyArea) (pSrc, pDst, pGC, srcx, srcy, w, h, dstx, dsty);

  if (REGION_NOTEMPTY(pScreen, &dst))
    add_copied(pGC->pScreen, &dst,
               dstx + pDst->x - srcx - pSrc->x,
               dsty + pDst->y - srcy - pSrc->y);

  if (REGION_NOTEMPTY(pScreen, &changed))
    add_changed(pGC->pScreen, &changed);

  REGION_UNINIT(pGC->pScreen, &dst);
  REGION_UNINIT(pGC->pScreen, &src);
  REGION_UNINIT(pGC->pScreen, &changed);

  GC_OP_EPILOGUE(pGC);

  return ret;
}


// CopyPlane - changed region is the destination rectangle, clipped by
// pCompositeClip

static RegionPtr vncHooksCopyPlane(DrawablePtr pSrc, DrawablePtr pDst,
                                   GCPtr pGC, int srcx, int srcy, int w, int h,
                                   int dstx, int dsty, unsigned long plane)
{
  BoxRec box;
  RegionRec reg;

  RegionPtr ret;

  GC_OP_PROLOGUE(pGC, CopyPlane);

  box.x1 = dstx + pDst->x;
  box.y1 = dsty + pDst->y;
  box.x2 = box.x1 + w;
  box.y2 = box.y1 + h;

  REGION_INIT(pGC->pScreen, &reg, &box, 0);
  REGION_INTERSECT(pGC->pScreen, &reg, &reg, pGC->pCompositeClip);

  ret = (*pGC->ops->CopyPlane) (pSrc, pDst, pGC, srcx, srcy, w, h,
                                dstx, dsty, plane);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

  GC_OP_EPILOGUE(pGC);

  return ret;
}

// PolyPoint - changed region is the bounding rect, clipped by pCompositeClip

static void vncHooksPolyPoint(DrawablePtr pDrawable, GCPtr pGC, int mode,
                              int npt, xPoint *pts)
{
  int minX, minY, maxX, maxY;
  int i;

  BoxRec box;
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, PolyPoint);

  if (npt == 0) {
    (*pGC->ops->PolyPoint) (pDrawable, pGC, mode, npt, pts);
    goto out;
  }

  minX = pts[0].x;
  maxX = pts[0].x;
  minY = pts[0].y;
  maxY = pts[0].y;

  if (mode == CoordModePrevious) {
    int x = pts[0].x;
    int y = pts[0].y;

    for (i = 1; i < npt; i++) {
      x += pts[i].x;
      y += pts[i].y;
      if (x < minX) minX = x;
      if (x > maxX) maxX = x;
      if (y < minY) minY = y;
      if (y > maxY) maxY = y;
    }
  } else {
    for (i = 1; i < npt; i++) {
      if (pts[i].x < minX) minX = pts[i].x;
      if (pts[i].x > maxX) maxX = pts[i].x;
      if (pts[i].y < minY) minY = pts[i].y;
      if (pts[i].y > maxY) maxY = pts[i].y;
    }
  }

  box.x1 = minX + pDrawable->x;
  box.y1 = minY + pDrawable->y;
  box.x2 = maxX + 1 + pDrawable->x;
  box.y2 = maxY + 1 + pDrawable->y;

  REGION_INIT(pGC->pScreen, &reg, &box, 0);
  REGION_INTERSECT(pGC->pScreen, &reg, &reg, pGC->pCompositeClip);

  (*pGC->ops->PolyPoint) (pDrawable, pGC, mode, npt, pts);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

out:
  GC_OP_EPILOGUE(pGC);
}

// Polylines - changed region is the union of the bounding rects of each line,
// clipped by pCompositeClip.  If there are more than MAX_RECTS_PER_OP lines,
// just use the bounding rect of all the lines.

static void vncHooksPolylines(DrawablePtr pDrawable, GCPtr pGC, int mode,
                              int npt, DDXPointPtr ppts)
{
  int nRegRects;
  xRectangle regRects[MAX_RECTS_PER_OP];

  int lw;

  RegionPtr reg;

  GC_OP_PROLOGUE(pGC, Polylines);

  if (npt == 0) {
    (*pGC->ops->Polylines) (pDrawable, pGC, mode, npt, ppts);
    goto out;
  }

  nRegRects = npt - 1;

  lw = pGC->lineWidth;
  if (lw == 0)
    lw = 1;

  if (npt == 1)
  {
    // a single point
    nRegRects = 1;
    regRects[0].x = pDrawable->x + ppts[0].x - lw;
    regRects[0].y = pDrawable->y + ppts[0].y - lw;
    regRects[0].width = 2*lw;
    regRects[0].height = 2*lw;
  }
  else
  {
    /*
     * mitered joins can project quite a way from
     * the line end; the 11 degree miter limit limits
     * this extension to lw / (2 * tan(11/2)), rounded up
     * and converted to int yields 6 * lw
     */

    int extra;

    int prevX, prevY, curX, curY;
    int rectX1, rectY1, rectX2, rectY2;
    int minX, minY, maxX, maxY;

    int i;

    extra = lw / 2;
    if (pGC->joinStyle == JoinMiter) {
      extra = 6 * lw;
    }

    prevX = ppts[0].x + pDrawable->x;
    prevY = ppts[0].y + pDrawable->y;
    minX = maxX = prevX;
    minY = maxY = prevY;

    for (i = 0; i < nRegRects; i++) {
      if (mode == CoordModeOrigin) {
        curX = pDrawable->x + ppts[i+1].x;
        curY = pDrawable->y + ppts[i+1].y;
      } else {
        curX = prevX + ppts[i+1].x;
        curY = prevY + ppts[i+1].y;
      }

      if (prevX > curX) {
        rectX1 = curX - extra;
        rectX2 = prevX + extra + 1;
      } else {
        rectX1 = prevX - extra;
        rectX2 = curX + extra + 1;
      }

      if (prevY > curY) {
        rectY1 = curY - extra;
        rectY2 = prevY + extra + 1;
      } else {
        rectY1 = prevY - extra;
        rectY2 = curY + extra + 1;
      }

      if (nRegRects <= MAX_RECTS_PER_OP) {
        regRects[i].x = rectX1;
        regRects[i].y = rectY1;
        regRects[i].width = rectX2 - rectX1;
        regRects[i].height = rectY2 - rectY1;
      } else {
        if (rectX1 < minX) minX = rectX1;
        if (rectY1 < minY) minY = rectY1;
        if (rectX2 > maxX) maxX = rectX2;
        if (rectY2 > maxY) maxY = rectY2;
      }

      prevX = curX;
      prevY = curY;
    }

    if (nRegRects > MAX_RECTS_PER_OP) {
      regRects[0].x = minX;
      regRects[0].y = minY;
      regRects[0].width = maxX - minX;
      regRects[0].height = maxY - minY;
      nRegRects = 1;
    }
  }

  reg = RECTS_TO_REGION(pGC->pScreen, nRegRects, regRects, CT_NONE);
  REGION_INTERSECT(pGC->pScreen, reg, reg, pGC->pCompositeClip);

  (*pGC->ops->Polylines) (pDrawable, pGC, mode, npt, ppts);

  add_changed(pGC->pScreen, reg);

  REGION_DESTROY(pGC->pScreen, reg);

out:
  GC_OP_EPILOGUE(pGC);
}

// PolySegment - changed region is the union of the bounding rects of each
// segment, clipped by pCompositeClip.  If there are more than MAX_RECTS_PER_OP
// segments, just use the bounding rect of all the segments.

static void vncHooksPolySegment(DrawablePtr pDrawable, GCPtr pGC, int nseg,
                                xSegment *segs)
{
  xRectangle regRects[MAX_RECTS_PER_OP];
  int nRegRects;

  int lw, extra;

  int rectX1, rectY1, rectX2, rectY2;
  int minX, minY, maxX, maxY;

  int i;

  RegionPtr reg;

  GC_OP_PROLOGUE(pGC, PolySegment);

  if (nseg == 0) {
    (*pGC->ops->PolySegment) (pDrawable, pGC, nseg, segs);
    goto out;
  }

  nRegRects = nseg;

  lw = pGC->lineWidth;
  extra = lw / 2;

  minX = maxX = segs[0].x1;
  minY = maxY = segs[0].y1;

  for (i = 0; i < nseg; i++) {
    if (segs[i].x1 > segs[i].x2) {
      rectX1 = pDrawable->x + segs[i].x2 - extra;
      rectX2 = pDrawable->x + segs[i].x1 + extra + 1;
    } else {
      rectX1 = pDrawable->x + segs[i].x1 - extra;
      rectX2 = pDrawable->x + segs[i].x2 + extra + 1;
    }

    if (segs[i].y1 > segs[i].y2) {
      rectY1 = pDrawable->y + segs[i].y2 - extra;
      rectY2 = pDrawable->y + segs[i].y1 + extra + 1;
    } else {
      rectY1 = pDrawable->y + segs[i].y1 - extra;
      rectY2 = pDrawable->y + segs[i].y2 + extra + 1;
    }

    if (nseg <= MAX_RECTS_PER_OP) {
      regRects[i].x = rectX1;
      regRects[i].y = rectY1;
      regRects[i].width = rectX2 - rectX1;
      regRects[i].height = rectY2 - rectY1;
    } else {
      if (rectX1 < minX) minX = rectX1;
      if (rectY1 < minY) minY = rectY1;
      if (rectX2 > maxX) maxX = rectX2;
      if (rectY2 > maxY) maxY = rectY2;
    }
  }

  if (nseg > MAX_RECTS_PER_OP) {
    regRects[0].x = minX;
    regRects[0].y = minY;
    regRects[0].width = maxX - minX;
    regRects[0].height = maxY - minY;
    nRegRects = 1;
  }

  reg = RECTS_TO_REGION(pGC->pScreen, nRegRects, regRects, CT_NONE);
  REGION_INTERSECT(pGC->pScreen, reg, reg, pGC->pCompositeClip);

  (*pGC->ops->PolySegment) (pDrawable, pGC, nseg, segs);

  add_changed(pGC->pScreen, reg);

  REGION_DESTROY(pGC->pScreen, reg);

out:
  GC_OP_EPILOGUE(pGC);
}

// PolyRectangle - changed region is the union of the bounding rects around
// each side of the outline rectangles, clipped by pCompositeClip.  If there
// are more than MAX_RECTS_PER_OP rectangles, just use the bounding rect of all
// the rectangles.

static void vncHooksPolyRectangle(DrawablePtr pDrawable, GCPtr pGC, int nrects,
                                  xRectangle *rects)
{
  xRectangle regRects[MAX_RECTS_PER_OP*4];
  int nRegRects;

  int lw, extra;

  int rectX1, rectY1, rectX2, rectY2;
  int minX, minY, maxX, maxY;

  int i;

  RegionPtr reg;

  GC_OP_PROLOGUE(pGC, PolyRectangle);

  if (nrects == 0) {
    (*pGC->ops->PolyRectangle) (pDrawable, pGC, nrects, rects);
    goto out;
  }

  nRegRects = nrects * 4;

  lw = pGC->lineWidth;
  extra = lw / 2;

  minX = maxX = rects[0].x;
  minY = maxY = rects[0].y;

  for (i = 0; i < nrects; i++) {
    if (nrects <= MAX_RECTS_PER_OP) {
      regRects[i*4].x = rects[i].x - extra + pDrawable->x;
      regRects[i*4].y = rects[i].y - extra + pDrawable->y;
      regRects[i*4].width = rects[i].width + 1 + 2 * extra;
      regRects[i*4].height = 1 + 2 * extra;

      regRects[i*4+1].x = rects[i].x - extra + pDrawable->x;
      regRects[i*4+1].y = rects[i].y - extra + pDrawable->y;
      regRects[i*4+1].width = 1 + 2 * extra;
      regRects[i*4+1].height = rects[i].height + 1 + 2 * extra;

      regRects[i*4+2].x = rects[i].x + rects[i].width - extra + pDrawable->x;
      regRects[i*4+2].y = rects[i].y - extra + pDrawable->y;
      regRects[i*4+2].width = 1 + 2 * extra;
      regRects[i*4+2].height = rects[i].height + 1 + 2 * extra;

      regRects[i*4+3].x = rects[i].x - extra + pDrawable->x;
      regRects[i*4+3].y = rects[i].y + rects[i].height - extra + pDrawable->y;
      regRects[i*4+3].width = rects[i].width + 1 + 2 * extra;
      regRects[i*4+3].height = 1 + 2 * extra;
    } else {
      rectX1 = pDrawable->x + rects[i].x - extra;
      rectY1 = pDrawable->y + rects[i].y - extra;
      rectX2 = pDrawable->x + rects[i].x + rects[i].width + extra+1;
      rectY2 = pDrawable->y + rects[i].y + rects[i].height + extra+1;
      if (rectX1 < minX) minX = rectX1;
      if (rectY1 < minY) minY = rectY1;
      if (rectX2 > maxX) maxX = rectX2;
      if (rectY2 > maxY) maxY = rectY2;
    }
  }

  if (nrects > MAX_RECTS_PER_OP) {
    regRects[0].x = minX;
    regRects[0].y = minY;
    regRects[0].width = maxX - minX;
    regRects[0].height = maxY - minY;
    nRegRects = 1;
  }

  reg = RECTS_TO_REGION(pGC->pScreen, nRegRects, regRects, CT_NONE);
  REGION_INTERSECT(pGC->pScreen, reg, reg, pGC->pCompositeClip);

  (*pGC->ops->PolyRectangle) (pDrawable, pGC, nrects, rects);

  add_changed(pGC->pScreen, reg);

  REGION_DESTROY(pGC->pScreen, reg);

out:
  GC_OP_EPILOGUE(pGC);
}

// PolyArc - changed region is the union of bounding rects around each arc,
// clipped by pCompositeClip.  If there are more than MAX_RECTS_PER_OP
// arcs, just use the bounding rect of all the arcs.

static void vncHooksPolyArc(DrawablePtr pDrawable, GCPtr pGC, int narcs,
                            xArc *arcs)
{
  xRectangle regRects[MAX_RECTS_PER_OP];
  int nRegRects;

  int lw, extra;

  int rectX1, rectY1, rectX2, rectY2;
  int minX, minY, maxX, maxY;

  int i;

  RegionPtr reg;

  GC_OP_PROLOGUE(pGC, PolyArc);

  if (narcs == 0) {
    (*pGC->ops->PolyArc) (pDrawable, pGC, narcs, arcs);
    goto out;
  }

  nRegRects = narcs;

  lw = pGC->lineWidth;
  if (lw == 0)
    lw = 1;
  extra = lw / 2;

  minX = maxX = arcs[0].x;
  minY = maxY = arcs[0].y;

  for (i = 0; i < narcs; i++) {
    if (narcs <= MAX_RECTS_PER_OP) {
      regRects[i].x = arcs[i].x - extra + pDrawable->x;
      regRects[i].y = arcs[i].y - extra + pDrawable->y;
      regRects[i].width = arcs[i].width + lw;
      regRects[i].height = arcs[i].height + lw;
    } else {
      rectX1 = pDrawable->x + arcs[i].x - extra;
      rectY1 = pDrawable->y + arcs[i].y - extra;
      rectX2 = pDrawable->x + arcs[i].x + arcs[i].width + lw;
      rectY2 = pDrawable->y + arcs[i].y + arcs[i].height + lw;
      if (rectX1 < minX) minX = rectX1;
      if (rectY1 < minY) minY = rectY1;
      if (rectX2 > maxX) maxX = rectX2;
      if (rectY2 > maxY) maxY = rectY2;
    }
  }

  if (narcs > MAX_RECTS_PER_OP) {
    regRects[0].x = minX;
    regRects[0].y = minY;
    regRects[0].width = maxX - minX;
    regRects[0].height = maxY - minY;
    nRegRects = 1;
  }

  reg = RECTS_TO_REGION(pGC->pScreen, nRegRects, regRects, CT_NONE);
  REGION_INTERSECT(pGC->pScreen, reg, reg, pGC->pCompositeClip);

  (*pGC->ops->PolyArc) (pDrawable, pGC, narcs, arcs);

  add_changed(pGC->pScreen, reg);

  REGION_DESTROY(pGC->pScreen, reg);

out:
  GC_OP_EPILOGUE(pGC);
}


// FillPolygon - changed region is the bounding rect around the polygon,
// clipped by pCompositeClip

static void vncHooksFillPolygon(DrawablePtr pDrawable, GCPtr pGC, int shape,
                                int mode, int count, DDXPointPtr pts)
{
  int minX, minY, maxX, maxY;
  int i;

  BoxRec box;
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, FillPolygon);

  if (count == 0) {
    (*pGC->ops->FillPolygon) (pDrawable, pGC, shape, mode, count, pts);
    goto out;
  }

  minX = pts[0].x;
  maxX = pts[0].x;
  minY = pts[0].y;
  maxY = pts[0].y;

  if (mode == CoordModePrevious) {
    int x = pts[0].x;
    int y = pts[0].y;

    for (i = 1; i < count; i++) {
      x += pts[i].x;
      y += pts[i].y;
      if (x < minX) minX = x;
      if (x > maxX) maxX = x;
      if (y < minY) minY = y;
      if (y > maxY) maxY = y;
    }
  } else {
    for (i = 1; i < count; i++) {
      if (pts[i].x < minX) minX = pts[i].x;
      if (pts[i].x > maxX) maxX = pts[i].x;
      if (pts[i].y < minY) minY = pts[i].y;
      if (pts[i].y > maxY) maxY = pts[i].y;
    }
  }

  box.x1 = minX + pDrawable->x;
  box.y1 = minY + pDrawable->y;
  box.x2 = maxX + 1 + pDrawable->x;
  box.y2 = maxY + 1 + pDrawable->y;

  REGION_INIT(pGC->pScreen, &reg, &box, 0);
  REGION_INTERSECT(pGC->pScreen, &reg, &reg, pGC->pCompositeClip);

  (*pGC->ops->FillPolygon) (pDrawable, pGC, shape, mode, count, pts);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

out:
  GC_OP_EPILOGUE(pGC);
}

// PolyFillRect - changed region is the union of the rectangles, clipped by
// pCompositeClip.  If there are more than MAX_RECTS_PER_OP rectangles, just
// use the bounding rect of all the rectangles.

static void vncHooksPolyFillRect(DrawablePtr pDrawable, GCPtr pGC, int nrects,
                                 xRectangle *rects)
{
  xRectangle regRects[MAX_RECTS_PER_OP];
  int nRegRects;
  int rectX1, rectY1, rectX2, rectY2;
  int minX, minY, maxX, maxY;
  int i;

  RegionPtr reg;

  GC_OP_PROLOGUE(pGC, PolyFillRect);

  if (nrects == 0) {
    (*pGC->ops->PolyFillRect) (pDrawable, pGC, nrects, rects);
    goto out;
  }

  nRegRects = nrects;
  minX = maxX = rects[0].x;
  minY = maxY = rects[0].y;

  for (i = 0; i < nrects; i++) {
    if (nrects <= MAX_RECTS_PER_OP) {
      regRects[i].x = rects[i].x + pDrawable->x;
      regRects[i].y = rects[i].y + pDrawable->y;
      regRects[i].width = rects[i].width;
      regRects[i].height = rects[i].height;
    } else {
      rectX1 = pDrawable->x + rects[i].x;
      rectY1 = pDrawable->y + rects[i].y;
      rectX2 = pDrawable->x + rects[i].x + rects[i].width;
      rectY2 = pDrawable->y + rects[i].y + rects[i].height;
      if (rectX1 < minX) minX = rectX1;
      if (rectY1 < minY) minY = rectY1;
      if (rectX2 > maxX) maxX = rectX2;
      if (rectY2 > maxY) maxY = rectY2;
    }
  }

  if (nrects > MAX_RECTS_PER_OP) {
    regRects[0].x = minX;
    regRects[0].y = minY;
    regRects[0].width = maxX - minX;
    regRects[0].height = maxY - minY;
    nRegRects = 1;
  }

  reg = RECTS_TO_REGION(pGC->pScreen, nRegRects, regRects, CT_NONE);
  REGION_INTERSECT(pGC->pScreen, reg, reg, pGC->pCompositeClip);

  (*pGC->ops->PolyFillRect) (pDrawable, pGC, nrects, rects);

  add_changed(pGC->pScreen, reg);

  REGION_DESTROY(pGC->pScreen, reg);

out:
  GC_OP_EPILOGUE(pGC);
}

// PolyFillArc - changed region is the union of bounding rects around each arc,
// clipped by pCompositeClip.  If there are more than MAX_RECTS_PER_OP arcs,
// just use the bounding rect of all the arcs.

static void vncHooksPolyFillArc(DrawablePtr pDrawable, GCPtr pGC, int narcs,
                                xArc *arcs)
{
  xRectangle regRects[MAX_RECTS_PER_OP];
  int nRegRects;

  int lw, extra;

  int rectX1, rectY1, rectX2, rectY2;
  int minX, minY, maxX, maxY;

  int i;

  RegionPtr reg;

  GC_OP_PROLOGUE(pGC, PolyFillArc);

  if (narcs == 0) {
    (*pGC->ops->PolyFillArc) (pDrawable, pGC, narcs, arcs);
    goto out;
  }

  nRegRects = narcs;

  lw = pGC->lineWidth;
  if (lw == 0)
    lw = 1;
  extra = lw / 2;

  minX = maxX = arcs[0].x;
  minY = maxY = arcs[0].y;

  for (i = 0; i < narcs; i++) {
    if (narcs <= MAX_RECTS_PER_OP) {
      regRects[i].x = arcs[i].x - extra + pDrawable->x;
      regRects[i].y = arcs[i].y - extra + pDrawable->y;
      regRects[i].width = arcs[i].width + lw;
      regRects[i].height = arcs[i].height + lw;
    } else {
      rectX1 = pDrawable->x + arcs[i].x - extra;
      rectY1 = pDrawable->y + arcs[i].y - extra;
      rectX2 = pDrawable->x + arcs[i].x + arcs[i].width + lw;
      rectY2 = pDrawable->y + arcs[i].y + arcs[i].height + lw;
      if (rectX1 < minX) minX = rectX1;
      if (rectY1 < minY) minY = rectY1;
      if (rectX2 > maxX) maxX = rectX2;
      if (rectY2 > maxY) maxY = rectY2;
    }
  }

  if (narcs > MAX_RECTS_PER_OP) {
    regRects[0].x = minX;
    regRects[0].y = minY;
    regRects[0].width = maxX - minX;
    regRects[0].height = maxY - minY;
    nRegRects = 1;
  }

  reg = RECTS_TO_REGION(pGC->pScreen, nRegRects, regRects, CT_NONE);
  REGION_INTERSECT(pGC->pScreen, reg, reg, pGC->pCompositeClip);

  (*pGC->ops->PolyFillArc) (pDrawable, pGC, narcs, arcs);

  add_changed(pGC->pScreen, reg);

  REGION_DESTROY(pGC->pScreen, reg);

out:
  GC_OP_EPILOGUE(pGC);
}

// GetTextBoundingRect - calculate a bounding rectangle around n chars of a
// font.  Not particularly accurate, but good enough.

static void GetTextBoundingRect(DrawablePtr pDrawable, FontPtr font, int x,
                                int y, int nchars, BoxPtr box)
{
  int ascent = max(FONTASCENT(font), FONTMAXBOUNDS(font, ascent));
  int descent = max(FONTDESCENT(font), FONTMAXBOUNDS(font, descent));
  int charWidth = max(FONTMAXBOUNDS(font,rightSideBearing),
                      FONTMAXBOUNDS(font,characterWidth));

  box->x1 = pDrawable->x + x;
  box->y1 = pDrawable->y + y - ascent;
  box->x2 = box->x1 + charWidth * nchars;
  box->y2 = box->y1 + ascent + descent;

  if (FONTMINBOUNDS(font,leftSideBearing) < 0)
    box->x1 += FONTMINBOUNDS(font,leftSideBearing);
}

// PolyText8 - changed region is bounding rect around count chars, clipped by
// pCompositeClip

static int vncHooksPolyText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                             int count, char *chars)
{
  int ret;
  BoxRec box;
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, PolyText8);

  if (count == 0) {
    ret = (*pGC->ops->PolyText8) (pDrawable, pGC, x, y, count, chars);
    goto out;
  }

  GetTextBoundingRect(pDrawable, pGC->font, x, y, count, &box);

  REGION_INIT(pGC->pScreen, &reg, &box, 0);
  REGION_INTERSECT(pGC->pScreen, &reg, &reg, pGC->pCompositeClip);

  ret = (*pGC->ops->PolyText8) (pDrawable, pGC, x, y, count, chars);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

out:
  GC_OP_EPILOGUE(pGC);

  return ret;
}

// PolyText16 - changed region is bounding rect around count chars, clipped by
// pCompositeClip

static int vncHooksPolyText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                              int count, unsigned short *chars)
{
  int ret;
  BoxRec box;
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, PolyText16);

  if (count == 0) {
    ret = (*pGC->ops->PolyText16) (pDrawable, pGC, x, y, count, chars);
    goto out;
  }

  GetTextBoundingRect(pDrawable, pGC->font, x, y, count, &box);

  REGION_INIT(pGC->pScreen, &reg, &box, 0);
  REGION_INTERSECT(pGC->pScreen, &reg, &reg, pGC->pCompositeClip);

  ret = (*pGC->ops->PolyText16) (pDrawable, pGC, x, y, count, chars);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

out:
  GC_OP_EPILOGUE(pGC);

  return ret;
}

// ImageText8 - changed region is bounding rect around count chars, clipped by
// pCompositeClip

static void vncHooksImageText8(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                               int count, char *chars)
{
  BoxRec box;
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, ImageText8);

  if (count == 0) {
    (*pGC->ops->ImageText8) (pDrawable, pGC, x, y, count, chars);
    goto out;
  }

  GetTextBoundingRect(pDrawable, pGC->font, x, y, count, &box);

  REGION_INIT(pGC->pScreen, &reg, &box, 0);
  REGION_INTERSECT(pGC->pScreen, &reg, &reg, pGC->pCompositeClip);

  (*pGC->ops->ImageText8) (pDrawable, pGC, x, y, count, chars);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

out:
  GC_OP_EPILOGUE(pGC);
}

// ImageText16 - changed region is bounding rect around count chars, clipped by
// pCompositeClip

static void vncHooksImageText16(DrawablePtr pDrawable, GCPtr pGC, int x, int y,
                                int count, unsigned short *chars)
{
  BoxRec box;
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, ImageText16);

  if (count == 0) {
    (*pGC->ops->ImageText16) (pDrawable, pGC, x, y, count, chars);
    goto out;
  }

  GetTextBoundingRect(pDrawable, pGC->font, x, y, count, &box);

  REGION_INIT(pGC->pScreen, &reg, &box, 0);
  REGION_INTERSECT(pGC->pScreen, &reg, &reg, pGC->pCompositeClip);

  (*pGC->ops->ImageText16) (pDrawable, pGC, x, y, count, chars);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

out:
  GC_OP_EPILOGUE(pGC);
}

// ImageGlyphBlt - changed region is bounding rect around nglyph chars, clipped
// by pCompositeClip

static void vncHooksImageGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x,
                                  int y, unsigned int nglyph,
                                  CharInfoPtr *ppci, void * pglyphBase)
{
  BoxRec box;
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, ImageGlyphBlt);

  if (nglyph == 0) {
    (*pGC->ops->ImageGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci,pglyphBase);
    goto out;
  }

  GetTextBoundingRect(pDrawable, pGC->font, x, y, nglyph, &box);

  REGION_INIT(pGC->pScreen, &reg, &box, 0);
  REGION_INTERSECT(pGC->pScreen, &reg, &reg, pGC->pCompositeClip);

  (*pGC->ops->ImageGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

out:
  GC_OP_EPILOGUE(pGC);
}

// PolyGlyphBlt - changed region is bounding rect around nglyph chars, clipped
// by pCompositeClip

static void vncHooksPolyGlyphBlt(DrawablePtr pDrawable, GCPtr pGC, int x,
                                 int y, unsigned int nglyph,
                                 CharInfoPtr *ppci, void * pglyphBase)
{
  BoxRec box;
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, PolyGlyphBlt);

  if (nglyph == 0) {
    (*pGC->ops->PolyGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci,pglyphBase);
    goto out;
  }

  GetTextBoundingRect(pDrawable, pGC->font, x, y, nglyph, &box);

  REGION_INIT(pGC->pScreen, &reg, &box, 0);
  REGION_INTERSECT(pGC->pScreen, &reg, &reg, pGC->pCompositeClip);

  (*pGC->ops->PolyGlyphBlt) (pDrawable, pGC, x, y, nglyph, ppci, pglyphBase);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

out:
  GC_OP_EPILOGUE(pGC);
}

// PushPixels - changed region is the given rectangle, clipped by
// pCompositeClip

static void vncHooksPushPixels(GCPtr pGC, PixmapPtr pBitMap,
                               DrawablePtr pDrawable, int w, int h, int x,
                               int y)
{
  BoxRec box;
  RegionRec reg;

  GC_OP_PROLOGUE(pGC, PushPixels);

  box.x1 = x + pDrawable->x;
  box.y1 = y + pDrawable->y;
  box.x2 = box.x1 + w;
  box.y2 = box.y1 + h;

  REGION_INIT(pGC->pScreen, &reg, &box, 0);
  REGION_INTERSECT(pGC->pScreen, &reg, &reg, pGC->pCompositeClip);

  (*pGC->ops->PushPixels) (pGC, pBitMap, pDrawable, w, h, x, y);

  add_changed(pGC->pScreen, &reg);

  REGION_UNINIT(pGC->pScreen, &reg);

  GC_OP_EPILOGUE(pGC);
}
