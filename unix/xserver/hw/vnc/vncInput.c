/* Copyright (C) 2009 TightVNC Team
 * Copyright (C) 2009, 2014 Red Hat, Inc.
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

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "xorg-version.h"

#include "vncInput.h"
#include "vncExtInit.h"
#include "RFBGlue.h"

#include "inputstr.h"
#include "inpututils.h"
#include "mi.h"
#include "mipointer.h"
#include "exevents.h"
#include "scrnintstr.h"
#include "xkbsrv.h"
#include "xkbstr.h"
#include "xserver-properties.h"
extern _X_EXPORT DevPrivateKey CoreDevicePrivateKey;
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

extern const unsigned short code_map_qnum_to_xorgevdev[];
extern const unsigned int code_map_qnum_to_xorgevdev_len;
extern const unsigned short code_map_qnum_to_xorgkbd[];
extern const unsigned int code_map_qnum_to_xorgkbd_len;

#define BUTTONS 7

DeviceIntPtr vncKeyboardDev;
DeviceIntPtr vncPointerDev;

static int oldButtonMask;
static int cursorPosX, cursorPosY;

static const unsigned short *codeMap;
static unsigned int codeMapLen;

static KeySym pressedKeys[256];

static int vncPointerProc(DeviceIntPtr pDevice, int onoff);
static void vncKeyboardBell(int percent, DeviceIntPtr device,
                            void * ctrl, int class);
static int vncKeyboardProc(DeviceIntPtr pDevice, int onoff);

static void vncKeysymKeyboardEvent(KeySym keysym, int down);

#define LOG_NAME "Input"

#define LOG_ERROR(...) vncLogError(LOG_NAME, __VA_ARGS__)
#define LOG_STATUS(...) vncLogStatus(LOG_NAME, __VA_ARGS__)
#define LOG_INFO(...) vncLogInfo(LOG_NAME, __VA_ARGS__)
#define LOG_DEBUG(...) vncLogDebug(LOG_NAME, __VA_ARGS__)

/*
 * Init input device.
 * This has to be called after core pointer/keyboard
 * initialization which unfortunately is after extensions
 * initialization (which means we cannot call it in
 * vncExtensionInit(). Check InitExtensions(),
 * InitCoreDevices() and InitInput() calls in dix/main.c.
 * Instead we call it from XserverDesktop at an appropriate
 * time.
 */
void vncInitInputDevice(void)
{
	int i, ret;

	if ((vncPointerDev != NULL) || (vncKeyboardDev != NULL))
		return;

	/*
	 * On Linux we try to provide the same key codes as Xorg with
	 * the evdev driver. On other platforms we mimic the older
	 * Xorg KBD driver.
	 */
#ifdef __linux__
	codeMap = code_map_qnum_to_xorgevdev;
	codeMapLen = code_map_qnum_to_xorgevdev_len;
#else
	codeMap = code_map_qnum_to_xorgkbd;
	codeMapLen = code_map_qnum_to_xorgkbd_len;
#endif

	for (i = 0;i < 256;i++)
		pressedKeys[i] = NoSymbol;

	ret = AllocDevicePair(serverClient, "TigerVNC",
	                      &vncPointerDev, &vncKeyboardDev,
	                      vncPointerProc, vncKeyboardProc,
			      FALSE);

	if (ret != Success)
		FatalError("Failed to initialize TigerVNC input devices\n");

	if (ActivateDevice(vncPointerDev, TRUE) != Success ||
	    ActivateDevice(vncKeyboardDev, TRUE) != Success)
		FatalError("Failed to activate TigerVNC devices\n");

	if (!EnableDevice(vncPointerDev, TRUE) ||
	    !EnableDevice(vncKeyboardDev, TRUE))
		FatalError("Failed to activate TigerVNC devices\n");

	vncPrepareInputDevices();
}

void vncPointerButtonAction(int buttonMask)
{
	int i;
	ValuatorMask mask;

	for (i = 0; i < BUTTONS; i++) {
		if ((buttonMask ^ oldButtonMask) & (1 << i)) {
			int action = (buttonMask & (1<<i)) ?
				     ButtonPress : ButtonRelease;
			valuator_mask_set_range(&mask, 0, 0, NULL);
			QueuePointerEvents(vncPointerDev, action, i + 1,
					   POINTER_RELATIVE, &mask);
		}
	}

	oldButtonMask = buttonMask;
}

void vncPointerMove(int x, int y)
{
	int valuators[2];
	ValuatorMask mask;

	if (cursorPosX == x && cursorPosY == y)
		return;

	valuators[0] = x;
	valuators[1] = y;
	valuator_mask_set_range(&mask, 0, 2, valuators);
	QueuePointerEvents(vncPointerDev, MotionNotify, 0,
	                   POINTER_ABSOLUTE, &mask);

	cursorPosX = x;
	cursorPosY = y;
}

void vncGetPointerPos(int *x, int *y)
{
	if (vncPointerDev != NULL) {
		ScreenPtr ptrScreen;

		miPointerGetPosition(vncPointerDev, &cursorPosX, &cursorPosY);

		/* Pointer coordinates are screen relative */
		ptrScreen = miPointerGetScreen(vncPointerDev);
		cursorPosX += ptrScreen->x;
		cursorPosY += ptrScreen->y;
	}

	*x = cursorPosX;
	*y = cursorPosY;
}

static int vncPointerProc(DeviceIntPtr pDevice, int onoff)
{
	BYTE map[BUTTONS + 1];
	DevicePtr pDev = (DevicePtr)pDevice;
	int i;
	/*
	 * NOTE: map[] array is one element longer than btn_labels[] array. This
	 * is not a bug.
	 */
	Atom btn_labels[BUTTONS];
	Atom axes_labels[2];

	switch (onoff) {
	case DEVICE_INIT:
		for (i = 0; i < BUTTONS + 1; i++)
			map[i] = i;

		btn_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
		btn_labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
		btn_labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
		btn_labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
		btn_labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);
		btn_labels[5] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_LEFT);
		btn_labels[6] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_HWHEEL_RIGHT);

		axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
		axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);

		InitPointerDeviceStruct(pDev, map, BUTTONS, btn_labels,
					(PtrCtrlProcPtr)NoopDDA,
					GetMotionHistorySize(),
					2, axes_labels);
		break;
	case DEVICE_ON:
		pDev->on = TRUE;
		break;
	case DEVICE_OFF:
		pDev->on = FALSE;
		break;
	case DEVICE_CLOSE:
		vncPointerDev = NULL;
		break;
	}

	return Success;
}

static void vncKeyboardBell(int percent, DeviceIntPtr device,
                            void * ctrl, int class)
{
	if (percent > 0)
		vncBell();
}

static void vncKeyboardCtrl(DeviceIntPtr pDevice, KeybdCtrl *ctrl)
{
	vncSetLEDState(ctrl->leds);
}

static int vncKeyboardProc(DeviceIntPtr pDevice, int onoff)
{
	DevicePtr pDev = (DevicePtr)pDevice;

	switch (onoff) {
	case DEVICE_INIT:
		InitKeyboardDeviceStruct(pDevice, NULL, vncKeyboardBell,
					 vncKeyboardCtrl);
		break;
	case DEVICE_ON:
		pDev->on = TRUE;
		break;
	case DEVICE_OFF:
		pDev->on = FALSE;
		break;
	case DEVICE_CLOSE:
		vncKeyboardDev = NULL;
		break;
	}

	return Success;
}

static inline void pressKey(DeviceIntPtr dev, int kc, Bool down, const char *msg)
{
	int action;

	if (msg != NULL)
		LOG_DEBUG("%s %d %s", msg, kc, down ? "down" : "up");

	action = down ? KeyPress : KeyRelease;
#if XORG_OLDER_THAN(1, 18, 0)
	QueueKeyboardEvents(dev, action, kc, NULL);
#else
	QueueKeyboardEvents(dev, action, kc);
#endif
}

/*
 * vncKeyboardEvent() - add X11 events for the given RFB key event
 */
void vncKeyboardEvent(KeySym keysym, unsigned xtcode, int down)
{
	/* Simple case: the client has specified the key */
	if (xtcode && xtcode < codeMapLen) {
		int keycode;

		keycode = codeMap[xtcode];
		if (!keycode) {
			/*
			 * Figure something out based on keysym if we
			 * cannot find a mapping.
			 */
			if (keysym)
				vncKeysymKeyboardEvent(keysym, down);
			return;
		}

		/*
		 * We update the state table in case we get a mix of
		 * events with and without key codes.
		 */
		if (down)
			pressedKeys[keycode] = keysym;
		else
			pressedKeys[keycode] = NoSymbol;

		pressKey(vncKeyboardDev, keycode, down, "raw keycode");
		mieqProcessInputEvents();
		return;
	}

	/*
	 * Advanced case: We have to figure out a sequence of keys that
	 *                result in the given keysym
	 */
	if (keysym)
		vncKeysymKeyboardEvent(keysym, down);
}

/* altKeysym is a table of alternative keysyms which have the same meaning. */

static struct altKeysym_t {
	KeySym a, b;
} altKeysym[] = {
	{ XK_Shift_L,		XK_Shift_R },
	{ XK_Control_L,		XK_Control_R },
	{ XK_Meta_L,		XK_Meta_R },
	{ XK_Alt_L,		XK_Alt_R },
	{ XK_Super_L,		XK_Super_R },
	{ XK_Hyper_L,		XK_Hyper_R },
	{ XK_KP_Space,		XK_space },
	{ XK_KP_Tab,		XK_Tab },
	{ XK_KP_Enter,		XK_Return },
	{ XK_KP_F1,		XK_F1 },
	{ XK_KP_F2,		XK_F2 },
	{ XK_KP_F3,		XK_F3 },
	{ XK_KP_F4,		XK_F4 },
	{ XK_KP_Home,		XK_Home },
	{ XK_KP_Left,		XK_Left },
	{ XK_KP_Up,		XK_Up },
	{ XK_KP_Right,		XK_Right },
	{ XK_KP_Down,		XK_Down },
	{ XK_KP_Page_Up,	XK_Page_Up },
	{ XK_KP_Page_Down,	XK_Page_Down },
	{ XK_KP_End,		XK_End },
	{ XK_KP_Begin,		XK_Begin },
	{ XK_KP_Insert,		XK_Insert },
	{ XK_KP_Delete,		XK_Delete },
	{ XK_KP_Equal,		XK_equal },
	{ XK_KP_Multiply,	XK_asterisk },
	{ XK_KP_Add,		XK_plus },
	{ XK_KP_Separator,	XK_comma },
	{ XK_KP_Subtract,	XK_minus },
	{ XK_KP_Decimal,	XK_period },
	{ XK_KP_Divide,		XK_slash },
	{ XK_KP_0,		XK_0 },
	{ XK_KP_1,		XK_1 },
	{ XK_KP_2,		XK_2 },
	{ XK_KP_3,		XK_3 },
	{ XK_KP_4,		XK_4 },
	{ XK_KP_5,		XK_5 },
	{ XK_KP_6,		XK_6 },
	{ XK_KP_7,		XK_7 },
	{ XK_KP_8,		XK_8 },
	{ XK_KP_9,		XK_9 },
	{ XK_ISO_Level3_Shift,	XK_Mode_switch },
};

/*
 * vncKeysymKeyboardEvent() - work out the best keycode corresponding
 * to the keysym sent by the viewer. This is basically impossible in
 * the general case, but we make a best effort by assuming that all
 * useful keysyms can be reached using just the Shift and
 * Level 3 (AltGr) modifiers. For core keyboards this is basically
 * always true, and should be true for most sane, western XKB layouts.
 */
static void vncKeysymKeyboardEvent(KeySym keysym, int down)
{
	int i;
	unsigned state, new_state;
	KeyCode keycode;

	unsigned level_three_mask;
	KeyCode shift_press, level_three_press;
	KeyCode shift_release[8], level_three_release[8];
	size_t shift_release_count, level_three_release_count;

	/*
	 * Release events must match the press event, so look up what
	 * keycode we sent for the press.
	 */
	if (!down) {
		for (i = 0;i < 256;i++) {
			if (pressedKeys[i] == keysym) {
				pressedKeys[i] = NoSymbol;
				pressKey(vncKeyboardDev, i, FALSE, "keycode");
				mieqProcessInputEvents();
				return;
			}
		}

		/*
		 * This can happen quite often as we ignore some
		 * key presses.
		 */
		LOG_DEBUG("Unexpected release of keysym 0x%x", keysym);
		return;
	}

	/* 
	 * Since we are checking the current state to determine if we need
	 * to fake modifiers, we must make sure that everything put on the
	 * input queue is processed before we start. Otherwise, shift may be
	 * stuck down.
	 */ 
	mieqProcessInputEvents();

	state = vncGetKeyboardState();

	keycode = vncKeysymToKeycode(keysym, state, &new_state);

	/*
	 * Shift+Alt is often mapped to Meta, so try that rather than
	 * allocating a new entry, faking shift, or using the dummy
	 * key entries that many layouts have.
	 */
	if ((state & ShiftMask) &&
	    ((keysym == XK_Alt_L) || (keysym == XK_Alt_R))) {
		KeyCode alt, meta;

		if (keysym == XK_Alt_L) {
			alt = vncKeysymToKeycode(XK_Alt_L, state & ~ShiftMask, NULL);
			meta = vncKeysymToKeycode(XK_Meta_L, state, NULL);
		} else {
			alt = vncKeysymToKeycode(XK_Alt_R, state & ~ShiftMask, NULL);
			meta = vncKeysymToKeycode(XK_Meta_R, state, NULL);
		}

		if ((meta != 0) && (alt == meta)) {
			LOG_DEBUG("Replacing Shift+Alt with Shift+Meta");
			keycode = meta;
			new_state = state;
		}
	}

	/* Try some equivalent keysyms if we couldn't find a perfect match */
	if (keycode == 0) {
		for (i = 0;i < sizeof(altKeysym)/sizeof(altKeysym[0]);i++) {
			KeySym altsym;

			if (altKeysym[i].a == keysym)
				altsym = altKeysym[i].b;
			else if (altKeysym[i].b == keysym)
				altsym = altKeysym[i].a;
			else
				continue;

			keycode = vncKeysymToKeycode(altsym, state, &new_state);
			if (keycode != 0)
				break;
		}
	}

	/* No matches. Will have to add a new entry... */
	if (keycode == 0) {
		keycode = vncAddKeysym(keysym, state);
		if (keycode == 0) {
			LOG_ERROR("Failure adding new keysym 0x%x", keysym);
			return;
		}

		LOG_INFO("Added unknown keysym 0x%x to keycode %d",
		         keysym, keycode);

		/*
		 * The state given to addKeysym() is just a hint and
		 * the actual result might still require some state
		 * changes.
		 */
		keycode = vncKeysymToKeycode(keysym, state, &new_state);
		if (keycode == 0) {
			LOG_ERROR("Newly added keysym 0x%x cannot be generated", keysym);
			return;
		}
	}

	/*
	 * X11 generally lets shift toggle the keys on the numeric pad
	 * the same way NumLock does. This is however not the case on
	 * other systems like Windows. As a result, some applications
	 * get confused when we do a fake shift to get the same effect
	 * that having NumLock active would produce.
	 *
	 * Not all clients have proper NumLock synchronisation (so we
	 * can avoid faking shift) so we try to avoid the fake shifts
	 * if we can use an alternative keysym.
	 */
	if (((state & ShiftMask) != (new_state & ShiftMask)) &&
	    vncGetAvoidShiftNumLock() && vncIsAffectedByNumLock(keycode)) {
	    	KeyCode keycode2;
	    	unsigned new_state2;

		LOG_DEBUG("Finding alternative to keysym 0x%x to avoid fake shift for numpad", keysym);

		for (i = 0;i < sizeof(altKeysym)/sizeof(altKeysym[0]);i++) {
			KeySym altsym;

			if (altKeysym[i].a == keysym)
				altsym = altKeysym[i].b;
			else if (altKeysym[i].b == keysym)
				altsym = altKeysym[i].a;
			else
				continue;

			keycode2 = vncKeysymToKeycode(altsym, state, &new_state2);
			if (keycode2 == 0)
				continue;

			if (((state & ShiftMask) != (new_state2 & ShiftMask)) &&
			    vncIsAffectedByNumLock(keycode2))
				continue;

			break;
		}

		if (i == sizeof(altKeysym)/sizeof(altKeysym[0]))
			LOG_DEBUG("No alternative keysym found");
		else {
			keycode = keycode2;
			new_state = new_state2;
		}
	}

	/*
	 * "Shifted Tab" is a bit of a mess. Some systems have varying,
	 * special keysyms for this symbol. VNC mandates that clients
	 * should always send the plain XK_Tab keysym and the server
	 * should deduce the meaning based on current Shift state.
	 * To comply with this, we will find the keycode that sends
	 * XK_Tab, and make sure that Shift isn't cleared. This can
	 * possibly result in a different keysym than XK_Tab, but that
	 * is the desired behaviour.
	 *
	 * Note: We never get ISO_Left_Tab here because it's already
	 *       been translated in VNCSConnectionST.
	 */
	if (keysym == XK_Tab && (state & ShiftMask))
		new_state |= ShiftMask;

	/*
	 * We need a bigger state change than just shift,
	 * so we need to know what the mask is for level 3 shifts.
	 */
	if ((new_state & ~ShiftMask) != (state & ~ShiftMask))
		level_three_mask = vncGetLevelThreeMask();
	else
		level_three_mask = 0;

	shift_press = level_three_press = 0;
	shift_release_count = level_three_release_count = 0;

	/* Need a fake press or release of shift? */
	if (!(state & ShiftMask) && (new_state & ShiftMask)) {
		shift_press = vncPressShift();
		if (shift_press == 0) {
			LOG_ERROR("Unable to find a modifier key for Shift");
			return;
		}

		pressKey(vncKeyboardDev, shift_press, TRUE, "temp shift");
	} else if ((state & ShiftMask) && !(new_state & ShiftMask)) {
		shift_release_count = vncReleaseShift(shift_release,
		                                      sizeof(shift_release)/sizeof(*shift_release));
		if (shift_release_count == 0) {
			LOG_ERROR("Unable to find the modifier key(s) for releasing Shift");
			return;
		}

		for (i = 0;i < shift_release_count;i++)
			pressKey(vncKeyboardDev, shift_release[i], FALSE, "temp shift");
	}

	/* Need a fake press or release of level three shift? */
	if (!(state & level_three_mask) && (new_state & level_three_mask)) {
		level_three_press = vncPressLevelThree();
		if (level_three_press == 0) {
			LOG_ERROR("Unable to find a modifier key for ISO_Level3_Shift/Mode_Switch");
			return;
		}

		pressKey(vncKeyboardDev, level_three_press, TRUE, "temp level 3 shift");
	} else if ((state & level_three_mask) && !(new_state & level_three_mask)) {
		level_three_release_count = vncReleaseLevelThree(level_three_release,
		                                                 sizeof(level_three_release)/sizeof(*level_three_release));
		if (level_three_release_count == 0) {
			LOG_ERROR("Unable to find the modifier key(s) for releasing ISO_Level3_Shift/Mode_Switch");
			return;
		}

		for (i = 0;i < level_three_release_count;i++)
			pressKey(vncKeyboardDev, level_three_release[i], FALSE, "temp level 3 shift");
	}

	/* Now press the actual key */
	pressKey(vncKeyboardDev, keycode, TRUE, "keycode");

	/* And store the mapping so that we can do a proper release later */
	for (i = 0;i < 256;i++) {
		if (i == keycode)
			continue;
		if (pressedKeys[i] == keysym) {
			LOG_ERROR("Keysym 0x%x generated by both keys %d and %d", keysym, i, keycode);
			pressedKeys[i] = NoSymbol;
		}
	}

	pressedKeys[keycode] = keysym;

	/* Undo any fake level three shift */
	if (level_three_press != 0)
		pressKey(vncKeyboardDev, level_three_press, FALSE, "temp level 3 shift");
	else if (level_three_release_count != 0) {
		for (i = 0;i < level_three_release_count;i++)
			pressKey(vncKeyboardDev, level_three_release[i], TRUE, "temp level 3 shift");
	}

	/* Undo any fake shift */
	if (shift_press != 0)
		pressKey(vncKeyboardDev, shift_press, FALSE, "temp shift");
	else if (shift_release_count != 0) {
		for (i = 0;i < shift_release_count;i++)
			pressKey(vncKeyboardDev, shift_release[i], TRUE, "temp shift");
	}

	/*
	 * When faking a modifier we are putting a keycode (which can
	 * currently activate the desired modifier) on the input
	 * queue. A future modmap change can change the mapping so
	 * that this keycode means something else entirely. Guard
	 * against this by processing the queue now.
	 */
	mieqProcessInputEvents();
}
