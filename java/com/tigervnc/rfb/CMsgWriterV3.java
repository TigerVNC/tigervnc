/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2011 Pierre Ossman for Cendio AB
 * Copyright (C) 2011 Brian P. Hinz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

package com.tigervnc.rfb;

import com.tigervnc.rdr.*;
import java.util.*;

public class CMsgWriterV3 extends CMsgWriter {

  public CMsgWriterV3(ConnParams cp_, OutStream os_) { super(cp_, os_); }

  synchronized public void writeClientInit(boolean shared) {
    os.writeU8(shared?1:0);
    endMsg();
  }

  synchronized public void startMsg(int type) {
    os.writeU8(type);
  }

  synchronized public void endMsg() {
    os.flush();
  }

  synchronized public void writeSetDesktopSize(int width, int height,
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

  synchronized public void writeFence(int flags, int len, byte[] data)
  {
    if (!cp.supportsFence)
      throw new Exception("Server does not support fences");
    if (len > 64)
      throw new Exception("Too large fence payload");
    if ((flags & ~fenceTypes.fenceFlagsSupported) != 0)
      throw new Exception("Unknown fence flags");
  
    startMsg(MsgTypes.msgTypeClientFence);
    os.pad(3);
  
    os.writeU32(flags);
  
    os.writeU8(len);
    os.writeBytes(data, 0, len);
  
    endMsg();
  }
  
  synchronized public void writeEnableContinuousUpdates(boolean enable,
                                           int x, int y, int w, int h)
  {
    if (!cp.supportsContinuousUpdates)
      throw new Exception("Server does not support continuous updates");
  
    startMsg(MsgTypes.msgTypeEnableContinuousUpdates);
  
    os.writeU8((enable?1:0));
  
    os.writeU16(x);
    os.writeU16(y);
    os.writeU16(w);
    os.writeU16(h);
  
    endMsg();
  }
}
