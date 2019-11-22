/* Copyright 2015 Pierre Ossman for Cendio AB
 * Copyright 2016-2019 Brian P. Hinz
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

package com.tigervnc.rfb;

import java.lang.Runtime;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.locks.*;

import com.tigervnc.rdr.*;
import com.tigervnc.rdr.Exception;

import static com.tigervnc.rfb.Decoder.DecoderFlags.*;

public class DecodeManager {

  static LogWriter vlog = new LogWriter("DecodeManager");

  public DecodeManager(CConnection conn) {
    int cpuCount;

    this.conn = conn; threadException = null;
    decoders = new Decoder[Encodings.encodingMax+1];

    queueMutex = new ReentrantLock();
    producerCond = queueMutex.newCondition();
    consumerCond = queueMutex.newCondition();

    cpuCount = Runtime.getRuntime().availableProcessors();
    if (cpuCount == 0) {
      vlog.error("Unable to determine the number of CPU cores on this system");
      cpuCount = 1;
    } else {
      vlog.info("Detected "+cpuCount+" CPU core(s)");
      // No point creating more threads than this, they'll just end up
      // wasting CPU fighting for locks
      if (cpuCount > 4)
        cpuCount = 4;
      // The overhead of threading is small, but not small enough to
      // ignore on single CPU systems
      if (cpuCount == 1)
        vlog.info("Decoding data on main thread");
      else
        vlog.info("Creating "+cpuCount+" decoder thread(s)");
    }

    freeBuffers = new ArrayDeque<MemOutStream>(cpuCount*2);
    workQueue = new ArrayDeque<QueueEntry>(cpuCount);
    threads = new ArrayList<DecodeThread>(cpuCount);
    while (cpuCount-- > 0) {
      // Twice as many possible entries in the queue as there
      // are worker threads to make sure they don't stall
      try {
      freeBuffers.addLast(new MemOutStream());
      freeBuffers.addLast(new MemOutStream());

      threads.add(new DecodeThread(this));
      } catch (IllegalStateException e) { }
    }

  }

  public void decodeRect(Rect r, int encoding,
                         ModifiablePixelBuffer pb)
  {
    Decoder decoder;
    MemOutStream bufferStream;

    QueueEntry entry;

    assert(pb != null);

    if (!Decoder.supported(encoding)) {
      vlog.error("Unknown encoding " + encoding);
      throw new Exception("Unknown encoding");
    }

    if (decoders[encoding] == null) {
      decoders[encoding] = Decoder.createDecoder(encoding);
      if (decoders[encoding] == null) {
        vlog.error("Unknown encoding " + encoding);
        throw new Exception("Unknown encoding");
      }
    }

    decoder = decoders[encoding];

    // Fast path for single CPU machines to avoid the context
    // switching overhead
    if (threads.size() == 1) {
      bufferStream = freeBuffers.getFirst();
      bufferStream.clear();
      decoder.readRect(r, conn.getInStream(), conn.server, bufferStream);
      decoder.decodeRect(r, (Object)bufferStream.data(), bufferStream.length(),
                         conn.server, pb);
      return;
    }

    // Wait for an available memory buffer
    queueMutex.lock();

    try {
      while (freeBuffers.isEmpty())
        try {
        producerCond.await();
        } catch (InterruptedException e) { }

      // Don't pop the buffer in case we throw an exception
      // whilst reading
      bufferStream = freeBuffers.getFirst();
    } finally {
      queueMutex.unlock();
    }

    // First check if any thread has encountered a problem
    throwThreadException();

    // Read the rect
    bufferStream.clear();
    decoder.readRect(r, conn.getInStream(), conn.server, bufferStream);

    // Then try to put it on the queue
    entry = new QueueEntry();

    entry.active = false;
    entry.rect = r;
    entry.encoding = encoding;
    entry.decoder = decoder;
    entry.server = conn.server;
    entry.pb = pb;
    entry.bufferStream = bufferStream;

    decoder.getAffectedRegion(r, bufferStream.data(),
                              bufferStream.length(), conn.server,
                              entry.affectedRegion);

    queueMutex.lock();

    try {
      // The workers add buffers to the end so it's safe to assume
      // the front is still the same buffer
      freeBuffers.removeFirst();

      workQueue.addLast(entry);

      // We only put a single entry on the queue so waking a single
      // thread is sufficient
      consumerCond.signal();
    } finally {
      queueMutex.unlock();
    }
  }

  public void flush()
  {
    queueMutex.lock();

    try {
      while (!workQueue.isEmpty())
        try {
        producerCond.await();
        } catch (InterruptedException e) { }
    } finally {
      queueMutex.unlock();
    }

    throwThreadException();
  }

  private void setThreadException(Exception e)
  {
    //os::AutoMutex a(queueMutex);
    queueMutex.lock();

    try {
      if (threadException != null)
        return;

      threadException =
        new Exception("Exception on worker thread: "+e.getMessage());
    } finally {
      queueMutex.unlock();
    }
  }

  private void throwThreadException()
  {
    //os::AutoMutex a(queueMutex);
    queueMutex.lock();

    try {
      if (threadException == null)
        return;

      Exception e = new Exception(threadException.getMessage());

      threadException = null;

      throw e;
    } finally {
      queueMutex.unlock();
    }
  }

  private class QueueEntry {

    public QueueEntry() {
      affectedRegion = new Region();
    }
    public boolean active;
    public Rect rect;
    public int encoding;
    public Decoder decoder;
    public ServerParams server;
    public ModifiablePixelBuffer pb;
    public MemOutStream bufferStream;
    public Region affectedRegion;
  }

  private class DecodeThread implements Runnable {

    public DecodeThread(DecodeManager manager)
    {
      this.manager = manager;

      stopRequested = false;

      (thread = new Thread(this, "Decoder Thread")).start();
    }

    public void stop()
    {
      //os::AutoMutex a(manager.queueMutex);
      manager.queueMutex.lock();

      try {
        if (!thread.isAlive())
          return;

        stopRequested = true;

        // We can't wake just this thread, so wake everyone
        manager.consumerCond.signalAll();
      } finally {
        manager.queueMutex.unlock();
      }
    }

    public void run()
    {
      manager.queueMutex.lock();

      while (!stopRequested) {
        QueueEntry entry;

        // Look for an available entry in the work queue
        entry = findEntry();
        if (entry == null) {
          // Wait and try again
          try {
            manager.consumerCond.await();
          } catch (InterruptedException e) { }
          continue;
        }

        // This is ours now
        entry.active = true;

        manager.queueMutex.unlock();

        // Do the actual decoding
        try {
          entry.decoder.decodeRect(entry.rect, entry.bufferStream.data(),
                                   entry.bufferStream.length(),
                                   entry.server, entry.pb);
        } catch (com.tigervnc.rdr.Exception e) {
          manager.setThreadException(e);
        } catch(java.lang.Exception e) {
          assert(false);
        }

        manager.queueMutex.lock();

        // Remove the entry from the queue and give back the memory buffer
        manager.freeBuffers.addLast(entry.bufferStream);
        manager.workQueue.remove(entry);
        entry = null;

        // Wake the main thread in case it is waiting for a memory buffer
        manager.producerCond.signal();
        // This rect might have been blocking multiple other rects, so
        // wake up every worker thread
        if (manager.workQueue.size() > 1)
          manager.consumerCond.signalAll();
      }

      manager.queueMutex.unlock();
    }

    protected QueueEntry findEntry()
    {
      Iterator<QueueEntry> iter;
      Region lockedRegion = new Region();

      if (manager.workQueue.isEmpty())
        return null;

      if (!manager.workQueue.peek().active)
        return manager.workQueue.peek();

      next:for (iter = manager.workQueue.iterator(); iter.hasNext();) {
        QueueEntry entry, entry2;

        Iterator<QueueEntry> iter2;

        entry = iter.next();

        // Another thread working on this?
        if (entry.active) {
          lockedRegion.assign_union(entry.affectedRegion);
          continue next;
        }

        // If this is an ordered decoder then make sure this is the first
        // rectangle in the queue for that decoder
        if ((entry.decoder.flags & DecoderOrdered) != 0) {
          for (iter2 = manager.workQueue.iterator(); iter2.hasNext() &&
               !(entry2 = iter2.next()).equals(entry);) {
            if (entry.encoding == entry2.encoding) {
              lockedRegion.assign_union(entry.affectedRegion);
              continue next;
            }
          }
        }

        // For a partially ordered decoder we must ask the decoder for each
        // pair of rectangles.
        if ((entry.decoder.flags & DecoderPartiallyOrdered) != 0) {
          for (iter2 = manager.workQueue.iterator(); iter2.hasNext() &&
               !(entry2 = iter2.next()).equals(entry);) {
            if (entry.encoding != entry2.encoding)
              continue;
            if (entry.decoder.doRectsConflict(entry.rect,
                                              entry.bufferStream.data(),
                                              entry.bufferStream.length(),
                                              entry2.rect,
                                              entry2.bufferStream.data(),
                                              entry2.bufferStream.length(),
                                              entry.server))
              lockedRegion.assign_union(entry.affectedRegion);
              continue next;
          }
        }

        // Check overlap with earlier rectangles
        if (!lockedRegion.intersect(entry.affectedRegion).is_empty()) {
          lockedRegion.assign_union(entry.affectedRegion);
          continue next;
        }

        return entry;

      }

      return null;
    }

    private DecodeManager manager;
    private boolean stopRequested;

    private Thread thread;

  }

  private CConnection conn;
  private Decoder[] decoders;

  private ArrayDeque<MemOutStream> freeBuffers;
  private ArrayDeque<QueueEntry> workQueue;

  private ReentrantLock queueMutex;
  private Condition producerCond;
  private Condition consumerCond;

  private List<DecodeThread> threads;
  private com.tigervnc.rdr.Exception threadException;

}
