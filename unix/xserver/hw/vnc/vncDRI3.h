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

#ifndef __VNCDRI3_H__
#define __VNCDRI3_H__

#ifdef DRI3

#include <dix.h>

extern const char *renderNode;

Bool vncDRI3Init(ScreenPtr screen);

Bool vncDRI3IsHardwarePixmap(PixmapPtr pixmap);
Bool vncDRI3IsHardwareDrawable(DrawablePtr drawable);

Bool vncDRI3SyncPixmapToGPU(PixmapPtr pixmap);
Bool vncDRI3SyncPixmapFromGPU(PixmapPtr pixmap);

Bool vncDRI3SyncDrawableToGPU(DrawablePtr drawable);
Bool vncDRI3SyncDrawableFromGPU(DrawablePtr drawable);

Bool vncDRI3DrawInit(ScreenPtr screen);

#endif

#endif
