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

#ifndef XORG_GLUE_H
#define XORG_GLUE_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
#  define __printf_attr(a, b) __attribute__((__format__ (__printf__, a, b)))
#  define __noreturn_attr __attribute__((noreturn))
#else
#  define __printf_attr(a, b)
#  define __noreturn_attr
#endif // __GNUC__

const char *vncGetDisplay(void);
unsigned long vncGetServerGeneration(void);

void vncFatalError(const char *format, ...) __printf_attr(1, 2) __noreturn_attr;

int vncGetScreenCount(void);

void vncGetScreenFormat(int scrIdx, int *depth, int *bpp,
                        int *trueColour, int *bigEndian,
                        int *redMask, int *greenMask, int *blueMask);

int vncGetScreenX(int scrIdx);
int vncGetScreenY(int scrIdx);

// These hide in xvnc.c or vncModule.c
void vncClientGone(int fd);
int vncRandRCanCreateScreenOutputs(int scrIdx, int extraOutputs);
int vncRandRCreateScreenOutputs(int scrIdx, int extraOutputs);
int vncRandRCanCreateModes(void);
void* vncRandRCreateMode(void* output, int width, int height);
void* vncRandRSetPreferredMode(void* output, void* mode);

#ifdef __cplusplus
}
#endif

#endif
