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

#include <rdr/types.h>
#include <rfb/Rect.h>

extern "C" {
#include "input.h"
/* The Xorg headers define macros that wreak havoc with STL */
#undef max
};

#include "xorg-version.h"

/*
 * Represents input device (keyboard + pointer)
 *
 * Is a singleton as input devices are global in the X server so
 * we do not have one per desktop (i.e. per screen).
 */
extern class InputDevice *vncInputDevice;

class InputDevice {
public:
	/*
	 * Press or release buttons. Relationship between buttonMask and
	 * buttons is specified in RFB protocol.
	 */
	void PointerButtonAction(int buttonMask);

	/* Move pointer to target location (point coords are absolute). */
	void PointerMove(const rfb::Point &point);

	/* Get current known location of the pointer */
	const rfb::Point &getPointerPos(void);

	/* Press or release one or more keys to get the given symbol */
	void KeyboardPress(rdr::U32 keysym) { keyEvent(keysym, true); }
	void KeyboardRelease(rdr::U32 keysym) { keyEvent(keysym, false); }

	/*
	 * Init input device.
	 * This has to be called after core pointer/keyboard
	 * initialization which unfortunately is after extesions
	 * initialization (which means we cannot call it in
	 * vncExtensionInit(). Check InitExtensions(),
	 * InitCoreDevices() and InitInput() calls in dix/main.c.
	 * Instead we call it from XserverDesktop at an appropriate
	 * time.
	 */
	void InitInputDevice(void);

private:
	InputDevice();

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

	bool isAffectedByNumLock(KeyCode keycode);

	KeyCode addKeysym(KeySym keysym, unsigned state);

private:
	static int pointerProc(DeviceIntPtr pDevice, int onoff);
	static int keyboardProc(DeviceIntPtr pDevice, int onoff);

#if XORG >= 17
	static void vncXkbProcessDeviceEvent(int screenNum,
	                                     InternalEvent *event,
	                                     DeviceIntPtr dev);
#else
	static void GetInitKeyboardMap(KeySymsPtr keysyms, CARD8 *modmap);
#endif

private:
	DeviceIntPtr keyboardDev;
	DeviceIntPtr pointerDev;

	int oldButtonMask;
	rfb::Point cursorPos;

	KeySym pressedKeys[256];

private:
	static InputDevice singleton;
};

#endif
