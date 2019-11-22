/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2019 Brian P. Hinz
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

public class RawDecoder extends Decoder {

  public RawDecoder() { super(DecoderFlags.DecoderPlain); }

  public void readRect(Rect r, InStream is,
                       ServerParams server, OutStream os)
  {
    os.copyBytes(is, r.area() * server.pf().bpp/8);
  }

  static LogWriter vlog = new LogWriter("RawDecoder");
  public void decodeRect(Rect r, Object buffer,
                         int buflen, ServerParams server,
                         ModifiablePixelBuffer pb)
  {
    assert(buflen >= r.area() * server.pf().bpp/8);
    pb.imageRect(server.pf(), r, (byte[])buffer);
  }

}
