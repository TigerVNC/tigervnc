/* Copyright (C) 2009 TightVNC Team
 * Copyright (C) 2009, 2010 Red Hat, Inc.
 * Copyright (C) 2009, 2010 TigerVNC Team
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

#include <rfb/VNCServerST.h>

extern "C" {
#include "input.h"
};

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
private:
	void keyEvent(rdr::U32 keysym, bool down);

	rfb::VNCServerST *server;
	DeviceIntPtr keyboardDev;
	DeviceIntPtr pointerDev;
	int oldButtonMask;
	rfb::Point cursorPos, oldCursorPos;
};

#endif
