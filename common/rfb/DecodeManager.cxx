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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

#include <core/LogWriter.h>
#include <core/Mutex.h>
#include <core/Region.h>
#include <core/string.h>

#include <rfb/CConnection.h>
#include <rfb/DecodeManager.h>
#include <rfb/Decoder.h>
#include <rfb/Exception.h>

#include <rdr/MemOutStream.h>

using namespace rfb;

static core::LogWriter vlog("DecodeManager");

DecodeManager::DecodeManager(CConnection *conn_) :
  conn(conn_), threadException(nullptr)
{
  size_t cpuCount;

  memset(decoders, 0, sizeof(decoders));

  memset(stats, 0, sizeof(stats));

  queueMutex = new core::Mutex();
  producerCond = new core::Condition(queueMutex);
  consumerCond = new core::Condition(queueMutex);

  cpuCount = core::Thread::getSystemCPUCount();
  if (cpuCount == 0) {
    vlog.error("Unable to determine the number of CPU cores on this system");
    cpuCount = 1;
  } else {
    vlog.info("Detected %d CPU core(s)", (int)cpuCount);
    // No point creating more threads than this, they'll just end up
    // wasting CPU fighting for locks
    if (cpuCount > 4)
      cpuCount = 4;
  }

  vlog.info("Creating %d decoder thread(s)", (int)cpuCount);

  while (cpuCount--) {
    // Twice as many possible entries in the queue as there
    // are worker threads to make sure they don't stall
    freeBuffers.push_back(new rdr::MemOutStream());
    freeBuffers.push_back(new rdr::MemOutStream());

    threads.push_back(new DecodeThread(this));
  }
}

DecodeManager::~DecodeManager()
{
  logStats();

  while (!threads.empty()) {
    delete threads.back();
    threads.pop_back();
  }

  while (!freeBuffers.empty()) {
    delete freeBuffers.back();
    freeBuffers.pop_back();
  }

  delete consumerCond;
  delete producerCond;
  delete queueMutex;

  for (Decoder* decoder : decoders)
    delete decoder;
}

bool DecodeManager::decodeRect(const core::Rect& r, int encoding,
                               ModifiablePixelBuffer* pb)
{
  Decoder *decoder;
  rdr::MemOutStream *bufferStream;
  int equiv;

  QueueEntry *entry;

  assert(pb != nullptr);

  if (!Decoder::supported(encoding)) {
    vlog.error("Unknown encoding %d", encoding);
    throw protocol_error("Unknown encoding");
  }

  if (!decoders[encoding]) {
    decoders[encoding] = Decoder::createDecoder(encoding);
    if (!decoders[encoding]) {
      vlog.error("Unknown encoding %d", encoding);
      throw protocol_error("Unknown encoding");
    }
  }

  decoder = decoders[encoding];

  // Wait for an available memory buffer
  queueMutex->lock();

  // FIXME: Should we return and let other things run here?
  while (freeBuffers.empty())
    producerCond->wait();

  // Don't pop the buffer in case we throw an exception
  // whilst reading
  bufferStream = freeBuffers.front();

  queueMutex->unlock();

  // First check if any thread has encountered a problem
  throwThreadException();

  // Read the rect
  bufferStream->clear();
  if (!decoder->readRect(r, conn->getInStream(), conn->server, bufferStream))
    return false;

  stats[encoding].rects++;
  stats[encoding].bytes += 12 + bufferStream->length();
  stats[encoding].pixels += r.area();
  equiv = 12 + r.area() * (conn->server.pf().bpp/8);
  stats[encoding].equivalent += equiv;

  // Then try to put it on the queue
  entry = new QueueEntry;

  entry->active = false;
  entry->rect = r;
  entry->encoding = encoding;
  entry->decoder = decoder;
  entry->server = &conn->server;
  entry->pb = pb;
  entry->bufferStream = bufferStream;

  decoder->getAffectedRegion(r, bufferStream->data(),
                             bufferStream->length(), conn->server,
                             &entry->affectedRegion);

  queueMutex->lock();

  // The workers add buffers to the end so it's safe to assume
  // the front is still the same buffer
  freeBuffers.pop_front();

  workQueue.push_back(entry);

  // We only put a single entry on the queue so waking a single
  // thread is sufficient
  consumerCond->signal();

  queueMutex->unlock();

  return true;
}

void DecodeManager::flush()
{
  queueMutex->lock();

  while (!workQueue.empty())
    producerCond->wait();

  queueMutex->unlock();

  throwThreadException();
}

void DecodeManager::logStats()
{
  size_t i;

  unsigned rects;
  unsigned long long pixels, bytes, equivalent;

  double ratio;

  rects = 0;
  pixels = bytes = equivalent = 0;

  for (i = 0;i < (sizeof(stats)/sizeof(stats[0]));i++) {
    // Did this class do anything at all?
    if (stats[i].rects == 0)
      continue;

    rects += stats[i].rects;
    pixels += stats[i].pixels;
    bytes += stats[i].bytes;
    equivalent += stats[i].equivalent;

    ratio = (double)stats[i].equivalent / stats[i].bytes;

    vlog.info("    %s: %s, %s", encodingName(i),
              core::siPrefix(stats[i].rects, "rects").c_str(),
              core::siPrefix(stats[i].pixels, "pixels").c_str());
    vlog.info("    %*s  %s (1:%g ratio)",
              (int)strlen(encodingName(i)), "",
              core::iecPrefix(stats[i].bytes, "B").c_str(), ratio);
  }

  ratio = (double)equivalent / bytes;

  vlog.info("  Total: %s, %s",
            core::siPrefix(rects, "rects").c_str(),
            core::siPrefix(pixels, "pixels").c_str());
  vlog.info("         %s (1:%g ratio)",
            core::iecPrefix(bytes, "B").c_str(), ratio);
}

void DecodeManager::setThreadException()
{
  core::AutoMutex a(queueMutex);

  if (threadException)
    return;

  threadException = std::current_exception();
}

void DecodeManager::throwThreadException()
{
  core::AutoMutex a(queueMutex);

  if (!threadException)
    return;

  try {
    std::rethrow_exception(threadException);
  } catch (...) {
    threadException = nullptr;
    throw;
  }
}

DecodeManager::DecodeThread::DecodeThread(DecodeManager* manager_)
  : manager(manager_), stopRequested(false)
{
  start();
}

DecodeManager::DecodeThread::~DecodeThread()
{
  stop();
  wait();
}

void DecodeManager::DecodeThread::stop()
{
  core::AutoMutex a(manager->queueMutex);

  if (!isRunning())
    return;

  stopRequested = true;

  // We can't wake just this thread, so wake everyone
  manager->consumerCond->broadcast();
}

void DecodeManager::DecodeThread::worker()
{
  manager->queueMutex->lock();

  while (!stopRequested) {
    DecodeManager::QueueEntry *entry;

    // Look for an available entry in the work queue
    entry = findEntry();
    if (entry == nullptr) {
      // Wait and try again
      manager->consumerCond->wait();
      continue;
    }

    // This is ours now
    entry->active = true;

    manager->queueMutex->unlock();

    // Do the actual decoding
    try {
      entry->decoder->decodeRect(entry->rect, entry->bufferStream->data(),
                                 entry->bufferStream->length(),
                                 *entry->server, entry->pb);
    } catch (std::exception& e) {
      manager->setThreadException();
    } catch(...) {
      assert(false);
    }

    manager->queueMutex->lock();

    // Remove the entry from the queue and give back the memory buffer
    manager->freeBuffers.push_back(entry->bufferStream);
    manager->workQueue.remove(entry);
    delete entry;

    // Wake the main thread in case it is waiting for a memory buffer
    manager->producerCond->signal();
    // This rect might have been blocking multiple other rects, so
    // wake up every worker thread
    if (manager->workQueue.size() > 1)
      manager->consumerCond->broadcast();
  }

  manager->queueMutex->unlock();
}

DecodeManager::QueueEntry* DecodeManager::DecodeThread::findEntry()
{
  core::Region lockedRegion;

  if (manager->workQueue.empty())
    return nullptr;

  if (!manager->workQueue.front()->active)
    return manager->workQueue.front();

  for (DecodeManager::QueueEntry* entry : manager->workQueue) {
    // Another thread working on this?
    if (entry->active)
      goto next;

    // If this is an ordered decoder then make sure this is the first
    // rectangle in the queue for that decoder
    if (entry->decoder->flags & DecoderOrdered) {
      for (DecodeManager::QueueEntry* entry2 : manager->workQueue) {
        if (entry2 == entry)
          break;
        if (entry->encoding == entry2->encoding)
          goto next;
      }
    }

    // For a partially ordered decoder we must ask the decoder for each
    // pair of rectangles.
    if (entry->decoder->flags & DecoderPartiallyOrdered) {
      for (DecodeManager::QueueEntry* entry2 : manager->workQueue) {
        if (entry2 == entry)
          break;
        if (entry->encoding != entry2->encoding)
          continue;
        if (entry->decoder->doRectsConflict(entry->rect,
                                            entry->bufferStream->data(),
                                            entry->bufferStream->length(),
                                            entry2->rect,
                                            entry2->bufferStream->data(),
                                            entry2->bufferStream->length(),
                                            *entry->server))
          goto next;
      }
    }

    // Check overlap with earlier rectangles
    if (!lockedRegion.intersect(entry->affectedRegion).is_empty())
      goto next;

    return entry;

next:
    lockedRegion.assign_union(entry->affectedRegion);
  }

  return nullptr;
}
