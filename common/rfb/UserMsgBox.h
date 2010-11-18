/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2010 TigerVNC Team
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
#ifndef __RFB_USERMSGBOX_H__
#define __RFB_USERMSGBOX_H__

namespace rfb {
  class UserMsgBox {
  public:
    enum MsgBoxFlags{
      M_OK = 0,
      M_OKCANCEL = 1,
      M_YESNO = 4,
      M_ICONERROR = 0x10,
      M_ICONQUESTION = 0x20,
      M_ICONWARNING = 0x30,
      M_ICONINFORMATION = 0x40,
      M_DEFBUTTON1 = 0,
      M_DEFBUTTON2 = 0x100
    };
    /* TODO Implement as function with variable arguments */
    virtual bool showMsgBox(int flags,const char* title, const char* text)=0;
  };
}

#endif
