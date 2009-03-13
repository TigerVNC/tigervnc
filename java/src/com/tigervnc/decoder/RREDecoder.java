package com.tightvnc.decoder;

import com.tightvnc.vncviewer.RfbInputStream;
import java.awt.Graphics;
import java.awt.Color;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;

//
// Class that used for decoding RRE encoded data.
//

public class RREDecoder extends RawDecoder {

  final static int EncodingRRE = 2;

  public RREDecoder(Graphics g, RfbInputStream is) {
    super(g, is);
  }

  public RREDecoder(Graphics g, RfbInputStream is, int frameBufferW,
                    int frameBufferH) {
    super(g, is, frameBufferW, frameBufferH);
  }

  //
  // Override handleRect method to decode RRE encoded data insted of
  // raw pixel data.
  //

  public void handleRect(int x, int y, int w, int h) throws IOException {

    //
    // Write encoding ID to record output stream
    //

    if (dos != null) {
      dos.writeInt(RREDecoder.EncodingRRE);
    }

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
    byte[] buf = new byte[nSubrects * (bytesPerPixel + 8)];
    rfbis.readFully(buf);
    DataInputStream ds = new DataInputStream(new ByteArrayInputStream(buf));

    //
    // Save decoded data to data output stream
    //
    if (dos != null) {
      dos.writeInt(nSubrects);
      dos.write(bg_buf);
      dos.write(buf);
    }

    int sx, sy, sw, sh;
    for (int j = 0; j < nSubrects; j++) {
      if (bytesPerPixel == 1) {
        pixel = getColor256()[ds.readUnsignedByte()];
      } else {
        ds.skip(4);
        pixel = new Color(buf[j*12+2] & 0xFF,
                          buf[j*12+1] & 0xFF,
                          buf[j*12]   & 0xFF);
      }
      sx = x + ds.readUnsignedShort();
      sy = y + ds.readUnsignedShort();
      sw = ds.readUnsignedShort();
      sh = ds.readUnsignedShort();

      graphics.setColor(pixel);
      graphics.fillRect(sx, sy, sw, sh);
    }
  }
}
