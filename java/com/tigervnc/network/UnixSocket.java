/* Copyright (C) 2026 TigerVNC Team
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
import java.lang.reflect.Method;
import java.net.SocketAddress;
import java.nio.channels.SocketChannel;

// Connects directly to a local Unix domain socket path, mirroring the
// C++ viewer's network::UnixSocket. java.net.UnixDomainSocketAddress and
// StandardProtocolFamily.UNIX only exist on Java 16+, so the lookups are
// done via reflection to keep this class (and the project) compilable
// under the Java 8 baseline; on an older JRE, connecting simply fails
// with a clear error instead of the class failing to load.
public class UnixSocket extends Socket {

  public UnixSocket(String path) throws Exception {
    this.path = path;
    closeFd = true;

    SocketDescriptor sock;
    try {
      sock = new SocketDescriptor();
      SocketChannel channel = openChannel();
      channel.connect(addressFor(path));
      sock.setChannel(channel);
    } catch (java.lang.Exception e) {
      throw new SocketException("unable to connect to Unix domain socket \""+
                                path+"\": "+e.getMessage());
    }

    instream = new FdInStream(sock);
    outstream = new FdOutStream(sock);
    ownStreams = true;
  }

  public int getMyPort() { return 0; }

  public String getPeerAddress() { return path; }
  public String getPeerName() { return path; }
  public int getPeerPort() { return 0; }
  public String getPeerEndpoint() { return path; }

  public boolean sameMachine() { return true; }

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

  public boolean isConnected() {
    return ((SocketDescriptor)getFd()).isConnected();
  }

  // -=- Reflection-based access to the Java 16+ Unix domain socket API.

  private static SocketChannel openChannel() throws IOException {
    try {
      Class<?> pfClass = Class.forName("java.net.ProtocolFamily");
      Class<?> spfClass = Class.forName("java.net.StandardProtocolFamily");
      Object unix = spfClass.getField("UNIX").get(null);
      Method open = SocketChannel.class.getMethod("open", pfClass);
      return (SocketChannel)open.invoke(null, unix);
    } catch (ReflectiveOperationException e) {
      throw new IOException(UNSUPPORTED_MESSAGE, e);
    }
  }

  private static SocketAddress addressFor(String path) throws IOException {
    try {
      Class<?> addrClass = Class.forName("java.net.UnixDomainSocketAddress");
      Method of = addrClass.getMethod("of", String.class);
      return (SocketAddress)of.invoke(null, path);
    } catch (ReflectiveOperationException e) {
      throw new IOException(UNSUPPORTED_MESSAGE, e);
    }
  }

  private static final String UNSUPPORTED_MESSAGE =
    "Unix domain sockets require Java 16 or newer";

  private final String path;
  private boolean closeFd;
  static LogWriter vlog = new LogWriter("UnixSocket");
}
