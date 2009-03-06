/* Copyright 2009 Pierre Ossman for Cendio AB
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

namespace rfb {


inline Pixel PixelFormat::pixelFromBuffer(const rdr::U8* buffer) const
{
  Pixel p;

  p = 0;

  if (bigEndian) {
    switch (bpp) {
    case 32:
      p |= ((Pixel)*(buffer++)) << 24;
      p |= ((Pixel)*(buffer++)) << 16;
    case 16:
      p |= ((Pixel)*(buffer++)) << 8;
    case 8:
      p |= *buffer;
    }
  } else {
    p |= buffer[0];
    if (bpp >= 16) {
      p |= ((Pixel)buffer[1]) << 8;
      if (bpp == 32)
        p |= ((Pixel)buffer[2]) << 16;
    }
  }

  return p;
}


inline void PixelFormat::rgbFromPixel(Pixel p, ColourMap* cm, rdr::U16 *r, rdr::U16 *g, rdr::U16 *b) const
{
  if (trueColour) {
    /* We don't need to mask since we shift out unwanted bits */
    *r = (p >> redShift) << redConvShift;
    *g = (p >> greenShift) << greenConvShift;
    *b = (p >> blueShift) << blueConvShift;
  } else if (cm) {
    int ir, ig, ib;
    cm->lookup(p, &ir, &ig, &ib);
    *r = ir;
    *g = ig;
    *b = ib;
  } else {
    // XXX just return 0 for colour map?
    *r = 0;
    *g = 0;
    *b = 0;
  }
}


inline void PixelFormat::rgbFromPixel(Pixel p, ColourMap* cm, rdr::U8 *r, rdr::U8 *g, rdr::U8 *b) const
{
  if (trueColour) {
    *r = (p >> redShift) << (redConvShift - 8);
    *g = (p >> greenShift) << (greenConvShift - 8);
    *b = (p >> blueShift) << (blueConvShift - 8);
  } else {
    rdr::U16 r2, g2, b2;

    rgbFromPixel(p, cm, &r2, &g2, &b2);

    *r = r2 >> 8;
    *g = g2 >> 8;
    *b = b2 >> 8;
  }
}


}

