/* Copyright 2014 Pierre Ossman <ossman@cendio.se> for Cendio AB
 * Copyright 2016 Brian P. Hinz
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

public class CopyRectDecoder extends Decoder {

  public CopyRectDecoder() { super(DecoderFlags.DecoderPlain); }

  public void readRect(Rect r, InStream is,
                       ConnParams cp, OutStream os)
  {
    os.copyBytes(is, 4);
  }

  public void decodeRect(Rect r, Object buffer,
                         int buflen, ConnParams cp,
                         ModifiablePixelBuffer pb)
  {
    MemInStream is = new MemInStream((byte[])buffer, 0, buflen);
    int srcX = is.readU16();
    int srcY = is.readU16();
    pb.copyRect(r, new Point(r.tl.x-srcX, r.tl.y-srcY));
  }

}
