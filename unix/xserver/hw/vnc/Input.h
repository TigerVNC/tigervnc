/* Copyright (C) 2009 TightVNC Team
 * Copyright (C) 2009, 2010 Red Hat, Inc.
 * Copyright (C) 2009, 2010 TigerVNC Team
 * Copyright 2013 Pierre Ossman for Cendio AB
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

/* Make sure macro doesn't conflict with macro in include/input.h. */
#ifndef INPUT_H_
#define INPUT_H_

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include <list>

#include <rfb/VNCServerST.h>

extern "C" {
#include "input.h"
};

#include "xorg-version.h"

/* Represents input device (keyboard + pointer) */
class InputDevice {
public:
	/* Create new InputDevice instance */
	InputDevice(rfb::VNCServerST *_server);

	/*
	 * Press or release buttons. Relationship between buttonMask and
	 * buttons is specified in RFB protocol.
	 */
	void PointerButtonAction(int buttonMask);

	/* Move pointer to target location (point coords are absolute). */
	void PointerMove(const rfb::Point &point);

	/*
	 * Send pointer position to clients. If not called then Move() calls
	 * won't be visible to VNC clients.
	 */
	void PointerSync(void);

	void KeyboardPress(rdr::U32 keysym) { keyEvent(keysym, true); }
	void KeyboardRelease(rdr::U32 keysym) { keyEvent(keysym, false); }

	/*
	 * Init input device. This cannot be done in the constructor
	 * because constructor is called during X server extensions
	 * initialization. Devices must be initialized after core
	 * pointer/keyboard initialization which is actually after extesions
	 * initialization. Check InitExtensions(), InitCoreDevices() and
	 * InitInput() calls in dix/main.c. Instead it is called from
	 * XserverDesktop at an appropriate time.
	 */
	void InitInputDevice(void);

private:
	void keyEvent(rdr::U32 keysym, bool down);

	/* Backend dependent functions below here */
	void PrepareInputDevices(void);

	unsigned getKeyboardState(void);
	unsigned getLevelThreeMask(void);

	KeyCode pressShift(void);
	std::list<KeyCode> releaseShift(void);

	KeyCode pressLevelThree(void);
	std::list<KeyCode> releaseLevelThree(void);

	KeyCode keysymToKeycode(KeySym keysym, unsigned state, unsigned *new_state);

	bool isLockModifier(KeyCode keycode, unsigned state);

	KeyCode addKeysym(KeySym keysym, unsigned state);

private:
#if XORG >= 17
	static void vncXkbProcessDeviceEvent(int screenNum,
	                                     InternalEvent *event,
	                                     DeviceIntPtr dev);
#endif

private:
	rfb::VNCServerST *server;
	bool initialized;
	DeviceIntPtr keyboardDev;
	DeviceIntPtr pointerDev;

	int oldButtonMask;
	rfb::Point cursorPos, oldCursorPos;

	KeySym pressedKeys[256];
};

#endif
