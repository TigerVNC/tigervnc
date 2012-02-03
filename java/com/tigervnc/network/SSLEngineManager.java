/*
 * Copyright (c) 2004, 2006, Oracle and/or its affiliates. All rights reserved.
 * Copyright (C) 2008  Trustin Heuiseung Lee
 * Copyright (C) 2012  TigerVNC Team
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

  private int applicationBufferSize;
  private int packetBufferSize;

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

    packetBufferSize = engine.getSession().getPacketBufferSize();
    applicationBufferSize = engine.getSession().getApplicationBufferSize();

    myAppData = ByteBuffer.allocate(applicationBufferSize);
    myNetData = ByteBuffer.allocate(packetBufferSize);
    peerAppData = ByteBuffer.allocate(applicationBufferSize);
    peerNetData = ByteBuffer.allocate(packetBufferSize);
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
            pull(peerNetData);
            //if (pull(peerNetData) < 0) {
                // Handle closed channel
            //}

            // Process incoming handshaking data
            peerNetData.flip();
            SSLEngineResult res = engine.unwrap(peerNetData, peerAppData);
            peerNetData.compact();
            hs = res.getHandshakeStatus();

            // Check status
            switch (res.getStatus()) {
            case OK :
                // Handle OK status
                break;

            // Handle other status: BUFFER_UNDERFLOW, BUFFER_OVERFLOW, CLOSED
            //...
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
                //myNetData.flip();

                push(myNetData);
                // Send the handshaking data to peer
                //while (myNetData.hasRemaining()) {
                //    if (push(myNetData) < 0) {
                //        // Handle closed channel
                //    }
                //}
                break;

            // Handle other status:  BUFFER_OVERFLOW, BUFFER_UNDERFLOW, CLOSED
            //...
            }
            break;

        case NEED_TASK :
            // Handle blocking tasks
            executeTasks();
            break;

        // Handle other status:  // FINISHED or NOT_HANDSHAKING
        //...
        }
      hs = engine.getHandshakeStatus();
    }

    // Processes after handshaking
    //...
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
    SSLEngineResult res;
    do {
      // Process incoming data
      int packetLength = pull(peerNetData);
      peerNetData.flip();
      res = engine.unwrap(peerNetData, peerAppData);
      peerNetData.compact();
      bytesRead += packetLength;
    } while (res.getStatus() != SSLEngineResult.Status.OK);
    peerAppData.flip();
    int n = Math.min(peerAppData.remaining(), length);
    peerAppData.get(data, dataPtr, n);
    peerAppData.compact();
    return n;
  }

  public int write(byte[] data, int dataPtr, int length) throws IOException {
    int n = 0;
//while (myAppData.hasRemaining()) {
    // Generate SSL/TLS encoded data (handshake or application data)
    // FIXME:
    // Need to make sure myAppData has space for data!
    myAppData.put(data, dataPtr, length);
    myAppData.flip();
    SSLEngineResult res = engine.wrap(myAppData, myNetData);
    myAppData.compact();

    // Process status of call
    //if (res.getStatus() == SSLEngineResult.Status.OK) {
        //myAppData.compact();

        // Send SSL/TLS encoded data to peer
        //while(myNetData.hasRemaining()) {
            int num = push(myNetData);
            if (num == -1) {
                // handle closed channel
            } else if (num == 0) {
                // no bytes written; try again later
            }
            //n += num;
        //}
    //}

    // Handle other status:  BUFFER_OVERFLOW, CLOSED
    //...
  //}
   return num;
  }

  private int push(ByteBuffer src) throws IOException {
    src.flip();
    int n = src.remaining();
    byte[] b = new byte[n];
    src.get(b);
    src.clear();
    outStream.writeBytes(b, 0, n);
    outStream.flush();
    return n;
  }

  private int pull(ByteBuffer dst) throws IOException {
    int packetLength = 0;
    inStream.checkNoWait(5);
    //if (!inStream.checkNoWait(5)) {
    //  return 0;
    //}

    byte[] header = new byte[5];
    inStream.readBytes(header, 0, 5);

    boolean tls;
    int h = header[0] & 0xFF;
    switch (header[0] & 0xFF) {
    case 20:  // change_cipher_spec
    case 21:  // alert
    case 22:  // handshake
    case 23:  // application_data
      tls = true;
      break;
    default:
      //  SSLv2 bad data
      tls = false;
    }

    if (tls) {
      int majorVersion = (int)(header[1] & 0xFF);
      if (majorVersion >= 3 && majorVersion < 10) {
        // SSLv3 or TLS
        packetLength = (int)(((header[3] << 8) | (header[4] & 0xFF)) & 0xFFFF) + 5;
        if (packetLength <= 5) {
          // Neither SSLv2 or TLSv1
          tls = false;
        }
      } else {
        // Neither SSLv2 or TLSv1
        tls = false;
      }
    }

    if (!tls) {
      boolean sslv2 = true;
      int headerLength = (int)((header[0] & 0xFF) & 0x80) != 0 ? 2 : 3;
      int majorVersion = (int)(header[headerLength + 1] & 0xFF);
      if (majorVersion >= 2 && majorVersion < 10) {
        // SSLv2
        if (headerLength == 2) {
          packetLength = (int)(((header[0] << 8) | (header[1] & 0xFF)) & 0x7FFF) + 2;
        } else {
          packetLength = (int)(((header[0] << 8) | (header[1] & 0xFF)) & 0x3FFF) + 3;
        }
        if (packetLength <= headerLength) {
          sslv2 = false;
        }
      } else {
        sslv2 = false;
      }

      if (!sslv2) {
        throw new IOException("not an SSL/TLS record");
      }
    }

    assert packetLength > 0;

    byte[] buf = new byte[packetLength - 5];
    inStream.readBytes(buf, 0, packetLength - 5);
    dst.put(header);
    dst.put(buf);
    return packetLength;
  }

  public SSLSession getSession() {
    return engine.getSession();
  }

}
