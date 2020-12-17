/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2016-2019 Brian P. Hinz
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

public class ManagedPixelBuffer extends FullFramePixelBuffer {

  public ManagedPixelBuffer() {
    datasize = 0;
    checkDataSize();
  }

  public ManagedPixelBuffer(PixelFormat pf, int w, int h)
  {
    super(pf, w, h, null);
    datasize = 0;
    checkDataSize();
  }

  public void setPF(PixelFormat pf) {
    format = pf; checkDataSize();
  }

  public void setSize(int w, int h) {
    width_ = w; height_ = h; checkDataSize();
  }

  final void checkDataSize() {
    int new_datasize = width_ * height_;
    if (datasize < new_datasize) {
      if (data != null) {
        datasize = 0; data = null;
      }
      if (new_datasize > 0) {
        ColorModel cm = format.getColorModel();
        data = cm.createCompatibleWritableRaster(width_, height_);
        image = new BufferedImage(cm, data, cm.isAlphaPremultiplied(), null);
        datasize = new_datasize;
      }
    }
  }

  protected int datasize;
  static LogWriter vlog = new LogWriter("ManagedPixelBuffer");
}
