/*
 * Copyright (c) 2002-2018 ymnk, JCraft,Inc. All rights reserved.
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
import java.io.InputStream;
import java.io.OutputStream;

public class ChannelExec extends ChannelSession {

  byte[] command = new byte[0];

  @Override
  public void start() throws JSchException {
    Session _session = getSession();
    try {
      sendRequests();
      Request request = new RequestExec(command);
      request.request(_session, this);
    } catch (Exception e) {
      if (e instanceof JSchException)
        throw (JSchException) e;
      throw new JSchException("ChannelExec", e);
    }

    if (io.in != null) {
      thread = _session.getThreadFactory().newThread(this::run);
      thread.setName("Exec thread " + _session.getHost());
      if (_session.daemon_thread) {
        thread.setDaemon(_session.daemon_thread);
      }
      thread.start();
    }
  }

  public void setCommand(String command) {
    this.command = Util.str2byte(command);
  }

  public void setCommand(byte[] command) {
    this.command = command;
  }

  @Override
  void init() throws JSchException {
    io.setInputStream(getSession().in);
    io.setOutputStream(getSession().out);
  }

  public void setErrStream(OutputStream out) {
    setExtOutputStream(out);
  }

  public void setErrStream(OutputStream out, boolean dontclose) {
    setExtOutputStream(out, dontclose);
  }

  public InputStream getErrStream() throws IOException {
    return getExtInputStream();
  }
}
