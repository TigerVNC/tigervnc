/* Copyright 2020 Samuel Mannehed for Cendio AB
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

#ifndef __WIN32TOUCHHANDLER_H__
#define __WIN32TOUCHHANDLER_H__

#include <windows.h>

#include "BaseTouchHandler.h"
#include "GestureEvent.h"

class Win32TouchHandler: public BaseTouchHandler {
  public:
    Win32TouchHandler(HWND hWnd);

    bool processEvent(UINT Msg, WPARAM wParam, LPARAM lParam);

  private:
    void handleWin32GestureEvent(GESTUREINFO gi);
    bool isSinglePan(GESTUREINFO gi);

  protected:
    virtual void fakeMotionEvent(const GestureEvent origEvent);
    virtual void fakeButtonEvent(bool press, int button,
                                 const GestureEvent origEvent);
    virtual void fakeKeyEvent(bool press, int keycode,
                              const GestureEvent origEvent);
  private:
    void pushFakeEvent(UINT Msg, WPARAM wParam, LPARAM lParam);

  private:
    HWND hWnd;

    bool gesturesConfigured;
    bool startedSinglePan;
    POINT gestureStart;

    bool gestureActive;
    bool ignoringGesture;

    int fakeButtonMask;
    POINT lastFakeMotionPos;
};

#endif // __WIN32TOUCHHANDLER_H__
