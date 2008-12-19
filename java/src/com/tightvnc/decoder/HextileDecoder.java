package com.tightvnc.decoder;

import com.tightvnc.decoder.common.Repaintable;
import com.tightvnc.vncviewer.RfbInputStream;
import java.awt.Color;
import java.awt.Graphics;

//
// Class that used for decoding hextile encoded data.
//

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

  //
  // Set private members methods
  //

  public void setRepainableControl(Repaintable r) {
    repainableControl = r;
  }

  // These colors should be kept between handleHextileSubrect() calls.
  private Color hextile_bg, hextile_fg;
  // Repaitable object
  private Repaintable repainableControl = null;

}
