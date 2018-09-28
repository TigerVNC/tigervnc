/* Copyright 2009-2014 Pierre Ossman for Cendio AB
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
      if (bpp == 32) {
        p |= ((Pixel)buffer[2]) << 16;
        p |= ((Pixel)buffer[3]) << 24;
      }
    }
  }

  return p;
}


inline void PixelFormat::bufferFromPixel(rdr::U8* buffer, Pixel p) const
{
  if (bigEndian) {
    switch (bpp) {
    case 32:
      *(buffer++) = (p >> 24) & 0xff;
      *(buffer++) = (p >> 16) & 0xff;
    case 16:
      *(buffer++) = (p >>  8) & 0xff;
    case 8:
      *(buffer++) = (p >>  0) & 0xff;
    }
  } else {
    buffer[0] = (p >>  0) & 0xff;
    if (bpp >= 16) {
      buffer[1] = (p >>  8) & 0xff;
      if (bpp == 32) {
        buffer[2] = (p >> 16) & 0xff;
        buffer[3] = (p >> 24) & 0xff;
      }
    }
  }
}


inline Pixel PixelFormat::pixelFromRGB(rdr::U16 red, rdr::U16 green, rdr::U16 blue) const
{
  Pixel p;

  p = (Pixel)downconvTable[(redBits-1)*256 + (red >> 8)] << redShift;
  p |= (Pixel)downconvTable[(greenBits-1)*256 + (green >> 8)] << greenShift;
  p |= (Pixel)downconvTable[(blueBits-1)*256 + (blue >> 8)] << blueShift;

  return p;
}


inline Pixel PixelFormat::pixelFromRGB(rdr::U8 red, rdr::U8 green, rdr::U8 blue) const
{
  Pixel p;

  p = (Pixel)downconvTable[(redBits-1)*256 + red] << redShift;
  p |= (Pixel)downconvTable[(greenBits-1)*256 + green] << greenShift;
  p |= (Pixel)downconvTable[(blueBits-1)*256 + blue] << blueShift;

  return p;
}


inline void PixelFormat::rgbFromPixel(Pixel p, rdr::U16 *r, rdr::U16 *g, rdr::U16 *b) const
{
  rdr::U8 _r, _g, _b;

  _r = p >> redShift;
  _g = p >> greenShift;
  _b = p >> blueShift;

  _r = upconvTable[(redBits-1)*256 + _r];
  _g = upconvTable[(greenBits-1)*256 + _g];
  _b = upconvTable[(blueBits-1)*256 + _b];

  *r = _r << 8 | _r;
  *g = _g << 8 | _g;
  *b = _b << 8 | _b;
}


inline void PixelFormat::rgbFromPixel(Pixel p, rdr::U8 *r, rdr::U8 *g, rdr::U8 *b) const
{
  rdr::U8 _r, _g, _b;

  _r = p >> redShift;
  _g = p >> greenShift;
  _b = p >> blueShift;

  *r = upconvTable[(redBits-1)*256 + _r];
  *g = upconvTable[(greenBits-1)*256 + _g];
  *b = upconvTable[(blueBits-1)*256 + _b];
}


}

