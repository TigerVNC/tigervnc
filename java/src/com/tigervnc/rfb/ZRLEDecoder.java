/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

package com.tigervnc.rfb;

import com.tigervnc.rdr.*;

public class ZRLEDecoder extends Decoder {

  public ZRLEDecoder(CMsgReader reader_) {
    reader = reader_;
    zis = new ZlibInStream();
  }

  public void readRect(Rect r, CMsgHandler handler) {
    InStream is = reader.getInStream();
    int[] buf = reader.getImageBuf(64 * 64 * 4);
    int bytesPerPixel = handler.cp.pf().bpp / 8;
    boolean bigEndian = handler.cp.pf().bigEndian;

    int length = is.readU32();
    zis.setUnderlying(is, length);
    Rect t = new Rect();

    for (t.tl.y = r.tl.y; t.tl.y < r.br.y; t.tl.y += 64) {

      t.br.y = Math.min(r.br.y, t.tl.y + 64);

      for (t.tl.x = r.tl.x; t.tl.x < r.br.x; t.tl.x += 64) {

        t.br.x = Math.min(r.br.x, t.tl.x + 64);

        int mode = zis.readU8();
        boolean rle = (mode & 128) != 0;
        int palSize = mode & 127;
        int[] palette = new int[128];

        if (bytesPerPixel > 1) {
          zis.readPixels(palette, palSize, 3, bigEndian);
        } else {
          for (int i = 0; i < palSize; i++) {
            palette[i] = zis.readPixel(bytesPerPixel, bigEndian);
          }
        }
        
        if (palSize == 1) {
          int pix = palette[0];
          handler.fillRect(t, pix);
          continue;
        }

        if (!rle) {
          if (palSize == 0) {

            // raw

            if (bytesPerPixel > 1) {
              zis.readPixels(buf, t.area(), 3, bigEndian);
            } else {
              zis.readPixels(buf, t.area(), bytesPerPixel, bigEndian);
            }

          } else {

            // packed pixels
            int bppp = ((palSize > 16) ? 8 :
                        ((palSize > 4) ? 4 : ((palSize > 2) ? 2 : 1)));

            int ptr = 0;

            for (int i = 0; i < t.height(); i++) {
              int eol = ptr + t.width();
              int b = 0;
              int nbits = 0;

              while (ptr < eol) {
                if (nbits == 0) {
                  b = zis.readU8();
                  nbits = 8;
                }
                nbits -= bppp;
                int index = (b >> nbits) & ((1 << bppp) - 1) & 127;
                buf[ptr++] = palette[index];
              }
            }
          }

        } else {

          if (palSize == 0) {

            // plain RLE

            int ptr = 0;
            int end = ptr + t.area();
            while (ptr < end) {
              int pix = (bytesPerPixel > 1 ? zis.readPixel(3, bigEndian) : 
                zis.readPixel(bytesPerPixel, bigEndian));
              int len = 1;
              int b;
              do {
                b = zis.readU8();
                len += b;
              } while (b == 255);

              if (!(len <= end - ptr))
                throw new Exception("ZRLEDecoder: assertion (len <= end - ptr)"
                                    +" failed");

              while (len-- > 0) buf[ptr++] = pix;
            }
          } else {

            // palette RLE

            int ptr = 0;
            int end = ptr + t.area();
            while (ptr < end) {
              int index = zis.readU8();
              int len = 1;
              if ((index & 128) != 0) {
                int b;
                do {
                  b = zis.readU8();
                  len += b;
                } while (b == 255);

                if (!(len <= end - ptr))
                  throw new Exception("ZRLEDecoder: assertion "
                                      +"(len <= end - ptr) failed");
              }

              index &= 127;

              int pix = palette[index];

              while (len-- > 0) buf[ptr++] = pix;
            }
          }
        }

        handler.imageRect(t, buf);
      }
    }

    zis.reset();
  }

  CMsgReader reader;
  ZlibInStream zis;
}
