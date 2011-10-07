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

package com.tigervnc.rfb;

import com.tigervnc.rdr.*;
import java.util.*;

public class CMsgWriterV3 extends CMsgWriter {

  public CMsgWriterV3(ConnParams cp_, OutStream os_) { super(cp_, os_); }

  public void writeClientInit(boolean shared) {
    os.writeU8(shared?1:0);
    endMsg();
  }

  public void startMsg(int type) {
    os.writeU8(type);
  }

  public void endMsg() {
    os.flush();
  }

  public void writeSetDesktopSize(int width, int height,
                                  ScreenSet layout)
	{
	  if (!cp.supportsSetDesktopSize)
	    throw new Exception("Server does not support SetDesktopSize");
	
	  startMsg(MsgTypes.msgTypeSetDesktopSize);
	  os.pad(1);
	
	  os.writeU16(width);
	  os.writeU16(height);
	
	  os.writeU8(layout.num_screens());
	  os.pad(1);
	
    for (Iterator iter = layout.screens.iterator(); iter.hasNext(); ) {
      Screen refScreen = (Screen)iter.next();
	    os.writeU32(refScreen.id);
	    os.writeU16(refScreen.dimensions.tl.x);
	    os.writeU16(refScreen.dimensions.tl.y);
	    os.writeU16(refScreen.dimensions.width());
	    os.writeU16(refScreen.dimensions.height());
	    os.writeU32(refScreen.flags);
	  }
	
	  endMsg();
	}
}
