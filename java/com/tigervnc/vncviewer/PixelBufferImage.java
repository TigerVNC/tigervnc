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

public class PixelBufferImage extends PixelBuffer implements ImageProducer
{
  public PixelBufferImage(int w, int h, CConn cc_, DesktopWindow desktop_) {
    cc = cc_;
    desktop = desktop_;
    PixelFormat nativePF = getNativePF();
    switch ((nativePF.depth > cc.serverPF.depth) ? cc.serverPF.depth : nativePF.depth) {
    case  8: 
      setPF(new PixelFormat(8,8,false,true,7,7,3,0,3,6));
      break;
    case 16: 
      setPF(new PixelFormat(16,16,false,true,0xF800,0x07C0,0x003E,0,0,0));
      break;
    case 24: 
      setPF(new PixelFormat(32,24,false,true,0xff,0xff,0xff,16,8,0));
      break;
    default:
      setPF(new PixelFormat(8,8,false,true,7,7,3,0,3,6));
      vlog.debug("Unsupported native PF, defaulting to depth 8");
    }
    resize(w, h);
  }

  // resize() resizes the image, preserving the image data where possible.
  public void resize(int w, int h) {
    if (w == width() && h == height()) return;

    int rowsToCopy = h < height() ? h : height();
    int copyWidth = w < width() ? w : width();
    int[] oldData = data;

    width_ = w;
    height_ = h;
    image = desktop.createImage(this);
    //image.setAccelerationPriority(1);

    data = new int[width() * height()];

    for (int i = 0; i < rowsToCopy; i++)
      System.arraycopy(oldData, copyWidth * i,
                       data, width() * i, copyWidth);
  }

  private PixelFormat getNativePF() {
    PixelFormat pf;
    cm = java.awt.Toolkit.getDefaultToolkit().getColorModel();
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

  // put() causes the given rectangle to be drawn using the given graphics
  // context.
  public void put(int x, int y, int w, int h, Graphics g) {
    if (ic != null) {
      ic.setPixels(x, y, w, h, cm, data, width() * y + x, width());
      ic.imageComplete(ImageConsumer.SINGLEFRAMEDONE);
    }
  }

  // fillRect(), imageRect(), maskRect() are inherited from PixelBuffer.  For
  // copyRect() we also need to tell the ImageConsumer that the pixels have
  // changed (this is done in the put() call for the others).

  public void copyRect(int x, int y, int w, int h, int srcX, int srcY) {
    super.copyRect(x, y, w, h, srcX, srcY);
    if (ic == null) return;
    ic.setPixels(x, y, w, h, cm, data, width() * y + x, width());
    ic.imageComplete(ImageConsumer.SINGLEFRAMEDONE);
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

  // ImageProducer methods

  public void updateColourMap() {
    cm = new IndexColorModel(8, nColours, reds, greens, blues);
  }

  public void addConsumer(ImageConsumer c) {
    if (ic == c) return;
    
    vlog.debug("adding consumer "+c);
    
    if (ic != null)
      vlog.error("Only one ImageConsumer allowed - discarding old one");
    
    ic = c;
    ic.setDimensions(width(), height());
    ic.setHints(ImageConsumer.RANDOMPIXELORDER);
    // Calling ic.setColorModel(cm) seemed to help in some earlier versions of
    // the JDK, but it shouldn't be necessary because we pass the ColorModel
    // with each setPixels() call.
    ic.setPixels(0, 0, width(), height(), cm, data, 0, width());
    ic.imageComplete(ImageConsumer.SINGLEFRAMEDONE);
  }

  public void removeConsumer(ImageConsumer c) {
    System.err.println("removeConsumer "+c);
    if (ic == c) ic = null;
  }

  public boolean isConsumer(ImageConsumer c) { return ic == c; }
  public void requestTopDownLeftRightResend(ImageConsumer c) {}
  public void startProduction(ImageConsumer c) { addConsumer(c); }

  Image image;
  ImageConsumer ic;

  int nColours;
  byte[] reds;
  byte[] greens;
  byte[] blues;

  CConn cc;
  DesktopWindow desktop;
  static LogWriter vlog = new LogWriter("PixelBufferImage");
}
