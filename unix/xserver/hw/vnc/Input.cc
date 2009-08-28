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

#include "Input.h"
#include "xorg-version.h"

extern "C" {
#include "mi.h"
}

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

/* Pointer device pre-declarations */
#define BUTTONS 5
static int pointerProc(DeviceIntPtr pDevice, int onoff);

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

