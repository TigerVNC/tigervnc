/* Copyright (C) 2009 TightVNC Team
 * Copyright (C) 2009 Red Hat, Inc.
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
static int pointerProc(DeviceIntPtr pDevice, int onoff);

static int keyboardProc(DeviceIntPtr pDevice, int onoff);
static KeySym KeyCodetoKeySym(KeySymsPtr keymap, int keycode, int col);
static KeyCode KeysymToKeycode(KeySymsPtr keymap, KeySym ks, int* col);

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
	: server(_server), oldButtonMask(0)
{
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
#endif
#if XORG < 111
	initEventq();
#endif
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

	initInputDevice();

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

	initInputDevice();

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

void InputDevice::initInputDevice(void)
{
#if XORG >= 17
	int ret;
	static int initialized = 0;

	if (initialized != 0)
		return;

	initialized = 1;

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
#endif
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
	n = GetKeyboardEvents(eventq, dev, action, kc, NULL);
	enqueueEvents(dev, n);
#else
	QueueKeyboardEvents(dev, action, kc, NULL);
#endif
}

#define IS_PRESSED(keyc, keycode) \
	((keyc)->down[(keycode) >> 3] & (1 << ((keycode) & 7)))

/*
 * ModifierState is a class which helps simplify generating a "fake" press or
 * release of shift, ctrl, alt, etc.  An instance of the class is created for
 * every modifier which may need to be pressed or released.  Then either
 * press() or release() may be called to make sure that the corresponding keys
 * are in the right state.  The destructor of the class automatically reverts
 * to the previous state.  Each modifier may have multiple keys associated with
 * it, so in the case of a fake release, this may involve releasing more than
 * one key.
 */

class ModifierState {
public:
	ModifierState(DeviceIntPtr _dev, int _modIndex)
		: modIndex(_modIndex), nKeys(0), keys(0), pressed(false),
		  dev(_dev) {}

	~ModifierState()
	{
		for (int i = 0; i < nKeys; i++)
			pressKey(dev, keys[i], !pressed, "fake keycode");
		delete [] keys;
	}

	void press()
	{
		int state, maxKeysPerMod, keycode;
#if XORG >= 17
		KeyCode *modmap = NULL;
#if XORG >= 111
		state = XkbStateFieldFromRec(&dev->master->key->xkbInfo->state);
#else /* XORG >= 111 */
		state = XkbStateFieldFromRec(&dev->u.master->key->xkbInfo->state);
#endif /* XORG >= 111 */
#else
		KeyClassPtr keyc = dev->key;
		state = keyc->state;
#endif
		if ((state & (1 << modIndex)) != 0)
			return;

#if XORG >= 17
		if (generate_modkeymap(serverClient, dev, &modmap,
				       &maxKeysPerMod) != Success) {
			vlog.error("generate_modkeymap failed");
			return;
		}

		if (maxKeysPerMod == 0) {
			vlog.debug("Keyboard has no modifiers");
			xfree(modmap);
			return;
		}

		keycode = modmap[modIndex * maxKeysPerMod];
		xfree(modmap);
#else
		maxKeysPerMod = keyc->maxKeysPerModifier;
		keycode = keyc->modifierKeyMap[modIndex * maxKeysPerMod];
#endif
		tempKeyEvent(keycode, true, maxKeysPerMod);
		pressed = true;
	}

	void release()
	{
		int state, maxKeysPerMod;
		KeyClassPtr keyc;
#if XORG >= 17
		KeyCode *modmap = NULL;

#if XORG >= 111
		keyc = dev->master->key;
#else /* XORG >= 111 */
		keyc = dev->u.master->key;
#endif /* XORG >= 111 */
		state = XkbStateFieldFromRec(&keyc->xkbInfo->state);
#else
		keyc = dev->key;
		state = keyc->state;
#endif
		if ((state & (1 << modIndex)) == 0)
			return;

#if XORG >= 17
		if (generate_modkeymap(serverClient, dev, &modmap,
				       &maxKeysPerMod) != Success) {
			vlog.error("generate_modkeymap failed");
			return;
		}

		if (maxKeysPerMod == 0) {
			vlog.debug("Keyboard has no modifiers");
			xfree(modmap);
			return;
		}
#else
		maxKeysPerMod = keyc->maxKeysPerModifier;
#endif

		for (int k = 0; k < maxKeysPerMod; k++) {
			int keycode;
			int index = modIndex * maxKeysPerMod + k;
#if XORG >= 17
			keycode = modmap[index];
#else
			keycode = keyc->modifierKeyMap[index];
#endif
			if (keycode && IS_PRESSED(keyc, keycode))
				tempKeyEvent(keycode, false, maxKeysPerMod);
		}
#if XORG >= 17
		xfree(modmap);
#endif
	}

private:
	void tempKeyEvent(int keycode, bool down, int maxKeysPerMod)
	{
		if (keycode) {
			if (!keys) keys = new int[maxKeysPerMod];
			keys[nKeys++] = keycode;
			pressKey(dev, keycode, down, "fake keycode");
		}
	}

	int modIndex;
	int nKeys;
	int *keys;
	bool pressed;
	DeviceIntPtr dev;
};


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
};

/*
 * keyEvent() - work out the best keycode corresponding to the keysym sent by
 * the viewer.  This is non-trivial because we can't assume much about the
 * local keyboard layout.  We must also find out which column of the keyboard
 * mapping the keysym is in, and alter the shift state appropriately.  Column 0
 * means both shift and "mode_switch" (AltGr) must be released, column 1 means
 * shift must be pressed and mode_switch released, column 2 means shift must be
 * released and mode_switch pressed, and column 3 means both shift and
 * mode_switch must be pressed.
 *
 * Magic, which dynamically adds keysym<->keycode mapping depends on X.Org
 * version. Quick explanation of that "magic":
 * 
 * 1.5
 * - has only one core keyboard so we have to keep core keyboard mapping
 *   synchronized with vncKeyboardDevice. Do it via SwitchCoreKeyboard()
 *
 * 1.6 (aka MPX - Multi pointer X)
 * - multiple master devices (= core devices) exists, keep vncKeyboardDevice
 *   synchronized with proper master device
 */

#if XORG >= 17
#define FREE_MAPS \
	do { \
	        xfree(modmap); \
	        xfree(keymap->map); \
	        xfree(keymap); \
	} while (0);
#else
#define FREE_MAPS
#endif

#if XORG >= 17
/*
 * Modifier keysyms must be handled differently. Instead of finding
 * the right row and collumn in the keymap, directly press/release
 * the keycode which is mapped as modifier with the same keysym.
 *
 * This will avoid issues when there are multiple modifier keysyms
 * in the keymap but only some of them are mapped as modifiers in
 * the modmap.
 *
 * Returns keycode of the modifier key.
 */

static inline int isModifier(KeySymsPtr keymap, KeyCode *modmap,
			      int maxKeysPerMod, rdr::U32 keysym)
{
	KeySym *map = keymap->map;
	KeyCode minKeyCode = keymap->minKeyCode;
	int mapWidth = keymap->mapWidth;
	int i, j, k;

	/* Find modifier index in the modmap */
	for (i = 0; i < 8; i++) {
		for (k = 0; k < maxKeysPerMod; k++) {
			int index = i * maxKeysPerMod + k;
			int keycode = modmap[index];

			if (keycode == 0)
				continue;

			for (j = 0; j < mapWidth; j++) {
				if (map[(keycode - minKeyCode) * mapWidth + j]
				    == keysym) {
					return keycode;
				}
			}
		}
	}

	return -1; /* Not a modifier */
}
#endif

void InputDevice::keyEvent(rdr::U32 keysym, bool down)
{
#if XORG < 17
	DeviceIntPtr master;
#endif
	KeyClassPtr keyc;
	KeySymsPtr keymap = NULL;
	KeySym *map = NULL;
	KeyCode minKeyCode, maxKeyCode;
	KeyCode *modmap = NULL;
	int mapWidth;
	unsigned int i;
	int j, k, state, maxKeysPerMod;
#if XORG >= 17
	KeybdCtrl ctrl;
#endif

	initInputDevice();

	/* 
	 * Since we are checking the current state to determine if we need
	 * to fake modifiers, we must make sure that everything put on the
	 * input queue is processed before we start. Otherwise, shift may be
	 * stuck down.
	 */ 
	mieqProcessInputEvents();

	if (keysym == XK_Caps_Lock) {
		vlog.debug("Ignoring caps lock");
		return;
	}

#if XORG >= 17
#if XORG >= 111
	keyc = keyboardDev->master->key;
#else /* XORG >= 111 */
	keyc = keyboardDev->u.master->key;
#endif /* XORG >= 111 */

	keymap = XkbGetCoreMap(keyboardDev);
	if (!keymap) {
		vlog.error("VNC keyboard device has no map");
		return;
	}

	if (generate_modkeymap(serverClient, keyboardDev, &modmap,
	    		       &maxKeysPerMod) != Success) {
		vlog.error("generate_modkeymap failed");
		xfree(keymap->map);
		xfree(keymap);
		return;
	}

	if (maxKeysPerMod == 0)
		vlog.debug("Keyboard has no modifiers");

	state = XkbStateFieldFromRec(&keyc->xkbInfo->state);
#else
	keyc = keyboardDev->key;
	state = keyc->state;
	maxKeysPerMod = keyc->maxKeysPerModifier;
	keymap = &keyc->curKeySyms;
	modmap = keyc->modifierKeyMap;
#endif
	map = keymap->map;
	minKeyCode = keymap->minKeyCode;
	maxKeyCode = keymap->maxKeyCode;
	mapWidth = keymap->mapWidth;

#if XORG >= 17
	/*
	 * No server-side key repeating, please. Some clients won't work well,
	 * check https://bugzilla.redhat.com/show_bug.cgi?id=607866.
	 */
	ctrl = keyboardDev->kbdfeed->ctrl;
	if (ctrl.autoRepeat != FALSE) {
		ctrl.autoRepeat = FALSE;
		XkbSetRepeatKeys(keyboardDev, -1, ctrl.autoRepeat);
	}
#endif

	/* find which modifier Mode_switch is on. */
	int modeSwitchMapIndex = 0;
	for (i = 3; i < 8; i++) {
		for (k = 0; k < maxKeysPerMod; k++) {
			int index = i * maxKeysPerMod + k;
			int keycode = modmap[index];

			if (keycode == 0)
				continue;

			for (j = 0; j < mapWidth; j++) {
				if (map[(keycode - minKeyCode) * mapWidth + j]
				    == XK_Mode_switch) {
					modeSwitchMapIndex = i;
					goto ModeSwitchFound;
				}
			}
		}
	}
ModeSwitchFound:

	int kc;
	int col = 0;

#if XORG >= 17
	if ((kc = isModifier(keymap, modmap, maxKeysPerMod, keysym)) != -1) {
		/*
		 * It is a modifier key event.
		 *
		 * Don't do any auto-repeat because the X server will translate
		 * each press into a release followed by a press.
		 */
		if (IS_PRESSED(keyc, kc) && down) {
			FREE_MAPS;
			return;
		}

		goto press;
	}
#endif

	if (maxKeysPerMod != 0) {
		if ((state & (1 << ShiftMapIndex)) != 0)
			col |= 1;
		if (modeSwitchMapIndex != 0 &&
		    ((state & (1 << modeSwitchMapIndex))) != 0)
			col |= 2;
	}

	kc = KeysymToKeycode(keymap, keysym, &col);

	/*
	 * Sort out the "shifted Tab" mess.  If we are sent a shifted Tab,
	 * generate a local shifted Tab regardless of what the "shifted Tab"
	 * keysym is on the local keyboard (it might be Tab, ISO_Left_Tab or
	 * HP's private BackTab keysym, and quite possibly some others too).
	 * We never get ISO_Left_Tab here because it's already been translated
	 * in VNCSConnectionST.
	 */
	if (maxKeysPerMod != 0 && keysym == XK_Tab &&
	    ((state & (1 << ShiftMapIndex))) != 0)
		col |= 1;

	if (kc == 0) {
		/*
		 * Not a direct match in the local keyboard mapping.  Check for
		 * alternative keysyms with the same meaning.
		 */
		for (i = 0; i < sizeof(altKeysym) / sizeof(altKeysym_t); i++) {
			if (keysym == altKeysym[i].a)
				kc = KeysymToKeycode(keymap, altKeysym[i].b,
						     &col);
			else if (keysym == altKeysym[i].b)
				kc = KeysymToKeycode(keymap, altKeysym[i].a,
						     &col);
			if (kc)
				break;
		}
	}

	if (kc == 0) {
		/* Dynamically add a new key to the keyboard mapping. */
		for (kc = maxKeyCode; kc >= minKeyCode; kc--) {
			if (map[(kc - minKeyCode) * mapWidth] != 0)
				continue;

			map[(kc - minKeyCode) * mapWidth] = keysym;
			col = 0;

			vlog.info("Added unknown keysym 0x%x to keycode %d",
				  keysym, kc);

#if XORG < 17
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
#else /* XORG < 17 */
			XkbApplyMappingChange(keyboardDev, keymap, minKeyCode,
					      maxKeyCode - minKeyCode + 1,
					      NULL, serverClient);
#if XORG >= 111
			XkbCopyDeviceKeymap(keyboardDev->master, keyboardDev);
#else
			XkbCopyDeviceKeymap(keyboardDev->u.master, keyboardDev);
#endif
#endif /* XORG < 17 */
			break;
		}
	}

	if (kc < minKeyCode) {
		vlog.info("Keyboard mapping full - ignoring unknown keysym "
			  "0x%x",keysym);
		FREE_MAPS;
		return;
	}

#if XORG < 17
	/*
	 * See if it's a modifier key.  If so, then don't do any auto-repeat,
	 * because the X server will translate each press into a release
	 * followed by a press.
	 */
	for (i = 0; i < 8; i++) {
		for (k = 0; k < maxKeysPerMod; k++) {
			int index = i * maxKeysPerMod + k;
			if (kc == modmap[index] && IS_PRESSED(keyc,kc) && down) {
				FREE_MAPS;
				return;
			}	
		}
	}
#else
	/*
	 * If you would like to press a key which is already pressed then
	 * viewer didn't send the "release" event. In this case release it
	 * before the press.
	 */
	if (IS_PRESSED(keyc, kc) && down) {
		vlog.debug("KeyRelease for %d wasn't sent, releasing", kc);
		pressKey(keyboardDev, kc, false, "fixing keycode");
	}
#endif

	if (maxKeysPerMod != 0) {
		ModifierState shift(keyboardDev, ShiftMapIndex);
		ModifierState modeSwitch(keyboardDev, modeSwitchMapIndex);
		if (down) {
			if (col & 1)
				shift.press();
			else
				shift.release();
			if (modeSwitchMapIndex) {
				if (col & 2)
					modeSwitch.press();
				else
					modeSwitch.release();
			}
		}
		/*
		 * Ensure ModifierState objects are not destroyed before
		 * pressKey call, otherwise fake modifier keypress can be lost.
		 */
		pressKey(keyboardDev, kc, down, "keycode");
	} else {
press:
		pressKey(keyboardDev, kc, down, "keycode");
	}


        FREE_MAPS;
	
	/*
	 * When faking a modifier we are putting a keycode (which can
	 * currently activate the desired modifier) on the input
	 * queue. A future modmap change can change the mapping so
	 * that this keycode means something else entirely. Guard
	 * against this by processing the queue now.
	 */
	mieqProcessInputEvents();
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
		XkbConvertCase
			    (syms[col&~1], &lsym, &usym);
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

/*
 * KeysymToKeycode() - find the keycode and column corresponding to the given
 * keysym.  The value of col passed in should be the column determined from the
 * current shift state.  If the keysym can be found in that column we prefer
 * that to finding it in a different column (which would require fake events to
 * alter the shift state).
 */
static KeyCode KeysymToKeycode(KeySymsPtr keymap, KeySym ks, int* col)
{
	int i, j;

	j = *col;
	for (i = keymap->minKeyCode; i <= keymap->maxKeyCode; i++) {
		if (KeyCodetoKeySym(keymap, i, j) == ks)
			return i;
	}

	for (j = 0; j < keymap->mapWidth; j++) {
		for (i = keymap->minKeyCode; i <= keymap->maxKeyCode; i++) {
			if (KeyCodetoKeySym(keymap, i, j) == ks) {
				*col = j;
				return i;
			}
		}
	}

	return 0;
}

#if XORG < 17
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

static Bool GetMappings(KeySymsPtr pKeySyms, CARD8 *pModMap)
{
	int i;

	for (i = 0; i < MAP_LENGTH; i++)
		pModMap[i] = NoSymbol;

	for (i = 0; i < MAP_LEN; i++) {
		switch (keyboardMap[i * KEYSYMS_PER_KEY]) {
		case XK_Shift_L:
		case XK_Shift_R:
			pModMap[i + MIN_KEY] = ShiftMask;
			break;
		case XK_Caps_Lock:
			pModMap[i + MIN_KEY] = LockMask;
			break;
		case XK_Control_L:
		case XK_Control_R:
			pModMap[i + MIN_KEY] = ControlMask;
			break;
		case XK_Alt_L:
		case XK_Alt_R:
			pModMap[i + MIN_KEY] = Mod1Mask;
			break;
		case XK_Num_Lock:
			pModMap[i + MIN_KEY] = Mod2Mask;
			break;
			/* No defaults for Mod3Mask yet */
		case XK_Super_L:
		case XK_Super_R:
		case XK_Hyper_L:
		case XK_Hyper_R:
			pModMap[i + MIN_KEY] = Mod4Mask;
			break;
		case XK_ISO_Level3_Shift:
		case XK_Mode_switch:
			pModMap[i + MIN_KEY] = Mod5Mask;
			break;
		}
	}

	pKeySyms->minKeyCode = MIN_KEY;
	pKeySyms->maxKeyCode = MAX_KEY;
	pKeySyms->mapWidth = KEYSYMS_PER_KEY;
	pKeySyms->map = keyboardMap;

	return TRUE;
}
#endif

static void keyboardBell(int percent, DeviceIntPtr device, pointer ctrl,
			 int class_)
{
	if (percent > 0)
		vncBell();
}

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
		GetMappings(&keySyms, modMap);
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
#if 0
	case DEVICE_CLOSE:
		break;
#endif
	}

	return Success;
}

