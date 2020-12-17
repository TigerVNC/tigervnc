/* Copyright 2016-2019 Brian P. Hinz
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

// -=- Modifiable generic pixel buffer class

package com.tigervnc.rfb;

import java.awt.image.*;
import java.awt.Color;
import java.awt.color.ColorSpace;
import java.lang.*;
import java.nio.*;
import java.util.*;

import static java.awt.image.DataBuffer.*;

public abstract class ModifiablePixelBuffer extends PixelBuffer
{

  public ModifiablePixelBuffer(PixelFormat pf, int w, int h)
  {
    super(pf, w, h);
  }

  protected ModifiablePixelBuffer()
  {
  }

  ///////////////////////////////////////////////
  // Access to pixel data
  //

  // Get a writeable pointer into the buffer
  //   Like getBuffer(), the pointer is to the top-left pixel of the
  //   specified Rect.
  public abstract WritableRaster getBufferRW(Rect r);
  // Commit the modified contents
  //   Ensures that the changes to the specified Rect is properly
  //   stored away and any temporary buffers are freed. The Rect given
  //   here needs to match the Rect given to the earlier call to
  //   getBufferRW().
  public abstract void commitBufferRW(Rect r);

  static LogWriter vlog = new LogWriter("ModifiablePixelBuffer");
  ///////////////////////////////////////////////
  // Basic rendering operations
  // These operations DO NOT clip to the pixelbuffer area, or trap overruns.

  // Fill a rectangle
  public void fillRect(Rect r, byte[] pix)
  {
    WritableRaster buf;
    int w, h;

    w = r.width();
    h = r.height();

    if (h == 0 || w ==0)
      return;

    buf = getBufferRW(r);

    ByteBuffer src =
      ByteBuffer.allocate(r.area()*format.bpp/8).order(format.getByteOrder());
    for (int i=0; i < r.area(); i++)
      src.put(pix);
    Raster raster = format.rasterFromBuffer(r, (ByteBuffer)src.rewind());
    synchronized(image) {
      buf.setDataElements(0, 0, raster);
    }

    commitBufferRW(r);
  }

  // Copy pixel data to the buffer
  public void imageRect(Rect r, byte[] pixels)
  {
    WritableRaster dest = getBufferRW(r);

    int length = r.area()*format.bpp/8;
    ByteBuffer src =
      ByteBuffer.wrap(pixels, 0, length).order(format.getByteOrder());
    Raster raster = format.rasterFromBuffer(r, src);
    synchronized(image) {
      dest.setDataElements(0, 0, raster);
    }

    commitBufferRW(r);
  }

  // Copy pixel data from one PixelBuffer location to another
  public void copyRect(Rect rect, Point move_by_delta)
  {
    Raster srcData;
    WritableRaster dstData;

    Rect drect, srect;

    drect = new Rect(rect.tl, rect.br);
    if (!drect.enclosed_by(getRect())) {
      String msg = "Destination rect %dx%d at %d,%d exceeds framebuffer %dx%d";
      vlog.error(String.format(msg, drect.width(), drect.height(),
                               drect.tl.x, drect.tl.y, width(), height()));
      drect = drect.intersect(getRect());
    }

    if (drect.is_empty())
      return;

    srect = drect.translate(move_by_delta.negate());
    if (!srect.enclosed_by(getRect())) {
      String msg = "Source rect %dx%d at %d,%d exceeds framebuffer %dx%d";
      vlog.error(String.format(msg, srect.width(), srect.height(),
                               srect.tl.x, srect.tl.y, width(), height()));
      srect = srect.intersect(getRect());
      // Need to readjust the destination now that the area has changed
      drect = srect.translate(move_by_delta);
    }

    if (srect.is_empty())
      return;

    srcData = getBuffer(srect);
    dstData = getBufferRW(drect);

    synchronized(image) {
      dstData.setDataElements(0, 0, srcData);
    }

    commitBufferRW(rect);
  }

  // Copy pixel data to the buffer through a mask
  //   pixels is a pointer to the pixel to be copied to r.tl.
  //   maskPos specifies the pixel offset in the mask to start from.
  //   mask_ is a pointer to the mask bits at (0,0).
  //   pStride and mStride are the strides of the pixel and mask buffers.
  public void maskRect(Rect r, Object pixels, byte[] mask_)
  {
    Rect cr = getRect().intersect(r);
    if (cr.is_empty()) return;
    WritableRaster data = getBufferRW(cr);

    // FIXME
    ColorModel cm = format.getColorModel();
    SampleModel sm =
      cm.createCompatibleSampleModel(r.width(), r.height());
    DataBuffer db = null;
    ByteBuffer src =
      ByteBuffer.wrap((byte[])pixels).order(format.getByteOrder());
    Buffer dst;
    switch (sm.getTransferType()) {
    case TYPE_INT:
      dst = IntBuffer.allocate(src.remaining()).put(src.asIntBuffer());
      db = new DataBufferInt(((IntBuffer)dst).array(), r.area());
      break;
    case TYPE_BYTE:
      db = new DataBufferByte(src.array(), r.area());
      break;
    case TYPE_SHORT:
      dst = ShortBuffer.allocate(src.remaining()).put(src.asShortBuffer());
      db = new DataBufferShort(((ShortBuffer)dst).array(), r.area());
      break;
    }
    assert(db != null);
    Raster raster =
      Raster.createRaster(sm, db, new java.awt.Point(0, 0));
    ColorConvertOp converter = format.getColorConvertOp(cm.getColorSpace());
    WritableRaster t = data.createCompatibleWritableRaster();
    converter.filter(raster, t);

    int w = cr.width();
    int h = cr.height();

    Point offset = new Point(cr.tl.x-r.tl.x, cr.tl.y-r.tl.y);

    int maskBytesPerRow = (w + 7) / 8;

    synchronized(image) {
      for (int y = 0; y < h; y++) {
        int cy = offset.y + y;
        for (int x = 0; x < w; x++) {
          int cx = offset.x + x;
          int byte_ = cy * maskBytesPerRow + y / 8;
          int bit = 7 - cx % 8;

          if ((mask_[byte_] & (1 << bit)) != 0)
            data.setDataElements(x+cx, y+cy, t.getDataElements(x+cx, y+cy, null));
        }
      }
    }

    commitBufferRW(r);
  }

  //   pixel is the Pixel value to be used where mask_ is set
  public void maskRect(Rect r, int pixel, byte[] mask)
  {
    // FIXME
  }

  // Render in a specific format
  //   Does the exact same thing as the above methods, but the given
  //   pixel values are defined by the given PixelFormat. 
  public void fillRect(PixelFormat pf, Rect dest, byte[] pix)
  {
    WritableRaster dstBuffer = getBufferRW(dest);

    ColorModel cm = pf.getColorModel();
    if (cm.isCompatibleRaster(dstBuffer) &&
        cm.isCompatibleSampleModel(dstBuffer.getSampleModel())) {
      fillRect(dest, pix);
    } else {
      ByteBuffer src =
        ByteBuffer.allocate(dest.area()*pf.bpp/8).order(pf.getByteOrder());
      for (int i=0; i < dest.area(); i++)
        src.put(pix);
      Raster raster = pf.rasterFromBuffer(dest, (ByteBuffer)src.rewind());
      ColorConvertOp converter = format.getColorConvertOp(cm.getColorSpace());
      synchronized(image) {
        converter.filter(raster, dstBuffer);
      }
    }

    commitBufferRW(dest);
  }

  public void imageRect(PixelFormat pf, Rect dest, byte[] pixels)
  {
    WritableRaster dstBuffer = getBufferRW(dest);

    ColorModel cm = pf.getColorModel();
    if (cm.isCompatibleRaster(dstBuffer) &&
        cm.isCompatibleSampleModel(dstBuffer.getSampleModel())) {
      imageRect(dest, pixels);
    } else {
      int length = dest.area()*pf.bpp/8;
      ByteBuffer src = ByteBuffer.wrap(pixels, 0, length).order(pf.getByteOrder());
      Raster raster = pf.rasterFromBuffer(dest, src);
      ColorConvertOp converter = format.getColorConvertOp(cm.getColorSpace());
      synchronized(image) {
        converter.filter(raster, dstBuffer);
      }
    }

    commitBufferRW(dest);
  }

  public void imageRect(PixelFormat pf, Rect dest, Raster pixels)
  {
    WritableRaster dstBuffer = getBufferRW(dest);

    ColorModel cm = pf.getColorModel();
    if (cm.isCompatibleRaster(dstBuffer) &&
        cm.isCompatibleSampleModel(dstBuffer.getSampleModel())) {
      synchronized(image) {
        dstBuffer.setDataElements(0, 0, pixels);
      }
    } else {
      ColorConvertOp converter = format.getColorConvertOp(cm.getColorSpace());
      synchronized(image) {
        converter.filter(pixels, dstBuffer);
      }
    }

    commitBufferRW(dest);
  }

}
