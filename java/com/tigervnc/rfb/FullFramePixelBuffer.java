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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

package com.tigervnc.rfb;

import java.awt.image.*;

public class FullFramePixelBuffer extends ModifiablePixelBuffer {

  public FullFramePixelBuffer(PixelFormat pf, int w, int h,
                              WritableRaster data_) {
    super(pf, w, h);
    data = data_;
  }

  protected FullFramePixelBuffer() {}

  public WritableRaster getBufferRW(Rect r)
  {
    synchronized(image) {
      return data.createWritableChild(r.tl.x, r.tl.y,
                                      r.width(), r.height(),
                                      0, 0, null);
    }
  }

  public void commitBufferRW(Rect r)
  {
  }

  public Raster getBuffer(Rect r)
  {
    synchronized(image) {
      Raster src =
        data.createChild(r.tl.x, r.tl.y, r.width(), r.height(), 0, 0, null);
      WritableRaster dst =
        data.createCompatibleWritableRaster(r.width(), r.height());
      dst.setDataElements(0, 0, src);
      return dst;
    }
  }

  protected WritableRaster data;
}
