package com.tightvnc.decoder;

import com.tightvnc.decoder.common.Repaintable;
import com.tightvnc.vncviewer.RfbInputStream;
import java.awt.Graphics;
import java.awt.Color;
import java.awt.Image;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.image.ImageObserver;
import java.io.IOException;
import java.util.zip.Deflater;
import java.util.zip.Inflater;

//
// Class that used for decoding Tight encoded data.
//

public class TightDecoder extends RawDecoder implements ImageObserver {

  final static int EncodingTight = 7;

  //
  // Tight decoder constants
  //

  final static int TightExplicitFilter = 0x04;
  final static int TightFill = 0x08;
  final static int TightJpeg = 0x09;
  final static int TightMaxSubencoding = 0x09;
  final static int TightFilterCopy = 0x00;
  final static int TightFilterPalette = 0x01;
  final static int TightFilterGradient = 0x02;
  final static int TightMinToCompress = 12;

  // Tight encoder's data.
  final static int tightZlibBufferSize = 512;

  public TightDecoder(Graphics g, RfbInputStream is) {
    super(g, is);
    tightInflaters = new Inflater[4];
  }

  public TightDecoder(Graphics g, RfbInputStream is, int frameBufferW,
                      int frameBufferH) {
    super(g, is, frameBufferW, frameBufferH);
    tightInflaters = new Inflater[4];
  }

  //
  // Set and get methods for private TightDecoder
  //

  public void setRepainableControl(Repaintable r) {
    repainatableControl = r;
  }

  //
  // JPEG processing statistic methods
  //

  public long getNumJPEGRects() {
    return statNumRectsTightJPEG;
  }

  public void setNumJPEGRects(int v) {
    statNumRectsTightJPEG = v;
  }

  //
  // Tight processing statistic methods
  //

  public long getNumTightRects() {
    return statNumRectsTight;
  }

  public void setNumTightRects(int v) {
    statNumRectsTight = v;
  }

  //
  // Handle a Tight-encoded rectangle.
  //

  public void handleRect(int x, int y, int w, int h) throws Exception {

    //
    // Write encoding ID to record output stream
    //

    if (dos != null) {
      dos.writeInt(TightDecoder.EncodingTight);
    }

    int comp_ctl = rfbis.readU8();

    if (dos != null) {
      // Tell the decoder to flush each of the four zlib streams.
      dos.writeByte(comp_ctl | 0x0F);
    }

    // Flush zlib streams if we are told by the server to do so.
    for (int stream_id = 0; stream_id < 4; stream_id++) {
      if ((comp_ctl & 1) != 0 && tightInflaters[stream_id] != null) {
        tightInflaters[stream_id] = null;
      }
      comp_ctl >>= 1;
    }

    // Check correctness of subencoding value.
    if (comp_ctl > TightDecoder.TightMaxSubencoding) {
      throw new Exception("Incorrect tight subencoding: " + comp_ctl);
    }

    // Handle solid-color rectangles.
    if (comp_ctl == TightDecoder.TightFill) {

      if (bytesPerPixel == 1) {
        int idx = rfbis.readU8();
        graphics.setColor(getColor256()[idx]);
        if (dos != null) {
          dos.writeByte(idx);
        }
      } else {
        byte[] buf = new byte[3];
        rfbis.readFully(buf);
        if (dos != null) {
          dos.write(buf);
        }
        Color bg = new Color(0xFF000000 | (buf[0] & 0xFF) << 16 |
                            (buf[1] & 0xFF) << 8 | (buf[2] & 0xFF));
        graphics.setColor(bg);
      }
      graphics.fillRect(x, y, w, h);
      repainatableControl.scheduleRepaint(x, y, w, h);
      return;

    }

    if (comp_ctl == TightDecoder.TightJpeg) {

      statNumRectsTightJPEG++;

      // Read JPEG data.
      byte[] jpegData = new byte[rfbis.readCompactLen()];
      rfbis.readFully(jpegData);
      if (dos != null) {
        recordCompactLen(jpegData.length);
        dos.write(jpegData);
      }

      // Create an Image object from the JPEG data.
      Image jpegImage = Toolkit.getDefaultToolkit().createImage(jpegData);

      // Remember the rectangle where the image should be drawn.
      jpegRect = new Rectangle(x, y, w, h);

      // Let the imageUpdate() method do the actual drawing, here just
      // wait until the image is fully loaded and drawn.
      synchronized(jpegRect) {
        Toolkit.getDefaultToolkit().prepareImage(jpegImage, -1, -1, this);
        try {
          // Wait no longer than three seconds.
          jpegRect.wait(3000);
        } catch (InterruptedException e) {
          throw new Exception("Interrupted while decoding JPEG image");
        }
      }

      // Done, jpegRect is not needed any more.
      jpegRect = null;
      return;

    } else {
      statNumRectsTight++;
    }

    // Read filter id and parameters.
    int numColors = 0, rowSize = w;
    byte[] palette8 = new byte[2];
    int[] palette24 = new int[256];
    boolean useGradient = false;
    if ((comp_ctl & TightDecoder.TightExplicitFilter) != 0) {
      int filter_id = rfbis.readU8();
      if (dos != null) {
        dos.writeByte(filter_id);
      }
      if (filter_id == TightDecoder.TightFilterPalette) {
        numColors = rfbis.readU8() + 1;
        if (dos != null) {
          dos.writeByte((numColors - 1));
        }
        if (bytesPerPixel == 1) {
          if (numColors != 2) {
            throw new Exception("Incorrect tight palette size: " + numColors);
          }
          rfbis.readFully(palette8);
          if (dos != null) {
            dos.write(palette8);
          }
        } else {
          byte[] buf = new byte[numColors * 3];
          rfbis.readFully(buf);
          if (dos != null) {
            dos.write(buf);
          }
          for (int i = 0; i < numColors; i++) {
           palette24[i] = ((buf[i * 3] & 0xFF) << 16 |
                           (buf[i * 3 + 1] & 0xFF) << 8 |
                           (buf[i * 3 + 2] & 0xFF));
          }
        }
        if (numColors == 2) {
          rowSize = (w + 7) / 8;
        }
      } else if (filter_id == TightDecoder.TightFilterGradient) {
        useGradient = true;
      } else if (filter_id != TightDecoder.TightFilterCopy) {
        throw new Exception("Incorrect tight filter id: " + filter_id);
      }
    }
    if (numColors == 0 && bytesPerPixel == 4)
      rowSize *= 3;

    // Read, optionally uncompress and decode data.
    int dataSize = h * rowSize;
    if (dataSize < TightDecoder.TightMinToCompress) {
      // Data size is small - not compressed with zlib.
      if (numColors != 0) {
        // Indexed colors.
        byte[] indexedData = new byte[dataSize];
        rfbis.readFully(indexedData);
        if (dos != null) {
          dos.write(indexedData);
        }
        if (numColors == 2) {
          // Two colors.
          if (bytesPerPixel == 1) {
            decodeMonoData(x, y, w, h, indexedData, palette8);
          } else {
            decodeMonoData(x, y, w, h, indexedData, palette24);
          }
        } else {
          // 3..255 colors (assuming bytesPixel == 4).
          int i = 0;
          for (int dy = y; dy < y + h; dy++) {
            for (int dx = x; dx < x + w; dx++) {
              pixels24[dy * framebufferWidth + dx] =
                palette24[indexedData[i++] & 0xFF];
            }
          }
        }
      } else if (useGradient) {
        // "Gradient"-processed data
        byte[] buf = new byte[w * h * 3];
        rfbis.readFully(buf);
        if (dos != null) {
          dos.write(buf);
        }
        decodeGradientData(x, y, w, h, buf);
      } else {
        // Raw truecolor data.
        if (bytesPerPixel == 1) {
          for (int dy = y; dy < y + h; dy++) {
            rfbis.readFully(pixels8, dy * framebufferWidth + x, w);
            if (dos != null) {
              dos.write(pixels8, dy * framebufferWidth + x, w);
            }
          }
        } else {
          byte[] buf = new byte[w * 3];
          int i, offset;
          for (int dy = y; dy < y + h; dy++) {
            rfbis.readFully(buf);
            if (dos != null) {
              dos.write(buf);
            }
            offset = dy * framebufferWidth + x;
            for (i = 0; i < w; i++) {
              pixels24[offset + i] =
                (buf[i * 3] & 0xFF) << 16 |
                (buf[i * 3 + 1] & 0xFF) << 8 |
                (buf[i * 3 + 2] & 0xFF);
            }
          }
        }
      }
    } else {
      // Data was compressed with zlib.
      int zlibDataLen = rfbis.readCompactLen();
      byte[] zlibData = new byte[zlibDataLen];
      rfbis.readFully(zlibData);
      int stream_id = comp_ctl & 0x03;
      if (tightInflaters[stream_id] == null) {
        tightInflaters[stream_id] = new Inflater();
      }
      Inflater myInflater = tightInflaters[stream_id];
      myInflater.setInput(zlibData);
      byte[] buf = new byte[dataSize];
      myInflater.inflate(buf);
      if (dos != null) {
        recordCompressedData(buf);
      }

      if (numColors != 0) {
        // Indexed colors.
        if (numColors == 2) {
          // Two colors.
          if (bytesPerPixel == 1) {
            decodeMonoData(x, y, w, h, buf, palette8);
          } else {
            decodeMonoData(x, y, w, h, buf, palette24);
          }
        } else {
          // More than two colors (assuming bytesPixel == 4).
          int i = 0;
          for (int dy = y; dy < y + h; dy++) {
            for (int dx = x; dx < x + w; dx++) {
              pixels24[dy * framebufferWidth + dx] =
                palette24[buf[i++] & 0xFF];
            }
          }
        }
      } else if (useGradient) {
        // Compressed "Gradient"-filtered data (assuming bytesPixel == 4).
        decodeGradientData(x, y, w, h, buf);
      } else {
        // Compressed truecolor data.
        if (bytesPerPixel == 1) {
          int destOffset = y * framebufferWidth + x;
          for (int dy = 0; dy < h; dy++) {
            System.arraycopy(buf, dy * w, pixels8, destOffset, w);
            destOffset += framebufferWidth;
          }
        } else {
          int srcOffset = 0;
          int destOffset, i;
          for (int dy = 0; dy < h; dy++) {
            myInflater.inflate(buf);
            destOffset = (y + dy) * framebufferWidth + x;
            for (i = 0; i < w; i++) {
              RawDecoder.pixels24[destOffset + i] =
                (buf[srcOffset] & 0xFF) << 16 |
                (buf[srcOffset + 1] & 0xFF) << 8 |
                (buf[srcOffset + 2] & 0xFF);
              srcOffset += 3;
            }
          }
        }
      }
    }
    handleUpdatedPixels(x, y, w, h);
  }

  //
  // Decode 1bpp-encoded bi-color rectangle (8-bit and 24-bit versions).
  //

  private void decodeMonoData(int x, int y, int w, int h, byte[] src, byte[] palette) {

    int dx, dy, n;
    int i = y * framebufferWidth + x;
    int rowBytes = (w + 7) / 8;
    byte b;

    for (dy = 0; dy < h; dy++) {
      for (dx = 0; dx < w / 8; dx++) {
        b = src[dy*rowBytes+dx];
        for (n = 7; n >= 0; n--)
          pixels8[i++] = palette[b >> n & 1];
      }
      for (n = 7; n >= 8 - w % 8; n--) {
        pixels8[i++] = palette[src[dy*rowBytes+dx] >> n & 1];
      }
      i += (framebufferWidth - w);
    }
  }

  private void decodeMonoData(int x, int y, int w, int h, byte[] src, int[] palette) {

    int dx, dy, n;
    int i = y * framebufferWidth + x;
    int rowBytes = (w + 7) / 8;
    byte b;

    for (dy = 0; dy < h; dy++) {
      for (dx = 0; dx < w / 8; dx++) {
        b = src[dy*rowBytes+dx];
        for (n = 7; n >= 0; n--)
          pixels24[i++] = palette[b >> n & 1];
      }
      for (n = 7; n >= 8 - w % 8; n--) {
        pixels24[i++] = palette[src[dy*rowBytes+dx] >> n & 1];
      }
      i += (framebufferWidth - w);
    }
  }

  //
  // Decode data processed with the "Gradient" filter.
  //

  private void decodeGradientData (int x, int y, int w, int h, byte[] buf) {

    int dx, dy, c;
    byte[] prevRow = new byte[w * 3];
    byte[] thisRow = new byte[w * 3];
    byte[] pix = new byte[3];
    int[] est = new int[3];

    int offset = y * framebufferWidth + x;

    for (dy = 0; dy < h; dy++) {

      /* First pixel in a row */
      for (c = 0; c < 3; c++) {
        pix[c] = (byte)(prevRow[c] + buf[dy * w * 3 + c]);
        thisRow[c] = pix[c];
      }
      pixels24[offset++] =
         (pix[0] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[2] & 0xFF);

      /* Remaining pixels of a row */
      for (dx = 1; dx < w; dx++) {
        for (c = 0; c < 3; c++) {
          est[c] = ((prevRow[dx * 3 + c] & 0xFF) + (pix[c] & 0xFF) -
                    (prevRow[(dx-1) * 3 + c] & 0xFF));
          if (est[c] > 0xFF) {
            est[c] = 0xFF;
          } else if (est[c] < 0x00) {
            est[c] = 0x00;
          }
          pix[c] = (byte)(est[c] + buf[(dy * w + dx) * 3 + c]);
          thisRow[dx * 3 + c] = pix[c];
        }
        pixels24[offset++] =
          (pix[0] & 0xFF) << 16 | (pix[1] & 0xFF) << 8 | (pix[2] & 0xFF);
      }

      System.arraycopy(thisRow, 0, prevRow, 0, w * 3);
      offset += (framebufferWidth - w);
    }
  }

  //
  // Override the ImageObserver interface method to handle drawing of
  // JPEG-encoded data.
  //

  public boolean imageUpdate(Image img, int infoflags,
                             int x, int y, int width, int height) {
    if ((infoflags & (ALLBITS | ABORT)) == 0) {
      return true; // We need more image data.
    } else {
      // If the whole image is available, draw it now.
      if ((infoflags & ALLBITS) != 0) {
        if (jpegRect != null) {
          synchronized(jpegRect) {
            graphics.drawImage(img, jpegRect.x, jpegRect.y, null);
            repainatableControl.scheduleRepaint(jpegRect.x, jpegRect.y,
                                                jpegRect.width, jpegRect.height);
            jpegRect.notify();
          }
        }
      }
      return false; // All image data was processed.
    }
  }

  //
  // Write an integer in compact representation (1..3 bytes) into the
  // recorded session file.
  //

  void recordCompactLen(int len) throws IOException {
    byte[] buf = new byte[3];
    int bytes = 0;
    buf[bytes++] = (byte)(len & 0x7F);
    if (len > 0x7F) {
      buf[bytes-1] |= 0x80;
      buf[bytes++] = (byte)(len >> 7 & 0x7F);
      if (len > 0x3FFF) {
	buf[bytes-1] |= 0x80;
	buf[bytes++] = (byte)(len >> 14 & 0xFF);
      }
    }
    if (dos != null) dos.write(buf, 0, bytes);
  }

  //
  // Compress and write the data into the recorded session file.
  //

  void recordCompressedData(byte[] data, int off, int len) throws IOException {
    Deflater deflater = new Deflater();
    deflater.setInput(data, off, len);
    int bufSize = len + len / 100 + 12;
    byte[] buf = new byte[bufSize];
    deflater.finish();
    int compressedSize = deflater.deflate(buf);
    recordCompactLen(compressedSize);
    if (dos != null) dos.write(buf, 0, compressedSize);
  }

  void recordCompressedData(byte[] data) throws IOException {
    recordCompressedData(data, 0, data.length);
  }

  //
  // Private members
  //

  private Inflater[] tightInflaters;
  // Since JPEG images are loaded asynchronously, we have to remember
  // their position in the framebuffer. Also, this jpegRect object is
  // used for synchronization between the rfbThread and a JVM's thread
  // which decodes and loads JPEG images.
  private Rectangle jpegRect;
  private Repaintable repainatableControl = null;
  // Jpeg decoding statistics
  private long statNumRectsTightJPEG = 0;
  // Tight decoding statistics
  private long statNumRectsTight = 0;
}
