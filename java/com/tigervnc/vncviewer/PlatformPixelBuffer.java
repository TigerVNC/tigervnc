/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2012 Brian P.Hinz
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

package com.tigervnc.vncviewer;

import java.awt.*;
import java.awt.image.*;
import java.nio.ByteOrder;

import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Point;

public class PlatformPixelBuffer extends FullFramePixelBuffer
{
  public PlatformPixelBuffer(PixelFormat pf,
                             int w, int h,
                             WritableRaster data)
  {
    super(pf, w, h, data);
    damage = new Rect(0, 0, w, h);
  }

  public void commitBufferRW(Rect r)
  {
    super.commitBufferRW(r);
    synchronized(damage) {
      Rect n = damage.union_boundary(r);
      damage.setXYWH(n.tl.x, n.tl.y, n.width(), n.height());
    }
  }

  public Rect getDamage() {
    Rect r = new Rect();

    synchronized(damage) {
      r.setXYWH(damage.tl.x, damage.tl.y, damage.width(), damage.height());
      damage.clear();
    }

    return r;
  }

  protected Rect damage;

}
