package com.tightvnc.decoder;

import com.tightvnc.vncviewer.RfbInputStream;
import java.io.IOException;
import java.io.DataOutput;
import java.awt.Graphics;
import java.awt.Image;
import java.awt.image.ColorModel;
import java.awt.image.DirectColorModel;
import java.awt.image.MemoryImageSource;
import java.awt.Color;
import java.awt.Toolkit;

//
// This is base decoder class.
// Other classes will be childs of RawDecoder.
//

public class RawDecoder {
  final static int EncodingRaw = 0;

  public RawDecoder(Graphics g, RfbInputStream is) {
    setGraphics(g);
    setRfbInputStream(is);
  }

  public RawDecoder(Graphics g, RfbInputStream is, int frameBufferW,
                    int frameBufferH) {
    setGraphics(g);
    setRfbInputStream(is);
    setFrameBufferSize(frameBufferW, frameBufferH);
    // FIXME: cm24 created in getColorModel24.
    // Remove if no bugs
    cm24 = new DirectColorModel(24, 0xFF0000, 0x00FF00, 0x0000FF);
  }

  //
  // Set methods to set value of non-static protected members of class
  //

  public void setRfbInputStream(RfbInputStream is) {
    rfbis = is;
  }

  public void setGraphics(Graphics g) {
    graphics = g;
  }

  public void setBPP(int bpp) {
    bytesPerPixel = bpp;
  }

  public void setFrameBufferSize(int w, int h) {
    framebufferWidth = w;
    framebufferHeight = h;
  }

  //
  // FIXME: Rename this method after we don't need RecordInterface
  // in RawDecoder class to record session
  //

  public void setDataOutputStream(DataOutput os) {
    dos = os;
  }

  //
  // Decodes Raw Pixels data and draw it into graphics
  //

  public void handleRect(int x, int y, int w, int h) throws IOException, Exception {

    //
    // Write encoding ID to record output stream
    //

    if ((dos != null) && (enableEncodingRecordWritting)) {
      dos.writeInt(RawDecoder.EncodingRaw);
    }

    if (bytesPerPixel == 1) {
      for (int dy = y; dy < y + h; dy++) {
        if (pixels8 != null) {
          rfbis.readFully(pixels8, dy * framebufferWidth + x, w);
        }
        //
        // Save decoded data to record output stream
        //
        if (dos != null) {
          dos.write(pixels8, dy * framebufferWidth + x, w);
        }
      }
    } else {
      byte[] buf = new byte[w * 4];
      int i, offset;
      for (int dy = y; dy < y + h; dy++) {
        rfbis.readFully(buf);
        //
        // Save decoded data to record output stream
        //
        if (dos != null) {
          dos.write(buf);
        }
        offset = dy * framebufferWidth + x;
        if (pixels24 != null) {
          for (i = 0; i < w; i++) {
            pixels24[offset + i] =
              (buf[i * 4 + 2] & 0xFF) << 16 |
              (buf[i * 4 + 1] & 0xFF) << 8 |
              (buf[i * 4] & 0xFF);
          } //for
        } // if
      } // for
    } // else
    handleUpdatedPixels(x, y, w, h);
  } // void

  //
  // Display newly updated area of pixels.
  //

  protected void handleUpdatedPixels(int x, int y, int w, int h) {
    // Draw updated pixels of the off-screen image.
    pixelsSource.newPixels(x, y, w, h);
    graphics.setClip(x, y, w, h);
    graphics.drawImage(rawPixelsImage, 0, 0, null);
    graphics.setClip(0, 0, framebufferWidth, framebufferHeight);
  }

  //
  // Updates pixels data.
  // This method must be called when framebuffer is resized
  // or BPP is changed.
  //

  public void update() {
    // Images with raw pixels should be re-allocated on every change
    // of geometry or pixel format.
    int fbWidth = framebufferWidth;
    int fbHeight = framebufferHeight;

    if (bytesPerPixel == 1) {
      pixels24 = null;
      pixels8 = new byte[fbWidth * fbHeight];
      pixelsSource = new MemoryImageSource(fbWidth, fbHeight, getColorModel8(),
                                           pixels8, 0, fbWidth);
    } else {
      pixels8 = null;
      pixels24 = new int[fbWidth * fbHeight];
      pixelsSource =
        new MemoryImageSource(fbWidth, fbHeight, cm24, pixels24, 0, fbWidth);
    }
    pixelsSource.setAnimated(true);
    rawPixelsImage = Toolkit.getDefaultToolkit().createImage(pixelsSource);
  }

  //
  // Private static members access methods
  //

  protected ColorModel getColorModel8() {
    if (cm8 == null) {
      cm8 = cm8 = new DirectColorModel(8, 7, (7 << 3), (3 << 6));
    }
    return cm8;
  }

  protected ColorModel getColorModel24() {
    if (cm24 == null) {
      cm24 = new DirectColorModel(24, 0xFF0000, 0x00FF00, 0x0000FF);
    }
    return cm24;
  }

  protected Color[]getColor256() {
    if (color256 == null) {
      color256 = new Color[256];
      for (int i = 0; i < 256; i++)
        color256[i] = new Color(cm8.getRGB(i));
    }
    return color256;
  }

  //
  // This method will be used by HextileDecoder to disable
  // double writting encoding id to record stream.
  //
  // FIXME: Try to find better solution than this.
  //

  protected void enableEncodingRecordWritting(boolean enable) {
    enableEncodingRecordWritting = enable;
  }

  //
  // Unique data for every decoder (? maybe not ?)
  //

  protected int bytesPerPixel = 4;
  protected int framebufferWidth = 0;
  protected int framebufferHeight = 0;
  protected RfbInputStream rfbis = null;
  protected Graphics graphics = null;
  protected DataOutput dos = null;
  protected boolean enableEncodingRecordWritting = true;

  //
  // This data must be shared between decoders
  //

  protected static byte []pixels8 = null;
  protected static int []pixels24 = null;
  protected static MemoryImageSource pixelsSource = null;
  protected static Image rawPixelsImage = null;

  //
  // Access to this static members only though protected methods
  //

  private static ColorModel cm8 = null;
  private static ColorModel cm24 = null;
  private static Color []color256 = null;
}
