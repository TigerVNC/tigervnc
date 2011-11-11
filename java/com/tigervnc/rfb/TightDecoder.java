/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright 2004-2005 Cendio AB.
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

package com.tigervnc.rfb;

import com.tigervnc.rdr.InStream;
import com.tigervnc.rdr.ZlibInStream;
import java.util.ArrayList;
import java.awt.image.BufferedImage;
import java.io.ByteArrayInputStream;
import javax.imageio.ImageIO;

public class TightDecoder extends Decoder {

  final static int TIGHT_MAX_WIDTH = 2048;

  // Compression control
  final static int rfbTightExplicitFilter = 0x04;
  final static int rfbTightFill = 0x08;
  final static int rfbTightJpeg = 0x09;
  final static int rfbTightMaxSubencoding = 0x09;

  // Filters to improve compression efficiency
  final static int rfbTightFilterCopy = 0x00;
  final static int rfbTightFilterPalette = 0x01;
  final static int rfbTightFilterGradient = 0x02;
  final static int rfbTightMinToCompress = 12;

  public TightDecoder(CMsgReader reader_) { 
    reader = reader_; 
    zis = new ZlibInStream[4];
    for (int i = 0; i < 4; i++)
      zis[i] = new ZlibInStream();
  }

  public void readRect(Rect r, CMsgHandler handler) 
  {
    InStream is = reader.getInStream();
    int[] buf = reader.getImageBuf(r.width() * r.height());
    boolean cutZeros = false;
    PixelFormat myFormat = handler.cp.pf();
    int bpp = handler.cp.pf().bpp;
    if (bpp == 32) {
      if (myFormat.is888()) {
        cutZeros = true;
      }
    }

    int comp_ctl = is.readU8();

    boolean bigEndian = handler.cp.pf().bigEndian;

    // Flush zlib streams if we are told by the server to do so.
    for (int i = 0; i < 4; i++) {
      if ((comp_ctl & 1) != 0) {
        zis[i].reset();
      }
      comp_ctl >>= 1;
    }

    // "Fill" compression type.
    if (comp_ctl == rfbTightFill) {
      int pix;
      if (cutZeros) {
        pix = is.readPixel(3, !bigEndian);
      } else {
        pix = (bpp == 8) ? is.readOpaque8() : is.readOpaque24B();
      }
      handler.fillRect(r, pix);
      return;
    }

    // "JPEG" compression type.
    if (comp_ctl == rfbTightJpeg) {
      // Read length
      int compressedLen = is.readCompactLength();
      if (compressedLen <= 0)
        vlog.info("Incorrect data received from the server.");

      // Allocate netbuf and read in data
      byte[] netbuf = new byte[compressedLen];
      is.readBytes(netbuf, 0, compressedLen);

      // Create an Image object from the JPEG data.
      BufferedImage jpeg = new BufferedImage(r.width(), r.height(), BufferedImage.TYPE_4BYTE_ABGR_PRE);
      jpeg.setAccelerationPriority(1);
      try {
        jpeg = ImageIO.read(new ByteArrayInputStream(netbuf));
      } catch (java.io.IOException e) {
        e.printStackTrace();
      }
      jpeg.getRGB(0, 0, r.width(), r.height(), buf, 0, r.width());
      jpeg = null;
      handler.imageRect(r, buf);
      return;
    }

    // Quit on unsupported compression type.
    if (comp_ctl > rfbTightMaxSubencoding) {
      throw new Exception("TightDecoder: bad subencoding value received");
    }

    // "Basic" compression type.
    int palSize = 0;
    int[] palette = new int[256];
    boolean useGradient = false;

    if ((comp_ctl & rfbTightExplicitFilter) != 0) {
      int filterId = is.readU8();

      switch (filterId) {
      case rfbTightFilterPalette:
        palSize = is.readU8() + 1;
        if (cutZeros) {
          is.readPixels(palette, palSize, 3, !bigEndian);
        } else {
          for (int i = 0; i < palSize; i++) {
            palette[i] = (bpp == 8) ? is.readOpaque8() : is.readOpaque24B();
          }
        }
        break;
      case rfbTightFilterGradient:
        useGradient = true;
        break;
      case rfbTightFilterCopy:
        break;
      default:
        throw new Exception("TightDecoder: unknown filter code recieved");
      }
    }

    int bppp = bpp;
    if (palSize != 0) {
      bppp = (palSize <= 2) ? 1 : 8;
    } else if (cutZeros) {
      bppp = 24;
    }

    // Determine if the data should be decompressed or just copied.
    int rowSize = (r.width() * bppp + 7) / 8;
    int dataSize = r.height() * rowSize;
    int streamId = -1;
    InStream input;
    if (dataSize < rfbTightMinToCompress) {
      input = is;
    } else {
      int length = is.readCompactLength();
      streamId = comp_ctl & 0x03;
      zis[streamId].setUnderlying(is, length);
      input = (ZlibInStream)zis[streamId];
    }

    if (palSize == 0) {
      // Truecolor data.
      if (useGradient) {
        vlog.info("useGradient");
        if (bpp == 32 && cutZeros) {
          vlog.info("FilterGradient24");
          FilterGradient24(r, input, dataSize, buf, handler);
        } else {
          vlog.info("FilterGradient");
          FilterGradient(r, input, dataSize, buf, handler);
        }
      } else {
        if (cutZeros) {
          input.readPixels(buf, r.area(), 3, !bigEndian);
        } else {
          for (int ptr=0; ptr < dataSize; ptr++)
            buf[ptr] = input.readU8();
        }
      }
    } else {
      int x, y, b;
      int ptr = 0;
      int bits;
      if (palSize <= 2) {
        // 2-color palette
        for (y = 0; y < r.height(); y++) {
          for (x = 0; x < r.width() / 8; x++) {
            bits = input.readU8();
            for(b = 7; b >= 0; b--) {
              buf[ptr++] = palette[bits >> b & 1];
            }
          }
          if (r.width() % 8 != 0) {
            bits = input.readU8();
            for (b = 7; b >= 8 - r.width() % 8; b--) {
              buf[ptr++] = palette[bits >> b & 1];
            }
          }
        }
      } else {
        // 256-color palette
        for (y = 0; y < r.height(); y++) {
          for (x = 0; x < r.width(); x++) {
            buf[ptr++] = palette[input.readU8()];
          }
        }
      }
    } 

    handler.imageRect(r, buf);

    if (streamId != -1) {
      zis[streamId].reset();
    }
  }

  private CMsgReader reader;
  private ZlibInStream[] zis;
  static LogWriter vlog = new LogWriter("TightDecoder");

  //
  // Decode data processed with the "Gradient" filter.
  //

  final private void FilterGradient24(Rect r, InStream is, int dataSize, int[] buf, CMsgHandler handler) {

    int x, y, c;
    int[] prevRow = new int[TIGHT_MAX_WIDTH * 3];
    int[] thisRow = new int[TIGHT_MAX_WIDTH * 3];
    int[] pix = new int[3];
    int[] est = new int[3];
    PixelFormat myFormat = handler.cp.pf();

    // Allocate netbuf and read in data
    int[] netbuf = new int[dataSize];
    is.readBytes(netbuf, 0, dataSize);

    int rectHeight = r.height();
    int rectWidth = r.width();

    for (y = 0; y < rectHeight; y++) {
      /* First pixel in a row */
      for (c = 0; c < 3; c++) {
        pix[c] = netbuf[y*rectWidth*3+c] + prevRow[c];
        thisRow[c] = pix[c];
      }
      if (myFormat.bigEndian) {
        buf[y*rectWidth] =  0xff000000 | (pix[2] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[0] & 0xff);
      } else {
        buf[y*rectWidth] =  0xff000000 | (pix[0] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[2] & 0xff);
      }

      /* Remaining pixels of a row */
      for (x = 1; x < rectWidth; x++) {
        for (c = 0; c < 3; c++) {
          est[c] = prevRow[x*3+c] + pix[c] - prevRow[(x-1)*3+c];
          if (est[c] > 0xFF) {
            est[c] = 0xFF;
          } else if (est[c] < 0) {
            est[c] = 0;
          }
          pix[c] = netbuf[(y*rectWidth+x)*3+c] + est[c];
          thisRow[x*3+c] = pix[c];
        }
        if (myFormat.bigEndian) {
          buf[y*rectWidth+x] =  0xff000000 | (pix[2] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[0] & 0xff);
        } else {
          buf[y*rectWidth+x] =  0xff000000 | (pix[0] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[2] & 0xff);
        }
      }

      System.arraycopy(thisRow, 0, prevRow, 0, prevRow.length);
    }
  }

  final private void FilterGradient(Rect r, InStream is, int dataSize, int[] buf, CMsgHandler handler) {

    int x, y, c;
    int[] prevRow = new int[TIGHT_MAX_WIDTH];
    int[] thisRow = new int[TIGHT_MAX_WIDTH];
    int[] pix = new int[3];
    int[] est = new int[3];
    PixelFormat myFormat = handler.cp.pf();

    // Allocate netbuf and read in data
    int[] netbuf = new int[dataSize];
    is.readBytes(netbuf, 0, dataSize);

    int rectHeight = r.height();
    int rectWidth = r.width();

    for (y = 0; y < rectHeight; y++) {
      /* First pixel in a row */
      if (myFormat.bigEndian) {
        buf[y*rectWidth] =  0xff000000 | (pix[2] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[0] & 0xff);
      } else {
        buf[y*rectWidth] =  0xff000000 | (pix[0] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[2] & 0xff);
      }
      for (c = 0; c < 3; c++)
        pix[c] += prevRow[c];

      /* Remaining pixels of a row */
      for (x = 1; x < rectWidth; x++) {
        for (c = 0; c < 3; c++) {
          est[c] = prevRow[x*3+c] + pix[c] - prevRow[(x-1)*3+c];
          if (est[c] > 255) {
            est[c] = 255;
          } else if (est[c] < 0) {
            est[c] = 0;
          }
        }

        // FIXME?
        System.arraycopy(pix, 0, netbuf, 0, netbuf.length);
        for (c = 0; c < 3; c++)
          pix[c] += est[c];

        System.arraycopy(thisRow, x*3, pix, 0, pix.length);

        if (myFormat.bigEndian) {
          buf[y*rectWidth+x] =  0xff000000 | (pix[2] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[0] & 0xff);
        } else {
          buf[y*rectWidth+x] =  0xff000000 | (pix[0] & 0xff)<<16 | (pix[1] & 0xff)<<8 | (pix[2] & 0xff);
        }
      }

      System.arraycopy(thisRow, 0, prevRow, 0, prevRow.length);
    }
  }

}
