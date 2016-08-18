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

const char *vncGetDisplay(void);
unsigned long vncGetServerGeneration(void);

int vncGetScreenCount(void);

void vncGetScreenFormat(int scrIdx, int *depth, int *bpp,
                        int *trueColour, int *bigEndian,
                        int *redMask, int *greenMask, int *blueMask);

int vncGetScreenX(int scrIdx);
int vncGetScreenY(int scrIdx);
int vncGetScreenWidth(int scrIdx);
int vncGetScreenHeight(int scrIdx);

int vncRandRResizeScreen(int scrIdx, int width, int height);
void vncRandRUpdateSetTime(int scrIdx);

int vncRandRHasOutputClones(int scrIdx);

int vncRandRGetOutputCount(int scrIdx);
int vncRandRGetAvailableOutputs(int scrIdx);

const char *vncRandRGetOutputName(int scrIdx, int outputIdx);

int vncRandRIsOutputEnabled(int scrIdx, int outputIdx);
int vncRandRIsOutputUsable(int scrIdx, int outputIdx);

int vncRandRDisableOutput(int scrIdx, int outputIdx);
int vncRandRReconfigureOutput(int scrIdx, int outputIdx, int x, int y,
                              int width, int height);

intptr_t vncRandRGetOutputId(int scrIdx, int outputIdx);
void vncRandRGetOutputDimensions(int scrIdx, int outputIdx,
                                 int *x, int *y, int *width, int *height);

// These hide in xvnc.c or vncModule.c
void vncClientGone(int fd);
int vncRandRCreateOutputs(int scrIdx, int extraOutputs);
void *vncRandRCreatePreferredMode(void *output, int width, int height);

#ifdef __cplusplus
}
#endif

#endif
