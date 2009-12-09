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
extern void
CopyKeyClass(DeviceIntPtr device, DeviceIntPtr master);
#endif
#include <X11/keysym.h>
#include <X11/Xutil.h>
#undef public
#undef class
}

using namespace rdr;
using namespace rfb;

static LogWriter vlog("Input");

#define BUTTONS 5
static int pointerProc(DeviceIntPtr pDevice, int onoff);

static int keyboardProc(DeviceIntPtr pDevice, int onoff);
static KeySym KeyCodetoKeySym(KeySymsPtr keymap, int keycode, int col);
static KeyCode KeysymToKeycode(KeySymsPtr keymap, KeySym ks, int* col);

/* Event queue is shared between all devices. */
#if XORG == 15
static xEvent *eventq = NULL;
#else
static EventList *eventq = NULL;
#endif

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
#else
			    (eventq + i)->event
#endif
			   );
	}
}

/* Pointer device methods */

PointerDevice::PointerDevice(rfb::VNCServerST *_server)
	: server(_server), oldButtonMask(0)
{
	dev = AddInputDevice(
#if XORG >= 16
			     serverClient,
#endif
			     pointerProc, TRUE);
	RegisterPointerDevice(dev);
	initEventq();
}

void PointerDevice::ButtonAction(int buttonMask)
{
	int i, n;

	for (i = 0; i < BUTTONS; i++) {
		if ((buttonMask ^ oldButtonMask) & (1 << i)) {
			int action = (buttonMask & (1<<i)) ?
				     ButtonPress : ButtonRelease;
			n = GetPointerEvents(eventq, dev, action, i + 1,
					     POINTER_RELATIVE, 0, 0, NULL);
			enqueueEvents(dev, n);

		}
	}

	oldButtonMask = buttonMask;
}

void PointerDevice::Move(const rfb::Point &pos)
{
	int n, valuators[2];

	if (pos.equals(cursorPos))
		return;

	valuators[0] = pos.x;
	valuators[1] = pos.y;
	n = GetPointerEvents(eventq, dev, MotionNotify, 0, POINTER_ABSOLUTE, 0,
			     2, valuators);
	enqueueEvents(dev, n);

	cursorPos = pos;
}

void PointerDevice::Sync(void)
{
	if (cursorPos.equals(oldCursorPos))
		return;

	oldCursorPos = cursorPos;
	server->setCursorPos(cursorPos);
	server->tryUpdate();
}

static int pointerProc(DeviceIntPtr pDevice, int onoff)
{
	BYTE map[BUTTONS + 1];
	DevicePtr pDev = (DevicePtr)pDevice;
	int i;

	switch (onoff) {
	case DEVICE_INIT:
		for (i = 0; i < BUTTONS + 1; i++)
			map[i] = i;

		InitPointerDeviceStruct(pDev, map, BUTTONS,
#if XORG == 15
					GetMotionHistory,
#endif
					(PtrCtrlProcPtr)NoopDDA,
					GetMotionHistorySize(), 2);
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

/* KeyboardDevice methods */

KeyboardDevice::KeyboardDevice(void)
{
	dev = AddInputDevice(
#if XORG >= 16
			     serverClient,
#endif
			     keyboardProc, TRUE);
	RegisterKeyboardDevice(dev);
	initEventq();
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
			generateXKeyEvent(keys[i], !pressed);
		delete [] keys;
	}

	void press()
	{
		KeyClassPtr keyc = dev->key;
		if (!(keyc->state & (1 << modIndex))) {
			int index = modIndex * keyc->maxKeysPerModifier;
			tempKeyEvent(keyc->modifierKeyMap[index], true);
			pressed = true;
		}
	}

	void release()
	{
		KeyClassPtr keyc = dev->key;
		if ((keyc->state & (1 << modIndex)) == 0)
			return;

		for (int k = 0; k < keyc->maxKeysPerModifier; k++) {
			int index = modIndex * keyc->maxKeysPerModifier + k;
			int keycode = keyc->modifierKeyMap[index];
			if (keycode && IS_PRESSED(keyc, keycode))
				tempKeyEvent(keycode, false);
		}
	}

private:
	void tempKeyEvent(int keycode, bool down)
	{
		if (keycode) {
			if (!keys) keys = new int[dev->key->maxKeysPerModifier];
			keys[nKeys++] = keycode;
			generateXKeyEvent(keycode, down);
		}
	}

	void generateXKeyEvent(int keycode, bool down)
	{
		int n, action;

		action = down ? KeyPress : KeyRelease;
		n = GetKeyboardEvents(eventq, dev, action, keycode);
		enqueueEvents(dev, n);

		vlog.debug("fake keycode %d %s", keycode,
			   down ? "down" : "up");
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

void KeyboardDevice::keyEvent(rdr::U32 keysym, bool down)
{
	DeviceIntPtr master;
	KeyClassPtr keyc = dev->key;
	KeySymsPtr keymap = &keyc->curKeySyms;
	KeySym *map = keymap->map;
	KeyCode minKeyCode = keymap->minKeyCode;
	KeyCode maxKeyCode = keymap->maxKeyCode;
	int mapWidth = keymap->mapWidth;
	unsigned int i, n;
	int j, k, action;

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

	/* find which modifier Mode_switch is on. */
	int modeSwitchMapIndex = 0;
	for (i = 3; i < 8; i++) {
		for (k = 0; k < keyc->maxKeysPerModifier; k++) {
			int index = i * keyc->maxKeysPerModifier + k;
			int keycode = keyc->modifierKeyMap[index];

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

	int col = 0;
	if ((keyc->state & (1 << ShiftMapIndex)) != 0)
		col |= 1;
	if (modeSwitchMapIndex != 0 &&
	    ((keyc->state & (1 << modeSwitchMapIndex))) != 0)
		col |= 2;

	int kc = KeysymToKeycode(keymap, keysym, &col);

	/*
	 * Sort out the "shifted Tab" mess.  If we are sent a shifted Tab,
	 * generate a local shifted Tab regardless of what the "shifted Tab"
	 * keysym is on the local keyboard (it might be Tab, ISO_Left_Tab or
	 * HP's private BackTab keysym, and quite possibly some others too).
	 * We never get ISO_Left_Tab here because it's already been translated
	 * in VNCSConnectionST.
	 */
	if (keysym == XK_Tab && ((keyc->state & (1 << ShiftMapIndex))) != 0)
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
#if XORG == 15
			master = inputInfo.keyboard;
#else
			master = dev->u.master;
#endif
			void *slave = dixLookupPrivate(&master->devPrivates,
						       CoreDevicePrivateKey);
			if (dev == slave) {
				dixSetPrivate(&master->devPrivates,
					      CoreDevicePrivateKey, NULL);
#if XORG == 15
				SwitchCoreKeyboard(dev);
#else
				CopyKeyClass(dev, master);
#endif
			}
			break;
		}
	}

	if (kc < minKeyCode) {
		vlog.info("Keyboard mapping full - ignoring unknown keysym "
			  "0x%x",keysym);
		return;
	}

	/*
	 * See if it's a modifier key.  If so, then don't do any auto-repeat,
	 * because the X server will translate each press into a release
	 * followed by a press.
	 */
	for (i = 0; i < 8; i++) {
		for (k = 0; k < keyc->maxKeysPerModifier; k++) {
			int index = i * keyc->maxKeysPerModifier + k;
			if (kc == keyc->modifierKeyMap[index] &&
			    IS_PRESSED(keyc,kc) && down)
				return;
		}
	}

	ModifierState shift(dev, ShiftMapIndex);
	ModifierState modeSwitch(dev, modeSwitchMapIndex);
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

	vlog.debug("keycode %d %s", kc, down ? "down" : "up");
	action = down ? KeyPress : KeyRelease;
	n = GetKeyboardEvents(eventq, dev, action, kc);
	enqueueEvents(dev, n);
	
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
		XConvertCase(syms[col&~1], &lsym, &usym);
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
	/* XK_Caps_Lock */ NoSymbol, NoSymbol,
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
};

static Bool GetMappings(KeySymsPtr pKeySyms, CARD8 *pModMap)
{
	int i;

	for (i = 0; i < MAP_LENGTH; i++)
		pModMap[i] = NoSymbol;

	for (i = 0; i < MAP_LEN; i++) {
		if (keyboardMap[i * KEYSYMS_PER_KEY] == XK_Caps_Lock)
			pModMap[i + MIN_KEY] = LockMask;
		else if (keyboardMap[i * KEYSYMS_PER_KEY] == XK_Shift_L ||
			 keyboardMap[i * KEYSYMS_PER_KEY] == XK_Shift_R)
			pModMap[i + MIN_KEY] = ShiftMask;
		else if (keyboardMap[i * KEYSYMS_PER_KEY] == XK_Control_L ||
			 keyboardMap[i * KEYSYMS_PER_KEY] == XK_Control_R)
			pModMap[i + MIN_KEY] = ControlMask;
		else if (keyboardMap[i * KEYSYMS_PER_KEY] == XK_Alt_L ||
			 keyboardMap[i * KEYSYMS_PER_KEY] == XK_Alt_R)
			pModMap[i + MIN_KEY] = Mod1Mask;
	}

	pKeySyms->minKeyCode = MIN_KEY;
	pKeySyms->maxKeyCode = MAX_KEY;
	pKeySyms->mapWidth = KEYSYMS_PER_KEY;
	pKeySyms->map = keyboardMap;

	return TRUE;
}

static void keyboardBell(int percent, DeviceIntPtr device, pointer ctrl,
			 int class_)
{
	if (percent > 0)
		vncBell();
}

static int keyboardProc(DeviceIntPtr pDevice, int onoff)
{
	KeySymsRec keySyms;
	CARD8 modMap[MAP_LENGTH];
	DevicePtr pDev = (DevicePtr)pDevice;

	switch (onoff) {
	case DEVICE_INIT:
		GetMappings(&keySyms, modMap);
		InitKeyboardDeviceStruct(pDev, &keySyms, modMap, keyboardBell,
					 (KbdCtrlProcPtr)NoopDDA);
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

