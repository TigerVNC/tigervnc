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

  private ByteBuffer myNetData;
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

    int pktBufSize = engine.getSession().getPacketBufferSize();
    myNetData = ByteBuffer.allocate(pktBufSize);
    peerNetData = ByteBuffer.allocate(pktBufSize);
  }

  public void doHandshake() throws Exception {

    // Begin handshake
    engine.beginHandshake();
    SSLEngineResult.HandshakeStatus hs = engine.getHandshakeStatus();

    // Process handshaking message
    SSLEngineResult res = null;
    int appBufSize = engine.getSession().getApplicationBufferSize();
    ByteBuffer peerAppData = ByteBuffer.allocate(appBufSize);
    ByteBuffer myAppData = ByteBuffer.allocate(appBufSize);
    while (hs != SSLEngineResult.HandshakeStatus.FINISHED &&
           hs != SSLEngineResult.HandshakeStatus.NOT_HANDSHAKING) {

      switch (hs) {

      case NEED_UNWRAP:
        // Receive handshaking data from peer
        peerNetData.flip();
        res = engine.unwrap(peerNetData, peerAppData);
        peerNetData.compact();
        hs = res.getHandshakeStatus();

        // Check status
        switch (res.getStatus()) {
          case BUFFER_UNDERFLOW:
            int avail = in.check(1, peerNetData.remaining(), false);
            in.readBytes(peerNetData, avail);
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
        // Generate handshaking data
        res = engine.wrap(myAppData, myNetData);
        hs = res.getHandshakeStatus();

        // Check status
        switch (res.getStatus()) {
          case OK:
            myNetData.flip();
            os.writeBytes(myNetData, myNetData.remaining());
            os.flush();
            myNetData.compact();
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

  public int read(ByteBuffer data, int length) throws IOException {
    // Read SSL/TLS encoded data from peer
    peerNetData.flip();
    SSLEngineResult res = engine.unwrap(peerNetData, data);
    peerNetData.compact();
    switch (res.getStatus()) {
      case OK :
        return res.bytesProduced();

      case BUFFER_UNDERFLOW:
        // attempt to drain the underlying buffer first
        int need = peerNetData.remaining();
        in.readBytes(peerNetData, in.check(1, need, true));
        break;

      case CLOSED:
        engine.closeInbound();
        break;

    }
    return 0;
  }

  public int write(ByteBuffer data, int length) throws IOException {
    int n = 0;
    while (data.hasRemaining()) {
      SSLEngineResult res = engine.wrap(data, myNetData);
      n += res.bytesConsumed();
      switch (res.getStatus()) {
        case OK:
          myNetData.flip();
          os.writeBytes(myNetData, myNetData.remaining());
          os.flush();
          myNetData.compact();
          break;

        case BUFFER_OVERFLOW:
          // Make room in the buffer by flushing the outstream
          myNetData.flip();
          os.writeBytes(myNetData, myNetData.remaining());
          myNetData.compact();
          break;

        case CLOSED:
          engine.closeOutbound();
          break;
      }
    }
    return n;
  }

  public SSLSession getSession() {
    return engine.getSession();
  }

}
