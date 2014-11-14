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

import java.io.IOException;
import java.lang.Exception;
import java.nio.*;
import java.nio.channels.*;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.net.UnknownHostException;
import java.util.Set;
import java.util.Iterator;

public class TcpListener extends SocketListener  {

  public static boolean socketsInitialised = false;

  public TcpListener(String listenaddr, int port, boolean localhostOnly,
			               SocketDescriptor sock, boolean close_) throws Exception {
    closeFd = close_;
    if (sock != null) {
      fd = sock;
      return;
    }

    TcpSocket.initSockets();
    try {
      channel = ServerSocketChannel.open();
      channel.configureBlocking(false);
    } catch(IOException e) {
      throw new Exception("unable to create listening socket: "+e.toString());
    }

    // - Bind it to the desired port
    InetAddress addr = null;

    try {
      if (localhostOnly) {
        addr = InetAddress.getByName(null);
      } else if (listenaddr != null) {
          addr = java.net.InetAddress.getByName(listenaddr);
      } else {
        addr = InetAddress.getByName("0.0.0.0");
      }
    } catch (UnknownHostException e) {
      throw new Exception(e.getMessage());
    }

    try {
      channel.socket().bind(new InetSocketAddress(addr, port));
    } catch (IOException e) {
      throw new Exception("unable to bind listening socket: "+e.toString());
    }

    // - Set it to be a listening socket
    try {
      selector = Selector.open();
      channel.register(selector, SelectionKey.OP_ACCEPT);
    } catch (IOException e) {
      throw new Exception("unable to set socket to listening mode: "+e.toString());
    }
  }

  public TcpListener(String listenaddr, int port) throws Exception {
    this(listenaddr, port, false, null, true);
  }

  protected void finalize() throws Exception {
    if (closeFd)
      try {
        ((SocketDescriptor)getFd()).close();
      } catch (IOException e) {
        throw new Exception(e.getMessage());
      }
  }

  public void shutdown() throws Exception {
    try {
      ((SocketDescriptor)getFd()).shutdown();
    } catch (IOException e) {
      throw new Exception(e.getMessage());
    }
  }

  public TcpSocket accept() {
    SocketChannel new_sock = null;

    // Accept an incoming connection
    try {
      if (selector.select(0) > 0) {
        Set<SelectionKey> keys = selector.selectedKeys();
        Iterator<SelectionKey> iter = keys.iterator();
        while (iter.hasNext()) {
          SelectionKey key = (SelectionKey)iter.next();
          iter.remove();
          if (key.isAcceptable()) {
            new_sock = channel.accept();
            break;
          }
        }
        keys.clear();
        if (new_sock == null)
          return null;
      }
    } catch (IOException e) {
      throw new SocketException("unable to accept new connection: "+e.toString());
    }

    // Disable Nagle's algorithm, to reduce latency
    try {
      new_sock.socket().setTcpNoDelay(true);
    } catch (java.net.SocketException e) {
      throw new SocketException(e.getMessage());
    }

    // Create the socket object & check connection is allowed
    SocketDescriptor fd = null;
    try {
      fd = new SocketDescriptor();
    } catch (java.lang.Exception e) {
      throw new SocketException(e.getMessage());
    }
    fd.setChannel(new_sock);
    TcpSocket s = new TcpSocket(fd);
    return s;
  }

  public int getMyPort() {
    return ((SocketDescriptor)getFd()).socket().getLocalPort();
  }

  private boolean closeFd;
  private ServerSocketChannel channel;
  private Selector selector;

}

