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

abstract public class Decoder {

  public static class DecoderFlags {
    // A constant for decoders that don't need anything special
    public static int DecoderPlain = 0;
    // All rects for this decoder must be handled in order
    public static int DecoderOrdered = 1 << 0;
    // Only some of the rects must be handled in order,
    // see doesRectsConflict()
    public static int DecoderPartiallyOrdered = 1 << 1;
  };

  public Decoder(int flags)
  {
    this.flags = flags;
  }

  abstract public void readRect(Rect r, InStream is,
                                ServerParams server, OutStream os);

  abstract public void decodeRect(Rect r, Object buffer,
                                  int buflen, ServerParams server,
                                  ModifiablePixelBuffer pb);

  public void getAffectedRegion(Rect rect, Object buffer,
                                int buflen, ServerParams server,
                                Region region)
  {
    region.reset(rect);
  }

  public boolean doRectsConflict(Rect rectA, Object bufferA,
                                 int buflenA, Rect rectB,
                                 Object bufferB, int buflenB,
                                 ServerParams server)
  {
    return false;
  }

  static public boolean supported(int encoding)
  {
    switch(encoding) {
    case Encodings.encodingRaw:
    case Encodings.encodingCopyRect:
    case Encodings.encodingRRE:
    case Encodings.encodingHextile:
    case Encodings.encodingZRLE:
    case Encodings.encodingTight:
      return true;
    default:
      return false;
    }
  }

  static public Decoder createDecoder(int encoding) {
    switch(encoding) {
    case Encodings.encodingRaw:
      return new RawDecoder();
    case Encodings.encodingCopyRect:
      return new CopyRectDecoder();
    case Encodings.encodingRRE:
      return new RREDecoder();
    case Encodings.encodingHextile:
      return new HextileDecoder();
    case Encodings.encodingZRLE:
      return new ZRLEDecoder();
    case Encodings.encodingTight:
      return new TightDecoder();
    default:
      return null;
    }
  }

  public final int flags;
}
