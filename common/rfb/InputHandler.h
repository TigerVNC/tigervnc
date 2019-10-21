/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
//
// InputHandler - abstract interface for accepting keyboard &
// pointer input and clipboard data.
//

#ifndef __RFB_INPUTHANDLER_H__
#define __RFB_INPUTHANDLER_H__

#include <rdr/types.h>
#include <rfb/Rect.h>
#include <rfb/util.h>

namespace rfb {

  class InputHandler {
  public:
    virtual ~InputHandler() {}
    virtual void keyEvent(rdr::U32 __unused_attr keysym,
                          rdr::U32 __unused_attr keycode,
                          bool __unused_attr down) { }
    virtual void pointerEvent(const Point& __unused_attr pos,
                              int __unused_attr buttonMask) { }
    virtual void clientCutText(const char* __unused_attr str) { }
  };

}
#endif
