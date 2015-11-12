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

#include <assert.h>
#include <string.h>

#include <rfb/CConnection.h>
#include <rfb/DecodeManager.h>
#include <rfb/Decoder.h>

#include <rfb/LogWriter.h>

#include <rdr/Exception.h>
#include <rdr/MemOutStream.h>

#include <os/Mutex.h>

using namespace rfb;

static LogWriter vlog("DecodeManager");

DecodeManager::DecodeManager(CConnection *conn) :
  conn(conn)
{
  int i;

  memset(decoders, 0, sizeof(decoders));

  queueMutex = new os::Mutex();
  producerCond = new os::Condition(queueMutex);
  consumerCond = new os::Condition(queueMutex);

  // Just a single thread for now as we haven't sorted out the
  // dependencies between rects
  for (i = 0;i < 1;i++) {
    // Twice as many possible entries in the queue as there
    // are worker threads to make sure they don't stall
    freeBuffers.push_back(new rdr::MemOutStream());
    freeBuffers.push_back(new rdr::MemOutStream());

    threads.push_back(new DecodeThread(this));
  }
}

DecodeManager::~DecodeManager()
{
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

  for (size_t i = 0; i < sizeof(decoders)/sizeof(decoders[0]); i++)
    delete decoders[i];
}

void DecodeManager::decodeRect(const Rect& r, int encoding,
                               ModifiablePixelBuffer* pb)
{
  Decoder *decoder;
  rdr::MemOutStream *bufferStream;

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

  while (freeBuffers.empty())
    producerCond->wait();

  // Don't pop the buffer in case we throw an exception
  // whilst reading
  bufferStream = freeBuffers.front();

  queueMutex->unlock();

  // Read the rect
  bufferStream->clear();
  decoder->readRect(r, conn->getInStream(), conn->cp, bufferStream);

  // Then try to put it on the queue
  entry = new QueueEntry;

  entry->active = false;
  entry->rect = r;
  entry->encoding = encoding;
  entry->decoder = decoder;
  entry->cp = &conn->cp;
  entry->pb = pb;
  entry->bufferStream = bufferStream;

  queueMutex->lock();

  // The workers add buffers to the end so it's safe to assume
  // the front is still the same buffer
  freeBuffers.pop_front();

  workQueue.push_back(entry);

  // We only put a single entry on the queue so waking a single
  // thread is sufficient
  consumerCond->signal();

  queueMutex->unlock();
}

void DecodeManager::flush()
{
  queueMutex->lock();

  while (!workQueue.empty())
    producerCond->wait();

  queueMutex->unlock();
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
                                 *entry->cp, entry->pb);
    } catch(...) {
      // FIXME: Try to get the exception back to the main thread
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

  if (manager->workQueue.empty())
    return NULL;

  if (!manager->workQueue.front()->active)
    return manager->workQueue.front();

  for (iter = manager->workQueue.begin();
       iter != manager->workQueue.end();
       ++iter) {
    // Another thread working on this?
    if ((*iter)->active)
      continue;

    // FIXME: check dependencies between rects

    return *iter;
  }

  return NULL;
}
