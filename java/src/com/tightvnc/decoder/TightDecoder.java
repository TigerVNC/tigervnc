package com.tightvnc.decoder;

import com.tightvnc.decoder.common.Repaintable;
import com.tightvnc.vncviewer.RfbInputStream;
import java.awt.Graphics;
import java.awt.Color;
import java.awt.Image;
import java.awt.Rectangle;
import java.awt.Toolkit;
import java.awt.image.ImageObserver;
import java.util.zip.Inflater;

//
// Class that used for decoding Tight encoded data.
//

public class TightDecoder extends RawDecoder {

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

  public int getNumJPEGRects() {
    return statNumRectsTightJPEG;
  }

  public void setNumJPEGRects(int v) {
    statNumRectsTightJPEG = v;
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
  private int statNumRectsTightJPEG = 0;
}
