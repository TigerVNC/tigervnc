/* Copyright 2024 Pierre Ossman for Cendio AB
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

#include "vncDRI3.h"

#include <fb.h>
#include <gcstruct.h>
#include <pixmapstr.h>

static DevPrivateKeyRec vncDRI3DrawScreenPrivateKey;
static DevPrivateKeyRec vncDRI3GCPrivateKey;

typedef struct vncDRI3DrawScreenPrivateRec {
  CloseScreenProcPtr CloseScreen;

  CreateGCProcPtr CreateGC;
  SourceValidateProcPtr SourceValidate;

  CompositeProcPtr Composite;
  GlyphsProcPtr Glyphs;
  CompositeRectsProcPtr CompositeRects;
  TrapezoidsProcPtr Trapezoids;
  TrianglesProcPtr Triangles;
  TriStripProcPtr TriStrip;
  TriFanProcPtr TriFan;
} vncDRI3DrawScreenPrivateRec, *vncDRI3DrawScreenPrivatePtr;

typedef struct vncDRI3GCPrivateRec {
  const GCFuncs *funcs;
  const GCOps *ops;
} vncDRI3GCPrivateRec, *vncDRI3GCPrivatePtr;

#define wrap(priv, real, mem, func) {\
  priv->mem = real->mem; \
  real->mem = func; \
}

#define unwrap(priv, real, mem) {\
  real->mem = priv->mem; \
}

static inline vncDRI3DrawScreenPrivatePtr vncDRI3DrawScreenPrivate(ScreenPtr screen)
{
  return (vncDRI3DrawScreenPrivatePtr)dixLookupPrivate(&(screen)->devPrivates, &vncDRI3DrawScreenPrivateKey);
}

static inline vncDRI3GCPrivatePtr vncDRI3GCPrivate(GCPtr gc)
{
  return (vncDRI3GCPrivatePtr)dixLookupPrivate(&(gc)->devPrivates, &vncDRI3GCPrivateKey);
}

static GCFuncs vncDRI3GCFuncs;
static GCOps vncDRI3GCOps;

/* GC functions */

static void vncDRI3ValidateGC(GCPtr gc, unsigned long changes,
                              DrawablePtr drawable)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  unwrap(gcPriv, gc, funcs);
  if (gcPriv->ops != NULL)
    unwrap(gcPriv, gc, ops);

  (*gc->funcs->ValidateGC)(gc, changes, drawable);

  if (vncDRI3IsHardwareDrawable(drawable)) {
    wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  } else {
    gcPriv->ops = NULL;
  }
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);
}

static void vncDRI3ChangeGC(GCPtr gc, unsigned long mask)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);
  unwrap(gcPriv, gc, funcs);
  if (gcPriv->ops != NULL)
    unwrap(gcPriv, gc, ops);
  (*gc->funcs->ChangeGC)(gc, mask);
  if (gcPriv->ops != NULL)
    wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);
}

static void vncDRI3CopyGC(GCPtr src, unsigned long mask, GCPtr dst)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(dst);
  unwrap(gcPriv, dst, funcs);
  if (gcPriv->ops != NULL)
    unwrap(gcPriv, dst, ops);
  (*dst->funcs->CopyGC)(src, mask, dst);
  if (gcPriv->ops != NULL)
    wrap(gcPriv, dst, ops, &vncDRI3GCOps);
  wrap(gcPriv, dst, funcs, &vncDRI3GCFuncs);
}

static void vncDRI3DestroyGC(GCPtr gc)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);
  unwrap(gcPriv, gc, funcs);
  if (gcPriv->ops != NULL)
    unwrap(gcPriv, gc, ops);
  (*gc->funcs->DestroyGC)(gc);
  if (gcPriv->ops != NULL)
    wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);
}

static void vncDRI3ChangeClip(GCPtr gc, int type, void *value, int nrects)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);
  unwrap(gcPriv, gc, funcs);
  if (gcPriv->ops != NULL)
    unwrap(gcPriv, gc, ops);
  (*gc->funcs->ChangeClip)(gc, type, value, nrects);
  if (gcPriv->ops != NULL)
    wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);
}

static void vncDRI3DestroyClip(GCPtr gc)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);
  unwrap(gcPriv, gc, funcs);
  if (gcPriv->ops != NULL)
    unwrap(gcPriv, gc, ops);
  (*gc->funcs->DestroyClip)(gc);
  if (gcPriv->ops != NULL)
    wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);
}

static void vncDRI3CopyClip(GCPtr dst, GCPtr src)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(dst);
  unwrap(gcPriv, dst, funcs);
  if (gcPriv->ops != NULL)
    unwrap(gcPriv, dst, ops);
  (*dst->funcs->CopyClip)(dst, src);
  if (gcPriv->ops != NULL)
    wrap(gcPriv, dst, ops, &vncDRI3GCOps);
  wrap(gcPriv, dst, funcs, &vncDRI3GCFuncs);
}

/* GC operations */

static void vncDRI3FillSpans(DrawablePtr drawable, GCPtr gc, int nInit,
                             DDXPointPtr pptInit, int *pwidthInit,
                             int fSorted)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->FillSpans)(drawable, gc, nInit, pptInit, pwidthInit, fSorted);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3SetSpans(DrawablePtr drawable, GCPtr gc, char *psrc,
                            DDXPointPtr ppt, int *pwidth, int nspans,
                            int fSorted)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->SetSpans)(drawable, gc, psrc, ppt, pwidth, nspans, fSorted);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3PutImage(DrawablePtr drawable, GCPtr gc, int depth,
                            int x, int y, int w, int h, int leftPad,
                            int format, char *pBits)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->PutImage)(drawable, gc, depth, x, y, w, h, leftPad, format, pBits);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static RegionPtr vncDRI3CopyArea(DrawablePtr src, DrawablePtr dst,
                                 GCPtr gc, int srcx, int srcy,
                                 int w, int h, int dstx, int dsty)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);
  RegionPtr ret;

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(dst);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  ret = (*gc->ops->CopyArea)(src, dst, gc, srcx, srcy, w, h, dstx, dsty);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(dst);

  return ret;
}

static RegionPtr vncDRI3CopyPlane(DrawablePtr src, DrawablePtr dst,
                                  GCPtr gc, int srcx, int srcy,
                                  int w, int h, int dstx, int dsty,
                                  unsigned long plane)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);
  RegionPtr ret;

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(dst);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  ret = (*gc->ops->CopyPlane)(src, dst, gc, srcx, srcy, w, h, dstx, dsty, plane);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(dst);

  return ret;
}

static void vncDRI3PolyPoint(DrawablePtr drawable, GCPtr gc, int mode,
                             int npt, xPoint *pts)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->PolyPoint)(drawable, gc, mode, npt, pts);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3Polylines(DrawablePtr drawable, GCPtr gc, int mode,
                             int npt, DDXPointPtr ppts)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->Polylines)(drawable, gc, mode, npt, ppts);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3PolySegment(DrawablePtr drawable, GCPtr gc, int nseg,
                               xSegment *segs)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->PolySegment)(drawable, gc, nseg, segs);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3PolyRectangle(DrawablePtr drawable, GCPtr gc,
                                 int nrects, xRectangle *rects)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->PolyRectangle)(drawable, gc, nrects, rects);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3PolyArc(DrawablePtr drawable, GCPtr gc, int narcs,
                           xArc *arcs)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->PolyArc)(drawable, gc, narcs, arcs);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3FillPolygon(DrawablePtr drawable, GCPtr gc,
                               int shape, int mode, int count,
                               DDXPointPtr pts)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->FillPolygon)(drawable, gc, shape, mode, count, pts);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3PolyFillRect(DrawablePtr drawable, GCPtr gc,
                                int nrects, xRectangle *rects)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->PolyFillRect)(drawable, gc, nrects, rects);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3PolyFillArc(DrawablePtr drawable, GCPtr gc,
                               int narcs, xArc *arcs)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->PolyFillArc)(drawable, gc, narcs, arcs);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static int vncDRI3PolyText8(DrawablePtr drawable, GCPtr gc,
                            int x, int y, int count, char *chars)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);
  int ret;

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  ret = (*gc->ops->PolyText8)(drawable, gc, x, y, count, chars);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);

  return ret;
}

static int vncDRI3PolyText16(DrawablePtr drawable, GCPtr gc,
                             int x, int y, int count,
                             unsigned short *chars)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);
  int ret;

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  ret = (*gc->ops->PolyText16)(drawable, gc, x, y, count, chars);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);

  return ret;
}

static void vncDRI3ImageText8(DrawablePtr drawable, GCPtr gc,
                              int x, int y, int count, char *chars)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->ImageText8)(drawable, gc, x, y, count, chars);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3ImageText16(DrawablePtr drawable, GCPtr gc,
                               int x, int y, int count,
                               unsigned short *chars)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->ImageText16)(drawable, gc, x, y, count, chars);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3ImageGlyphBlt(DrawablePtr drawable, GCPtr gc, int x,
                                 int y, unsigned int nglyph,
                                 CharInfoPtr *ppci, void * pglyphBase)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->ImageGlyphBlt)(drawable, gc, x, y, nglyph, ppci, pglyphBase);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3PolyGlyphBlt(DrawablePtr drawable, GCPtr gc, int x,
                                int y, unsigned int nglyph,
                                CharInfoPtr *ppci, void * pglyphBase)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->PolyGlyphBlt)(drawable, gc, x, y, nglyph, ppci, pglyphBase);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static void vncDRI3PushPixels(GCPtr gc, PixmapPtr pBitMap,
                              DrawablePtr drawable,
                              int w, int h, int x, int y)
{
  vncDRI3GCPrivatePtr gcPriv = vncDRI3GCPrivate(gc);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(gcPriv, gc, funcs);
  unwrap(gcPriv, gc, ops);
  (*gc->ops->PushPixels)(gc, pBitMap, drawable, w, h, x, y);
  wrap(gcPriv, gc, ops, &vncDRI3GCOps);
  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  vncDRI3SyncDrawableToGPU(drawable);
}

static GCFuncs vncDRI3GCFuncs = {
  .ValidateGC = vncDRI3ValidateGC,
  .ChangeGC = vncDRI3ChangeGC,
  .CopyGC = vncDRI3CopyGC,
  .DestroyGC = vncDRI3DestroyGC,
  .ChangeClip = vncDRI3ChangeClip,
  .DestroyClip = vncDRI3DestroyClip,
  .CopyClip = vncDRI3CopyClip,
};

static GCOps vncDRI3GCOps = {
  .FillSpans = vncDRI3FillSpans,
  .SetSpans = vncDRI3SetSpans,
  .PutImage = vncDRI3PutImage,
  .CopyArea = vncDRI3CopyArea,
  .CopyPlane = vncDRI3CopyPlane,
  .PolyPoint = vncDRI3PolyPoint,
  .Polylines = vncDRI3Polylines,
  .PolySegment = vncDRI3PolySegment,
  .PolyRectangle = vncDRI3PolyRectangle,
  .PolyArc = vncDRI3PolyArc,
  .FillPolygon = vncDRI3FillPolygon,
  .PolyFillRect = vncDRI3PolyFillRect,
  .PolyFillArc = vncDRI3PolyFillArc,
  .PolyText8 = vncDRI3PolyText8,
  .PolyText16 = vncDRI3PolyText16,
  .ImageText8 = vncDRI3ImageText8,
  .ImageText16 = vncDRI3ImageText16,
  .ImageGlyphBlt = vncDRI3ImageGlyphBlt,
  .PolyGlyphBlt = vncDRI3PolyGlyphBlt,
  .PushPixels = vncDRI3PushPixels,
};

static Bool vncDRI3CreateGC(GCPtr gc)
{
  ScreenPtr screen = gc->pScreen;
  vncDRI3DrawScreenPrivatePtr screenPriv = vncDRI3DrawScreenPrivate(screen);
  vncDRI3GCPrivatePtr gcPriv;
  Bool ret;

  unwrap(screenPriv, screen, CreateGC);
  ret = (*screen->CreateGC)(gc);
  wrap(screenPriv, screen, CreateGC, vncDRI3CreateGC);

  gcPriv = vncDRI3GCPrivate(gc);

  wrap(gcPriv, gc, funcs, &vncDRI3GCFuncs);

  return ret;
}

static void vncDRI3SourceValidate(DrawablePtr drawable,
                                  int x, int y,
                                  int width, int height,
                                  unsigned int subWindowMode)
{
  ScreenPtr screen = drawable->pScreen;
  vncDRI3DrawScreenPrivatePtr screenPriv = vncDRI3DrawScreenPrivate(screen);

  /* FIXME: Compute what we need to sync */
  vncDRI3SyncDrawableFromGPU(drawable);

  unwrap(screenPriv, screen, SourceValidate);
  screen->SourceValidate(drawable, x, y, width, height, subWindowMode);
  wrap(screenPriv, screen, SourceValidate, vncDRI3SourceValidate);
}

static void vncDRI3Composite(CARD8 op, PicturePtr pSrc,
                             PicturePtr pMask, PicturePtr pDst,
                             INT16 xSrc, INT16 ySrc, INT16 xMask,
                             INT16 yMask, INT16 xDst, INT16 yDst,
                             CARD16 width, CARD16 height)
{
  ScreenPtr screen = pDst->pDrawable->pScreen;
  PictureScreenPtr ps = GetPictureScreen(screen);
  vncDRI3DrawScreenPrivatePtr screenPriv = vncDRI3DrawScreenPrivate(screen);

  vncDRI3SyncDrawableFromGPU(pDst->pDrawable);

  unwrap(screenPriv, ps, Composite);
  (*ps->Composite)(op, pSrc, pMask, pDst, xSrc, ySrc,
                   xMask, yMask, xDst, yDst, width, height);
  wrap(screenPriv, ps, Composite, vncDRI3Composite);

  vncDRI3SyncDrawableToGPU(pDst->pDrawable);
}

static void vncDRI3Glyphs(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
                          PictFormatPtr maskFormat,
                          INT16 xSrc, INT16 ySrc, int nlists,
                          GlyphListPtr lists, GlyphPtr * glyphs)
{
  ScreenPtr screen = pDst->pDrawable->pScreen;
  PictureScreenPtr ps = GetPictureScreen(screen);
  vncDRI3DrawScreenPrivatePtr screenPriv = vncDRI3DrawScreenPrivate(screen);

  vncDRI3SyncDrawableFromGPU(pDst->pDrawable);

  unwrap(screenPriv, ps, Glyphs);
  (*ps->Glyphs)(op, pSrc, pDst, maskFormat, xSrc, ySrc,
                nlists, lists, glyphs);
  wrap(screenPriv, ps, Glyphs, vncDRI3Glyphs);

  vncDRI3SyncDrawableToGPU(pDst->pDrawable);
}

static void vncDRI3CompositeRects(CARD8 op, PicturePtr pDst,
            xRenderColor * color, int nRect, xRectangle *rects)
{
  ScreenPtr screen = pDst->pDrawable->pScreen;
  PictureScreenPtr ps = GetPictureScreen(screen);
  vncDRI3DrawScreenPrivatePtr screenPriv = vncDRI3DrawScreenPrivate(screen);

  vncDRI3SyncDrawableFromGPU(pDst->pDrawable);

  unwrap(screenPriv, ps, CompositeRects);
  (*ps->CompositeRects)(op, pDst, color, nRect, rects);
  wrap(screenPriv, ps, CompositeRects, vncDRI3CompositeRects);

  vncDRI3SyncDrawableToGPU(pDst->pDrawable);
}

static void vncDRI3Trapezoids(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
            PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
            int ntrap, xTrapezoid * traps)
{
  ScreenPtr screen = pDst->pDrawable->pScreen;
  PictureScreenPtr ps = GetPictureScreen(screen);
  vncDRI3DrawScreenPrivatePtr screenPriv = vncDRI3DrawScreenPrivate(screen);

  /* FIXME: Rarely used, so we just sync everything */
  vncDRI3SyncDrawableFromGPU(pDst->pDrawable);

  unwrap(screenPriv, ps, Trapezoids);
  (*ps->Trapezoids)(op, pSrc, pDst, maskFormat, xSrc, ySrc,
                    ntrap, traps);
  wrap(screenPriv, ps, Trapezoids, vncDRI3Trapezoids);

  vncDRI3SyncDrawableToGPU(pDst->pDrawable);
}

static void vncDRI3Triangles(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
            PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
            int ntri, xTriangle * tris)
{
  ScreenPtr screen = pDst->pDrawable->pScreen;
  PictureScreenPtr ps = GetPictureScreen(screen);
  vncDRI3DrawScreenPrivatePtr screenPriv = vncDRI3DrawScreenPrivate(screen);

  /* FIXME: Rarely used, so we just sync everything */
  vncDRI3SyncDrawableFromGPU(pDst->pDrawable);

  unwrap(screenPriv, ps, Triangles);
  (*ps->Triangles)(op, pSrc, pDst, maskFormat, xSrc, ySrc, ntri, tris);
  wrap(screenPriv, ps, Triangles, vncDRI3Triangles);

  vncDRI3SyncDrawableToGPU(pDst->pDrawable);
}

static void vncDRI3TriStrip(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
            PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
            int npoint, xPointFixed * points)
{
  ScreenPtr screen = pDst->pDrawable->pScreen;
  PictureScreenPtr ps = GetPictureScreen(screen);
  vncDRI3DrawScreenPrivatePtr screenPriv = vncDRI3DrawScreenPrivate(screen);

  /* FIXME: Rarely used, so we just sync everything */
  vncDRI3SyncDrawableFromGPU(pDst->pDrawable);

  unwrap(screenPriv, ps, TriStrip);
  (*ps->TriStrip)(op, pSrc, pDst, maskFormat, xSrc, ySrc,
                  npoint, points);
  wrap(screenPriv, ps, TriStrip, vncDRI3TriStrip)

  vncDRI3SyncDrawableToGPU(pDst->pDrawable);
}

static void vncDRI3TriFan(CARD8 op, PicturePtr pSrc, PicturePtr pDst,
            PictFormatPtr maskFormat, INT16 xSrc, INT16 ySrc,
            int npoint, xPointFixed * points)
{
  ScreenPtr screen = pDst->pDrawable->pScreen;
  PictureScreenPtr ps = GetPictureScreen(screen);
  vncDRI3DrawScreenPrivatePtr screenPriv = vncDRI3DrawScreenPrivate(screen);

  /* FIXME: Rarely used, so we just sync everything */
  vncDRI3SyncDrawableFromGPU(pDst->pDrawable);

  unwrap(screenPriv, ps, TriFan);
  (*ps->TriFan)(op, pSrc, pDst, maskFormat, xSrc, ySrc, npoint, points);
  wrap(screenPriv, ps, TriFan, vncDRI3TriFan);

  vncDRI3SyncDrawableToGPU(pDst->pDrawable);
}

static Bool vncDRI3DrawCloseScreen(ScreenPtr screen)
{
  vncDRI3DrawScreenPrivatePtr screenPriv = vncDRI3DrawScreenPrivate(screen);
  PictureScreenPtr ps;

  unwrap(screenPriv, screen, CloseScreen);

  unwrap(screenPriv, screen, CreateGC);
  unwrap(screenPriv, screen, SourceValidate);

  ps = GetPictureScreenIfSet(screen);
  if (ps) {
    unwrap(screenPriv, ps, Composite);
    unwrap(screenPriv, ps, Glyphs);
    unwrap(screenPriv, ps, CompositeRects);
    unwrap(screenPriv, ps, Trapezoids);
    unwrap(screenPriv, ps, Triangles);
    unwrap(screenPriv, ps, TriStrip);
    unwrap(screenPriv, ps, TriFan);
  }

  return (*screen->CloseScreen)(screen);
}

Bool vncDRI3DrawInit(ScreenPtr screen)
{
  vncDRI3DrawScreenPrivatePtr screenPriv;
  PictureScreenPtr ps;

  if (!dixRegisterPrivateKey(&vncDRI3DrawScreenPrivateKey,
                             PRIVATE_SCREEN,
                             sizeof(vncDRI3DrawScreenPrivateRec)))
    return FALSE;
  if (!dixRegisterPrivateKey(&vncDRI3GCPrivateKey, PRIVATE_GC,
                             sizeof(vncDRI3GCPrivateRec)))
    return FALSE;

  screenPriv = vncDRI3DrawScreenPrivate(screen);

  wrap(screenPriv, screen, CloseScreen, vncDRI3DrawCloseScreen);

  wrap(screenPriv, screen, CreateGC, vncDRI3CreateGC);
  wrap(screenPriv, screen, SourceValidate, vncDRI3SourceValidate);

  ps = GetPictureScreenIfSet(screen);
  if (ps) {
    wrap(screenPriv, ps, Composite, vncDRI3Composite);
    wrap(screenPriv, ps, Glyphs, vncDRI3Glyphs);
    wrap(screenPriv, ps, CompositeRects, vncDRI3CompositeRects);
    wrap(screenPriv, ps, Trapezoids, vncDRI3Trapezoids);
    wrap(screenPriv, ps, Triangles, vncDRI3Triangles);
    wrap(screenPriv, ps, TriStrip, vncDRI3TriStrip);
    wrap(screenPriv, ps, TriFan, vncDRI3TriFan);
  }

  return TRUE;
}