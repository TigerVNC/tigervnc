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
#include <core/Region.h>
#include <core/i18n.h>
#include <core/string.h>

#include <rfb/CConnection.h>
#include <rfb/DecodeManager.h>
#include <rfb/Decoder.h>
#include <rfb/Exception.h>

#include <rdr/MemOutStream.h>

using namespace rfb;

static core::LogWriter vlog("DecodeManager");

DecodeManager::DecodeManager(CConnection *conn_) :
  conn(conn_), partialEntry(nullptr), threadException(nullptr)
{
  size_t cpuCount;

  memset(decoders, 0, sizeof(decoders));

  memset(stats, 0, sizeof(stats));

  cpuCount = std::thread::hardware_concurrency();
  if (cpuCount == 0) {
    vlog.error(_("Unable to determine the number of CPU cores on this system"));
    cpuCount = 1;
  } else {
    vlog.info(_("Detected %d CPU core(s)"), (int)cpuCount);
    // No point creating more threads than this, they'll just end up
    // wasting CPU fighting for locks
    if (cpuCount > 4)
      cpuCount = 4;
  }

  vlog.info(_("Creating %d decoder thread(s)"), (int)cpuCount);

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

  for (Decoder* decoder : decoders)
    delete decoder;

  delete partialEntry;
}

bool DecodeManager::decodeRect(const core::Rect& r, int encoding,
                               ModifiablePixelBuffer* pb)
{
  int equiv;

  assert(pb != nullptr);

  if (partialEntry == nullptr) {
    Decoder *decoder;
    rdr::MemOutStream *bufferStream;

    if (!Decoder::supported(encoding)) {
      vlog.error(_("Unknown encoding %d"), encoding);
      throw protocol_error("Unknown encoding");
    }

    if (!decoders[encoding]) {
      decoders[encoding] = Decoder::createDecoder(encoding);
      if (!decoders[encoding]) {
        vlog.error(_("Unknown encoding %d"), encoding);
        throw protocol_error("Unknown encoding");
      }
    }

    decoder = decoders[encoding];

    // Wait for an available memory buffer
    std::unique_lock<std::mutex> lock(queueMutex);

    // FIXME: Should we return and let other things run here?
    while (freeBuffers.empty())
      producerCond.wait(lock);

    // Don't pop the buffer as we don't know if we'll finish filling it
    // in a single round
    bufferStream = freeBuffers.front();
    bufferStream->clear();

    lock.unlock();

    partialEntry = new QueueEntry();

    partialEntry->active = false;
    partialEntry->rect = r;
    partialEntry->encoding = encoding;
    partialEntry->decoder = decoder;
    partialEntry->server = &conn->server;
    partialEntry->pb = pb;
    partialEntry->bufferStream = bufferStream;

    beforePos = conn->getInStream()->pos();
  } else {
    assert(partialEntry->rect == r);
    assert(partialEntry->encoding == encoding);
    assert(partialEntry->pb == pb);
  }

  // First check if any thread has encountered a problem
  throwThreadException();

  // Read the rect
  if (!partialEntry->decoder->readRect(
        partialEntry->rect, conn->getInStream(), conn->server,
        partialEntry->bufferStream))
    return false;

  partialEntry->decoder->getAffectedRegion(
    r, partialEntry->bufferStream->data(),
    partialEntry->bufferStream->length(), conn->server,
    &partialEntry->affectedRegion);

  stats[encoding].rects++;
  stats[encoding].bytes += 12 + conn->getInStream()->pos() - beforePos;
  stats[encoding].pixels += r.area();
  equiv = 12 + r.area() * (conn->server.pf().bpp/8);
  stats[encoding].equivalent += equiv;

  // Then try to put it on the queue

  std::unique_lock<std::mutex> lock(queueMutex);

  // The workers add buffers to the end so it's safe to assume
  // the front is still the same buffer
  freeBuffers.pop_front();

  workQueue.push_back(partialEntry);
  partialEntry = nullptr;

  // We only put a single entry on the queue so waking a single
  // thread is sufficient
  consumerCond.notify_one();

  lock.unlock();

  return true;
}

void DecodeManager::flush()
{
  std::unique_lock<std::mutex> lock(queueMutex);

  while (!workQueue.empty())
    producerCond.wait(lock);

  lock.unlock();

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
              // TRANSLATORS: Will get a SI prefix before (k/M/G/...)
              core::siPrefix(stats[i].rects, _("rects")).c_str(),
              // TRANSLATORS: Will get a SI prefix before (k/M/G/...)
              core::siPrefix(stats[i].pixels, _("pixels")).c_str());
    vlog.info("    %*s  %s (1:%g %s)",
              (int)strlen(encodingName(i)), "",
              // TRANSLATORS: Short form of bytes
              core::iecPrefix(stats[i].bytes, _("B")).c_str(),
              ratio, _("ratio"));
  }

  ratio = (double)equivalent / bytes;

  vlog.info("  %s %s, %s", _("Total:"),
            // TRANSLATORS: Will get a SI prefix before (k/M/G/...)
            core::siPrefix(rects, _("rects")).c_str(),
            // TRANSLATORS: Will get a SI prefix before (k/M/G/...)
            core::siPrefix(pixels, _("pixels")).c_str());
  vlog.info("  %*s %s (1:%g %s)",
            (int)strlen(_("Total:")), "",
            // TRANSLATORS: Short form of bytes
            core::iecPrefix(bytes, _("B")).c_str(),
            ratio, _("ratio"));
}

void DecodeManager::setThreadException()
{
  const std::lock_guard<std::mutex> lock(queueMutex);

  if (threadException)
    return;

  threadException = std::current_exception();
}

void DecodeManager::throwThreadException()
{
  const std::lock_guard<std::mutex> lock(queueMutex);

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
  : manager(manager_), thread(nullptr), stopRequested(false)
{
  start();
}

DecodeManager::DecodeThread::~DecodeThread()
{
  stop();
  if (thread != nullptr) {
    thread->join();
    delete thread;
  }
}

void DecodeManager::DecodeThread::start()
{
  assert(thread == nullptr);

  thread = new std::thread(&DecodeThread::worker, this);
}

void DecodeManager::DecodeThread::stop()
{
  const std::lock_guard<std::mutex> lock(manager->queueMutex);

  if (thread == nullptr)
    return;

  stopRequested = true;

  // We can't wake just this thread, so wake everyone
  manager->consumerCond.notify_all();
}

void DecodeManager::DecodeThread::worker()
{
  std::unique_lock<std::mutex> lock(manager->queueMutex);

  while (!stopRequested) {
    DecodeManager::QueueEntry *entry;

    // Look for an available entry in the work queue
    entry = findEntry();
    if (entry == nullptr) {
      // Wait and try again
      manager->consumerCond.wait(lock);
      continue;
    }

    // This is ours now
    entry->active = true;

    lock.unlock();

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

    lock.lock();

    // Remove the entry from the queue and give back the memory buffer
    manager->freeBuffers.push_back(entry->bufferStream);
    manager->workQueue.remove(entry);
    delete entry;

    // Wake the main thread in case it is waiting for a memory buffer
    manager->producerCond.notify_one();
    // This rect might have been blocking multiple other rects, so
    // wake up every worker thread
    if (manager->workQueue.size() > 1)
      manager->consumerCond.notify_all();
  }
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
