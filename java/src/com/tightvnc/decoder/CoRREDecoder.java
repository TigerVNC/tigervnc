package com.tightvnc.decoder;

import com.tightvnc.vncviewer.RfbInputStream;
import java.awt.Graphics;
import java.awt.Color;
import java.io.IOException;

//
// Class that used for decoding CoRRE encoded data.
//

public class CoRREDecoder extends RawDecoder {

  public CoRREDecoder(Graphics g, RfbInputStream is) {
    super(g, is);
  }

  public CoRREDecoder(Graphics g, RfbInputStream is, int frameBufferW,
                      int frameBufferH) {
    super(g, is, frameBufferW, frameBufferH);
  }

  //
  // Override handleRect method to decode CoRRE encoded data insted of
  // raw pixel data.
  //

  public void handleRect(int x, int y, int w, int h) throws IOException {
    int nSubrects = rfbis.readU32();

    byte[] bg_buf = new byte[bytesPerPixel];
    rfbis.readFully(bg_buf);
    Color pixel;
    if (bytesPerPixel == 1) {
      pixel = getColor256()[bg_buf[0] & 0xFF];
    } else {
      pixel = new Color(bg_buf[2] & 0xFF, bg_buf[1] & 0xFF, bg_buf[0] & 0xFF);
    }
    graphics.setColor(pixel);
    graphics.fillRect(x, y, w, h);

    byte[] buf = new byte[nSubrects * (bytesPerPixel + 4)];
    rfbis.readFully(buf);

    //
    // Save decoded data to RecordInterface
    //

    if (rec.canWrite()) {
      rec.writeIntBE(nSubrects);
      rec.write(bg_buf);
      rec.write(buf);
    }

    int sx, sy, sw, sh;
    int i = 0;

    for (int j = 0; j < nSubrects; j++) {
      if (bytesPerPixel == 1) {
        pixel = getColor256()[buf[i++] & 0xFF];
      } else {
        pixel = new Color(buf[i+2] & 0xFF, buf[i+1] & 0xFF, buf[i] & 0xFF);
        i += 4;
      }
      sx = x + (buf[i++] & 0xFF);
      sy = y + (buf[i++] & 0xFF);
      sw = buf[i++] & 0xFF;
      sh = buf[i++] & 0xFF;

      graphics.setColor(pixel);
      graphics.fillRect(sx, sy, sw, sh);
    }
  }
}
