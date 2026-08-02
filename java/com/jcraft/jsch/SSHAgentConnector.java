/*
 * Copyright (c) 2011 ymnk, JCraft,Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 * and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 * conditions and the following disclaimer in the documentation and/or other materials provided with
 * the distribution.
 *
 * 3. The names of the authors may not be used to endorse or promote products derived from this
 * software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL JCRAFT, INC. OR ANY CONTRIBUTORS TO THIS SOFTWARE BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package com.jcraft.jsch;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.channels.SocketChannel;
import java.nio.file.Path;
import java.nio.file.Paths;

public class SSHAgentConnector implements AgentConnector {
  private static final int MAX_AGENT_REPLY_LEN = 256 * 1024;

  private USocketFactory factory;
  private Path usocketPath;

  public SSHAgentConnector() throws AgentProxyException {
    this(getUSocketFactory(), getSshAuthSocket());
  }

  public SSHAgentConnector(Path usocketPath) throws AgentProxyException {
    this(getUSocketFactory(), usocketPath);
  }

  public SSHAgentConnector(USocketFactory factory) throws AgentProxyException {
    this(factory, getSshAuthSocket());
  }

  public SSHAgentConnector(USocketFactory factory, Path usocketPath) {
    this.factory = factory;
    this.usocketPath = usocketPath;
  }

  @Override
  public String getName() {
    return "ssh-agent";
  }

  @Override
  @SuppressWarnings("try")
  public boolean isAvailable() {
    try (SocketChannel foo = open()) {
      return true;
    } catch (IOException e) {
      return false;
    }
  }

  private SocketChannel open() throws IOException {
    return factory.connect(usocketPath);
  }

  @Override
  public void query(Buffer buffer) throws AgentProxyException {
    try (SocketChannel sock = open()) {
      writeFull(sock, buffer, 0, buffer.getLength());
      buffer.rewind();
      int i = readFull(sock, buffer, 0, 4); // length
      i = buffer.getInt();
      if (i <= 0 || i > MAX_AGENT_REPLY_LEN) {
        throw new AgentProxyException("Illegal length: " + i);
      }
      buffer.rewind();
      buffer.checkFreeSize(i);
      i = readFull(sock, buffer, 0, i);
    } catch (IOException e) {
      throw new AgentProxyException(e.toString(), e);
    }
  }

  private static USocketFactory getUSocketFactory() throws AgentProxyException {
    try {
      return new UnixDomainSocketFactory();
    } catch (AgentProxyException e) {
      try {
        return new JUnixSocketFactory();
      } catch (LinkageError ee) {
        AgentProxyException eee = new AgentProxyException("junixsocket library unavailable");
        eee.addSuppressed(e);
        eee.addSuppressed(ee);
        throw eee;
      } catch (AgentProxyException ee) {
        ee.addSuppressed(e);
        throw e;
      }
    }
  }

  private static Path getSshAuthSocket() throws AgentProxyException {
    String ssh_auth_sock = Util.getSystemEnv("SSH_AUTH_SOCK");
    if (ssh_auth_sock == null) {
      throw new AgentProxyException("SSH_AUTH_SOCK is not defined.");
    }
    return Paths.get(ssh_auth_sock);
  }

  private static int readFull(SocketChannel sock, Buffer buffer, int s, int len)
      throws IOException {
    ByteBuffer bb = ByteBuffer.wrap(buffer.buffer, s, len);
    int _len = len;
    while (len > 0) {
      int j = sock.read(bb);
      if (j < 0)
        return -1;
      if (j > 0) {
        len -= j;
      }
    }
    return _len;
  }

  private static int writeFull(SocketChannel sock, Buffer buffer, int s, int len)
      throws IOException {
    ByteBuffer bb = ByteBuffer.wrap(buffer.buffer, s, len);
    int _len = len;
    while (len > 0) {
      int j = sock.write(bb);
      if (j < 0)
        return -1;
      if (j > 0) {
        len -= j;
      }
    }
    return _len;
  }
}
