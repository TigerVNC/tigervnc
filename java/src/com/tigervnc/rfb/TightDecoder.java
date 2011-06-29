/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2004-2005 Cendio AB. All rights reserved.
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
import java.awt.image.PixelGrabber;
import java.awt.Image;
import java.util.ArrayList;

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

    int bytesPerPixel = handler.cp.pf().bpp / 8;
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
        byte[] elem = new byte[3];
        is.readBytes(elem, 0, 3);
        if (bigEndian) {
          pix =
            (elem[2] & 0xFF) << 16 | (elem[1] & 0xFF) << 8 | (elem[0] & 0xFF) | 0xFF << 24;
        } else {
          pix =
            (elem[0] & 0xFF) << 16 | (elem[1] & 0xFF) << 8 | (elem[2] & 0xFF) | 0xFF << 24;
        }
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
      Image jpeg = java.awt.Toolkit.getDefaultToolkit().createImage(netbuf);
      PixelGrabber pg = new PixelGrabber(jpeg, 0, 0, r.width(), r.height(), true);
      try {
        boolean ret = pg.grabPixels();
        if (!ret)
          vlog.info("failed to grab pixels");
      } catch (InterruptedException e) {
        e.printStackTrace();
      }
      Object pixels = pg.getPixels();
      buf = (pixels instanceof byte[]) ? 
        convertByteArrayToIntArray((byte[])pixels) : (int[])pixels;
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
          byte[] elem = new byte[3];
          for (int i = 0; i < palSize; i++) {
            is.readBytes(elem, 0, 3);
            if (bigEndian) {
              palette[i] =
                (elem[2] & 0xFF) << 16 | (elem[1] & 0xFF) << 8 | (elem[0] & 0xFF) | 0xFF << 24;
            } else {
              palette[i] =
                (elem[0] & 0xFF) << 16 | (elem[1] & 0xFF) << 8 | (elem[2] & 0xFF) | 0xFF << 24;
            }
          }
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
          byte[] elem = new byte[3];
          for (int i = 0; i < r.area(); i++) {
            input.readBytes(elem, 0, 3);
            if (bigEndian) {
              buf[i] =
                (elem[2] & 0xFF) << 16 | (elem[1] & 0xFF) << 8 | (elem[0] & 0xFF) | 0xFF << 24;
            } else {
              buf[i] =
                (elem[0] & 0xFF) << 16 | (elem[1] & 0xFF) << 8 | (elem[2] & 0xFF) | 0xFF << 24;
            }
          }
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

  private static int convertByteArrayToInt(byte[] bytes) {
    return (bytes[0] << 32) | (bytes[1] << 24) | (bytes[2] << 16) | (bytes[3] << 8) | bytes[4];
  }

  private static byte[] convertIntToByteArray(int integer) {
    byte[] bytes = new byte[4];
    bytes[0] =(byte)( integer >> 24 );
    bytes[1] =(byte)( (integer << 8) >> 24 );
    bytes[2] =(byte)( (integer << 16) >> 24 );
    bytes[3] =(byte)( (integer << 24) >> 24 );
    return bytes;
  }
  private static int[] convertByteArrayToIntArray(byte[] bytes) {
    vlog.info("convertByteArrayToIntArray");
    ArrayList integers = new ArrayList();
    for (int index = 0; index < bytes.length; index += 4) {
      byte[] fourBytes = new byte[4];
      fourBytes[0] = bytes[index];
      fourBytes[1] = bytes[index+1];
      fourBytes[2] = bytes[index+2];
      fourBytes[3] = bytes[index+3];
      int integer = convertByteArrayToInt(fourBytes);
      integers.add(new Integer(integer));
    }
    int[] ints = new int[bytes.length/4];
    for (int index = 0; index < integers.size() ; index++) {
      ints[index] = ((Integer)integers.get(index)).intValue();
    }
    return ints;
  }

  private static byte[] convertIntArrayToByteArray(int[] integers) {
    byte[] bytes = new byte[integers.length*4];
    for (int index = 0; index < integers.length; index++) {
      byte[] integerBytes = convertIntToByteArray(integers[index]);
      bytes[index*4] = integerBytes[0];
      bytes[1 + (index*4)] = integerBytes[1];
      bytes[2 + (index*4)] = integerBytes[2];
      bytes[3 + (index*4)] = integerBytes[3];
    }
    return bytes;
  }
 
  //
  // Decode data processed with the "Gradient" filter.
  //

  final private void FilterGradient24(Rect r, InStream is, int dataSize, int[] buf, CMsgHandler handler) {

    int x, y, c;
    int[] prevRow = new int[TIGHT_MAX_WIDTH * 3];
    int[] thisRow = new int[TIGHT_MAX_WIDTH * 3];
    int[] pix = new int[3];
    int[] est = new int[3];

    // Allocate netbuf and read in data
    int[] netbuf = new int[dataSize];
    for (int i = 0; i < dataSize; i++)
      netbuf[i] = is.readU8();
    //is.readBytes(netbuf, 0, dataSize);

    PixelFormat myFormat = handler.cp.pf();
    int rectHeight = r.height();
    int rectWidth = r.width();

    for (y = 0; y < rectHeight; y++) {
      /* First pixel in a row */
      for (c = 0; c < 3; c++) {
        pix[c] = netbuf[y*rectWidth*3+c] + prevRow[c];
        thisRow[c] = pix[c];
      }
      if (myFormat.bigEndian) {
        buf[y*rectWidth] =
          (pix[0] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[2] & 0xFF) | 0xFF << 24;
      } else {
        buf[y*rectWidth] =
          (pix[2] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[0] & 0xFF) | 0xFF << 24;
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
          buf[y*rectWidth] =
            (pix[2] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[0] & 0xFF) | 0xFF << 24;
        } else {
          buf[y*rectWidth] =
            (pix[0] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[2] & 0xFF) | 0xFF << 24;
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

    // Allocate netbuf and read in data
    int[] netbuf = new int[dataSize];
    for (int i = 0; i < dataSize; i++)
      netbuf[i] = is.readU8();
    //is.readBytes(netbuf, 0, dataSize);

    PixelFormat myFormat = handler.cp.pf();
    int rectHeight = r.height();
    int rectWidth = r.width();

    for (y = 0; y < rectHeight; y++) {
      /* First pixel in a row */
      if (myFormat.bigEndian) {
        buf[y*rectWidth] =
          (pix[2] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[0] & 0xFF) | 0xFF << 24;
      } else {
        buf[y*rectWidth] =
          (pix[0] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[2] & 0xFF) | 0xFF << 24;
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
          buf[y*rectWidth+x] =
            (pix[2] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[0] & 0xFF) | 0xFF << 24;
        } else {
          buf[y*rectWidth+x] =
            (pix[0] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[2] & 0xFF) | 0xFF << 24;
        }

      }

      System.arraycopy(thisRow, 0, prevRow, 0, prevRow.length);
    }
  }

}
