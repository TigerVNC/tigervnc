/* Copyright 2014 Pierre Ossman for Cendio AB
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

#define UIN CONCAT2E(U,INBPP)
#define UOUT CONCAT2E(U,OUTBPP)

#define SWAP16(n) ((((n) & 0xff) << 8) | (((n) >> 8) & 0xff))
#define SWAP32(n) (((n) >> 24) | (((n) & 0x00ff0000) >> 8) | \
                   (((n) & 0x0000ff00) << 8) | ((n) << 24))

#define SWAPIN CONCAT2E(SWAP,INBPP)
#define SWAPOUT CONCAT2E(SWAP,OUTBPP)

#if INBPP == 32

void PixelFormat::directBufferFromBufferFrom888(rdr::UOUT* dst,
                                                const PixelFormat &srcPF,
                                                const rdr::U8* src,
                                                int w, int h,
                                                int dstStride,
                                                int srcStride) const
{
  const rdr::U8 *r, *g, *b;
  int dstPad, srcPad;

  const rdr::U8 *redDownTable, *greenDownTable, *blueDownTable;

  redDownTable = &downconvTable[(redBits-1)*256];
  greenDownTable = &downconvTable[(greenBits-1)*256];
  blueDownTable = &downconvTable[(blueBits-1)*256];

  if (srcPF.bigEndian) {
    r = src + (24 - srcPF.redShift)/8;
    g = src + (24 - srcPF.greenShift)/8;
    b = src + (24 - srcPF.blueShift)/8;
  } else {
    r = src + srcPF.redShift/8;
    g = src + srcPF.greenShift/8;
    b = src + srcPF.blueShift/8;
  }

  dstPad = (dstStride - w);
  srcPad = (srcStride - w) * 4;
  while (h--) {
    int w_ = w;
    while (w_--) {
      rdr::UOUT d;

      d = redDownTable[*r] << redShift;
      d |= greenDownTable[*g] << greenShift;
      d |= blueDownTable[*b] << blueShift;

#if OUTBPP != 8
      if (endianMismatch)
        d = SWAPOUT(d);
#endif

      *dst = d;

      dst++;
      r += 4;
      g += 4;
      b += 4;
    }
    dst += dstPad;
    r += srcPad;
    g += srcPad;
    b += srcPad;
  }
}

#endif /* INBPP == 32 */

#if OUTBPP == 32

void PixelFormat::directBufferFromBufferTo888(rdr::U8* dst,
                                              const PixelFormat &srcPF,
                                              const rdr::UIN* src,
                                              int w, int h,
                                              int dstStride,
                                              int srcStride) const
{
  rdr::U8 *r, *g, *b, *x;
  int dstPad, srcPad;

  const rdr::U8 *redUpTable, *greenUpTable, *blueUpTable;

  redUpTable = &upconvTable[(srcPF.redBits-1)*256];
  greenUpTable = &upconvTable[(srcPF.greenBits-1)*256];
  blueUpTable = &upconvTable[(srcPF.blueBits-1)*256];

  if (bigEndian) {
    r = dst + (24 - redShift)/8;
    g = dst + (24 - greenShift)/8;
    b = dst + (24 - blueShift)/8;
    x = dst + (24 - (48 - redShift - greenShift - blueShift))/8;
  } else {
    r = dst + redShift/8;
    g = dst + greenShift/8;
    b = dst + blueShift/8;
    x = dst + (48 - redShift - greenShift - blueShift)/8;
  }

  dstPad = (dstStride - w) * 4;
  srcPad = (srcStride - w);
  while (h--) {
    int w_ = w;
    while (w_--) {
      rdr::UIN s;

      s = *src;

#if INBPP != 8
      if (srcPF.endianMismatch)
        s = SWAPIN(s);
#endif

      *r = redUpTable[(s >> srcPF.redShift) & 0xff];
      *g = greenUpTable[(s >> srcPF.greenShift) & 0xff];
      *b = blueUpTable[(s >> srcPF.blueShift) & 0xff];
      *x = 0;

      r += 4;
      g += 4;
      b += 4;
      x += 4;
      src++;
    }
    r += dstPad;
    g += dstPad;
    b += dstPad;
    x += dstPad;
    src += srcPad;
  }
}

#endif /* OUTBPP == 32 */
