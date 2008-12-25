package com.tightvnc.decoder;

import com.tightvnc.vncviewer.InStream;
import com.tightvnc.vncviewer.RfbInputStream;
import com.tightvnc.vncviewer.ZlibInStream;
import java.awt.Graphics;
import com.tightvnc.vncviewer.MemInStream;
import java.awt.Color;
import java.awt.Toolkit;
import java.awt.image.MemoryImageSource;
import java.io.IOException;

//
// Class that used for decoding ZRLE encoded data.
//

public class ZRLEDecoder extends RawDecoder {

  final static int EncodingZRLE = 16;

  public ZRLEDecoder(Graphics g, RfbInputStream is) {
    super(g, is);
  }

  public ZRLEDecoder(Graphics g, RfbInputStream is, int frameBufferW,
                     int frameBufferH) {
    super(g, is, frameBufferW, frameBufferH);
  }

  //
  // Handle a ZRLE-encoded rectangle.
  //
  // FIXME: Currently, session recording is not fully supported for ZRLE.
  //

  public void handleRect(int x, int y, int w, int h) throws IOException, Exception {

    //
    // Write encoding ID to record output stream
    //

    if (dos != null) {
      dos.writeInt(ZRLEDecoder.EncodingZRLE);
    }

    if (zrleInStream == null)
      zrleInStream = new ZlibInStream();

    int nBytes = rfbis.readU32();
    if (nBytes > 64 * 1024 * 1024)
      throw new Exception("ZRLE decoder: illegal compressed data size");

    if (zrleBuf == null || zrleBufLen < nBytes) {
      zrleBufLen = nBytes + 4096;
      zrleBuf = new byte[zrleBufLen];
    }

    // FIXME: Do not wait for all the data before decompression.
    rfbis.readFully(zrleBuf, 0, nBytes);

    //
    // Override handleRect method to decode RRE encoded data insted of
    // raw pixel data.
    //

    if (dos != null) {
      if (!zrleRecWarningShown) {
        System.out.println("Warning: ZRLE session can be recorded" +
                           " only from the beginning");
        System.out.println("Warning: Recorded file may be corrupted");
        zrleRecWarningShown = true;
      }
    }

    zrleInStream.setUnderlying(new MemInStream(zrleBuf, 0, nBytes), nBytes);

    for (int ty = y; ty < y+h; ty += 64) {

      int th = Math.min(y+h-ty, 64);

      for (int tx = x; tx < x+w; tx += 64) {

        int tw = Math.min(x+w-tx, 64);

        int mode = zrleInStream.readU8();
        boolean rle = (mode & 128) != 0;
        int palSize = mode & 127;
        int[] palette = new int[128];

        readZrlePalette(palette, palSize);

        if (palSize == 1) {
          int pix = palette[0];
          Color c = (bytesPerPixel == 1) ?
            getColor256()[pix] : new Color(0xFF000000 | pix);
          graphics.setColor(c);
          graphics.fillRect(tx, ty, tw, th);
          continue;
        }

        if (!rle) {
          if (palSize == 0) {
            readZrleRawPixels(tw, th);
          } else {
            readZrlePackedPixels(tw, th, palette, palSize);
          }
        } else {
          if (palSize == 0) {
            readZrlePlainRLEPixels(tw, th);
          } else {
            readZrlePackedRLEPixels(tw, th, palette);
          }
        }
        handleUpdatedZrleTile(tx, ty, tw, th);
      }
    }
    zrleInStream.reset();
  }

  //
  // Override update() method cause we have own data that
  // must be updated when framebuffer is resized or BPP is changed
  //

  public void update() {
    // Images with raw pixels should be re-allocated on every change
    // of geometry or pixel format.
    int fbWidth = framebufferWidth;
    int fbHeight = framebufferHeight;

    if (bytesPerPixel == 1) {
      RawDecoder.pixels24 = null;
      RawDecoder.pixels8 = new byte[fbWidth * fbHeight];
      RawDecoder.pixelsSource = new MemoryImageSource(fbWidth, fbHeight, getColorModel8(), pixels8, 0, fbWidth);
      zrleTilePixels24 = null;
      zrleTilePixels8 = new byte[64 * 64];
    } else {
      RawDecoder.pixels8 = null;
      RawDecoder.pixels24 = new int[fbWidth * fbHeight];
      RawDecoder.pixelsSource =
        new MemoryImageSource(fbWidth, fbHeight, getColorModel24(), pixels24, 0, fbWidth);
      zrleTilePixels8 = null;
      zrleTilePixels24 = new int[64 * 64];
    }
    RawDecoder.pixelsSource.setAnimated(true);
    RawDecoder.rawPixelsImage = Toolkit.getDefaultToolkit().createImage(pixelsSource);
  }

  //
  // Copy pixels from zrleTilePixels8 or zrleTilePixels24, then update.
  //

  private void handleUpdatedZrleTile(int x, int y, int w, int h) {
    Object src, dst;
    if (bytesPerPixel == 1) {
      src = zrleTilePixels8; dst = pixels8;
    } else {
      src = zrleTilePixels24; dst = pixels24;
    }
    int offsetSrc = 0;
    int offsetDst = (y * framebufferWidth + x);
    for (int j = 0; j < h; j++) {
      System.arraycopy(src, offsetSrc, dst, offsetDst, w);
      offsetSrc += w;
      offsetDst += framebufferWidth;
    }
    handleUpdatedPixels(x, y, w, h);
  }

  //
  // Private methods for reading ZRLE data
  //

  private int readPixel(InStream is) throws Exception {
    int pix;
    if (bytesPerPixel == 1) {
      pix = is.readU8();
    } else {
      int p1 = is.readU8();
      int p2 = is.readU8();
      int p3 = is.readU8();
      pix = (p3 & 0xFF) << 16 | (p2 & 0xFF) << 8 | (p1 & 0xFF);
    }
    return pix;
  }

  private void readPixels(InStream is, int[] dst, int count) throws Exception {
    if (bytesPerPixel == 1) {
      byte[] buf = new byte[count];
      is.readBytes(buf, 0, count);
      for (int i = 0; i < count; i++) {
        dst[i] = (int)buf[i] & 0xFF;
      }
    } else {
      byte[] buf = new byte[count * 3];
      is.readBytes(buf, 0, count * 3);
      for (int i = 0; i < count; i++) {
        dst[i] = ((buf[i*3+2] & 0xFF) << 16 |
                  (buf[i*3+1] & 0xFF) << 8 |
                  (buf[i*3] & 0xFF));
      }
    }
  }

  private void readZrlePalette(int[] palette, int palSize) throws Exception {
    readPixels(zrleInStream, palette, palSize);
  }

  private void readZrleRawPixels(int tw, int th) throws Exception {
    if (bytesPerPixel == 1) {
      zrleInStream.readBytes(zrleTilePixels8, 0, tw * th);
    } else {
      readPixels(zrleInStream, zrleTilePixels24, tw * th); ///
    }
  }

  private void readZrlePackedPixels(int tw, int th, int[] palette, int palSize)
    throws Exception {

    int bppp = ((palSize > 16) ? 8 :
                ((palSize > 4) ? 4 : ((palSize > 2) ? 2 : 1)));
    int ptr = 0;

    for (int i = 0; i < th; i++) {
      int eol = ptr + tw;
      int b = 0;
      int nbits = 0;

      while (ptr < eol) {
        if (nbits == 0) {
          b = zrleInStream.readU8();
          nbits = 8;
        }
        nbits -= bppp;
        int index = (b >> nbits) & ((1 << bppp) - 1) & 127;
        if (bytesPerPixel == 1) {
          zrleTilePixels8[ptr++] = (byte)palette[index];
        } else {
          zrleTilePixels24[ptr++] = palette[index];
        }
      }
    }
  }

  private void readZrlePlainRLEPixels(int tw, int th) throws Exception {
    int ptr = 0;
    int end = ptr + tw * th;
    while (ptr < end) {
      int pix = readPixel(zrleInStream);
      int len = 1;
      int b;
      do {
        b = zrleInStream.readU8();
        len += b;
      } while (b == 255);

      if (!(len <= end - ptr))
        throw new Exception("ZRLE decoder: assertion failed" +
                            " (len <= end-ptr)");

      if (bytesPerPixel == 1) {
        while (len-- > 0) zrleTilePixels8[ptr++] = (byte)pix;
      } else {
        while (len-- > 0) zrleTilePixels24[ptr++] = pix;
      }
    }
  }

  private void readZrlePackedRLEPixels(int tw, int th, int[] palette)
    throws Exception {

    int ptr = 0;
    int end = ptr + tw * th;
    while (ptr < end) {
      int index = zrleInStream.readU8();
      int len = 1;
      if ((index & 128) != 0) {
        int b;
        do {
          b = zrleInStream.readU8();
          len += b;
        } while (b == 255);

        if (!(len <= end - ptr))
          throw new Exception("ZRLE decoder: assertion failed" +
                              " (len <= end - ptr)");
      }

      index &= 127;
      int pix = palette[index];

      if (bytesPerPixel == 1) {
        while (len-- > 0) zrleTilePixels8[ptr++] = (byte)pix;
      } else {
        while (len-- > 0) zrleTilePixels24[ptr++] = pix;
      }
    }
  }

  //
  // ZRLE encoder's data.
  //

  private byte[] zrleBuf;
  private int zrleBufLen = 0;
  private byte[] zrleTilePixels8;
  private int[] zrleTilePixels24;
  private ZlibInStream zrleInStream;
  private boolean zrleRecWarningShown = false;
}
