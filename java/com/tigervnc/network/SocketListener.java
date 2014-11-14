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

// -=- SocketListener - abstract base-class for any kind of network stream/socket

package com.tigervnc.network;

import java.nio.channels.*;
import java.nio.channels.spi.SelectorProvider;

abstract public class SocketListener {

  public SocketListener() {}

  // shutdown() stops the socket from accepting further connections
  abstract public void shutdown() throws Exception;

  // accept() returns a new Socket object if there is a connection
  // attempt in progress AND if the connection passes the filter
  // if one is installed.  Otherwise, returns 0.
  abstract public Socket accept();

  public FileDescriptor getFd() {return fd;}

  protected FileDescriptor fd;

}
