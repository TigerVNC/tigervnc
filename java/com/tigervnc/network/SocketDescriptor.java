/* 
 * Copyright (C) 2012 TigerVNC Team
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
package com.tigervnc.network;

import java.io.IOException;

import java.net.SocketAddress;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.SelectorProvider;

import java.util.Set;
import java.util.Iterator;

import com.tigervnc.rdr.Exception;

public class SocketDescriptor extends SocketChannel 
                              implements FileDescriptor {

  public SocketDescriptor() throws Exception {
    super(SelectorProvider.provider());
    try {
      channel = SocketChannel.open();
      channel.configureBlocking(false);
      selector = Selector.open();
    } catch (IOException e) {
      throw new Exception(e.toString());
    }
    try {
      channel.register(selector, SelectionKey.OP_READ | SelectionKey.OP_WRITE );
    } catch (java.nio.channels.ClosedChannelException e) {
      throw new Exception(e.toString());
    }
  }

  synchronized public int read(byte[] buf, int bufPtr, int length) throws Exception {
    int n;
    ByteBuffer b = ByteBuffer.allocate(length);
    try {
      n = channel.read(b);
    } catch (java.io.IOException e) {
      throw new Exception(e.toString());
    }
    if (n <= 0)
      return (n == 0) ? -1 : 0;
    b.flip();
    b.get(buf, bufPtr, n);
    b.clear();
    return n;

  }

  synchronized public int write(byte[] buf, int bufPtr, int length) throws Exception {
    int n;
    ByteBuffer b = ByteBuffer.allocate(length);
    b.put(buf, bufPtr, length);
    b.flip();
    try {
      n = channel.write(b);
    } catch (java.io.IOException e) {
      throw new Exception(e.toString());
    }
    b.clear();
    return n;
  }

  synchronized public int select(int interestOps, int timeout) throws Exception {
    int n;
    try {
      if (timeout == 0) {
        n = selector.selectNow();
      } else {
        n = selector.select(timeout);
      }
    } catch (java.io.IOException e) {
      throw new Exception(e.toString());
    }
    if (n == 0)
      return -1;
    Set keys = selector.selectedKeys();
    Iterator iter = keys.iterator();
    while (iter.hasNext()) {
      SelectionKey key = (SelectionKey)iter.next();
      if ((key.readyOps() & interestOps) != 0) { 
        n = 1;
        break;
      } else {
        n = -1;
      }
    }
    keys.clear();
    return n;
  }

  public int write(ByteBuffer buf) throws Exception {
    int n = 0;
    try {
      n = channel.write(buf);
    } catch (java.io.IOException e) {
      throw new Exception(e.toString());
    }
    return n;
  }

  public long write(ByteBuffer[] buf, int offset, int length) 
        throws IOException
  {
    long n = 0;
    try {
      n = channel.write(buf, offset, length);
    } catch (java.io.IOException e) {
      throw new Exception(e.toString());
    }
    return n;
  }

  public int read(ByteBuffer buf) throws IOException {
    int n = 0;
    try {
      n = channel.read(buf);
    } catch (java.io.IOException e) {
      throw new Exception(e.toString());
    }
    return n;
  }

  public long read(ByteBuffer[] buf, int offset, int length) 
        throws IOException
  {
    long n = 0;
    try {
      n = channel.read(buf, offset, length);
    } catch (java.io.IOException e) {
      throw new Exception(e.toString());
    }
    return n;
  }

  public java.net.Socket socket() {
    return channel.socket();
  }

  public boolean isConnectionPending() {
    return channel.isConnectionPending();
  }

  public boolean connect(SocketAddress remote) throws IOException {
    return channel.connect(remote);
  }

  public boolean finishConnect() throws IOException {
    return channel.finishConnect();
  }

  public boolean isConnected() {
    return channel.isConnected();
  }

  protected void implConfigureBlocking(boolean block) throws IOException {
    channel.configureBlocking(block);
  }

  protected synchronized void implCloseSelectableChannel() throws IOException {
    channel.close();
    notifyAll();
  }

  protected void setChannel(SocketChannel channel_) {
    try {
      if (channel != null)
        channel.close();
      if (selector != null)
        selector.close();
      channel = channel_;
      channel.configureBlocking(false);
      selector = Selector.open();
    } catch (java.io.IOException e) {
      throw new Exception(e.toString());
    }
    try {
      channel.register(selector, SelectionKey.OP_READ | SelectionKey.OP_WRITE );
    } catch (java.nio.channels.ClosedChannelException e) {
      System.out.println(e.toString());
    }
  }
  
  protected SocketChannel channel;
  protected Selector selector;

}
