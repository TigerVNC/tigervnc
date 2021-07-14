/* Copyright (C) 2009 TightVNC Team
 * Copyright (C) 2009 Red Hat, Inc.
 * Copyright 2013-2018 Pierre Ossman for Cendio AB
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

#include <stdio.h>

#include <X11/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "xkbsrv.h"
#include "xkbstr.h"
#include "eventstr.h"
#include "scrnintstr.h"
#include "mi.h"

#include "vncInput.h"

#ifndef KEYBOARD_OR_FLOAT
#define KEYBOARD_OR_FLOAT MASTER_KEYBOARD
#endif

#if XORG_OLDER_THAN(1, 18, 0)
#define GetMaster(dev, type) ((dev)->master)
#endif

extern DeviceIntPtr vncKeyboardDev;

static const KeyCode fakeKeys[] = {
#ifdef __linux__
    92, 203, 204, 205, 206, 207
#else
    8, 124, 125, 156, 127, 128
#endif
    };

static void vncXkbProcessDeviceEvent(int screenNum,
                                     InternalEvent *event,
                                     DeviceIntPtr dev);

/* Stolen from libX11 */
static Bool
XkbTranslateKeyCode(register XkbDescPtr xkb, KeyCode key,
                    register unsigned int mods, unsigned int *mods_rtrn,
                    KeySym *keysym_rtrn)
{
	XkbKeyTypeRec *type;
	int col,nKeyGroups;
	unsigned preserve,effectiveGroup;
	KeySym *syms;

	if (mods_rtrn!=NULL)
		*mods_rtrn = 0;

	nKeyGroups= XkbKeyNumGroups(xkb,key);
	if ((!XkbKeycodeInRange(xkb,key))||(nKeyGroups==0)) {
		if (keysym_rtrn!=NULL)
			*keysym_rtrn = NoSymbol;
		return False;
	}

	syms = XkbKeySymsPtr(xkb,key);

	/* find the offset of the effective group */
	col = 0;
	effectiveGroup= XkbGroupForCoreState(mods);
	if ( effectiveGroup>=nKeyGroups ) {
		unsigned groupInfo= XkbKeyGroupInfo(xkb,key);
		switch (XkbOutOfRangeGroupAction(groupInfo)) {
		default:
			effectiveGroup %= nKeyGroups;
			break;
		case XkbClampIntoRange:
			effectiveGroup = nKeyGroups-1;
			break;
		case XkbRedirectIntoRange:
			effectiveGroup = XkbOutOfRangeGroupNumber(groupInfo);
			if (effectiveGroup>=nKeyGroups)
				effectiveGroup= 0;
			break;
		}
	}
	col= effectiveGroup*XkbKeyGroupsWidth(xkb,key);
	type = XkbKeyKeyType(xkb,key,effectiveGroup);

	preserve= 0;
	if (type->map) { /* find the column (shift level) within the group */
		register int i;
		register XkbKTMapEntryPtr entry;
		for (i=0,entry=type->map;i<type->map_count;i++,entry++) {
			if ((entry->active)&&((mods&type->mods.mask)==entry->mods.mask)) {
				col+= entry->level;
				if (type->preserve)
					preserve= type->preserve[i].mask;
				break;
			}
		}
	}

	if (keysym_rtrn!=NULL)
		*keysym_rtrn= syms[col];
	if (mods_rtrn)
		*mods_rtrn= type->mods.mask&(~preserve);

	return (syms[col]!=NoSymbol);
}

static XkbAction *XkbKeyActionPtr(XkbDescPtr xkb, KeyCode key, unsigned int mods)
{
	XkbKeyTypeRec *type;
	int col,nKeyGroups;
	unsigned effectiveGroup;
	XkbAction *acts;

	if (!XkbKeyHasActions(xkb, key))
		return NULL;

	nKeyGroups= XkbKeyNumGroups(xkb,key);
	if ((!XkbKeycodeInRange(xkb,key))||(nKeyGroups==0))
		return NULL;

	acts = XkbKeyActionsPtr(xkb,key);

	/* find the offset of the effective group */
	col = 0;
	effectiveGroup= XkbGroupForCoreState(mods);
	if ( effectiveGroup>=nKeyGroups ) {
		unsigned groupInfo= XkbKeyGroupInfo(xkb,key);
		switch (XkbOutOfRangeGroupAction(groupInfo)) {
		default:
			effectiveGroup %= nKeyGroups;
			break;
		case XkbClampIntoRange:
			effectiveGroup = nKeyGroups-1;
			break;
		case XkbRedirectIntoRange:
			effectiveGroup = XkbOutOfRangeGroupNumber(groupInfo);
			if (effectiveGroup>=nKeyGroups)
				effectiveGroup= 0;
			break;
		}
	}
	col= effectiveGroup*XkbKeyGroupsWidth(xkb,key);
	type = XkbKeyKeyType(xkb,key,effectiveGroup);

	if (type->map) { /* find the column (shift level) within the group */
		register int i;
		register XkbKTMapEntryPtr entry;
		for (i=0,entry=type->map;i<type->map_count;i++,entry++) {
			if ((entry->active)&&((mods&type->mods.mask)==entry->mods.mask)) {
				col+= entry->level;
				break;
			}
		}
	}

	return &acts[col];
}

static unsigned XkbKeyEffectiveGroup(XkbDescPtr xkb, KeyCode key, unsigned int mods)
{
	int nKeyGroups;
	unsigned effectiveGroup;

	nKeyGroups= XkbKeyNumGroups(xkb,key);
	if ((!XkbKeycodeInRange(xkb,key))||(nKeyGroups==0))
		return 0;

	effectiveGroup= XkbGroupForCoreState(mods);
	if ( effectiveGroup>=nKeyGroups ) {
		unsigned groupInfo= XkbKeyGroupInfo(xkb,key);
		switch (XkbOutOfRangeGroupAction(groupInfo)) {
		default:
			effectiveGroup %= nKeyGroups;
			break;
		case XkbClampIntoRange:
			effectiveGroup = nKeyGroups-1;
			break;
		case XkbRedirectIntoRange:
			effectiveGroup = XkbOutOfRangeGroupNumber(groupInfo);
			if (effectiveGroup>=nKeyGroups)
				effectiveGroup= 0;
			break;
		}
	}

	return effectiveGroup;
}

void vncPrepareInputDevices(void)
{
	/*
	 * Not ideal since these callbacks do not stack, but it's the only
	 * decent way we can reliably catch events for both the slave and
	 * master device.
	 */
	mieqSetHandler(ET_KeyPress, vncXkbProcessDeviceEvent);
	mieqSetHandler(ET_KeyRelease, vncXkbProcessDeviceEvent);
}

unsigned vncGetKeyboardState(void)
{
	DeviceIntPtr master;

	master = GetMaster(vncKeyboardDev, KEYBOARD_OR_FLOAT);
	return XkbStateFieldFromRec(&master->key->xkbInfo->state);
}

unsigned vncGetLevelThreeMask(void)
{
	unsigned state;
	KeyCode keycode;
	XkbDescPtr xkb;
	XkbAction *act;

	/* Group state is still important */
	state = vncGetKeyboardState();
	state &= ~0xff;

	keycode = vncKeysymToKeycode(XK_ISO_Level3_Shift, state, NULL);
	if (keycode == 0) {
		keycode = vncKeysymToKeycode(XK_Mode_switch, state, NULL);
		if (keycode == 0)
			return 0;
	}

	xkb = GetMaster(vncKeyboardDev, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;

	act = XkbKeyActionPtr(xkb, keycode, state);
	if (act == NULL)
		return 0;
	if (act->type != XkbSA_SetMods)
		return 0;

	if (act->mods.flags & XkbSA_UseModMapMods)
		return xkb->map->modmap[keycode];
	else
		return act->mods.mask;
}

KeyCode vncPressShift(void)
{
	unsigned state;

	XkbDescPtr xkb;
	unsigned int key;

	state = vncGetKeyboardState();
	if (state & ShiftMask)
		return 0;

	xkb = GetMaster(vncKeyboardDev, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		XkbAction *act;
		unsigned char mask;

		act = XkbKeyActionPtr(xkb, key, state);
		if (act == NULL)
			continue;

		if (act->type != XkbSA_SetMods)
			continue;

		if (act->mods.flags & XkbSA_UseModMapMods)
			mask = xkb->map->modmap[key];
		else
			mask = act->mods.mask;

		if ((mask & ShiftMask) == ShiftMask)
			return key;
	}

	return 0;
}

size_t vncReleaseShift(KeyCode *keys, size_t maxKeys)
{
	size_t count;

	unsigned state;

	DeviceIntPtr master;
	XkbDescPtr xkb;
	unsigned int key;

	state = vncGetKeyboardState();
	if (!(state & ShiftMask))
		return 0;

	count = 0;

	master = GetMaster(vncKeyboardDev, KEYBOARD_OR_FLOAT);
	xkb = master->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		XkbAction *act;
		unsigned char mask;

		if (!key_is_down(master, key, KEY_PROCESSED))
			continue;

		act = XkbKeyActionPtr(xkb, key, state);
		if (act == NULL)
			continue;

		if (act->type != XkbSA_SetMods)
			continue;

		if (act->mods.flags & XkbSA_UseModMapMods)
			mask = xkb->map->modmap[key];
		else
			mask = act->mods.mask;

		if (!(mask & ShiftMask))
			continue;

		if (count >= maxKeys)
			return 0;

		keys[count++] = key;
	}

	return count;
}

KeyCode vncPressLevelThree(void)
{
	unsigned state, mask;

	KeyCode keycode;
	XkbDescPtr xkb;
	XkbAction *act;

	mask = vncGetLevelThreeMask();
	if (mask == 0)
		return 0;

	state = vncGetKeyboardState();
	if (state & mask)
		return 0;

	keycode = vncKeysymToKeycode(XK_ISO_Level3_Shift, state, NULL);
	if (keycode == 0) {
		keycode = vncKeysymToKeycode(XK_Mode_switch, state, NULL);
		if (keycode == 0)
			return 0;
	}

	xkb = GetMaster(vncKeyboardDev, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;

	act = XkbKeyActionPtr(xkb, keycode, state);
	if (act == NULL)
		return 0;
	if (act->type != XkbSA_SetMods)
		return 0;

	return keycode;
}

size_t vncReleaseLevelThree(KeyCode *keys, size_t maxKeys)
{
	size_t count;

	unsigned state, mask;

	DeviceIntPtr master;
	XkbDescPtr xkb;
	unsigned int key;

	mask = vncGetLevelThreeMask();
	if (mask == 0)
		return 0;

	state = vncGetKeyboardState();
	if (!(state & mask))
		return 0;

	count = 0;

	master = GetMaster(vncKeyboardDev, KEYBOARD_OR_FLOAT);
	xkb = master->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		XkbAction *act;
		unsigned char key_mask;

		if (!key_is_down(master, key, KEY_PROCESSED))
			continue;

		act = XkbKeyActionPtr(xkb, key, state);
		if (act == NULL)
			continue;

		if (act->type != XkbSA_SetMods)
			continue;

		if (act->mods.flags & XkbSA_UseModMapMods)
			key_mask = xkb->map->modmap[key];
		else
			key_mask = act->mods.mask;

		if (!(key_mask & mask))
			continue;

		if (count >= maxKeys)
			return 0;

		keys[count++] = key;
	}

	return count;
}

KeyCode vncKeysymToKeycode(KeySym keysym, unsigned state, unsigned *new_state)
{
	XkbDescPtr xkb;
	unsigned int key; // KeyCode has insufficient range for the loop
	KeyCode fallback;
	KeySym ks;
	unsigned level_three_mask;

	if (new_state != NULL)
		*new_state = state;

	fallback = 0;
	xkb = GetMaster(vncKeyboardDev, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;
	for (key = xkb->min_key_code; key <= xkb->max_key_code; key++) {
		unsigned int state_out;
		KeySym dummy;
		size_t fakeIdx;

		XkbTranslateKeyCode(xkb, key, state, &state_out, &ks);
		if (ks == NoSymbol)
			continue;

		/*
		 * Despite every known piece of documentation on
		 * XkbTranslateKeyCode() stating that mods_rtrn returns
		 * the unconsumed modifiers, in reality it always
		 * returns the _potentially consumed_ modifiers.
		 */
		state_out = state & ~state_out;
		if (state_out & LockMask)
			XkbConvertCase(ks, &dummy, &ks);

		if (ks != keysym)
			continue;

		/*
		 * Some keys are never sent by a real keyboard and are
		 * used in the default layouts as a fallback for
		 * modifiers. Make sure we use them last as some
		 * applications can be confused by these normally
		 * unused keys.
		 */
		for (fakeIdx = 0;
		     fakeIdx < sizeof(fakeKeys)/sizeof(fakeKeys[0]);
		     fakeIdx++) {
			if (key == fakeKeys[fakeIdx]) {
				if (fallback == 0)
					fallback = key;
				break;
			}
		}
		if (fakeIdx < sizeof(fakeKeys)/sizeof(fakeKeys[0]))
			continue;

		return key;
	}

	/* Use the fallback key, if one was found */
	if (fallback != 0)
		return fallback;

	if (new_state == NULL)
		return 0;

	*new_state = (state & ~ShiftMask) |
	             ((state & ShiftMask) ? 0 : ShiftMask);
	key = vncKeysymToKeycode(keysym, *new_state, NULL);
	if (key != 0)
		return key;

	level_three_mask = vncGetLevelThreeMask();
	if (level_three_mask == 0)
		return 0;

	*new_state = (state & ~level_three_mask) | 
	             ((state & level_three_mask) ? 0 : level_three_mask);
	key = vncKeysymToKeycode(keysym, *new_state, NULL);
	if (key != 0)
		return key;

	*new_state = (state & ~(ShiftMask | level_three_mask)) | 
	             ((state & ShiftMask) ? 0 : ShiftMask) |
	             ((state & level_three_mask) ? 0 : level_three_mask);
	key = vncKeysymToKeycode(keysym, *new_state, NULL);
	if (key != 0)
		return key;

	return 0;
}

int vncIsAffectedByNumLock(KeyCode keycode)
{
	unsigned state;

	KeyCode numlock_keycode;
	unsigned numlock_mask;

	XkbDescPtr xkb;
	XkbAction *act;

	unsigned group;
	XkbKeyTypeRec *type;

	/* Group state is still important */
	state = vncGetKeyboardState();
	state &= ~0xff;

	/*
	 * Not sure if hunting for a virtual modifier called "NumLock",
	 * or following the keysym Num_Lock is the best approach. We
	 * try the latter.
	 */
	numlock_keycode = vncKeysymToKeycode(XK_Num_Lock, state, NULL);
	if (numlock_keycode == 0)
		return 0;

	xkb = GetMaster(vncKeyboardDev, KEYBOARD_OR_FLOAT)->key->xkbInfo->desc;

	act = XkbKeyActionPtr(xkb, numlock_keycode, state);
	if (act == NULL)
		return 0;
	if (act->type != XkbSA_LockMods)
		return 0;

	if (act->mods.flags & XkbSA_UseModMapMods)
		numlock_mask = xkb->map->modmap[keycode];
	else
		numlock_mask = act->mods.mask;

	group = XkbKeyEffectiveGroup(xkb, keycode, state);
	type = XkbKeyKeyType(xkb, keycode, group);
	if ((type->mods.mask & numlock_mask) == 0)
		return 0;

	return 1;
}

KeyCode vncAddKeysym(KeySym keysym, unsigned state)
{
	DeviceIntPtr master;
	XkbDescPtr xkb;
	unsigned int key;

	XkbEventCauseRec cause;
	XkbChangesRec changes;

	int types[1];
	KeySym *syms;
	KeySym upper, lower;

	master = GetMaster(vncKeyboardDev, KEYBOARD_OR_FLOAT);
	xkb = master->key->xkbInfo->desc;
	for (key = xkb->max_key_code; key >= xkb->min_key_code; key--) {
		if (XkbKeyNumGroups(xkb, key) == 0)
			break;
	}

	if (key < xkb->min_key_code)
		return 0;

	memset(&changes, 0, sizeof(changes));
	memset(&cause, 0, sizeof(cause));

	XkbSetCauseUnknown(&cause);

	/*
	 * Tools like xkbcomp get confused if there isn't a name
	 * assigned to the keycode we're trying to use.
	 */
	if (xkb->names && xkb->names->keys &&
	    (xkb->names->keys[key].name[0] == '\0')) {
		xkb->names->keys[key].name[0] = 'I';
		xkb->names->keys[key].name[1] = '0' + (key / 100) % 10;
		xkb->names->keys[key].name[2] = '0' + (key /  10) % 10;
		xkb->names->keys[key].name[3] = '0' + (key /   1) % 10;

		changes.names.changed |= XkbKeyNamesMask;
		changes.names.first_key = key;
		changes.names.num_keys = 1;
	}

	/* FIXME: Verify that ONE_LEVEL/ALPHABETIC isn't screwed up */

	/*
	 * For keysyms that are affected by Lock, we are better off
	 * using ALPHABETIC rather than ONE_LEVEL as the latter
	 * generally cannot produce lower case when Lock is active.
	 */
	XkbConvertCase(keysym, &lower, &upper);
	if (upper == lower)
		types[XkbGroup1Index] = XkbOneLevelIndex;
	else
		types[XkbGroup1Index] = XkbAlphabeticIndex;

	XkbChangeTypesOfKey(xkb, key, 1, XkbGroup1Mask, types, &changes.map);

	syms = XkbKeySymsPtr(xkb,key);
	if (upper == lower)
		syms[0] = keysym;
	else {
		syms[0] = lower;
		syms[1] = upper;
	}

	changes.map.changed |= XkbKeySymsMask;
	changes.map.first_key_sym = key;
	changes.map.num_key_syms = 1;

	XkbSendNotification(master, &changes, &cause);

	return key;
}

static void vncXkbProcessDeviceEvent(int screenNum,
                                     InternalEvent *event,
                                     DeviceIntPtr dev)
{
	unsigned int backupctrls;
	XkbControlsPtr ctrls;

	if (event->device_event.sourceid != vncKeyboardDev->id) {
		dev->public.processInputProc(event, dev);
		return;
	}

	/*
	 * We need to bypass AccessX since it is timing sensitive and
	 * the network can cause fake event delays.
	 */
	ctrls = dev->key->xkbInfo->desc->ctrls;
	backupctrls = ctrls->enabled_ctrls;
	ctrls->enabled_ctrls &= ~XkbAllFilteredEventsMask;

	/*
	 * This flag needs to be set for key repeats to be properly
	 * respected.
	 */
	if ((event->device_event.type == ET_KeyPress) &&
	    key_is_down(dev, event->device_event.detail.key, KEY_PROCESSED))
		event->device_event.key_repeat = TRUE;

	dev->public.processInputProc(event, dev);

	ctrls->enabled_ctrls = backupctrls;
}
