package com.tightvnc.decoder;

import com.tightvnc.vncviewer.RfbInputStream;
import java.awt.Graphics;

//
// Class that used for decoding ZRLE encoded data.
//

public class ZRLEDecoder extends RawDecoder  {

  public ZRLEDecoder(Graphics g, RfbInputStream is) {
    super(g, is);
  }

  public ZRLEDecoder(Graphics g, RfbInputStream is, int frameBufferW,
                     int frameBufferH) {
    super(g, is, frameBufferW, frameBufferH);
  }

}
