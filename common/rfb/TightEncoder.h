/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander
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
#ifndef __RFB_TIGHTENCODER_H__
#define __RFB_TIGHTENCODER_H__

#include <rdr/MemOutStream.h>
#include <rdr/ZlibOutStream.h>
#include <rfb/TransImageGetter.h>
#include <rfb/Encoder.h>

// FIXME: Check if specifying extern "C" is really necessary.
#include <stdio.h>
extern "C" {
#include <jpeglib.h>
}

namespace rfb {

  struct TIGHT_CONF {
    unsigned int maxRectSize, maxRectWidth;
    unsigned int monoMinRectSize;
    int idxZlibLevel, monoZlibLevel, rawZlibLevel;
    int idxMaxColorsDivisor;
    int palMaxColorsWithJPEG;
    int jpegQuality;
    JPEG_SUBSAMP jpegSubsampling;
  };

  //
  // C-style structures to store palette entries and compression paramentes.
  // Such code probably should be converted into C++ classes.
  //

  struct TIGHT_COLOR_LIST {
    TIGHT_COLOR_LIST *next;
    int idx;
    rdr::U32 rgb;
  };

  struct TIGHT_PALETTE_ENTRY {
    TIGHT_COLOR_LIST *listNode;
    int numPixels;
  };

  struct TIGHT_PALETTE {
    TIGHT_PALETTE_ENTRY entry[256];
    TIGHT_COLOR_LIST *hash[256];
    TIGHT_COLOR_LIST list[256];
  };

  //
  // Compression level stuff. The following array contains various
  // encoder parameters for each of 10 compression levels (0..9).
  // Last three parameters correspond to JPEG quality levels (0..9).
  //
  // NOTE: s_conf[9].maxRectSize should be >= s_conf[i].maxRectSize,
  // where i in [0..8]. RequiredBuffSize() method depends on this.
  // FIXME: Is this comment obsolete?
  //

  
  class TightEncoder : public Encoder {
  public:
    static Encoder* create(SMsgWriter* writer);
    virtual void setCompressLevel(int level);
    virtual void setQualityLevel(int level);
    virtual void setFineQualityLevel(int quality, JPEG_SUBSAMP subsampling);
    virtual int getNumRects(const Rect &r);
    virtual bool writeRect(const Rect& r, TransImageGetter* ig, Rect* actual);
    virtual ~TightEncoder();

  private:
    TightEncoder(SMsgWriter* writer);
    bool checkSolidTile(Rect& r, rdr::U32* colorPtr, bool needSameColor);
    void extendSolidArea(const Rect& r, rdr::U32 colorValue, Rect& er);
    void findBestSolidArea(Rect& r, rdr::U32 colorValue, Rect& bestr);
    void sendRectSimple(const Rect& r);
    void writeSubrect(const Rect& r, bool forceSolid = false);

    void compressData(const void *buf, unsigned int length,
                      rdr::ZlibOutStream *zos, int zlibLevel,
                      rdr::OutStream *os);

    int paletteInsert(rdr::U32 rgb, int numPixels, int bpp);
    void paletteReset(void);

    void fastFillPalette8(const rdr::U8 *buffer, int stride, const Rect &r);
    void fastFillPalette16(const rdr::U8 *buffer, int stride, const Rect &r);
    void fastFillPalette32(const rdr::U8 *buffer, int stride, const Rect &r);

    void fillPalette8(rdr::U8 *data, int count);
    void fillPalette16(rdr::U16 *data, int count);
    void fillPalette32(rdr::U32 *data, int count);

    unsigned int packPixels8(rdr::U8 *buf, unsigned int count);
    unsigned int packPixels16(rdr::U16 *buf, unsigned int count);
    unsigned int packPixels32(rdr::U32 *buf, unsigned int count);

    void tightEncode8(const Rect& r, rdr::OutStream *os, bool forceSolid);
    void tightEncode16(const Rect& r, rdr::OutStream *os, bool forceSolid);
    void tightEncode32(const Rect& r, rdr::OutStream *os, bool forceSolid);

    bool checkSolidTile8(Rect& r, rdr::U32 *colorPtr, bool needSameColor);
    bool checkSolidTile16(Rect& r, rdr::U32 *colorPtr, bool needSameColor);
    bool checkSolidTile32(Rect& r, rdr::U32 *colorPtr, bool needSameColor);

    void encodeSolidRect8(rdr::U8 *buf, rdr::OutStream *os);
    void encodeSolidRect16(rdr::U16 *buf, rdr::OutStream *os);
    void encodeSolidRect32(rdr::U32 *buf, rdr::OutStream *os);

    void encodeFullColorRect8(rdr::U8 *buf, const Rect& r, rdr::OutStream *os);
    void encodeFullColorRect16(rdr::U16 *buf, const Rect& r, rdr::OutStream *os);
    void encodeFullColorRect32(rdr::U32 *buf, const Rect& r, rdr::OutStream *os);

    void encodeMonoRect8(rdr::U8 *buf, const Rect& r, rdr::OutStream *os);
    void encodeMonoRect16(rdr::U16 *buf, const Rect& r, rdr::OutStream *os);
    void encodeMonoRect32(rdr::U32 *buf, const Rect& r, rdr::OutStream *os);

    void encodeIndexedRect16(rdr::U16 *buf, const Rect& r, rdr::OutStream *os);
    void encodeIndexedRect32(rdr::U32 *buf, const Rect& r, rdr::OutStream *os);

    void encodeJpegRect(const Rect& r, rdr::OutStream *os);

    SMsgWriter* writer;
    rdr::MemOutStream mos;
    rdr::ZlibOutStream zos[4];
    JpegCompressor jc;
    TransImageGetter *ig;
    PixelFormat serverpf, clientpf;

    bool pack24;
    int palMaxColors, palNumColors;
    rdr::U32 monoBackground, monoForeground;
    TIGHT_PALETTE palette;

    static const int defaultCompressLevel;
    static const TIGHT_CONF conf[];

    const TIGHT_CONF* pconf;
    int jpegQuality;
    JPEG_SUBSAMP jpegSubsampling;
  };

}

#endif
