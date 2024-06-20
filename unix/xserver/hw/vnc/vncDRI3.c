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

#include <errno.h>
#include <fcntl.h>
#include <glob.h>
#include <unistd.h>

#ifdef HAVE_GBM
#include <gbm.h>
#endif

#include "vncDRI3.h"
#include <dri3.h>
#include <fb.h>
#include <misyncshm.h>

#ifdef FB_ACCESS_WRAPPER
#error "This code is not compatible with accessors"
#endif

const char *renderNode = "auto";

static DevPrivateKeyRec vncDRI3ScreenPrivateKey;
static DevPrivateKeyRec vncDRI3PixmapPrivateKey;

typedef struct vncDRI3ScreenPrivate {
  CloseScreenProcPtr CloseScreen;

  DestroyPixmapProcPtr DestroyPixmap;

#ifdef HAVE_GBM
  const char *devicePath;
  struct gbm_device *device;
  int fd;
#endif
} vncDRI3ScreenPrivateRec, *vncDRI3ScreenPrivatePtr;

typedef struct vncDRI3PixmapPrivate {
#ifdef HAVE_GBM
  struct gbm_bo *bo;
#endif
} vncDRI3PixmapPrivateRec, *vncDRI3PixmapPrivatePtr;

#define wrap(priv, real, mem, func) {\
  priv->mem = real->mem; \
  real->mem = func; \
}

#define unwrap(priv, real, mem) {\
  real->mem = priv->mem; \
}

static inline vncDRI3ScreenPrivatePtr vncDRI3ScreenPrivate(ScreenPtr screen)
{
  return (vncDRI3ScreenPrivatePtr)dixLookupPrivate(&(screen)->devPrivates, &vncDRI3ScreenPrivateKey);
}

static inline vncDRI3PixmapPrivatePtr vncDRI3PixmapPrivate(PixmapPtr pixmap)
{
  return (vncDRI3PixmapPrivatePtr)dixLookupPrivate(&(pixmap)->devPrivates, &vncDRI3PixmapPrivateKey);
}

static int vncDRI3Open(ScreenPtr screen, RRProviderPtr provider,
                       int *fd)
{
#ifdef HAVE_GBM
  vncDRI3ScreenPrivatePtr screenPriv = vncDRI3ScreenPrivate(screen);

  *fd = open(screenPriv->devicePath, O_RDWR|O_CLOEXEC);
  if (*fd < 0)
    return BadAlloc;

  return Success;
#else
  return BadAlloc;
#endif
}

/* Taken from glamor */
#ifdef HAVE_GBM
static uint32_t gbm_format_for_depth(CARD8 depth)
{
  switch (depth) {
  case 16:
    return GBM_FORMAT_RGB565;
  case 24:
    return GBM_FORMAT_XRGB8888;
  case 30:
    return GBM_FORMAT_ARGB2101010;
  default:
    ErrorF("unexpected depth: %d\n", depth);
  case 32:
    return GBM_FORMAT_ARGB8888;
  }
}
#endif

static PixmapPtr vncPixmapFromFd(ScreenPtr screen, int fd,
                                 CARD16 width, CARD16 height,
                                 CARD16 stride, CARD8 depth,
                                 CARD8 bpp)
{
#ifdef HAVE_GBM
  vncDRI3ScreenPrivatePtr screenPriv = vncDRI3ScreenPrivate(screen);
  vncDRI3PixmapPrivatePtr pixmapPriv;

  struct gbm_import_fd_data data;
  struct gbm_bo *bo;
  PixmapPtr pixmap;

  if (bpp != sizeof(FbBits)*8) {
    ErrorF("Incompatible bits per pixel given for GPU buffer\n");
    return NULL;
  }

  if ((stride % sizeof(FbBits)) != 0) {
    ErrorF("Incompatible stride given for GPU buffer\n");
    return NULL;
  }

  data.fd = fd;
  data.width = width;
  data.height = height;
  data.stride = stride;
  data.format = gbm_format_for_depth(depth);

  bo = gbm_bo_import(screenPriv->device, GBM_BO_IMPORT_FD, &data,
                     GBM_BO_USE_RENDERING | GBM_BO_USE_LINEAR);
  if (bo == NULL)
    return NULL;

  pixmap = screen->CreatePixmap(screen, width, height, depth, 0);

  pixmapPriv = vncDRI3PixmapPrivate(pixmap);
  pixmapPriv->bo = bo;

  vncDRI3SyncPixmapFromGPU(pixmap);

  return pixmap;
#else
  return NULL;
#endif
}

static Bool vncDRI3DestroyPixmap(PixmapPtr pixmap)
{
  ScreenPtr screen = pixmap->drawable.pScreen;
  vncDRI3ScreenPrivatePtr screenPriv = vncDRI3ScreenPrivate(screen);
  Bool ret;

#ifdef HAVE_GBM
  if (pixmap->refcnt == 1) {
    vncDRI3PixmapPrivatePtr pixmapPriv = vncDRI3PixmapPrivate(pixmap);

    if (pixmapPriv->bo != NULL) {
      gbm_bo_destroy(pixmapPriv->bo);
      pixmapPriv->bo = NULL;
    }
  }
#endif

  unwrap(screenPriv, screen, DestroyPixmap);
  ret = screen->DestroyPixmap(pixmap);
  wrap(screenPriv, screen, DestroyPixmap, vncDRI3DestroyPixmap);

  return ret;
}

#ifdef HAVE_GBM
static int vncDRI3FdFromPixmapVisitWindow(WindowPtr window, void *data)
{
  ScreenPtr screen = window->drawable.pScreen;
  PixmapPtr pixmap = data;

  if ((*screen->GetWindowPixmap)(window) == pixmap)
    window->drawable.serialNumber = NEXT_SERIAL_NUMBER;

  return WT_WALKCHILDREN;
}
#endif

static int vncFdFromPixmap(ScreenPtr screen, PixmapPtr pixmap,
                           CARD16 *stride, CARD32 *size)
{
#ifdef HAVE_GBM
  vncDRI3ScreenPrivatePtr screenPriv = vncDRI3ScreenPrivate(screen);
  vncDRI3PixmapPrivatePtr pixmapPriv = vncDRI3PixmapPrivate(pixmap);

  if (pixmap->drawable.bitsPerPixel != sizeof(FbBits)*8) {
    ErrorF("Incompatible bits per pixel given for pixmap\n");
    return -1;
  }

  if (pixmapPriv->bo == NULL) {
    /* GBM_BO_USE_LINEAR ? */
    pixmapPriv->bo = gbm_bo_create(screenPriv->device,
                                   pixmap->drawable.width,
                                   pixmap->drawable.height,
                                   gbm_format_for_depth(pixmap->drawable.depth),
                                   GBM_BO_USE_RENDERING | GBM_BO_USE_LINEAR);
    if (pixmapPriv->bo == NULL) {
      ErrorF("Failed to create GPU buffer: %s\n", strerror(errno));
      return -1;
    }

    if ((gbm_bo_get_stride(pixmapPriv->bo) % sizeof(FbBits)) != 0) {
      ErrorF("Incompatible stride for created GPU buffer\n");
      gbm_bo_destroy(pixmapPriv->bo);
      pixmapPriv->bo = NULL;
      return -1;
    }

    /* Force re-validation of any gc:s */
    pixmap->drawable.serialNumber = NEXT_SERIAL_NUMBER;
    WalkTree(screen, vncDRI3FdFromPixmapVisitWindow, pixmap);
  }

  vncDRI3SyncPixmapToGPU(pixmap);

  *stride = gbm_bo_get_stride(pixmapPriv->bo);
  /* FIXME */
  *size = *stride * gbm_bo_get_height(pixmapPriv->bo);

  return gbm_bo_get_fd(pixmapPriv->bo);
#else
  return -1;
#endif
}

Bool vncDRI3IsHardwarePixmap(PixmapPtr pixmap)
{
#ifdef HAVE_GBM
  vncDRI3PixmapPrivatePtr pixmapPriv = vncDRI3PixmapPrivate(pixmap);
  return pixmapPriv->bo != NULL;
#else
  return FALSE;
#endif
}

Bool vncDRI3IsHardwareDrawable(DrawablePtr drawable)
{
  PixmapPtr pixmap;
  int xoff, yoff;

  fbGetDrawablePixmap(drawable, pixmap, xoff, yoff);
  (void)xoff;
  (void)yoff;

  return vncDRI3IsHardwarePixmap(pixmap);
}

Bool vncDRI3SyncPixmapToGPU(PixmapPtr pixmap)
{
#ifdef HAVE_GBM
  vncDRI3PixmapPrivatePtr pixmapPriv = vncDRI3PixmapPrivate(pixmap);

  int width, height;

  FbBits *bo_data;
  uint32_t bo_stride;
  void *map_data;
  uint32_t bo_bpp;

  FbBits *pixmap_data;
  int pixmap_stride;
  int pixmap_bpp;

  pixman_bool_t ret;

  if (pixmapPriv->bo == NULL)
    return TRUE;

  width = gbm_bo_get_width(pixmapPriv->bo);
  height = gbm_bo_get_height(pixmapPriv->bo);

  map_data = NULL;
  bo_data = gbm_bo_map(pixmapPriv->bo, 0, 0, width, height,
                       GBM_BO_TRANSFER_WRITE, &bo_stride, &map_data);
  if (bo_data == NULL) {
    ErrorF("Could not map GPU buffer: %s\n", strerror(errno));
    return FALSE;
  }

  bo_bpp = gbm_bo_get_bpp(pixmapPriv->bo);
  assert(bo_bpp == sizeof(FbBits)*8);
  assert((bo_stride % (bo_bpp/8)) == 0);

  fbGetPixmapBitsData(pixmap, pixmap_data,
                      pixmap_stride, pixmap_bpp);
  assert(pixmap_data != NULL);
  assert(pixmap_bpp == sizeof(FbBits)*8);

  assert(bo_bpp == pixmap_bpp);

  /* Try accelerated copy first */
  ret = pixman_blt((uint32_t*)pixmap_data, (uint32_t*)bo_data,
                   pixmap_stride, bo_stride / (bo_bpp/8),
                   pixmap_bpp, bo_bpp, 0, 0, 0, 0, width, height);
  if (!ret) {
    /* Fall back to slow pure C version */
    fbBlt(pixmap_data, pixmap_stride, 0,
          bo_data, bo_stride / (bo_bpp/8), 0,
          width * bo_bpp, height,
          GXcopy, FB_ALLONES, bo_bpp, FALSE, FALSE);
  }

  gbm_bo_unmap(pixmapPriv->bo, map_data);
#endif

  return TRUE;
}

Bool vncDRI3SyncPixmapFromGPU(PixmapPtr pixmap)
{
#ifdef HAVE_GBM
  vncDRI3PixmapPrivatePtr pixmapPriv = vncDRI3PixmapPrivate(pixmap);

  int width, height;

  FbBits *bo_data;
  uint32_t bo_stride;
  void *map_data;
  uint32_t bo_bpp;

  FbBits *pixmap_data;
  int pixmap_stride;
  int pixmap_bpp;

  pixman_bool_t ret;

  if (pixmapPriv->bo == NULL)
    return TRUE;

  width = gbm_bo_get_width(pixmapPriv->bo);
  height = gbm_bo_get_height(pixmapPriv->bo);

  map_data = NULL;
  bo_data = gbm_bo_map(pixmapPriv->bo, 0, 0, width, height,
                       GBM_BO_TRANSFER_READ, &bo_stride, &map_data);
  if (bo_data == NULL) {
    ErrorF("Could not map GPU buffer: %s\n", strerror(errno));
    return FALSE;
  }

  bo_bpp = gbm_bo_get_bpp(pixmapPriv->bo);
  assert(bo_bpp == sizeof(FbBits)*8);
  assert((bo_stride % (bo_bpp/8)) == 0);

  fbGetPixmapBitsData(pixmap, pixmap_data,
                      pixmap_stride, pixmap_bpp);
  assert(pixmap_data != NULL);
  assert(pixmap_bpp == sizeof(FbBits)*8);

  assert(bo_bpp == pixmap_bpp);

  /* Try accelerated copy first */
  ret = pixman_blt((uint32_t*)bo_data, (uint32_t*)pixmap_data,
                   bo_stride / (bo_bpp/8), pixmap_stride,
                   bo_bpp, pixmap_bpp, 0, 0, 0, 0, width, height);
  if (!ret) {
    /* Fall back to slow pure C version */
    fbBlt(bo_data, bo_stride / (bo_bpp/8), 0,
          pixmap_data, pixmap_stride, 0,
          width * bo_bpp, height,
          GXcopy, FB_ALLONES, bo_bpp, FALSE, FALSE);
  }

  gbm_bo_unmap(pixmapPriv->bo, map_data);
#endif

  return TRUE;
}

Bool vncDRI3SyncDrawableToGPU(DrawablePtr drawable)
{
  PixmapPtr pixmap;
  int xoff, yoff;

  fbGetDrawablePixmap(drawable, pixmap, xoff, yoff);
  (void)xoff;
  (void)yoff;

  return vncDRI3SyncPixmapToGPU(pixmap);
}

Bool vncDRI3SyncDrawableFromGPU(DrawablePtr drawable)
{
  PixmapPtr pixmap;
  int xoff, yoff;

  fbGetDrawablePixmap(drawable, pixmap, xoff, yoff);
  (void)xoff;
  (void)yoff;

  return vncDRI3SyncPixmapFromGPU(pixmap);
}

static const dri3_screen_info_rec vncDRI3ScreenInfo = {
  .version = 1,

  .open = vncDRI3Open,
  .pixmap_from_fd = vncPixmapFromFd,
  .fd_from_pixmap = vncFdFromPixmap,
};

static Bool vncDRI3CloseScreen(ScreenPtr screen)
{
  vncDRI3ScreenPrivatePtr screenPriv = vncDRI3ScreenPrivate(screen);

  unwrap(screenPriv, screen, CloseScreen);

  unwrap(screenPriv, screen, DestroyPixmap);

#ifdef HAVE_GBM
  gbm_device_destroy(screenPriv->device);
  screenPriv->device = NULL;

  close(screenPriv->fd);
  screenPriv->fd = -1;
#endif

  return (*screen->CloseScreen)(screen);
}

Bool vncDRI3Init(ScreenPtr screen)
{
  vncDRI3ScreenPrivatePtr screenPriv;

  /*
   * We don't queue any gbm operations, so we don't have to do anything
   * more than simply activate this extension.
   */
#ifdef HAVE_XSHMFENCE
  if (!miSyncShmScreenInit(screen))
    return FALSE;
#endif

  /* Empty render node is interpreted as disabling DRI3 */
  if (renderNode[0] == '\0')
    return TRUE;

  if ((renderNode[0] != '/') &&
      (strcasecmp(renderNode, "auto") != 0)) {
    ErrorF("Invalid render node path \"%s\"\n", renderNode);
    return FALSE;
  }

  if (!dixRegisterPrivateKey(&vncDRI3ScreenPrivateKey, PRIVATE_SCREEN,
                             sizeof(vncDRI3ScreenPrivateRec)))
    return FALSE;
  if (!dixRegisterPrivateKey(&vncDRI3PixmapPrivateKey, PRIVATE_PIXMAP,
                             sizeof(vncDRI3PixmapPrivateRec)))
    return FALSE;

  if (!vncDRI3DrawInit(screen))
    return FALSE;

#ifdef HAVE_GBM
  screenPriv = vncDRI3ScreenPrivate(screen);

  if (strcasecmp(renderNode, "auto") == 0) {
    glob_t globbuf;
    int ret;

    ret = glob("/dev/dri/renderD*", 0, NULL, &globbuf);
    if (ret == GLOB_NOMATCH) {
      ErrorF("Could not find any render nodes\n");
      return FALSE;
    }
    if (ret != 0) {
      ErrorF("Failure enumerating render nodes\n");
      return FALSE;
    }

    screenPriv->devicePath = NULL;
    for (size_t i = 0;i < globbuf.gl_pathc;i++) {
      if (access(globbuf.gl_pathv[i], R_OK|W_OK) == 0) {
        screenPriv->devicePath = strdup(globbuf.gl_pathv[i]);
        break;
      }
    }

    globfree(&globbuf);

    if (screenPriv->devicePath == NULL) {
      ErrorF("Could not find any available render nodes\n");
      return FALSE;
    }
  } else {
    screenPriv->devicePath = renderNode;
  }

  screenPriv->fd = open(screenPriv->devicePath, O_RDWR|O_CLOEXEC);
  if (screenPriv->fd < 0) {
    ErrorF("Failed to open \"%s\": %s\n",
           screenPriv->devicePath, strerror(errno));
    return FALSE;
  }

  screenPriv->device = gbm_create_device(screenPriv->fd);
  if (screenPriv->device == NULL) {
    close(screenPriv->fd);
    screenPriv->fd = -1;
    ErrorF("Could create GPU render device\n");
    return FALSE;
  }
#else
  ErrorF("Built without GBM support\n");
  return FALSE;
#endif

  wrap(screenPriv, screen, CloseScreen, vncDRI3CloseScreen);

  wrap(screenPriv, screen, DestroyPixmap, vncDRI3DestroyPixmap);

  return dri3_screen_init(screen, &vncDRI3ScreenInfo);
}
