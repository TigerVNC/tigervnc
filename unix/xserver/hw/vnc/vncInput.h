/* Copyright (C) 2009 TightVNC Team
 * Copyright (C) 2009, 2010 Red Hat, Inc.
 * Copyright (C) 2009, 2010 TigerVNC Team
 * Copyright 2013-2015 Pierre Ossman for Cendio AB
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

#ifndef __VNCINPUT_H__
#define __VNCINPUT_H__

#include <stdlib.h>
#include <X11/X.h>

#ifdef __cplusplus
extern "C" {
#endif

void vncInitInputDevice(void);

void vncPointerButtonAction(int buttonMask);
void vncPointerMove(int x, int y);
void vncGetPointerPos(int *x, int *y);

void vncKeyboardEvent(KeySym keysym, unsigned xtcode, int down);

/* Backend dependent functions below here */

void vncPrepareInputDevices(void);

unsigned vncGetKeyboardState(void);
unsigned vncGetLevelThreeMask(void);

KeyCode vncPressShift(void);
size_t vncReleaseShift(KeyCode *keys, size_t maxKeys);

KeyCode vncPressLevelThree(void);
size_t vncReleaseLevelThree(KeyCode *keys, size_t maxKeys);

KeyCode vncKeysymToKeycode(KeySym keysym, unsigned state, unsigned *new_state);

int vncIsAffectedByNumLock(KeyCode keycode);

KeyCode vncAddKeysym(KeySym keysym, unsigned state);

#ifdef __cplusplus
}
#endif

#endif
