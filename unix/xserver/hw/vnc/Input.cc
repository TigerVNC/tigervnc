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

#include <rfb/LogWriter.h>
#include "Input.h"
#include "xorg-version.h"
#include "vncExtInit.h"

extern "C" {
#define public c_public
#define class c_class
#include "inputstr.h"
#if XORG >= 110
#include "inpututils.h"
#endif
#include "mi.h"
#if XORG >= 16
#include "exevents.h"
#endif
#if XORG == 16
extern void
CopyKeyClass(DeviceIntPtr device, DeviceIntPtr master);
#endif
#if XORG >= 17
#include "xkbsrv.h"
#include "xkbstr.h"
#include "xserver-properties.h"
extern _X_EXPORT DevPrivateKey CoreDevicePrivateKey;
#endif
#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#undef public
#undef class
}

#if XORG >= 110
#define Xfree free
#endif

using namespace rdr;
using namespace rfb;

static LogWriter vlog("Input");

#define BUTTONS 7

/* Event queue is shared between all devices. */
#if XORG == 15
static xEvent *eventq = NULL;
#elif XORG < 111
static EventList *eventq = NULL;
#endif

#if XORG < 111
static void initEventq(void)
{
	/* eventq is never free()-ed because it exists during server life. */
	if (eventq == NULL) {
#if XORG == 15
		eventq = (xEvent *)xcalloc(sizeof(xEvent),
					   GetMaximumEventsNum());
		if (!eventq)
			FatalError("Couldn't allocate eventq\n");
#else
		GetEventList(&eventq);
#endif
	}
}
#endif /* XORG < 111 */

#if XORG < 111
static void enqueueEvents(DeviceIntPtr dev, int n)
{
	int i;

	for (i = 0; i < n; i++) {
		/*
		 * Passing arguments in global variable eventq is probably not
		 * good programming practise but in this case it is safe and
		 * clear.
		 */
		mieqEnqueue(dev,
#if XORG == 15
			    eventq + i
#elif XORG == 16
			    (eventq + i)->event
#else
			    (InternalEvent *) (eventq + i)->event
#endif
			   );
	}
}
#endif /* XORG < 111 */

InputDevice::InputDevice(rfb::VNCServerST *_server)
	: server(_server), initialized(false), oldButtonMask(0)
{
	int i;

#if XORG < 111
	initEventq();
#endif

	for (i = 0;i < 256;i++)
		pressedKeys[i] = NoSymbol;
}

void InputDevice::PointerButtonAction(int buttonMask)
{
	int i;
#if XORG < 111
	int n;
#endif
#if XORG >= 110
	ValuatorMask mask;
#endif

	for (i = 0; i < BUTTONS; i++) {
		if ((buttonMask ^ oldButtonMask) & (1 << i)) {
			int action = (buttonMask & (1<<i)) ?
				     ButtonPress : ButtonRelease;
#if XORG < 110
			n = GetPointerEvents(eventq, pointerDev, action, i + 1,
					     POINTER_RELATIVE, 0, 0, NULL);
			enqueueEvents(pointerDev, n);
#elif XORG < 111
			valuator_mask_set_range(&mask, 0, 0, NULL);
			n = GetPointerEvents(eventq, pointerDev, action, i + 1,
					     POINTER_RELATIVE, &mask);
			enqueueEvents(pointerDev, n);
#else
			valuator_mask_set_range(&mask, 0, 0, NULL);
			QueuePointerEvents(pointerDev, action, i + 1,
					   POINTER_RELATIVE, &mask);
#endif
		}
	}

	oldButtonMask = buttonMask;
}

void InputDevice::PointerMove(const rfb::Point &pos)
{
	int valuators[2];
#if XORG < 111
	int n;
#endif
#if XORG >= 110
	ValuatorMask mask;
#endif

	if (pos.equals(cursorPos))
		return;

	valuators[0] = pos.x;
	valuators[1] = pos.y;
#if XORG < 110
	n = GetPointerEvents(eventq, pointerDev, MotionNotify, 0, POINTER_ABSOLUTE, 0,
			     2, valuators);
	enqueueEvents(pointerDev, n);
#elif XORG < 111
	valuator_mask_set_range(&mask, 0, 2, valuators);
	n = GetPointerEvents(eventq, pointerDev, MotionNotify, 0, POINTER_ABSOLUTE,
			     &mask);
	enqueueEvents(pointerDev, n);
#else
	valuator_mask_set_range(&mask, 0, 2, valuators);
	QueuePointerEvents(pointerDev, MotionNotify, 0, POINTER_ABSOLUTE, &mask);
#endif

	cursorPos = pos;
}

void InputDevice::PointerSync(void)
{
	if (cursorPos.equals(oldCursorPos))
		return;

	oldCursorPos = cursorPos;
	server->setCursorPos(cursorPos);
}

static int pointerProc(DeviceIntPtr pDevice, int onoff)
{
	BYTE map[BUTTONS + 1];
	DevicePtr pDev = (DevicePtr)pDevice;
	int i;
#if XORG >= 17
	/*
	 * NOTE: map[] array is one element longer than btn_labels[] array. This
	 * is not a bug.
	 */
	Atom btn_labels[BUTTONS];
	Atom axes_labels[2];
#endif

	switch (onoff) {
	case DEVICE_INIT:
		for (i = 0; i < BUTTONS + 1; i++)
			map[i] = i;

#if XORG >= 17
		btn_labels[0] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_LEFT);
		btn_labels[1] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_MIDDLE);
		btn_labels[2] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_RIGHT);
		btn_labels[3] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_UP);
		btn_labels[4] = XIGetKnownProperty(BTN_LABEL_PROP_BTN_WHEEL_DOWN);

		axes_labels[0] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_X);
		axes_labels[1] = XIGetKnownProperty(AXIS_LABEL_PROP_REL_Y);
#endif

		InitPointerDeviceStruct(pDev, map, BUTTONS,
#if XORG == 15
					GetMotionHistory,
#elif XORG >= 17
					btn_labels,
#endif
					(PtrCtrlProcPtr)NoopDDA,
					GetMotionHistorySize(), 2
#if XORG >= 17
					, axes_labels
#endif
					);
		break;
	case DEVICE_ON:
		pDev->on = TRUE;
		break;
	case DEVICE_OFF:
		pDev->on = FALSE;
		break;
#if 0
	case DEVICE_CLOSE:
		break;
#endif
	}

	return Success;
}

static void keyboardBell(int percent, DeviceIntPtr device, pointer ctrl,
			 int class_)
{
	if (percent > 0)
		vncBell();
}

extern void GetInitKeyboardMap(KeySymsPtr keysyms, CARD8 *modmap);

static int keyboardProc(DeviceIntPtr pDevice, int onoff)
{
#if XORG < 17
	KeySymsRec keySyms;
	CARD8 modMap[MAP_LENGTH];
#endif
	DevicePtr pDev = (DevicePtr)pDevice;

	switch (onoff) {
	case DEVICE_INIT:
#if XORG < 17
		GetInitKeyboardMap(&keySyms, modMap);
#endif
		InitKeyboardDeviceStruct(
#if XORG >= 17
					 pDevice, NULL,
#else
					 pDev, &keySyms, modMap,
#endif
					 keyboardBell, (KbdCtrlProcPtr)NoopDDA);
		break;
	case DEVICE_ON:
		pDev->on = TRUE;
		break;
	case DEVICE_OFF:
		pDev->on = FALSE;
		break;
	}

	return Success;
}

void InputDevice::InitInputDevice(void)
{
	if (initialized)
		return;

	initialized = true;

#if XORG < 17
	pointerDev = AddInputDevice(
#if XORG >= 16
				    serverClient,
#endif
				    pointerProc, TRUE);
	RegisterPointerDevice(pointerDev);

	keyboardDev = AddInputDevice(
#if XORG >= 16
				     serverClient,
#endif
				     keyboardProc, TRUE);
	RegisterKeyboardDevice(keyboardDev);

	if (ActivateDevice(pointerDev) != Success ||
	    ActivateDevice(keyboardDev) != Success)
		FatalError("Failed to activate TigerVNC devices\n");

	if (!EnableDevice(pointerDev) ||
	    !EnableDevice(keyboardDev))
		FatalError("Failed to enable TigerVNC devices\n");
#else /* < 17 */
	int ret;

	ret = AllocDevicePair(serverClient, "TigerVNC", &pointerDev,
			      &keyboardDev, pointerProc, keyboardProc,
			      FALSE);

	if (ret != Success)
		FatalError("Failed to initialize TigerVNC input devices\n");

	if (ActivateDevice(pointerDev, TRUE) != Success ||
	    ActivateDevice(keyboardDev, TRUE) != Success)
		FatalError("Failed to activate TigerVNC devices\n");

	if (!EnableDevice(pointerDev, TRUE) ||
	    !EnableDevice(keyboardDev, TRUE))
		FatalError("Failed to activate TigerVNC devices\n");
#endif /* 17 */

	PrepareInputDevices();
}

static inline void pressKey(DeviceIntPtr dev, int kc, bool down, const char *msg)
{
	int action;
#if XORG < 111
	unsigned int n;
#endif

	if (msg != NULL)
		vlog.debug("%s %d %s", msg, kc, down ? "down" : "up");

	action = down ? KeyPress : KeyRelease;
#if XORG < 111
	n = GetKeyboardEvents(eventq, dev, action, kc);
	enqueueEvents(dev, n);
#else
	QueueKeyboardEvents(dev, action, kc, NULL);
#endif
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
 * keyEvent() - work out the best keycode corresponding to the keysym sent by
 * the viewer. This is basically impossible in the general case, but we make
 * a best effort by assuming that all useful keysyms can be reached using
 * just the Shift and Level 3 (AltGr) modifiers. For core keyboards this is
 * basically always true, and should be true for most sane, western XKB
 * layouts.
 */
void InputDevice::keyEvent(rdr::U32 keysym, bool down)
{
	int i;
	unsigned state, new_state;
	KeyCode keycode;

	unsigned level_three_mask;
	KeyCode shift_press, level_three_press;
	std::list<KeyCode> shift_release, level_three_release;

	/*
	 * Release events must match the press event, so look up what
	 * keycode we sent for the press.
	 */
	if (!down) {
		for (i = 0;i < 256;i++) {
			if (pressedKeys[i] == keysym) {
				pressedKeys[i] = NoSymbol;
				pressKey(keyboardDev, i, false, "keycode");
				mieqProcessInputEvents();
				return;
			}
		}

		/*
		 * This can happen quite often as we ignore some
		 * key presses.
		 */
		vlog.debug("Unexpected release of keysym 0x%x", keysym);
		return;
	}

	/* 
	 * Since we are checking the current state to determine if we need
	 * to fake modifiers, we must make sure that everything put on the
	 * input queue is processed before we start. Otherwise, shift may be
	 * stuck down.
	 */ 
	mieqProcessInputEvents();

	state = getKeyboardState();

	keycode = keysymToKeycode(keysym, state, &new_state);

	/* Try some equivalent keysyms if we couldn't find a perfect match */
	if (keycode == 0) {
		for (i = 0;i < sizeof(altKeysym)/sizeof(altKeysym[0]);i++) {
			if (altKeysym[i].a == keysym) {
				keycode = keysymToKeycode(altKeysym[i].b,
				                          state, &new_state);
				if (keycode != 0)
					break;
			}
			if (altKeysym[i].b == keysym) {
				keycode = keysymToKeycode(altKeysym[i].a,
				                          state, &new_state);
				if (keycode != 0)
					break;
			}
		}
	}

	/* We don't have lock synchronisation... */
	if (isLockModifier(keycode, new_state)) {
		vlog.debug("Ignoring lock key (e.g. caps lock)");
		return;
	}

	/* No matches. Will have to add a new entry... */
	if (keycode == 0) {
		keycode = addKeysym(keysym, state);
		if (keycode == 0) {
			vlog.error("Failure adding new keysym 0x%x", keysym);
			return;
		}

		vlog.info("Added unknown keysym 0x%x to keycode %d",
			  keysym, keycode);

		new_state = state;
	}

	/*
	 * We need a bigger state change than just shift,
	 * so we need to know what the mask is for level 3 shifts.
	 */
	if ((new_state & ~ShiftMask) != (state & ~ShiftMask))
		level_three_mask = getLevelThreeMask();
	else
		level_three_mask = 0;

	shift_press = level_three_press = 0;

	/* Need a fake press or release of shift? */
	if (!(state & ShiftMask) && (new_state & ShiftMask)) {
		shift_press = pressShift();
		if (shift_press == 0) {
			vlog.error("Unable to find a modifier key for Shift");
			return;
		}

		pressKey(keyboardDev, shift_press, true, "temp shift");
	} else if ((state & ShiftMask) && !(new_state & ShiftMask)) {
		std::list<KeyCode>::const_iterator iter;

		shift_release = releaseShift();
		if (shift_release.empty()) {
			vlog.error("Unable to find the modifier key(s) for releasing Shift");
			return;
		}

		for (iter = shift_release.begin();iter != shift_release.end();++iter)
			pressKey(keyboardDev, *iter, false, "temp shift");
	}

	/* Need a fake press or release of level three shift? */
	if (!(state & level_three_mask) && (new_state & level_three_mask)) {
		level_three_press = pressLevelThree();
		if (level_three_press == 0) {
			vlog.error("Unable to find a modifier key for ISO_Level3_Shift/Mode_Switch");
			return;
		}

		pressKey(keyboardDev, level_three_press, true, "temp level 3 shift");
	} else if ((state & level_three_mask) && !(new_state & level_three_mask)) {
		std::list<KeyCode>::const_iterator iter;

		level_three_release = releaseLevelThree();
		if (level_three_release.empty()) {
			vlog.error("Unable to find the modifier key(s) for releasing ISO_Level3_Shift/Mode_Switch");
			return;
		}

		for (iter = level_three_release.begin();iter != level_three_release.end();++iter)
			pressKey(keyboardDev, *iter, false, "temp level 3 shift");
	}

	/* Now press the actual key */
	pressKey(keyboardDev, keycode, true, "keycode");

	/* And store the mapping so that we can do a proper release later */
	for (i = 0;i < 256;i++) {
		if (i == keycode)
			continue;
		if (pressedKeys[i] == keysym) {
			vlog.error("Keysym 0x%x generated by both keys %d and %d", keysym, i, keycode);
			pressedKeys[i] = NoSymbol;
		}
	}

	pressedKeys[keycode] = keysym;

	/* Undo any fake level three shift */
	if (level_three_press != 0)
		pressKey(keyboardDev, level_three_press, false, "temp level 3 shift");
	else if (!level_three_release.empty()) {
		std::list<KeyCode>::const_iterator iter;
		for (iter = level_three_release.begin();iter != level_three_release.end();++iter)
			pressKey(keyboardDev, *iter, true, "temp level 3 shift");
	}

	/* Undo any fake shift */
	if (shift_press != 0)
		pressKey(keyboardDev, shift_press, false, "temp shift");
	else if (!shift_release.empty()) {
		std::list<KeyCode>::const_iterator iter;
		for (iter = shift_release.begin();iter != shift_release.end();++iter)
			pressKey(keyboardDev, *iter, true, "temp shift");
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
