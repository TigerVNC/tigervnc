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

#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)

#define UBPP CONCAT2E(U,BPP)

void ZRLEEncoder::writePaletteTile(int width, int height,
                                   const rdr::UBPP* buffer, int stride,
                                   const PixelFormat& pf,
                                   const Palette& palette)
{
  const int bitsPerPackedPixel[] = {
    0, 1, 2, 2, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4
  };

  int bppp;
  int pad;

  assert(palette.size() > 1);
  assert(palette.size() <= 16);

  zos.writeU8(palette.size());
  writePalette(pf, palette);

  bppp = bitsPerPackedPixel[palette.size()-1];
  pad = stride - width;

  for (int i = 0; i < height; i++) {
    int w;

    rdr::U8 nbits = 0;
    rdr::U8 byte = 0;

    w = width;
    while (w--) {
      rdr::UBPP pix = *buffer++;
      rdr::U8 index = palette.lookup(pix);
      byte = (byte << bppp) | index;
      nbits += bppp;
      if (nbits >= 8) {
        zos.writeU8(byte);
        nbits = 0;
      }
    }
    if (nbits > 0) {
      byte <<= 8 - nbits;
      zos.writeU8(byte);
    }

    buffer += pad;
  }
}

void ZRLEEncoder::writePaletteRLETile(int width, int height,
                                      const rdr::UBPP* buffer, int stride,
                                      const PixelFormat& pf,
                                      const Palette& palette)
{
  int pad;

  rdr::UBPP prevColour;
  int runLength;

  assert(palette.size() > 1);
  assert(palette.size() <= 127);

  zos.writeU8(palette.size() | 0x80);
  writePalette(pf, palette);

  pad = stride - width;

  prevColour = *buffer;
  runLength = 0;

  while (height--) {
    int w = width;
    while (w--) {
      if (prevColour != *buffer) {
        if (runLength == 1)
          zos.writeU8(palette.lookup(prevColour));
        else {
          zos.writeU8(palette.lookup(prevColour) | 0x80);

          while (runLength > 255) {
            zos.writeU8(255);
            runLength -= 255;
          }
          zos.writeU8(runLength - 1);
        }

        prevColour = *buffer;
        runLength = 0;
      }

      runLength++;
      buffer++;
    }
    buffer += pad;
  }
  if (runLength == 1)
    zos.writeU8(palette.lookup(prevColour));
  else {
    zos.writeU8(palette.lookup(prevColour) | 0x80);

    while (runLength > 255) {
      zos.writeU8(255);
      runLength -= 255;
    }
    zos.writeU8(runLength - 1);
  }
}
