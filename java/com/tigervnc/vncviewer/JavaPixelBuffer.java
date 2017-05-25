/* Copyright (C) 2012-2016 Brian P. Hinz
 * Copyright (C) 2012 D. R. Commander.  All Rights Reserved.
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
import java.nio.*;

import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Point;

public class JavaPixelBuffer extends PlatformPixelBuffer 
{

  public JavaPixelBuffer(int w, int h) {
    super(getPreferredPF(), w, h,
          getPreferredPF().getColorModel().createCompatibleWritableRaster(w,h));
    ColorModel cm = format.getColorModel();
    image = new BufferedImage(cm, data, cm.isAlphaPremultiplied(), null);
    image.setAccelerationPriority(1);
  }

  public WritableRaster getBufferRW(Rect r)
  {
    synchronized(image) {
      return ((BufferedImage)image)
        .getSubimage(r.tl.x, r.tl.y, r.width(), r.height()).getRaster();
    }
  }

  public Raster getBuffer(Rect r)
  {
    Rectangle rect =
      new Rectangle(r.tl.x, r.tl.y, r.width(), r.height());
    synchronized(image) {
      return ((BufferedImage)image).getData(rect);
    }
  }

  public void fillRect(Rect r, byte[] pix)
  {
    ColorModel cm = format.getColorModel();
    int pixel =
      ByteBuffer.wrap(pix).order(format.getByteOrder()).asIntBuffer().get(0);
    Color c = new Color(cm.getRGB(pixel));
    synchronized(image) {
      Graphics2D g2 = (Graphics2D)image.getGraphics();
      g2.setColor(c);
      g2.fillRect(r.tl.x, r.tl.y, r.width(), r.height());
      g2.dispose();
    }

    commitBufferRW(r);
  }

  public void copyRect(Rect rect, Point move_by_delta)
  {
    synchronized(image) {
      Graphics2D g2 = (Graphics2D)image.getGraphics();
      g2.copyArea(rect.tl.x - move_by_delta.x,
                  rect.tl.y - move_by_delta.y,
                  rect.width(), rect.height(),
                  move_by_delta.x, move_by_delta.y);
      g2.dispose();
    }

    commitBufferRW(rect);
  }

  private static PixelFormat getPreferredPF()
  {
    GraphicsEnvironment ge =
      GraphicsEnvironment.getLocalGraphicsEnvironment();
    GraphicsDevice gd = ge.getDefaultScreenDevice();
    GraphicsConfiguration gc = gd.getDefaultConfiguration();
    ColorModel cm = gc.getColorModel();
    int depth = ((cm.getPixelSize() > 24) ? 24 : cm.getPixelSize());
    int bpp = (depth > 16 ? 32 : (depth > 8 ? 16 : 8));
    ByteOrder byteOrder = ByteOrder.nativeOrder();
    boolean bigEndian = (byteOrder == ByteOrder.BIG_ENDIAN ? true : false);
    boolean trueColour = true;
    int redShift    = cm.getComponentSize()[0] + cm.getComponentSize()[1];
    int greenShift  = cm.getComponentSize()[0];
    int blueShift   = 0;
    int redMask   = ((int)Math.pow(2, cm.getComponentSize()[2]) - 1);
    int greenMask = ((int)Math.pow(2, cm.getComponentSize()[1]) - 1);
    int blueMmask = ((int)Math.pow(2, cm.getComponentSize()[0]) - 1);
    return new PixelFormat(bpp, depth, bigEndian, trueColour,
                           redMask, greenMask, blueMmask,
                           redShift, greenShift, blueShift);
  }

}
