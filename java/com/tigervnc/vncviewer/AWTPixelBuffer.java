/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2012 TigerVNC Team.
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

package com.tigervnc.vncviewer;

import java.awt.*;
import java.awt.image.*;

import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Exception;

public class AWTPixelBuffer extends PlatformPixelBuffer
{
  public AWTPixelBuffer(int w, int h, CConn cc_, DesktopWindow desktop_) {
    super(w, h, cc_, desktop_);
  }

  public void setPF(PixelFormat pf) {
    super.setPF(pf);
    if (source != null)
      source.newPixels(data, cm, 0, width_);
  }

  public void updateColourMap() {
    cm = new IndexColorModel(8, nColours, reds, greens, blues);
    if (source != null)
      source.newPixels(data, cm, 0, width_);
  }
  
  // resize() resizes the image, preserving the image data where possible.
  public void resize(int w, int h) {
    if (w == width() && h == height()) return;

    int rowsToCopy = h < height() ? h : height();
    int copyWidth = w < width() ? w : width();
    int[] oldData = data;

    width_ = w;
    height_ = h;

    data = new int[width() * height()];
    for (int i = 0; i < rowsToCopy; i++)
      System.arraycopy(oldData, copyWidth * i, 
                       data, width_ * i, copyWidth);

    source = new MemoryImageSource(w, h, cm, data, 0, w);
    source.setAnimated(true);
    source.setFullBufferUpdates(false);
    image = desktop.createImage(source);
    source.newPixels(data, cm, 0, width_);
  }

  public void fillRect(int x, int y, int w, int h, int pix) {
    super.fillRect(x, y, w, h, pix);
    source.newPixels(x, y, w, h, true);
  }

  public void imageRect(int x, int y, int w, int h, Object pix) {
    if (pix instanceof Image) {
      PixelGrabber pg = 
        new PixelGrabber((Image)pix, 0, 0, w, h, data, (width_ * y) + x, width_);
      try {
        pg.grabPixels(0);
      } catch (InterruptedException e) {
        vlog.error("Tight Decoding: Wrong JPEG data recieved");
      }
    } else {
      for (int j = 0; j < h; j++)
        System.arraycopy(pix, (w*j), data, width_ * (y + j) + x, w);
    }
    source.newPixels(x, y, w, h, true);
  }

  public void copyRect(int x, int y, int w, int h, int srcX, int srcY) {
    super.copyRect(x, y, w, h, srcX, srcY);
    source.newPixels(x, y, w, h, true);
  }

  public Image getImage() {
    return (Image)image;
  }

  Image image;
  MemoryImageSource source;

  static LogWriter vlog = new LogWriter("AWTPixelBuffer");
}
