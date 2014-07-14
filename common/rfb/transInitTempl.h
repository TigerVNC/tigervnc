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
//
// transInitTempl.h - templates for functions to initialise lookup tables for
// the translation functions.
//
// This file is #included after having set the following macros:
// BPPOUT - 8, 16 or 32

#if !defined(BPPOUT)
#error "transInitTempl.h: BPPOUT not defined"
#endif

namespace rfb {

// CONCAT2E concatenates its arguments, expanding them if they are macros

#ifndef CONCAT2E
#define CONCAT2(a,b) a##b
#define CONCAT2E(a,b) CONCAT2(a,b)
#endif

#ifndef SWAP16
#define SWAP16(n) ((((n) & 0xff) << 8) | (((n) >> 8) & 0xff))
#endif

#ifndef SWAP32
#define SWAP32(n) (((n) >> 24) | (((n) & 0x00ff0000) >> 8) | \
                   (((n) & 0x0000ff00) << 8) | ((n) << 24))
#endif

#define OUTPIXEL rdr::CONCAT2E(U,BPPOUT)
#define SWAPOUT CONCAT2E(SWAP,BPPOUT)
#define initSimpleOUT          CONCAT2E(initSimple,BPPOUT)
#define initRGBOUT             CONCAT2E(initRGB,BPPOUT)
#define initOneRGBTableOUT     CONCAT2E(initOneRGBTable,BPPOUT)
#define initOneRGBCubeTableOUT CONCAT2E(initOneRGBCubeTable,BPPOUT)

#ifndef TRANS_INIT_TEMPL_ENDIAN_TEST
#define TRANS_INIT_TEMPL_ENDIAN_TEST
  static rdr::U32 endianTest = 1;
  static bool nativeBigEndian = *(rdr::U8*)(&endianTest) != 1;
#endif

void initSimpleOUT (rdr::U8** tablep, const PixelFormat& inPF,
                    const PixelFormat& outPF)
{
  if (inPF.bpp != 8 && inPF.bigEndian != nativeBigEndian)
    throw Exception("Internal error: inPF is not native endian");

  int size = 1 << inPF.bpp;

  delete [] *tablep;
  *tablep = new rdr::U8[size * sizeof(OUTPIXEL)];
  OUTPIXEL* table = (OUTPIXEL*)*tablep;

  for (int i = 0; i < size; i++) {
    int r = (i >> inPF.redShift)   & inPF.redMax;
    int g = (i >> inPF.greenShift) & inPF.greenMax;
    int b = (i >> inPF.blueShift)  & inPF.blueMax;
      
    r = (r * outPF.redMax   + inPF.redMax/2)   / inPF.redMax;
    g = (g * outPF.greenMax + inPF.greenMax/2) / inPF.greenMax;
    b = (b * outPF.blueMax  + inPF.blueMax/2)  / inPF.blueMax;
      
    table[i] = ((r << outPF.redShift)   |
                (g << outPF.greenShift) |
                (b << outPF.blueShift));
#if (BPPOUT != 8)
    if (outPF.bigEndian != nativeBigEndian)
      table[i] = SWAPOUT (table[i]);
#endif
  }
}

static void initOneRGBTableOUT (OUTPIXEL* table, int inMax, int outMax,
                                int outShift, bool swap)
{
  int size = inMax + 1;

  for (int i = 0; i < size; i++) {
    table[i] = ((i * outMax + inMax / 2) / inMax) << outShift;
#if (BPPOUT != 8)
    if (swap)
      table[i] = SWAPOUT (table[i]);
#endif
  }
}

void initRGBOUT (rdr::U8** tablep, const PixelFormat& inPF,
                 const PixelFormat& outPF)
{
  if (inPF.bpp != 8 && inPF.bigEndian != nativeBigEndian)
    throw Exception("Internal error: inPF is not native endian");

  int size = inPF.redMax + inPF.greenMax + inPF.blueMax + 3;

  delete [] *tablep;
  *tablep = new rdr::U8[size * sizeof(OUTPIXEL)];

  OUTPIXEL* redTable = (OUTPIXEL*)*tablep;
  OUTPIXEL* greenTable = redTable + inPF.redMax + 1;
  OUTPIXEL* blueTable = greenTable + inPF.greenMax + 1;

  bool swap = (outPF.bigEndian != nativeBigEndian);

  initOneRGBTableOUT (redTable, inPF.redMax, outPF.redMax, 
                           outPF.redShift, swap);
  initOneRGBTableOUT (greenTable, inPF.greenMax, outPF.greenMax,
                           outPF.greenShift, swap);
  initOneRGBTableOUT (blueTable, inPF.blueMax, outPF.blueMax,
                           outPF.blueShift, swap);
}


#undef OUTPIXEL
#undef initSimpleOUT
#undef initRGBOUT
#undef initOneRGBTableOUT
}
