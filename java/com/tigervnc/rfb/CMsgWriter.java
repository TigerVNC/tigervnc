/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2009-2011 Pierre Ossman for Cendio AB
 * Copyright (C) 2011-2019 Brian P. Hinz
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
import java.nio.ByteBuffer;
import java.nio.charset.Charset;
import java.util.*;

public class CMsgWriter {

  protected CMsgWriter(ServerParams server_, OutStream os_)
  {
    server = server_;
    os = os_;
  }

  synchronized public void writeClientInit(boolean shared) {
    os.writeU8(shared?1:0);
    endMsg();
  }

  synchronized public void writeSetPixelFormat(PixelFormat pf)
  {
    startMsg(MsgTypes.msgTypeSetPixelFormat);
    os.pad(3);
    pf.write(os);
    endMsg();
  }

  synchronized public void writeSetEncodings(List<Integer> encodings)
  {
    startMsg(MsgTypes.msgTypeSetEncodings);
    os.skip(1);
    os.writeU16(encodings.size());
    for (Iterator<Integer> i = encodings.iterator(); i.hasNext();)
      os.writeU32(i.next());
    endMsg();
  }

  synchronized public void writeSetDesktopSize(int width, int height,
                                               ScreenSet layout)
	{
	  if (!server.supportsSetDesktopSize)
	    throw new Exception("Server does not support SetDesktopSize");

	  startMsg(MsgTypes.msgTypeSetDesktopSize);
	  os.pad(1);

	  os.writeU16(width);
	  os.writeU16(height);

	  os.writeU8(layout.num_screens());
	  os.pad(1);

    for (Iterator<Screen> iter = layout.screens.iterator(); iter.hasNext(); ) {
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

  synchronized public void writeFramebufferUpdateRequest(Rect r, boolean incremental)
  {
    startMsg(MsgTypes.msgTypeFramebufferUpdateRequest);
    os.writeU8(incremental?1:0);
    os.writeU16(r.tl.x);
    os.writeU16(r.tl.y);
    os.writeU16(r.width());
    os.writeU16(r.height());
    endMsg();
  }

  synchronized public void writeEnableContinuousUpdates(boolean enable,
                                           int x, int y, int w, int h)
  {
    if (!server.supportsContinuousUpdates)
      throw new Exception("Server does not support continuous updates");

    startMsg(MsgTypes.msgTypeEnableContinuousUpdates);

    os.writeU8((enable?1:0));

    os.writeU16(x);
    os.writeU16(y);
    os.writeU16(w);
    os.writeU16(h);

    endMsg();
  }

  synchronized public void writeFence(int flags, int len, byte[] data)
  {
    if (!server.supportsFence)
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

  synchronized public void writeKeyEvent(int keysym, boolean down)
  {
    startMsg(MsgTypes.msgTypeKeyEvent);
    os.writeU8(down?1:0);
    os.pad(2);
    os.writeU32(keysym);
    endMsg();
  }

  synchronized public void writePointerEvent(Point pos, int buttonMask)
  {
    Point p = new Point(pos.x,pos.y);
    if (p.x < 0) p.x = 0;
    if (p.y < 0) p.y = 0;
    if (p.x >= server.width()) p.x = server.width() - 1;
    if (p.y >= server.height()) p.y = server.height() - 1;

    startMsg(MsgTypes.msgTypePointerEvent);
    os.writeU8(buttonMask);
    os.writeU16(p.x);
    os.writeU16(p.y);
    endMsg();
  }

  synchronized public void writeClientCutText(String str, int len)
  {
    startMsg(MsgTypes.msgTypeClientCutText);
    os.pad(3);
    os.writeU32(len);
    Charset latin1 = Charset.forName("ISO-8859-1");
    ByteBuffer bytes = latin1.encode(str);
    os.writeBytes(bytes.array(), 0, len);
    endMsg();
  }

  synchronized protected void startMsg(int type) {
    os.writeU8(type);
  }

  synchronized protected void endMsg() {
    os.flush();
  }

  protected ServerParams server;
  protected OutStream os;
}
