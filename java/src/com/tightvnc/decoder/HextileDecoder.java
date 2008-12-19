package com.tightvnc.decoder;

import com.tightvnc.vncviewer.RfbInputStream;
import java.awt.Graphics;

public class HextileDecoder extends RawDecoder {

  // Contstants used in the Hextile decoder
  final static int
    HextileRaw                 = 1,
    HextileBackgroundSpecified = 2,
    HextileForegroundSpecified = 4,
    HextileAnySubrects         = 8,
    HextileSubrectsColoured    = 16;

  public HextileDecoder(Graphics g, RfbInputStream is) {
    super(g, is);
  }

  public HextileDecoder(Graphics g, RfbInputStream is, int frameBufferW,
                        int frameBufferH) {
    super(g, is, frameBufferW, frameBufferH);
  }

}
