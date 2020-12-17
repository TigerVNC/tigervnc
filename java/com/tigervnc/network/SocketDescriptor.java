/* Copyright (C) 2012 Brian P. Hinz
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

import java.net.SocketAddress;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.SelectorProvider;

import java.util.Set;
import java.util.Iterator;

import com.tigervnc.rdr.Exception;

public class SocketDescriptor implements FileDescriptor {

  public SocketDescriptor() throws Exception {
    DefaultSelectorProvider();
    try {
      channel = SocketChannel.open();
      channel.configureBlocking(false);
      writeSelector = Selector.open();
      readSelector = Selector.open();
    } catch (IOException e) {
      throw new Exception(e.getMessage());
    }
    try {
      channel.register(writeSelector, SelectionKey.OP_WRITE);
      channel.register(readSelector, SelectionKey.OP_READ);
    } catch (java.nio.channels.ClosedChannelException e) {
      throw new Exception(e.getMessage());
    }
  }

  public void shutdown() throws IOException {
    try {
      channel.socket().shutdownInput();
      channel.socket().shutdownOutput();
    } catch(IOException e) {
      throw new IOException(e.getMessage());
    }
  }

  public void close() throws IOException {
    try {
      channel.close();
    } catch(IOException e) {
      throw new IOException(e.getMessage());
    }
  }

  private static SelectorProvider DefaultSelectorProvider() {
    // kqueue() selector provider on OS X is not working, fall back to select() for now
    //String os = System.getProperty("os.name");
    //if (os.startsWith("Mac OS X"))
    //  System.setProperty("java.nio.channels.spi.SelectorProvider","sun.nio.ch.PollSelectorProvider");
    return SelectorProvider.provider();
  }

  synchronized public int select(int interestOps, Integer timeout) throws Exception {
    int n;
    Selector selector;
    if ((interestOps & SelectionKey.OP_READ) != 0) {
      selector = readSelector;
    } else {
      selector = writeSelector;
    }
    selector.selectedKeys().clear();
    try {
      if (timeout == null) {
        n = selector.select();
      } else {
        int tv = timeout.intValue();
        switch(tv) {
        case 0:
          n = selector.selectNow();
          break;
        default:
          n = selector.select((long)tv);
          break;
        }
      }
    } catch (java.io.IOException e) {
      throw new Exception(e.getMessage());
    }
    return n;
  }

  public int write(ByteBuffer buf, int len) throws Exception {
    try {
      int n = channel.write((ByteBuffer)buf.slice().limit(len));
      buf.position(buf.position()+n);
      return n;
    } catch (java.io.IOException e) {
      throw new Exception(e.getMessage());
    }
  }

  public int read(ByteBuffer buf, int len) throws Exception {
    try {
      int n = channel.read((ByteBuffer)buf.slice().limit(len));
      buf.position(buf.position()+n);
      return (n < 0) ? 0 : n;
    } catch (java.lang.Exception e) {
      throw new Exception(e.getMessage());
    }
  }

  public java.net.Socket socket() {
    return channel.socket();
  }

  public SocketAddress getRemoteAddress() throws IOException {
    if (isConnected())
      return channel.socket().getRemoteSocketAddress();
    return null;
  }

  public SocketAddress getLocalAddress() throws IOException {
    if (isConnected())
      return channel.socket().getLocalSocketAddress();
    return null;
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

  protected void setChannel(SocketChannel channel_) {
    try {
      if (channel != null)
        channel.close();
      if (readSelector != null)
        readSelector.close();
      if (writeSelector != null)
        writeSelector.close();
      channel = channel_;
      channel.configureBlocking(false);
      writeSelector = Selector.open();
      readSelector = Selector.open();
    } catch (java.io.IOException e) {
      throw new Exception(e.getMessage());
    }
    try {
      channel.register(writeSelector, SelectionKey.OP_WRITE);
      channel.register(readSelector, SelectionKey.OP_READ);
    } catch (java.nio.channels.ClosedChannelException e) {
      System.out.println(e.toString());
    }
  }

  protected SocketChannel channel;
  protected Selector writeSelector;
  protected Selector readSelector;

}
