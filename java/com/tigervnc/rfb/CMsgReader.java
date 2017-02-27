/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2011-2017 Brian P. Hinz
 * Copyright (C) 2017 Pierre Ossman for Cendio AB
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

//
// CMsgReader - class for reading RFB messages on the client side
// (i.e. messages from server to client).
//

package com.tigervnc.rfb;

import java.awt.image.*;
import java.nio.ByteBuffer;
import java.nio.CharBuffer;
import java.nio.charset.Charset;

import com.tigervnc.rdr.*;

public class CMsgReader {

  protected CMsgReader(CMsgHandler handler_, InStream is_)
  {
    imageBufIdealSize = 0;
    handler = handler_;
    is = is_;
    imageBuf = null;
    imageBufSize = 0;
    nUpdateRectsLeft = 0;
  }

  public void readServerInit()
  {
    int width = is.readU16();
    int height = is.readU16();
    handler.setDesktopSize(width, height);
    PixelFormat pf = new PixelFormat();
    pf.read(is);
    handler.setPixelFormat(pf);
    String name = is.readString();
    handler.setName(name);
    handler.serverInit();
  }

  public void readMsg()
  {
    if (nUpdateRectsLeft == 0) {
      int type = is.readU8();

      switch (type) {
      case MsgTypes.msgTypeSetColourMapEntries:
        readSetColourMapEntries();
        break;
      case MsgTypes.msgTypeBell:
        readBell();
        break;
      case MsgTypes.msgTypeServerCutText:
        readServerCutText();
        break;
      case MsgTypes.msgTypeFramebufferUpdate:
        readFramebufferUpdate();
        break;
      case MsgTypes.msgTypeServerFence:
        readFence();
        break;
      case MsgTypes.msgTypeEndOfContinuousUpdates:
        readEndOfContinuousUpdates();
        break;
      default:
        throw new Exception("unknown message type");
      }
    } else {
      int x = is.readU16();
      int y = is.readU16();
      int w = is.readU16();
      int h = is.readU16();
      int encoding = is.readS32();

      switch (encoding) {
      case Encodings.pseudoEncodingLastRect:
        nUpdateRectsLeft = 1;     // this rectangle is the last one
        break;
      case Encodings.pseudoEncodingCursor:
        readSetCursor(w, h, new Point(x,y));
        break;
      case Encodings.pseudoEncodingCursorWithAlpha:
        readSetCursorWithAlpha(w, h, new Point(x,y));
        break;
      case Encodings.pseudoEncodingDesktopName:
        readSetDesktopName(x, y, w, h);
        break;
      case Encodings.pseudoEncodingDesktopSize:
        handler.setDesktopSize(w, h);
        break;
      case Encodings.pseudoEncodingExtendedDesktopSize:
        readExtendedDesktopSize(x, y, w, h);
        break;
      case Encodings.pseudoEncodingClientRedirect:
        nUpdateRectsLeft = 0;
        readClientRedirect(x, y, w, h);
        return;
      default:
        readRect(new Rect(x, y, x+w, y+h), encoding);
        break;
      };

      nUpdateRectsLeft--;
      if (nUpdateRectsLeft == 0)
        handler.framebufferUpdateEnd();
    }
  }

  protected void readSetColourMapEntries()
  {
    is.skip(1);
    int firstColour = is.readU16();
    int nColours = is.readU16();
    int[] rgbs = new int[nColours * 3];
    for (int i = 0; i < nColours * 3; i++)
      rgbs[i] = is.readU16();
    handler.setColourMapEntries(firstColour, nColours, rgbs);
  }

  protected void readBell()
  {
    handler.bell();
  }

  protected void readServerCutText()
  {
    is.skip(3);
    int len = is.readU32();
    if (len > 256*1024) {
      is.skip(len);
      vlog.error("cut text too long ("+len+" bytes) - ignoring");
      return;
    }
    byte[] buf = new byte[len];
    is.readBytes(buf, 0, len);
    Charset latin1 = Charset.forName("ISO-8859-1");
    CharBuffer chars = latin1.decode(ByteBuffer.wrap(buf));
    handler.serverCutText(chars.toString(), len);
  }

  protected void readFence()
  {
    int flags;
    int len;
    byte[] data = new byte[64];

    is.skip(3);

    flags = is.readU32();

    len = is.readU8();
    if (len > data.length) {
      System.out.println("Ignoring fence with too large payload\n");
      is.skip(len);
      return;
    }

    is.readBytes(data, 0, len);

    handler.fence(flags, len, data);
  }

  protected void readEndOfContinuousUpdates()
  {
    handler.endOfContinuousUpdates();
  }

  protected void readFramebufferUpdate()
  {
    is.skip(1);
    nUpdateRectsLeft = is.readU16();
    handler.framebufferUpdateStart();
  }

  protected void readRect(Rect r, int encoding)
  {
    if ((r.br.x > handler.cp.width) || (r.br.y > handler.cp.height)) {
      vlog.error("Rect too big: "+r.width()+"x"+r.height()+" at "+
                  r.tl.x+","+r.tl.y+" exceeds "+handler.cp.width+"x"+
                  handler.cp.height);
      throw new Exception("Rect too big");
    }

    if (r.is_empty())
      vlog.error("Ignoring zero size rect");

    handler.dataRect(r, encoding);
  }

  protected void readSetCursor(int width, int height, Point hotspot)
  {
    int data_len = width * height * (handler.cp.pf().bpp/8);
    int mask_len = ((width+7)/8) * height;
    ByteBuffer data = ByteBuffer.allocate(data_len);
    ByteBuffer mask = ByteBuffer.allocate(mask_len);

    int x, y;
    byte[] buf = new byte[width*height*4];
    ByteBuffer in;
    ByteBuffer out;

    is.readBytes(data, data_len);
    is.readBytes(mask, mask_len);

    int maskBytesPerRow = (width+7)/8;
    in = (ByteBuffer)data.duplicate().mark();
    out = (ByteBuffer)ByteBuffer.wrap(buf).mark();
    for (y = 0;y < height;y++) {
      for (x = 0;x < width;x++) {
        int byte_ = y * maskBytesPerRow + x / 8;
        int bit = 7 - x % 8;

        // NOTE: BufferedImage needs ARGB, rather than RGBA
        if ((mask.get(byte_) & (1 << bit)) != 0)
          out.put((byte)255);
        else
          out.put((byte)0);

        handler.cp.pf().rgbFromBuffer(out, in.duplicate(), 1);

        in.position(in.position() + handler.cp.pf().bpp/8);
        out.position(out.reset().position() + 4).mark();
      }
    }

    handler.setCursor(width, height, hotspot, buf);
  }

  protected void readSetCursorWithAlpha(int width, int height, Point hotspot)
  {
    int encoding;

    PixelFormat rgbaPF =
      new PixelFormat(32, 32, false, true, 255, 255, 255, 16, 8, 0);
    ManagedPixelBuffer pb =
      new ManagedPixelBuffer(rgbaPF, width, height);
    PixelFormat origPF;

    DataBufferInt buf;

    encoding = is.readS32();

    origPF = handler.cp.pf();
    handler.cp.setPF(rgbaPF);
    handler.readAndDecodeRect(pb.getRect(), encoding, pb);
    handler.cp.setPF(origPF);

    if (pb.getRect().area() == 0)
      return;

    // ARGB with pre-multiplied alpha works best for BufferedImage
    buf = (DataBufferInt)pb.getBufferRW(pb.getRect()).getDataBuffer();
    ByteBuffer bbuf =
      ByteBuffer.allocate(pb.area()*4).order(rgbaPF.getByteOrder());
    bbuf.asIntBuffer().put(buf.getData()).flip().mark();

    for (int i = 0;i < pb.area();i++) {
      byte alpha = bbuf.get(bbuf.position()+3);

      bbuf.put(i*4+3, (byte)(bbuf.get(i*4+2)));
      bbuf.put(i*4+2, (byte)(bbuf.get(i*4+1)));
      bbuf.put(i*4+1, (byte)(bbuf.get(i*4+0))); 
      bbuf.put(i*4+0, (byte)alpha);

      bbuf.position(bbuf.position() + 4);
    }

    handler.setCursor(width, height, hotspot,
                      bbuf.array());
  }

  protected void readSetDesktopName(int x, int y, int w, int h)
  {
    String name = is.readString();

    if (x != 0 || y != 0 || w != 0 || h != 0) {
      vlog.error("Ignoring DesktopName rect with non-zero position/size");
    } else {
      handler.setName(name);
    }

  }

  protected void readExtendedDesktopSize(int x, int y, int w, int h)
  {
    int screens, i;
    int id, flags;
    int sx, sy, sw, sh;
    ScreenSet layout = new ScreenSet();

    screens = is.readU8();
    is.skip(3);

    for (i = 0;i < screens;i++) {
      id = is.readU32();
      sx = is.readU16();
      sy = is.readU16();
      sw = is.readU16();
      sh = is.readU16();
      flags = is.readU32();

      layout.add_screen(new Screen(id, sx, sy, sw, sh, flags));
    }

    handler.setExtendedDesktopSize(x, y, w, h, layout);
  }

  protected void readClientRedirect(int x, int y, int w, int h)
  {
    int port = is.readU16();
    String host = is.readString();
    String x509subject = is.readString();

    if (x != 0 || y != 0 || w != 0 || h != 0)
      vlog.error("Ignoring ClientRedirect rect with non-zero position/size");
    else
      handler.clientRedirect(port, host, x509subject);
  }

  public int[] getImageBuf(int required) { return getImageBuf(required, 0, 0); }

  public int[] getImageBuf(int required, int requested, int nPixels)
  {
    int requiredBytes = required;
    int requestedBytes = requested;
    int size = requestedBytes;
    if (size > imageBufIdealSize) size = imageBufIdealSize;

    if (size < requiredBytes)
      size = requiredBytes;

    if (imageBufSize < size) {
      imageBufSize = size;
      imageBuf = new int[imageBufSize];
    }
    if (nPixels != 0)
      nPixels = imageBufSize / (handler.cp.pf().bpp / 8);
    return imageBuf;
  }

  public InStream getInStream() { return is; }

  public int imageBufIdealSize;

  protected CMsgHandler handler;
  protected InStream is;
  protected int nUpdateRectsLeft;
  protected int[] imageBuf;
  protected int imageBufSize;

  static LogWriter vlog = new LogWriter("CMsgReader");
}
