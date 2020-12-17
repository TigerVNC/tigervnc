/* Copyright (C) 2000-2003 Constantin Kaplinsky.  All Rights Reserved.
 * Copyright (C) 2011 D. R. Commander.  All Rights Reserved.
 * Copyright 2014-2018 Pierre Ossman for Cendio AB
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
#ifndef __RFB_ENCODEMANAGER_H__
#define __RFB_ENCODEMANAGER_H__

#include <vector>

#include <rdr/types.h>
#include <rfb/PixelBuffer.h>
#include <rfb/Region.h>
#include <rfb/Timer.h>

namespace rfb {
  class SConnection;
  class Encoder;
  class UpdateInfo;
  class PixelBuffer;
  class RenderedCursor;
  struct Rect;

  struct RectInfo;

  class EncodeManager : public Timer::Callback {
  public:
    EncodeManager(SConnection* conn);
    ~EncodeManager();

    void logStats();

    // Hack to let ConnParams calculate the client's preferred encoding
    static bool supported(int encoding);

    bool needsLosslessRefresh(const Region& req);
    int getNextLosslessRefresh(const Region& req);

    void pruneLosslessRefresh(const Region& limits);

    void writeUpdate(const UpdateInfo& ui, const PixelBuffer* pb,
                     const RenderedCursor* renderedCursor);

    void writeLosslessRefresh(const Region& req, const PixelBuffer* pb,
                              const RenderedCursor* renderedCursor,
                              size_t maxUpdateSize);

  protected:
    virtual bool handleTimeout(Timer* t);

    void doUpdate(bool allowLossy, const Region& changed,
                  const Region& copied, const Point& copy_delta,
                  const PixelBuffer* pb,
                  const RenderedCursor* renderedCursor);
    void prepareEncoders(bool allowLossy);

    Region getLosslessRefresh(const Region& req, size_t maxUpdateSize);

    int computeNumRects(const Region& changed);

    Encoder *startRect(const Rect& rect, int type);
    void endRect();

    void writeCopyRects(const Region& copied, const Point& delta);
    void writeSolidRects(Region *changed, const PixelBuffer* pb);
    void findSolidRect(const Rect& rect, Region *changed, const PixelBuffer* pb);
    void writeRects(const Region& changed, const PixelBuffer* pb);

    void writeSubRect(const Rect& rect, const PixelBuffer *pb);

    bool checkSolidTile(const Rect& r, const rdr::U8* colourValue,
                        const PixelBuffer *pb);
    void extendSolidAreaByBlock(const Rect& r, const rdr::U8* colourValue,
                                const PixelBuffer *pb, Rect* er);
    void extendSolidAreaByPixel(const Rect& r, const Rect& sr,
                                const rdr::U8* colourValue,
                                const PixelBuffer *pb, Rect* er);

    PixelBuffer* preparePixelBuffer(const Rect& rect,
                                    const PixelBuffer *pb, bool convert);

    bool analyseRect(const PixelBuffer *pb,
                     struct RectInfo *info, int maxColours);

  protected:
    // Preprocessor generated, optimised methods
    inline bool checkSolidTile(const Rect& r, rdr::U8 colourValue,
                               const PixelBuffer *pb);
    inline bool checkSolidTile(const Rect& r, rdr::U16 colourValue,
                               const PixelBuffer *pb);
    inline bool checkSolidTile(const Rect& r, rdr::U32 colourValue,
                               const PixelBuffer *pb);

    inline bool analyseRect(int width, int height,
                            const rdr::U8* buffer, int stride,
                            struct RectInfo *info, int maxColours);
    inline bool analyseRect(int width, int height,
                            const rdr::U16* buffer, int stride,
                            struct RectInfo *info, int maxColours);
    inline bool analyseRect(int width, int height,
                            const rdr::U32* buffer, int stride,
                            struct RectInfo *info, int maxColours);

  protected:
    SConnection *conn;

    std::vector<Encoder*> encoders;
    std::vector<int> activeEncoders;

    Region lossyRegion;
    Region recentlyChangedRegion;
    Region pendingRefreshRegion;

    Timer recentChangeTimer;

    struct EncoderStats {
      unsigned rects;
      unsigned long long bytes;
      unsigned long long pixels;
      unsigned long long equivalent;
    };
    typedef std::vector< std::vector<struct EncoderStats> > StatsVector;

    unsigned updates;
    EncoderStats copyStats;
    StatsVector stats;
    int activeType;
    int beforeLength;

    class OffsetPixelBuffer : public FullFramePixelBuffer {
    public:
      OffsetPixelBuffer() {}
      virtual ~OffsetPixelBuffer() {}

      void update(const PixelFormat& pf, int width, int height,
                  const rdr::U8* data_, int stride);

    private:
      virtual rdr::U8* getBufferRW(const Rect& r, int* stride);
    };

    OffsetPixelBuffer offsetPixelBuffer;
    ManagedPixelBuffer convertedPixelBuffer;
  };
}

#endif
