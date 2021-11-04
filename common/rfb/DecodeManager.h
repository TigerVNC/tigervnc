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

#include <os/Thread.h>

#include <rfb/Region.h>
#include <rfb/encodings.h>

namespace os {
  class Condition;
  class Mutex;
}

namespace rdr {
  struct Exception;
  class MemOutStream;
}

namespace rfb {
  class CConnection;
  class Decoder;
  class ModifiablePixelBuffer;
  struct Rect;

  class DecodeManager {
  public:
    DecodeManager(CConnection *conn);
    ~DecodeManager();

    bool decodeRect(const Rect& r, int encoding,
                    ModifiablePixelBuffer* pb);

    void flush();

  private:
    void logStats();

    void setThreadException(const rdr::Exception& e);
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
      Rect rect;
      int encoding;
      Decoder* decoder;
      const ServerParams* server;
      ModifiablePixelBuffer* pb;
      rdr::MemOutStream* bufferStream;
      Region affectedRegion;
    };

    std::list<rdr::MemOutStream*> freeBuffers;
    std::list<QueueEntry*> workQueue;

    os::Mutex* queueMutex;
    os::Condition* producerCond;
    os::Condition* consumerCond;

  private:
    class DecodeThread : public os::Thread {
    public:
      DecodeThread(DecodeManager* manager);
      ~DecodeThread();

      void stop();

    protected:
      void worker();
      DecodeManager::QueueEntry* findEntry();

    private:
      DecodeManager* manager;

      bool stopRequested;
    };

    std::list<DecodeThread*> threads;
    rdr::Exception *threadException;
  };
}

#endif
