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

/*
  This header defines an interface for X RandR operations. It is
  implemented by a corresponding RandrGlue.c, either with internal
  calls (for Xvnc/vncmodule.so) or Xlib calls (x0vncserver).
 */

#ifndef RANDR_GLUE_H
#define RANDR_GLUE_H

#ifdef __cplusplus
extern "C" {
#endif

int vncGetScreenWidth(void);
int vncGetScreenHeight(void);

int vncRandRIsValidScreenSize(int width, int height);
int vncRandRResizeScreen(int width, int height);
void vncRandRUpdateSetTime(void);

int vncRandRHasOutputClones(void);

int vncRandRGetOutputCount(void);
int vncRandRGetAvailableOutputs(void);

char *vncRandRGetOutputName(int outputIdx);

int vncRandRIsOutputEnabled(int outputIdx);
int vncRandRIsOutputUsable(int outputIdx);
int vncRandRIsOutputConnected(int outputIdx);

int vncRandRCheckOutputMode(int outputIdx, int width, int height);

int vncRandRDisableOutput(int outputIdx);
int vncRandRReconfigureOutput(int outputIdx, int x, int y,
                              int width, int height);

unsigned int vncRandRGetOutputId(int outputIdx);
int vncRandRGetOutputDimensions(int outputIdx,
                                 int *x, int *y, int *width, int *height);

int vncRandRCanCreateOutputs(int extraOutputs);
int vncRandRCreateOutputs(int extraOutputs);

#ifdef __cplusplus
}
#endif

#endif
