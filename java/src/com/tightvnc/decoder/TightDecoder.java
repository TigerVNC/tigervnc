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
