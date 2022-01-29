/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2012 Brian P. Hinz
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

package com.tigervnc.network;

import com.tigervnc.rdr.FdInStream;
import com.tigervnc.rdr.FdOutStream;
import com.tigervnc.rdr.Exception;
import com.tigervnc.rfb.LogWriter;

import java.io.IOException;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.net.UnknownHostException;
import java.nio.*;
import java.nio.channels.*;

import java.util.Set;
import java.util.Iterator;

public class TcpSocket extends Socket {

  // -=- Socket initialisation
  public static boolean socketsInitialised = false;
  public static void initSockets() {
    if (socketsInitialised)
      return;
    socketsInitialised = true;
  }

  // -=- TcpSocket

  public TcpSocket(SocketDescriptor sock, boolean close) {
    super(new FdInStream(sock), new FdOutStream(sock), true);
    closeFd = close;
  }

  public TcpSocket(SocketDescriptor sock) {
    this(sock, true);
  }

  public TcpSocket(String host, int port) throws Exception {
    closeFd = true;
    SocketDescriptor sock = null;
    InetAddress addr = null;
    boolean result = false;

    // - Create a socket
    initSockets();

    try {
      addr = java.net.InetAddress.getByName(host);
    } catch(UnknownHostException e) {
      throw new Exception("unable to resolve host by name: "+e.toString());
    }

    try {
      sock = new SocketDescriptor();
    } catch(Exception e) {
      throw new SocketException("unable to create socket: "+e.toString());
    }

    /* Attempt to connect to the remote host */
    try {
      result = sock.connect(new InetSocketAddress(addr, port));
      Selector selector = Selector.open();
      SelectionKey connect_key =
        sock.socket().getChannel().register(selector, SelectionKey.OP_CONNECT);
      // Try for the connection for 3000ms
      while (selector.select(3000) > 0) {
        while (!result) {

          Set keys = selector.selectedKeys();
          Iterator i = keys.iterator();

          while (i.hasNext()) {
            SelectionKey key = (SelectionKey)i.next();

            // Remove the current key
            i.remove();

            // Attempt a connection
            if (key.isConnectable()) {
              if (sock.isConnectionPending())
                sock.finishConnect();
              result = true;
            }
          }
        }
      }
      if (!result)
        throw new SocketException("unable to connect to socket: Host is down");
    } catch(java.io.IOException e) {
      throw new SocketException("unable to connect:"+e.getMessage());
    }

    // Disable Nagle's algorithm, to reduce latency
    enableNagles(sock, false);

    // Create the input and output streams
    instream = new FdInStream(sock);
    outstream = new FdOutStream(sock);
    ownStreams = true;
  }

  protected void finalize() throws Exception {
    if (closeFd)
      try {
        ((SocketDescriptor)getFd()).close();
      } catch (IOException e) {
        throw new Exception(e.getMessage());
      }
  }

  public int getMyPort() {
    return getSockPort();
  }

  public String getPeerAddress() {
    InetAddress peer = ((SocketDescriptor)getFd()).socket().getInetAddress();
    if (peer != null)
      return peer.getHostAddress();
    return "";
  }

  public String getPeerName() {
    InetAddress peer = ((SocketDescriptor)getFd()).socket().getInetAddress();
    if (peer != null)
      return peer.getHostName();
    return "";
  }

  public int getPeerPort() {
    int port = ((SocketDescriptor)getFd()).socket().getPort();
    return port;
  }

  public String getPeerEndpoint() {
    String address = getPeerAddress();
    int port = getPeerPort();
    return address+"::"+port;
  }

  public boolean sameMachine() throws Exception {
    try {
      SocketAddress peeraddr = ((SocketDescriptor)getFd()).getRemoteAddress();
      SocketAddress myaddr = ((SocketDescriptor)getFd()).getLocalAddress();
      return myaddr.equals(peeraddr);
    } catch (IOException e) {
      throw new Exception(e.getMessage());
    }
  }

  public void shutdown() throws Exception {
    super.shutdown();
    try {
      ((SocketDescriptor)getFd()).shutdown();
    } catch (IOException e) {
      throw new Exception(e.getMessage());
    }
  }

  public void close() throws IOException {
    ((SocketDescriptor)getFd()).close();
  }

  public static boolean enableNagles(SocketDescriptor sock, boolean enable) {
    try {
      sock.channel.socket().setTcpNoDelay(!enable);
    } catch(java.net.SocketException e) {
      vlog.error("unable to setsockopt TCP_NODELAY: "+e.getMessage());
      return false;
    }
    return true;
  }

  public static boolean isSocket(java.net.Socket sock) {
    return sock.getClass().toString().equals("com.tigervnc.net.Socket");
  }

  public boolean isConnected() {
    return ((SocketDescriptor)getFd()).isConnected();
  }

  public int getSockPort() {
    return ((SocketDescriptor)getFd()).socket().getLocalPort();
  }

  /* Tunnelling support. */
  public static int findFreeTcpPort() {
    java.net.ServerSocket sock;
    int port;
    try {
      sock = new java.net.ServerSocket(0);
      port = sock.getLocalPort();
      sock.close();
    } catch (java.io.IOException e) {
      throw new SocketException("unable to create socket: "+e.toString());
    }
    return port;
  }

  private boolean closeFd;
  static LogWriter vlog = new LogWriter("TcpSocket");

}


