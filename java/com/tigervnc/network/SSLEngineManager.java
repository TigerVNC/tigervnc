/* Copyright (C) 2012,2014 Brian P. Hinz
 * Copyright (C) 2012 D. R. Commander.  All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, 5th Floor, Boston, MA 02110-1301 USA
 */
 package com.tigervnc.network;

import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import javax.net.ssl.*;
import javax.net.ssl.SSLEngineResult.*;
import java.util.concurrent.Executor;
import java.util.concurrent.Executors;

import com.tigervnc.rdr.FdInStream;
import com.tigervnc.rdr.FdOutStream;

public class SSLEngineManager {

  private SSLEngine engine = null;

  private int appBufSize;
  private int pktBufSize;

  private ByteBuffer myAppData;
  private ByteBuffer myNetData;
  private ByteBuffer peerAppData;
  private ByteBuffer peerNetData;

  private Executor executor;
  private FdInStream in;
  private FdOutStream os;

  public SSLEngineManager(SSLEngine sslEngine, FdInStream is_,
                          FdOutStream os_) throws IOException {

    in = is_;
    os = os_;
    engine = sslEngine;

    executor = Executors.newSingleThreadExecutor();

    pktBufSize = engine.getSession().getPacketBufferSize();
    appBufSize = engine.getSession().getApplicationBufferSize();

    myAppData =
      ByteBuffer.allocate(Math.max(appBufSize, os.getBufSize()));
    myNetData = ByteBuffer.allocate(pktBufSize);
    peerAppData = ByteBuffer.allocate(appBufSize);
    peerNetData = ByteBuffer.allocate(pktBufSize);
  }

  public void doHandshake() throws Exception {

    // Begin handshake
    engine.beginHandshake();
    SSLEngineResult.HandshakeStatus hs = engine.getHandshakeStatus();

    // Process handshaking message
    while (hs != SSLEngineResult.HandshakeStatus.FINISHED &&
           hs != SSLEngineResult.HandshakeStatus.NOT_HANDSHAKING) {

      switch (hs) {

      case NEED_UNWRAP:
        // Receive handshaking data from peer
        peerNetData.flip();
        SSLEngineResult res = engine.unwrap(peerNetData, peerAppData);
        peerNetData.compact();
        hs = res.getHandshakeStatus();
        // Check status
        switch (res.getStatus()) {
          case BUFFER_UNDERFLOW:
            int max = Math.min(peerNetData.remaining(), in.getBufSize());
            int m = in.check(1, max, true);
            int pos = peerNetData.position();
            in.readBytes(peerNetData.array(), pos, m);
            peerNetData.position(pos+m);
            peerNetData.flip();
            peerNetData.compact();
            break;

          case OK:
            // Process incoming handshaking data
            break;

          case CLOSED:
            engine.closeInbound();
            break;

        }
        break;

      case NEED_WRAP:
        // Empty the local network packet buffer.
        myNetData.clear();

        // Generate handshaking data
        res = engine.wrap(myAppData, myNetData);
        hs = res.getHandshakeStatus();

        // Check status
        switch (res.getStatus()) {
          case OK:
            myAppData.compact();
            myNetData.flip();
            os.writeBytes(myNetData.array(), 0, myNetData.remaining());
            os.flush();
            myNetData.clear();
            break;

          case BUFFER_OVERFLOW:
            // FIXME: How much larger should the buffer be?
            break;

          case CLOSED:
            engine.closeOutbound();
            break;

        }
        break;

      case NEED_TASK:
        // Handle blocking tasks
        executeTasks();
        break;
      }
      hs = engine.getHandshakeStatus();
    }
  }

  private void executeTasks() {
    Runnable task;
    while ((task = engine.getDelegatedTask()) != null) {
      executor.execute(task);
    }
  }

  public int read(byte[] data, int dataPtr, int length) throws IOException {
    // Read SSL/TLS encoded data from peer
    int bytesRead = 0;
    peerNetData.flip();
    SSLEngineResult res = engine.unwrap(peerNetData, peerAppData);
    peerNetData.compact();
    switch (res.getStatus()) {
      case OK :
        bytesRead = Math.min(length, res.bytesProduced());
        peerAppData.flip();
        peerAppData.get(data, dataPtr, bytesRead);
        peerAppData.compact();
        break;

      case BUFFER_UNDERFLOW:
        // need more net data
        int pos = peerNetData.position();
        // attempt to drain the underlying buffer first
        int need = peerNetData.remaining();
        int avail = in.check(1, in.getBufSize(), false);
        if (avail < need)
          avail = in.check(1, Math.min(need, in.getBufSize()), true);
        in.readBytes(peerNetData.array(), pos, Math.min(need, avail));
        peerNetData.position(pos+Math.min(need, avail));
        break;

      case CLOSED:
        engine.closeInbound();
        break;

    }
    return bytesRead;
  }

  public int write(byte[] data, int dataPtr, int length) throws IOException {
    int n = 0;
    myAppData.put(data, dataPtr, length);
    myAppData.flip();
    while (myAppData.hasRemaining()) {
      SSLEngineResult res = engine.wrap(myAppData, myNetData);
      n += res.bytesConsumed();
      switch (res.getStatus()) {
        case OK:
          break;

        case BUFFER_OVERFLOW:
          // Make room in the buffer by flushing the outstream
          myNetData.flip();
          os.writeBytes(myNetData.array(), 0, myNetData.remaining());
          os.flush();
          myNetData.clear();
          break;

        case CLOSED:
          engine.closeOutbound();
          break;
      }
    }
    myAppData.clear();
    myNetData.flip();
    os.writeBytes(myNetData.array(), 0, myNetData.remaining());
    os.flush();
    myNetData.clear();
    return n;
  }

  public SSLSession getSession() {
    return engine.getSession();
  }

}
