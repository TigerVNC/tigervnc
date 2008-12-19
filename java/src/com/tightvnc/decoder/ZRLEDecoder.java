package com.tightvnc.decoder;

import com.tightvnc.vncviewer.InStream;
import com.tightvnc.vncviewer.RfbInputStream;
import com.tightvnc.vncviewer.ZlibInStream;
import java.awt.Graphics;

//
// Class that used for decoding ZRLE encoded data.
//

public class ZRLEDecoder extends RawDecoder {

  public ZRLEDecoder(Graphics g, RfbInputStream is) {
    super(g, is);
  }

  public ZRLEDecoder(Graphics g, RfbInputStream is, int frameBufferW,
                     int frameBufferH) {
    super(g, is, frameBufferW, frameBufferH);
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
