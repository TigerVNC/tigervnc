package com.tightvnc.decoder;

import com.tightvnc.vncviewer.RecordInterface;
import com.tightvnc.vncviewer.RfbInputStream;
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

  public RawDecoder(Graphics g, RfbInputStream is) {
    setGraphics(g);
    setRfbInputStream(is);
    // FIXME: cm24 created in getColorModel24.
    // Remove if no bugs
    cm24 = new DirectColorModel(24, 0xFF0000, 0x00FF00, 0x0000FF);
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

  public void setRfbInputStream(RfbInputStream is) {
    rfbis = is;
  }

  public void setGraphics(Graphics g) {
    graphics = g;
  }

  public void setBPP(int bpp) {
    bytesPerPixel = bpp;
  }

  //
  // FIXME: This method may be useless in future, remove if so
  //

  public int getBPP() {
    return bytesPerPixel;
  }

  public void setFrameBufferSize(int w, int h) {
    framebufferWidth = w;
    framebufferHeight = h;
  }

  //
  // Updates pixels data.
  // This methods must be called when framebuffer is resized
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
  // Private static members access methdos
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
  // Unique data for every decoder (? maybe not ?)
  //
  protected int bytesPerPixel = 4;
  protected int framebufferWidth = 0;
  protected int framebufferHeight = 0;
  protected RfbInputStream rfbis = null;
  protected Graphics graphics = null;
  protected RecordInterface rec = null;

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
