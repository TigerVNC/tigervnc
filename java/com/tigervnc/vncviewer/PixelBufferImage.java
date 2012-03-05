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

//
// PixelBufferImage is an PixelBuffer which also acts as an ImageProducer.
// Currently it only supports 8-bit colourmapped pixel format.
//

package com.tigervnc.vncviewer;

import java.awt.*;
import java.awt.image.*;
import java.nio.ByteOrder;

import com.tigervnc.rfb.*;

public class PixelBufferImage extends PixelBuffer
{
  public PixelBufferImage(int w, int h, CConn cc_, DesktopWindow desktop_) {
    cc = cc_;
    desktop = desktop_;
    PixelFormat nativePF = getNativePF();
    if (nativePF.depth > cc.serverPF.depth) {
      setPF(cc.serverPF);
    } else {
      setPF(nativePF);
    }
    resize(w, h);
  }

  // resize() resizes the image, preserving the image data where possible.
  public void resize(int w, int h) {
    if (w == width() && h == height()) return;

    width_ = w;
    height_ = h;
    switch (format.depth) {
    case  3: 
      // Fall-through to depth 8
    case  6: 
      // Fall-through to depth 8
    case 8:
      image = new BufferedImage(w, h, BufferedImage.TYPE_BYTE_INDEXED);
      break;
    default:
      GraphicsEnvironment ge =
        GraphicsEnvironment.getLocalGraphicsEnvironment();
      GraphicsDevice gd = ge.getDefaultScreenDevice();
      GraphicsConfiguration gc = gd.getDefaultConfiguration();
      image = gc.createCompatibleImage(w, h, Transparency.OPAQUE);
      break;
    }
    image.setAccelerationPriority(1);
    graphics = image.createGraphics();
  }

  public PixelFormat getNativePF() {
    PixelFormat pf;
    cm = tk.getColorModel();
    if (cm.getColorSpace().getType() == java.awt.color.ColorSpace.TYPE_RGB) {
      int depth = cm.getPixelSize();
      int bpp = (depth > 16 ? 32 : (depth > 8 ? 16 : 8));
      ByteOrder byteOrder = ByteOrder.nativeOrder();
      boolean bigEndian = (byteOrder == ByteOrder.BIG_ENDIAN ? true : false);
      boolean trueColour = (depth > 8 ? true : false);
      int redShift    = cm.getComponentSize()[0] + cm.getComponentSize()[1];
      int greenShift  = cm.getComponentSize()[0];
      int blueShift   = 0;
      pf = new PixelFormat(bpp, depth, bigEndian, trueColour,
        (depth > 8 ? 0xff : 0),
        (depth > 8 ? 0xff : 0),
        (depth > 8 ? 0xff : 0),
        (depth > 8 ? redShift : 0),
        (depth > 8 ? greenShift : 0),
        (depth > 8 ? blueShift : 0));
    } else {
      pf = new PixelFormat(8, 8, false, false, 7, 7, 3, 0, 3, 6);
    }
    vlog.debug("Native pixel format is "+pf.print());
    return pf;
  }

  public void fillRect(int x, int y, int w, int h, int pix) {
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
  }

  public void imageRect(int x, int y, int w, int h, Object pix) {
    if (pix instanceof java.awt.Image) {
      graphics.drawImage((Image)pix, x, y, w, h, null); 
    } else {
      Image img = tk.createImage(new MemoryImageSource(w, h, cm, (int[])pix, 0, w));
      graphics.drawImage(img, x, y, w, h, null); 
      img.flush();
    }
  }

  public void copyRect(int x, int y, int w, int h, int srcX, int srcY) {
    graphics.copyArea(srcX, srcY, w, h, x - srcX, y - srcY);
  }

  // setColourMapEntries() changes some of the entries in the colourmap.
  // However these settings won't take effect until updateColourMap() is
  // called.  This is because getting java to recalculate its internal
  // translation table and redraw the screen is expensive.

  public void setColourMapEntries(int firstColour, int nColours_,
                                               int[] rgbs) {
    nColours = nColours_;
    reds = new byte[nColours];
    blues = new byte[nColours];
    greens = new byte[nColours];
    for (int i = 0; i < nColours; i++) {
      reds[firstColour+i] = (byte)(rgbs[i*3]   >> 8);
      greens[firstColour+i] = (byte)(rgbs[i*3+1] >> 8);
      blues[firstColour+i] = (byte)(rgbs[i*3+2] >> 8);
    }
  }

  public void updateColourMap() {
    cm = new IndexColorModel(8, nColours, reds, greens, blues);
  }

  private static Toolkit tk = java.awt.Toolkit.getDefaultToolkit();

  Graphics2D graphics;
  BufferedImage image;
  ImageConsumer ic;

  int nColours;
  byte[] reds;
  byte[] greens;
  byte[] blues;

  CConn cc;
  DesktopWindow desktop;
  static LogWriter vlog = new LogWriter("PixelBufferImage");
}
