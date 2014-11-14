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

// -=- Socket - abstract base-class for any kind of network stream/socket

package com.tigervnc.network;

import com.tigervnc.rdr.*;
import java.nio.channels.*;
import java.nio.channels.spi.SelectorProvider;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

abstract public class Socket {

  public Socket(FileDescriptor fd) {
    instream = new FdInStream(fd);
    outstream = new FdOutStream(fd);
    ownStreams = true; isShutdown_ = false;
    queryConnection = false;
  }

  public FdInStream inStream() {return instream;}
  public FdOutStream outStream() {return outstream;}
  public FileDescriptor getFd() {return outstream.getFd();}

  // if shutdown() is overridden then the override MUST call on to here
  public void shutdown() {isShutdown_ = true;}
  public void close() throws IOException {getFd().close();}
  public final boolean isShutdown() {return isShutdown_;}

  // information about this end of the socket
  abstract public int getMyPort();

  // information about the remote end of the socket
  abstract public String getPeerAddress(); // a string e.g. "192.168.0.1"
  abstract public String getPeerName();
  abstract public int getPeerPort();
  abstract public String getPeerEndpoint(); // <address>::<port>

  // Is the remote end on the same machine?
  abstract public boolean sameMachine();

  public void setRequiresQuery() {queryConnection = true;}
  public final boolean requiresQuery() {return queryConnection;}

  protected Socket() {
    instream = null; outstream = null; ownStreams = false;
    isShutdown_ = false; queryConnection = false;
  }

  protected Socket(FdInStream i, FdOutStream o, boolean own) {
    instream = i; outstream = o; ownStreams = own;
    isShutdown_ = false; queryConnection = false;
  }

  protected FdInStream instream;
  protected FdOutStream outstream;
  boolean ownStreams;
  boolean isShutdown_;
  boolean queryConnection;
}
