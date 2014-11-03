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
  private FdInStream inStream;
  private FdOutStream outStream;

  public SSLEngineManager(SSLEngine sslEngine, FdInStream is,
                          FdOutStream os) throws IOException {

    inStream = is;
    outStream = os;
    engine = sslEngine;

    executor = Executors.newSingleThreadExecutor();

    pktBufSize = engine.getSession().getPacketBufferSize();
    appBufSize = engine.getSession().getApplicationBufferSize();

    myAppData = ByteBuffer.allocate(appBufSize);
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
        switch (res.getStatus()) {
          case BUFFER_UNDERFLOW:
            int len = Math.min(peerNetData.remaining(), inStream.getBufSize());
            int m = inStream.check(1, len, false);
            byte[] buf = new byte[m];
            inStream.readBytes(buf, 0, m);
            peerNetData.put(buf, 0, m);
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
    
      case NEED_WRAP :
        // Empty the local network packet buffer.
        myNetData.clear();
    
        // Generate handshaking data
        res = engine.wrap(myAppData, myNetData);
        hs = res.getHandshakeStatus();
    
        // Check status
        switch (res.getStatus()) {
          case OK :
            myAppData.compact();
            myNetData.flip();
            int n = myNetData.remaining();
            byte[] b = new byte[n];
            myNetData.get(b);
            myNetData.clear();
            outStream.writeBytes(b, 0, n);
            outStream.flush();
            break;
    
          case BUFFER_OVERFLOW:
            // FIXME: How much larger should the buffer be?
            // fallthrough
    
          case CLOSED:
            engine.closeOutbound();
            break;
    
        }
        break;
      case NEED_TASK :
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
    int len = Math.min(pktBufSize,inStream.getBufSize());
    int bytesRead = inStream.check(1,len,false);
    byte[] buf = new byte[bytesRead];
    inStream.readBytes(buf, 0, bytesRead);
    if (peerNetData.remaining() < bytesRead) {
      peerNetData.flip();
      ByteBuffer b = ByteBuffer.allocate(peerNetData.remaining() + bytesRead);
      b.put(peerNetData);
      peerNetData = b;
    }
    peerNetData.put(buf);
    peerNetData.flip();
    SSLEngineResult res = engine.unwrap(peerNetData, peerAppData);
    peerNetData.compact();
    switch (res.getStatus()) {
      case OK :
        peerAppData.flip();
        peerAppData.get(data, dataPtr, res.bytesProduced());
        peerAppData.compact();
        break;

      case BUFFER_UNDERFLOW:
        // normal (need more net data)
        break;

      case CLOSED:
        engine.closeInbound();
        break;

    }
    return res.bytesProduced();
  }

  public int write(byte[] data, int dataPtr, int length) throws IOException {
    int n = 0;
    // FIXME: resize myAppData if necessary
    myAppData.put(data, dataPtr, length);
    myAppData.flip();
    while (myAppData.hasRemaining()) {
      SSLEngineResult res = engine.wrap(myAppData, myNetData);
      n += res.bytesConsumed();
      switch (res.getStatus()) {
        case BUFFER_OVERFLOW:
          ByteBuffer b = ByteBuffer.allocate(myNetData.capacity() + myAppData.remaining());
          myNetData.flip();
          b.put(myNetData);
          myNetData = b;
          break;
        case CLOSED:
          engine.closeOutbound();
          break;
      }
    }
    myAppData.clear();
    myNetData.flip();
    int len = myNetData.remaining();
    byte[] buf = new byte[len];
    myNetData.get(buf);
    myNetData.clear();
    outStream.writeBytes(buf, 0, len);
    outStream.flush();
    return n;
  }

  public SSLSession getSession() {
    return engine.getSession();
  }

}
