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

#include <rfb/CConnection.h>
#include <rfb/DecodeManager.h>
#include <rfb/Decoder.h>
#include <rfb/Exception.h>
#include <rfb/Region.h>

#include <rfb/LogWriter.h>

#include <rdr/Exception.h>
#include <rdr/MemOutStream.h>

#include <os/Mutex.h>

using namespace rfb;

static LogWriter vlog("DecodeManager");

DecodeManager::DecodeManager(CConnection *conn) :
  conn(conn), threadException(NULL)
{
  size_t cpuCount;

  memset(decoders, 0, sizeof(decoders));

  memset(stats, 0, sizeof(stats));

  queueMutex = new os::Mutex();
  producerCond = new os::Condition(queueMutex);
  consumerCond = new os::Condition(queueMutex);

  cpuCount = os::Thread::getSystemCPUCount();
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

  delete threadException;

  while (!freeBuffers.empty()) {
    delete freeBuffers.back();
    freeBuffers.pop_back();
  }

  delete consumerCond;
  delete producerCond;
  delete queueMutex;

  for (size_t i = 0; i < sizeof(decoders)/sizeof(decoders[0]); i++)
    delete decoders[i];
}

bool DecodeManager::decodeRect(const Rect& r, int encoding,
                               ModifiablePixelBuffer* pb)
{
  Decoder *decoder;
  rdr::MemOutStream *bufferStream;
  int equiv;

  QueueEntry *entry;

  assert(pb != NULL);

  if (!Decoder::supported(encoding)) {
    vlog.error("Unknown encoding %d", encoding);
    throw rdr::Exception("Unknown encoding");
  }

  if (!decoders[encoding]) {
    decoders[encoding] = Decoder::createDecoder(encoding);
    if (!decoders[encoding]) {
      vlog.error("Unknown encoding %d", encoding);
      throw rdr::Exception("Unknown encoding");
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
  try {
    if (!decoder->readRect(r, conn->getInStream(), conn->server, bufferStream))
      return false;
  } catch (rdr::Exception& e) {
    throw Exception("Error reading rect: %s", e.str());
  }

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

  char a[1024], b[1024];

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

    siPrefix(stats[i].rects, "rects", a, sizeof(a));
    siPrefix(stats[i].pixels, "pixels", b, sizeof(b));
    vlog.info("    %s: %s, %s", encodingName(i), a, b);
    iecPrefix(stats[i].bytes, "B", a, sizeof(a));
    vlog.info("    %*s  %s (1:%g ratio)",
              (int)strlen(encodingName(i)), "",
              a, ratio);
  }

  ratio = (double)equivalent / bytes;

  siPrefix(rects, "rects", a, sizeof(a));
  siPrefix(pixels, "pixels", b, sizeof(b));
  vlog.info("  Total: %s, %s", a, b);
  iecPrefix(bytes, "B", a, sizeof(a));
  vlog.info("         %s (1:%g ratio)", a, ratio);
}

void DecodeManager::setThreadException(const rdr::Exception& e)
{
  os::AutoMutex a(queueMutex);

  if (threadException != NULL)
    return;

  threadException = new rdr::Exception("Exception on worker thread: %s", e.str());
}

void DecodeManager::throwThreadException()
{
  os::AutoMutex a(queueMutex);

  if (threadException == NULL)
    return;

  rdr::Exception e(*threadException);

  delete threadException;
  threadException = NULL;

  throw e;
}

DecodeManager::DecodeThread::DecodeThread(DecodeManager* manager)
{
  this->manager = manager;

  stopRequested = false;

  start();
}

DecodeManager::DecodeThread::~DecodeThread()
{
  stop();
  wait();
}

void DecodeManager::DecodeThread::stop()
{
  os::AutoMutex a(manager->queueMutex);

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
    if (entry == NULL) {
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
    } catch (rdr::Exception& e) {
      manager->setThreadException(e);
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
  std::list<DecodeManager::QueueEntry*>::iterator iter;
  Region lockedRegion;

  if (manager->workQueue.empty())
    return NULL;

  if (!manager->workQueue.front()->active)
    return manager->workQueue.front();

  for (iter = manager->workQueue.begin();
       iter != manager->workQueue.end();
       ++iter) {
    DecodeManager::QueueEntry* entry;

    std::list<DecodeManager::QueueEntry*>::iterator iter2;

    entry = *iter;

    // Another thread working on this?
    if (entry->active)
      goto next;

    // If this is an ordered decoder then make sure this is the first
    // rectangle in the queue for that decoder
    if (entry->decoder->flags & DecoderOrdered) {
      for (iter2 = manager->workQueue.begin(); iter2 != iter; ++iter2) {
        if (entry->encoding == (*iter2)->encoding)
          goto next;
      }
    }

    // For a partially ordered decoder we must ask the decoder for each
    // pair of rectangles.
    if (entry->decoder->flags & DecoderPartiallyOrdered) {
      for (iter2 = manager->workQueue.begin(); iter2 != iter; ++iter2) {
        if (entry->encoding != (*iter2)->encoding)
          continue;
        if (entry->decoder->doRectsConflict(entry->rect,
                                            entry->bufferStream->data(),
                                            entry->bufferStream->length(),
                                            (*iter2)->rect,
                                            (*iter2)->bufferStream->data(),
                                            (*iter2)->bufferStream->length(),
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

  return NULL;
}
