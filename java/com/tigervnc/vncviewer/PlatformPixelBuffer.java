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

abstract public class PlatformPixelBuffer extends PixelBuffer
{
  public PlatformPixelBuffer(int w, int h, CConn cc_, DesktopWindow desktop_) {
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
  abstract public void resize(int w, int h);

  public PixelFormat getNativePF() {
    PixelFormat pf;
    cm = tk.getColorModel();
    if (cm.getColorSpace().getType() == java.awt.color.ColorSpace.TYPE_RGB) {
      int depth = ((cm.getPixelSize() > 24) ? 24 : cm.getPixelSize());
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

  abstract public void imageRect(int x, int y, int w, int h, Object pix);

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

  protected static Toolkit tk = Toolkit.getDefaultToolkit();

  abstract public Image getImage();

  protected Image image;

  int nColours;
  byte[] reds;
  byte[] greens;
  byte[] blues;

  CConn cc;
  DesktopWindow desktop;
  static LogWriter vlog = new LogWriter("PlatformPixelBuffer");
}
