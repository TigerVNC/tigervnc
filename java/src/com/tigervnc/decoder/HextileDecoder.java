package com.tightvnc.decoder;

import com.tightvnc.decoder.common.Repaintable;
import com.tightvnc.vncviewer.RfbInputStream;
import java.awt.Color;
import java.awt.Graphics;
import java.io.IOException;

//
// Class that used for decoding hextile encoded data.
//

public class HextileDecoder extends RawDecoder {

  final static int EncodingHextile = 5;

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

  //
  // Override handleRect method to decode Hextile encoded data insted of
  // raw pixel data.
  //

  public void handleRect(int x, int y, int w, int h) throws IOException,
                                                            Exception {

    //
    // Write encoding ID to record output stream
    //

    if (dos != null) {
      dos.writeInt(HextileDecoder.EncodingHextile);
    }

    hextile_bg = new Color(0);
    hextile_fg = new Color(0);

    for (int ty = y; ty < y + h; ty += 16) {
      int th = 16;
      if (y + h - ty < 16)
        th = y + h - ty;

      for (int tx = x; tx < x + w; tx += 16) {
        int tw = 16;
        if (x + w - tx < 16)
          tw = x + w - tx;

        handleHextileSubrect(tx, ty, tw, th);
      }
      if (repainableControl != null)
        repainableControl.scheduleRepaint(x, y, w, h);
    }
    if (repainableControl != null)
      repainableControl.scheduleRepaint(x, y, w, h);
  }

  //
  // Handle one tile in the Hextile-encoded data.
  //

  private void handleHextileSubrect(int tx, int ty, int tw, int th)
    throws IOException, Exception {

    int subencoding = rfbis.readU8();

    //
    // Save decoded data to data output stream
    //

    if (dos != null) {
      dos.writeByte((byte)subencoding);
    }

    // Is it a raw-encoded sub-rectangle?
    if ((subencoding & HextileRaw) != 0) {
      //
      // Disable encoding id writting to record stream
      // in super (RawDecoder) class, cause we write subencoding ID
      // in this class (see code above).
      //

      super.enableEncodingRecordWritting(false);
      super.handleRect(tx, ty, tw, th);
      super.handleUpdatedPixels(tx, ty, tw, th);
      super.enableEncodingRecordWritting(true);
      return;
    }

    // Read and draw the background if specified.
    byte[] cbuf = new byte[bytesPerPixel];
    if ((subencoding & HextileBackgroundSpecified) != 0) {
      rfbis.readFully(cbuf);
      if (bytesPerPixel == 1) {
        hextile_bg = getColor256()[cbuf[0] & 0xFF];
      } else {
        hextile_bg = new Color(cbuf[2] & 0xFF, cbuf[1] & 0xFF, cbuf[0] & 0xFF);
      }

      //
      // Save decoded data to data output stream
      //

      if (dos != null) {
        dos.write(cbuf);
      }
    }
    graphics.setColor(hextile_bg);
    graphics.fillRect(tx, ty, tw, th);

    // Read the foreground color if specified.
    if ((subencoding & HextileForegroundSpecified) != 0) {
      rfbis.readFully(cbuf);
      if (bytesPerPixel == 1) {
        hextile_fg = getColor256()[cbuf[0] & 0xFF];
      } else {
        hextile_fg = new Color(cbuf[2] & 0xFF, cbuf[1] & 0xFF, cbuf[0] & 0xFF);
      }

      //
      // Save decoded data to data output stream
      //

      if (dos != null) {
        dos.write(cbuf);
      }
    }

    // Done with this tile if there is no sub-rectangles.
    if ((subencoding & HextileAnySubrects) == 0)
      return;

    int nSubrects = rfbis.readU8();
    int bufsize = nSubrects * 2;
    if ((subencoding & HextileSubrectsColoured) != 0) {
      bufsize += nSubrects * bytesPerPixel;
    }
    byte[] buf = new byte[bufsize];
    rfbis.readFully(buf);

    //
    // Save decoded data to data output stream
    //

    if (dos != null) {
      dos.writeByte((byte)nSubrects);
      dos.write(buf);
    }

    int b1, b2, sx, sy, sw, sh;
    int i = 0;

    if ((subencoding & HextileSubrectsColoured) == 0) {

      // Sub-rectangles are all of the same color.
      graphics.setColor(hextile_fg);
      for (int j = 0; j < nSubrects; j++) {
        b1 = buf[i++] & 0xFF;
        b2 = buf[i++] & 0xFF;
        sx = tx + (b1 >> 4);
        sy = ty + (b1 & 0xf);
        sw = (b2 >> 4) + 1;
        sh = (b2 & 0xf) + 1;
        graphics.fillRect(sx, sy, sw, sh);
      }
    } else if (bytesPerPixel == 1) {

      // BGR233 (8-bit color) version for colored sub-rectangles.
      for (int j = 0; j < nSubrects; j++) {
        hextile_fg = getColor256()[buf[i++] & 0xFF];
        b1 = buf[i++] & 0xFF;
        b2 = buf[i++] & 0xFF;
        sx = tx + (b1 >> 4);
        sy = ty + (b1 & 0xf);
        sw = (b2 >> 4) + 1;
        sh = (b2 & 0xf) + 1;
        graphics.setColor(hextile_fg);
        graphics.fillRect(sx, sy, sw, sh);
      }

    } else {

      // Full-color (24-bit) version for colored sub-rectangles.
      for (int j = 0; j < nSubrects; j++) {
        hextile_fg = new Color(buf[i+2] & 0xFF,
                               buf[i+1] & 0xFF,
                               buf[i] & 0xFF);
        i += 4;
        b1 = buf[i++] & 0xFF;
        b2 = buf[i++] & 0xFF;
        sx = tx + (b1 >> 4);
        sy = ty + (b1 & 0xf);
        sw = (b2 >> 4) + 1;
        sh = (b2 & 0xf) + 1;
        graphics.setColor(hextile_fg);
        graphics.fillRect(sx, sy, sw, sh);
      }

    }
  }

  // These colors should be kept between handleHextileSubrect() calls.
  private Color hextile_bg, hextile_fg;
  // Repaitable object
  private Repaintable repainableControl = null;
}
