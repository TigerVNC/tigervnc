/* Copyright 2019 Aaron Sowry for Cendio AB
 * Copyright 2019-2020 Pierre Ossman for Cendio AB
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

#ifndef __XINPUTTOUCHHANDLER_H__
#define __XINPUTTOUCHHANDLER_H__

class XInputTouchHandler {
  public:
    XInputTouchHandler(Window wnd);

    bool grabPointer();
    void ungrabPointer();

    void processEvent(const XIDeviceEvent* devev);

  protected:
    void preparePointerEvent(XEvent* dst, const XIDeviceEvent* src);
    void fakeMotionEvent(const XIDeviceEvent* origEvent);
    void fakeButtonEvent(bool press, int button,
                         const XIDeviceEvent* origEvent);

  private:
    void pushFakeEvent(XEvent* event);

  private:
    Window wnd;
    int fakeStateMask;
    bool trackingTouch;
    int trackedTouchPoint;
};

#endif
