package com.tightvnc.decoder;

import com.tightvnc.vncviewer.RfbInputStream;
import java.awt.Graphics;
import java.io.IOException;

//
// Class that used for decoding CopyRect encoded data.
//

public class CopyRectDecoder extends RawDecoder {

  final static int EncodingCopyRect = 1;

  public CopyRectDecoder(Graphics g, RfbInputStream is) {
    super(g, is);
  }

  public CopyRectDecoder(Graphics g, RfbInputStream is, int frameBufferW,
                         int frameBufferH) {
    super(g, is, frameBufferW, frameBufferH);
  }

  //
  // Override handleRect method handle CopyRect
  //

  public void handleRect(int x, int y, int w, int h) throws IOException {

    //
    // Write encoding ID to record output stream
    //

    if (dos != null) {
      dos.writeInt(CopyRectDecoder.EncodingCopyRect);
    }

    int copyRectSrcX = rfbis.readU16();
    int copyRectSrcY = rfbis.readU16();

    // If the session is being recorded:
    if (dos != null) {
      dos.writeShort(copyRectSrcX);
      dos.writeShort(copyRectSrcY);
    }

    graphics.copyArea(copyRectSrcX, copyRectSrcY, w, h,
                      x - copyRectSrcX, y - copyRectSrcY);
  }
}
