/* Copyright (C) 2009 TightVNC Team
 * Copyright (C) 2009 Red Hat, Inc.
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "Input.h"
#include "xorg-version.h"

extern "C" {
#define public c_public
#define class c_class
#include "inputstr.h"
#ifndef XKB_IN_SERVER
#define XKB_IN_SERVER
#endif
#ifdef XKB
/*
 * This include is needed to use XkbConvertCase instead of XConvertCase even if
 * we don't use XKB extension.
 */
#include <xkbsrv.h>
#endif
/* These defines give us access to all keysyms we need */
#define XK_PUBLISHING
#define XK_TECHNICAL
#include <X11/keysym.h>
#include <X11/XF86keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef public
#undef class
}

#if XORG < 17

#define IS_PRESSED(dev, keycode) \
	((dev)->key->down[(keycode) >> 3] & (1 << ((keycode) & 7)))

/* Fairly standard US PC Keyboard */

#define MIN_KEY 8
#define MAX_KEY 255
#define MAP_LEN (MAX_KEY - MIN_KEY + 1)
#define KEYSYMS_PER_KEY 2
KeySym keyboardMap[MAP_LEN * KEYSYMS_PER_KEY] = {
	NoSymbol, NoSymbol,
	XK_Escape, NoSymbol,
	XK_1, XK_exclam,
	XK_2, XK_at,
	XK_3, XK_numbersign,
	XK_4, XK_dollar,
	XK_5, XK_percent,
	XK_6, XK_asciicircum,
	XK_7, XK_ampersand,
	XK_8, XK_asterisk,
	XK_9, XK_parenleft,
	XK_0, XK_parenright,
	XK_minus, XK_underscore,
	XK_equal, XK_plus,
	XK_BackSpace, NoSymbol,
	XK_Tab, NoSymbol,
	XK_q, XK_Q,
	XK_w, XK_W,
	XK_e, XK_E,
	XK_r, XK_R,
	XK_t, XK_T,
	XK_y, XK_Y,
	XK_u, XK_U,
	XK_i, XK_I,
	XK_o, XK_O,
	XK_p, XK_P,
	XK_bracketleft, XK_braceleft,
	XK_bracketright, XK_braceright,
	XK_Return, NoSymbol,
	XK_Control_L, NoSymbol,
	XK_a, XK_A,
	XK_s, XK_S,
	XK_d, XK_D,
	XK_f, XK_F,
	XK_g, XK_G,
	XK_h, XK_H,
	XK_j, XK_J,
	XK_k, XK_K,
	XK_l, XK_L,
	XK_semicolon, XK_colon,
	XK_apostrophe, XK_quotedbl,
	XK_grave, XK_asciitilde,
	XK_Shift_L, NoSymbol,
	XK_backslash, XK_bar,
	XK_z, XK_Z,
	XK_x, XK_X,
	XK_c, XK_C,
	XK_v, XK_V,
	XK_b, XK_B,
	XK_n, XK_N,
	XK_m, XK_M,
	XK_comma, XK_less,
	XK_period, XK_greater,
	XK_slash, XK_question,
	XK_Shift_R, NoSymbol,
	XK_KP_Multiply, NoSymbol,
	XK_Alt_L, XK_Meta_L,
	XK_space, NoSymbol,
	XK_Caps_Lock, NoSymbol,
	XK_F1, NoSymbol,
	XK_F2, NoSymbol,
	XK_F3, NoSymbol,
	XK_F4, NoSymbol,
	XK_F5, NoSymbol,
	XK_F6, NoSymbol,
	XK_F7, NoSymbol,
	XK_F8, NoSymbol,
	XK_F9, NoSymbol,
	XK_F10, NoSymbol,
	XK_Num_Lock, XK_Pointer_EnableKeys,
	XK_Scroll_Lock, NoSymbol,
	XK_KP_Home, XK_KP_7,
	XK_KP_Up, XK_KP_8,
	XK_KP_Prior, XK_KP_9,
	XK_KP_Subtract, NoSymbol,
	XK_KP_Left, XK_KP_4,
	XK_KP_Begin, XK_KP_5,
	XK_KP_Right, XK_KP_6,
	XK_KP_Add, NoSymbol,
	XK_KP_End, XK_KP_1,
	XK_KP_Down, XK_KP_2,
	XK_KP_Next, XK_KP_3,
	XK_KP_Insert, XK_KP_0,
	XK_KP_Delete, XK_KP_Decimal,
	NoSymbol, NoSymbol,
	NoSymbol, NoSymbol,
	NoSymbol, NoSymbol,
	XK_F11, NoSymbol,
	XK_F12, NoSymbol,
	XK_Home, NoSymbol,
	XK_Up, NoSymbol,
	XK_Prior, NoSymbol,
	XK_Left, NoSymbol,
	NoSymbol, NoSymbol,
	XK_Right, NoSymbol,
	XK_End, NoSymbol,
	XK_Down, NoSymbol,
	XK_Next, NoSymbol,
	XK_Insert, NoSymbol,
	XK_Delete, NoSymbol,
	XK_KP_Enter, NoSymbol,
	XK_Control_R, NoSymbol,
	XK_Pause, XK_Break,
	XK_Print, XK_Execute,
	XK_KP_Divide, NoSymbol,
	XK_Alt_R, XK_Meta_R,
	NoSymbol, NoSymbol,
	XK_Super_L, NoSymbol,
	XK_Super_R, NoSymbol,
	XK_Menu, NoSymbol,
};

void GetInitKeyboardMap(KeySymsPtr keysyms, CARD8 *modmap)
{
	int i;

	for (i = 0; i < MAP_LENGTH; i++)
		modmap[i] = NoSymbol;

	for (i = 0; i < MAP_LEN; i++) {
		switch (keyboardMap[i * KEYSYMS_PER_KEY]) {
		case XK_Shift_L:
		case XK_Shift_R:
			modmap[i + MIN_KEY] = ShiftMask;
			break;
		case XK_Caps_Lock:
			modmap[i + MIN_KEY] = LockMask;
			break;
		case XK_Control_L:
		case XK_Control_R:
			modmap[i + MIN_KEY] = ControlMask;
			break;
		case XK_Alt_L:
		case XK_Alt_R:
			modmap[i + MIN_KEY] = Mod1Mask;
			break;
		case XK_Num_Lock:
			modmap[i + MIN_KEY] = Mod2Mask;
			break;
			/* No defaults for Mod3Mask yet */
		case XK_Super_L:
		case XK_Super_R:
		case XK_Hyper_L:
		case XK_Hyper_R:
			modmap[i + MIN_KEY] = Mod4Mask;
			break;
		case XK_ISO_Level3_Shift:
		case XK_Mode_switch:
			modmap[i + MIN_KEY] = Mod5Mask;
			break;
		}
	}

	keysyms->minKeyCode = MIN_KEY;
	keysyms->maxKeyCode = MAX_KEY;
	keysyms->mapWidth = KEYSYMS_PER_KEY;
	keysyms->map = keyboardMap;
}

void InputDevice::PrepareInputDevices(void)
{
	/* Don't need to do anything here */
}

unsigned InputDevice::getKeyboardState(void)
{
	return keyboardDev->key->state;
}

unsigned InputDevice::getLevelThreeMask(void)
{
	int i, j, k;

	int minKeyCode, mapWidth;
	KeySym *map;

	int maxKeysPerMod;
	CARD8 *modmap;

	minKeyCode = keyboardDev->key->curKeySyms.minKeyCode;
	mapWidth = keyboardDev->key->curKeySyms.mapWidth;
	map = keyboardDev->key->curKeySyms.map;

	maxKeysPerMod = keyboardDev->key->maxKeysPerModifier;
	modmap = keyboardDev->key->modifierKeyMap;

	for (i = 3; i < 8; i++) {
		for (k = 0; k < maxKeysPerMod; k++) {
			int index = i * maxKeysPerMod + k;
			int keycode = modmap[index];

			if (keycode == 0)
				continue;

			for (j = 0; j < mapWidth; j++) {
				KeySym keysym;
				keysym = map[(keycode - minKeyCode) * mapWidth + j];
				if (keysym == XK_Mode_switch)
					return 1 << i;
			}
		}
	}

	return 0;
}

KeyCode InputDevice::pressShift(void)
{
	int maxKeysPerMod;

	maxKeysPerMod = keyboardDev->key->maxKeysPerModifier;
	return keyboardDev->key->modifierKeyMap[ShiftMapIndex * maxKeysPerMod];
}

std::list<KeyCode> InputDevice::releaseShift(void)
{
	std::list<KeyCode> keys;

	int maxKeysPerMod;

	maxKeysPerMod = keyboardDev->key->maxKeysPerModifier;
	for (int k = 0; k < maxKeysPerMod; k++) {
		int keycode;
		int index;

		index = ShiftMapIndex * maxKeysPerMod + k;

		keycode = keyboardDev->key->modifierKeyMap[index];
		if (keycode == 0)
			continue;

		if (!IS_PRESSED(keyboardDev, keycode))
			continue;

		keys.push_back(keycode);
	}

	return keys;
}

KeyCode InputDevice::pressLevelThree(void)
{
	unsigned mask, index;
	int maxKeysPerMod;

	mask = getLevelThreeMask();
	if (mask == 0)
		return 0;

	index = ffs(mask) - 1;
	maxKeysPerMod = keyboardDev->key->maxKeysPerModifier;
	return keyboardDev->key->modifierKeyMap[index * maxKeysPerMod];
}

std::list<KeyCode> InputDevice::releaseLevelThree(void)
{
	std::list<KeyCode> keys;

	unsigned mask, msindex;
	int maxKeysPerMod;

	mask = getLevelThreeMask();
	if (mask == 0)
		return keys;

	msindex = ffs(mask) - 1;

	maxKeysPerMod = keyboardDev->key->maxKeysPerModifier;
	for (int k = 0; k < maxKeysPerMod; k++) {
		int keycode;
		int index;

		index = msindex * maxKeysPerMod + k;

		keycode = keyboardDev->key->modifierKeyMap[index];
		if (keycode == 0)
			continue;

		if (!IS_PRESSED(keyboardDev, keycode))
			continue;

		keys.push_back(keycode);
	}

	return keys;
}

static KeySym KeyCodetoKeySym(KeySymsPtr keymap, int keycode, int col)
{
	int per = keymap->mapWidth;
	KeySym *syms;
	KeySym lsym, usym;

	if ((col < 0) || ((col >= per) && (col > 3)) ||
	    (keycode < keymap->minKeyCode) || (keycode > keymap->maxKeyCode))
		return NoSymbol;

	syms = &keymap->map[(keycode - keymap->minKeyCode) * per];
	if (col >= 4)
		return syms[col];

	if (col > 1) {
		while ((per > 2) && (syms[per - 1] == NoSymbol))
			per--;
		if (per < 3)
			col -= 2;
	}

	if ((per <= (col|1)) || (syms[col|1] == NoSymbol)) {
		XkbConvertCase(syms[col&~1], &lsym, &usym);
		if (!(col & 1))
			return lsym;
		/*
		 * I'm commenting out this logic because it's incorrect even
		 * though it was copied from the Xlib sources.  The X protocol
		 * book quite clearly states that where a group consists of
		 * element 1 being a non-alphabetic keysym and element 2 being
		 * NoSymbol that you treat the second element as being the
		 * same as the first.  This also tallies with the behaviour
		 * produced by the installed Xlib on my linux box (I believe
		 * this is because it uses some XKB code rather than the
		 * original Xlib code - compare XKBBind.c with KeyBind.c in
		 * lib/X11).
		 */
#if 0
		else if (usym == lsym)
			return NoSymbol;
#endif
		else
			return usym;
	}

	return syms[col];
}

KeyCode InputDevice::keysymToKeycode(KeySym keysym, unsigned state, unsigned *new_state)
{
	int i, j;
	unsigned mask;

	KeySymsPtr keymap;
	int mapWidth;

	mask = getLevelThreeMask();

	keymap = &keyboardDev->key->curKeySyms;

	/*
	 * Column 0 means both shift and "mode_switch" (AltGr) must be released,
	 * column 1 means shift must be pressed and mode_switch released,
	 * column 2 means shift must be released and mode_switch pressed, and
	 * column 3 means both shift and mode_switch must be pressed.
	 */
	j = 0;
	if (state & ShiftMask)
		j |= 0x1;
	if (state & mask)
		j |= 0x2;

	*new_state = state;
	for (i = keymap->minKeyCode; i <= keymap->maxKeyCode; i++) {
		if (KeyCodetoKeySym(keymap, i, j) == keysym)
			return i;
	}

	/* Only the first four columns have well-defined meaning */
	mapWidth = keymap->mapWidth;
	if (mapWidth > 4)
		mapWidth = 4;

	for (j = 0; j < mapWidth; j++) {
		for (i = keymap->minKeyCode; i <= keymap->maxKeyCode; i++) {
			if (KeyCodetoKeySym(keymap, i, j) == keysym) {
				*new_state = state & ~(ShiftMask|mask);
				if (j & 0x1)
					*new_state |= ShiftMask;
				if (j & 0x2)
					*new_state |= mask;

				return i;
			}
		}
	}

	return 0;
}

bool InputDevice::isLockModifier(KeyCode keycode, unsigned state)
{
	int i, j, k;

	int minKeyCode, mapWidth;
	KeySym *map;

	int maxKeysPerMod;
	CARD8 *modmap;

	int num_lock_index;

	minKeyCode = keyboardDev->key->curKeySyms.minKeyCode;
	mapWidth = keyboardDev->key->curKeySyms.mapWidth;
	map = keyboardDev->key->curKeySyms.map;

	maxKeysPerMod = keyboardDev->key->maxKeysPerModifier;
	modmap = keyboardDev->key->modifierKeyMap;

	/* Caps Lock is fairly easy as it has a dedicated modmap entry */
	for (k = 0; k < maxKeysPerMod; k++) {
		int index;

		index = LockMapIndex * maxKeysPerMod + k;
		if (keycode == modmap[index])
			return true;
	}

	/* For Num Lock we need to find the correct modmap entry */
	num_lock_index = i;
	for (i = 3; i < 8; i++) {
		for (k = 0; k < maxKeysPerMod; k++) {
			int index = i * maxKeysPerMod + k;
			int keycode = modmap[index];

			if (keycode == 0)
				continue;

			for (j = 0; j < mapWidth; j++) {
				KeySym keysym;
				keysym = map[(keycode - minKeyCode) * mapWidth + j];
				if (keysym == XK_Num_Lock) {
					num_lock_index = i;
					goto done;
				}
			}
		}
	}
done:

	if (num_lock_index == 0)
		return false;

	/* Now we can look in the modmap */ 
	for (k = 0; k < maxKeysPerMod; k++) {
		int index;

		index = num_lock_index * maxKeysPerMod + k;
		if (keycode == modmap[index])
			return true;
	}

	return false;
}

KeyCode InputDevice::addKeysym(KeySym keysym, unsigned state)
{
	KeyCode kc;

	int minKeyCode, maxKeyCode, mapWidth;
	KeySym *map;

	minKeyCode = keyboardDev->key->curKeySyms.minKeyCode;
	maxKeyCode = keyboardDev->key->curKeySyms.maxKeyCode;
	mapWidth = keyboardDev->key->curKeySyms.mapWidth;
	map = keyboardDev->key->curKeySyms.map;

	/*
	 * Magic, which dynamically adds keysym<->keycode mapping
	 * depends on X.Org version. Quick explanation of that "magic":
	 * 
	 * 1.5
	 * - has only one core keyboard so we have to keep core
	 *   keyboard mapping synchronized with vncKeyboardDevice. Do
	 *   it via SwitchCoreKeyboard()
	 *
	 * 1.6 (aka MPX - Multi pointer X)
	 * - multiple master devices (= core devices) exists, keep
	 *   vncKeyboardDevice synchronized with proper master device
	 */
	for (kc = maxKeyCode; kc >= minKeyCode; kc--) {
		DeviceIntPtr master;

		if (map[(kc - minKeyCode) * mapWidth] != 0)
			continue;

		map[(kc - minKeyCode) * mapWidth] = keysym;

#if XORG == 15
		master = inputInfo.keyboard;
#else
		master = keyboardDev->u.master;
#endif
		void *slave = dixLookupPrivate(&master->devPrivates,
					       CoreDevicePrivateKey);
		if (keyboardDev == slave) {
			dixSetPrivate(&master->devPrivates,
				      CoreDevicePrivateKey, NULL);
#if XORG == 15
			SwitchCoreKeyboard(keyboardDev);
#else
			CopyKeyClass(keyboardDev, master);
#endif
		}

		return kc;
	}

	return 0;
}

#endif

