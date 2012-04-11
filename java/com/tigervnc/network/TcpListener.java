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
        // FIXME: need to be sure we get the wildcard address?
        addr = InetAddress.getByName(null);
        //addr = InetAddress.getLocalHost();
      }
    } catch (UnknownHostException e) {
      System.out.println(e.toString());
      System.exit(-1);
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

//  TcpListener::~TcpListener() {
//    if (closeFd) closesocket(fd);
//  }

  public void shutdown() {
    //shutdown(getFd(), 2);
  }

  public TcpSocket accept() {
    SocketChannel new_sock = null;

    // Accept an incoming connection
    try {
      if (selector.select(0) > 0) {
        Set keys = selector.selectedKeys();
        Iterator iter = keys.iterator();
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
    TcpSocket.enableNagles(new_sock, false);

    // Create the socket object & check connection is allowed
    SocketDescriptor fd = null;
    try {
      fd = new SocketDescriptor();
    } catch (java.lang.Exception e) {
      System.out.println(e.toString());
      System.exit(-1);
    }
    fd.setChannel(new_sock);
    TcpSocket s = new TcpSocket(fd);
    //if (filter && !filter->verifyConnection(s)) {
    //  delete s;
    //  return 0;
    //}
    return s;
  }

/*
void TcpListener::getMyAddresses(std::list<char*>* result) {
  const hostent* addrs = gethostbyname(0);
  if (addrs == 0)
    throw rdr::SystemException("gethostbyname", errorNumber);
  if (addrs->h_addrtype != AF_INET)
    throw rdr::Exception("getMyAddresses: bad family");
  for (int i=0; addrs->h_addr_list[i] != 0; i++) {
    const char* addrC = inet_ntoa(*((struct in_addr*)addrs->h_addr_list[i]));
    char* addr = new char[strlen(addrC)+1];
    strcpy(addr, addrC);
    result->push_back(addr);
  }
}
  */

  //public int getMyPort() {
  //  return TcpSocket.getSockPort();
  //}

  private boolean closeFd;
  private ServerSocketChannel channel;
  private Selector selector;

}

