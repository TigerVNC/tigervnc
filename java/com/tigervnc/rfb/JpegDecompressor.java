/* Copyright (C) 2016 Brian P. Hinz
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

import java.awt.*;
import java.awt.image.*;
import java.io.*;
import java.nio.ByteBuffer;
import javax.imageio.*;
import javax.imageio.stream.*;

public class JpegDecompressor {

  public JpegDecompressor() {}

  public void decompress(ByteBuffer jpegBuf, int jpegBufLen,
    PixelBuffer pb, Rect r, PixelFormat pf)
  {

    byte[] src = new byte[jpegBufLen];

    jpegBuf.get(src);
    try {
      ImageIO.setUseCache(false);
      BufferedImage jpeg =
        ImageIO.read(new MemoryCacheImageInputStream(new ByteArrayInputStream(src)));
      jpeg.setAccelerationPriority(1);
      synchronized(pb.getImage()) {
        Graphics2D g2 = (Graphics2D)pb.getImage().getGraphics();
        g2.drawImage(jpeg, r.tl.x, r.tl.y, r.width(), r.height(), null);
        g2.dispose();
      }
      jpeg.flush();
    } catch (IOException e) {
      throw new Exception(e.getMessage());
    }
  }
}
