/* Copyright 2015 Pierre Ossman for Cendio AB
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

#ifndef __RFB_DECODEMANAGER_H__
#define __RFB_DECODEMANAGER_H__

#include <list>

#include <core/Region.h>
#include <core/Thread.h>

#include <rfb/encodings.h>

namespace core {
  class Condition;
  class Mutex;
  struct Rect;
}

namespace rdr {
  class MemOutStream;
}

namespace rfb {

  class CConnection;
  class Decoder;
  class ModifiablePixelBuffer;

  class DecodeManager {
  public:
    DecodeManager(CConnection *conn);
    ~DecodeManager();

    bool decodeRect(const core::Rect& r, int encoding,
                    ModifiablePixelBuffer* pb);

    void flush();

  private:
    void logStats();

    void setThreadException(const std::exception& e);
    void throwThreadException();

  private:
    CConnection *conn;
    Decoder *decoders[encodingMax+1];

    struct DecoderStats {
      unsigned rects;
      unsigned long long bytes;
      unsigned long long pixels;
      unsigned long long equivalent;
    };

    DecoderStats stats[encodingMax+1];

    struct QueueEntry {
      bool active;
      core::Rect rect;
      int encoding;
      Decoder* decoder;
      const ServerParams* server;
      ModifiablePixelBuffer* pb;
      rdr::MemOutStream* bufferStream;
      core::Region affectedRegion;
    };

    std::list<rdr::MemOutStream*> freeBuffers;
    std::list<QueueEntry*> workQueue;

    core::Mutex* queueMutex;
    core::Condition* producerCond;
    core::Condition* consumerCond;

  private:
    class DecodeThread : public core::Thread {
    public:
      DecodeThread(DecodeManager* manager);
      ~DecodeThread();

      void stop();

    protected:
      void worker() override;
      DecodeManager::QueueEntry* findEntry();

    private:
      DecodeManager* manager;

      bool stopRequested;
    };

    std::list<DecodeThread*> threads;
    std::exception *threadException;
  };

}

#endif
