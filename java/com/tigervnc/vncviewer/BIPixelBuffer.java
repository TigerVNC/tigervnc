/* Copyright (C) 2012 Brian P. Hinz
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

import com.tigervnc.rfb.*;
import com.tigervnc.rfb.Exception;

public class BIPixelBuffer extends PlatformPixelBuffer implements ImageObserver
{
  public BIPixelBuffer(int w, int h, CConn cc_, DesktopWindow desktop_) {
    super(w, h, cc_, desktop_);
    clip = new Rectangle();
  }

  public void setPF(PixelFormat pf) {
    super.setPF(pf);
    createImage(width(), height());
  }

  public void updateColourMap() {
    cm = new IndexColorModel(8, nColours, reds, greens, blues);
    createImage(width_, height_);
  }
  
  // resize() resizes the image, preserving the image data where possible.
  public void resize(int w, int h) {
    if (w == width() && h == height())
      return;

    width_ = w;
    height_ = h;
    createImage(w, h);
  }

  private void createImage(int w, int h) {
    if (w == 0 || h == 0) return;
    WritableRaster wr;
    if (cm instanceof IndexColorModel)
      wr = ((IndexColorModel)cm).createCompatibleWritableRaster(w, h);
    else
      wr = ((DirectColorModel)cm).createCompatibleWritableRaster(w, h);
    image = new BufferedImage(cm, wr, true, null);
    db = wr.getDataBuffer();
  }

  public void fillRect(int x, int y, int w, int h, int pix) {
    Graphics2D graphics = (Graphics2D)image.getGraphics();
    switch (format.depth) {
    case 24:
      graphics.setColor(new Color(pix)); 
      graphics.fillRect(x, y, w, h); 
      break;
    default:
      Color color = new Color((0xff << 24) | (cm.getRed(pix) << 16) |
                              (cm.getGreen(pix) << 8) | (cm.getBlue(pix)));
      graphics.setColor(color); 
      graphics.fillRect(x, y, w, h); 
      break;
    }
    graphics.dispose();
  }

  public void imageRect(int x, int y, int w, int h, Object pix) {
    if (pix instanceof Image) {
      Image img = (Image)pix;
      clip = new Rectangle(x, y, w, h); 
      synchronized(clip) {
        tk.prepareImage(img, -1, -1, this);
        try {
          clip.wait(1000);
        } catch (InterruptedException e) {
          throw new Exception("Error decoding JPEG data");
        }
      } 
      clip = null;
      img.flush();
    } else {
      if (image.getSampleModel().getTransferType() == DataBuffer.TYPE_BYTE) {
        byte[] bytes = new byte[((int[])pix).length];
        for (int i = 0; i < bytes.length; i++)
          bytes[i] = (byte)((int[])pix)[i];
        pix = bytes;
      }
      image.getSampleModel().setDataElements(x, y, w, h, pix, db); 
    }
  }

  public void copyRect(int x, int y, int w, int h, int srcX, int srcY) {
    Graphics2D graphics = (Graphics2D)image.getGraphics();
    graphics.copyArea(srcX, srcY, w, h, x - srcX, y - srcY);
    graphics.dispose();
  }

  public Image getImage() {
    return (Image)image;
  }

  public boolean imageUpdate(Image img, int infoflags, int x, int y, int w, int h) {
    if ((infoflags & (ALLBITS | ABORT)) == 0) {
      return true;
    } else {
      if ((infoflags & ALLBITS) != 0) {
        if (clip != null) {
          synchronized(clip) {
            Graphics2D graphics = (Graphics2D)image.getGraphics();
            graphics.drawImage(img, clip.x, clip.y, clip.width, clip.height, null); 
            graphics.dispose();
            clip.notify();
          }
        }
      }
      return false;
    }
  }

  BufferedImage image;
  DataBuffer db;
  Rectangle clip;

  static LogWriter vlog = new LogWriter("BIPixelBuffer");
}
