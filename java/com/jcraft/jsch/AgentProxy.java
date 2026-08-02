/*
 * Copyright (c) 2012 ymnk, JCraft,Inc. All rights reserved.
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

import java.util.Vector;

class AgentProxy {

  private static final byte SSH_AGENTC_REQUEST_RSA_IDENTITIES = 1;
  private static final byte SSH_AGENT_RSA_IDENTITIES_ANSWER = 2;
  private static final byte SSH_AGENTC_RSA_CHALLENGE = 3;
  private static final byte SSH_AGENT_RSA_RESPONSE = 4;
  private static final byte SSH_AGENT_FAILURE = 5;
  private static final byte SSH_AGENT_SUCCESS = 6;
  private static final byte SSH_AGENTC_ADD_RSA_IDENTITY = 7;
  private static final byte SSH_AGENTC_REMOVE_RSA_IDENTITY = 8;
  private static final byte SSH_AGENTC_REMOVE_ALL_RSA_IDENTITIES = 9;

  private static final byte SSH2_AGENTC_REQUEST_IDENTITIES = 11;
  private static final byte SSH2_AGENT_IDENTITIES_ANSWER = 12;
  private static final byte SSH2_AGENTC_SIGN_REQUEST = 13;
  private static final byte SSH2_AGENT_SIGN_RESPONSE = 14;
  private static final byte SSH2_AGENTC_ADD_IDENTITY = 17;
  private static final byte SSH2_AGENTC_REMOVE_IDENTITY = 18;
  private static final byte SSH2_AGENTC_REMOVE_ALL_IDENTITIES = 19;

  private static final byte SSH_AGENTC_ADD_SMARTCARD_KEY = 20;
  private static final byte SSH_AGENTC_REMOVE_SMARTCARD_KEY = 21;

  private static final byte SSH_AGENTC_LOCK = 22;
  private static final byte SSH_AGENTC_UNLOCK = 23;

  private static final byte SSH_AGENTC_ADD_RSA_ID_CONSTRAINED = 24;
  private static final byte SSH2_AGENTC_ADD_ID_CONSTRAINED = 25;
  private static final byte SSH_AGENTC_ADD_SMARTCARD_KEY_CONSTRAINED = 26;

  private static final byte SSH_AGENT_CONSTRAIN_LIFETIME = 1;
  private static final byte SSH_AGENT_CONSTRAIN_CONFIRM = 2;

  private static final byte SSH2_AGENT_FAILURE = 30;

  private static final byte SSH_COM_AGENT2_FAILURE = 102;

  // private static final byte SSH_AGENT_OLD_SIGNATURE = 0x1;
  private static final int SSH_AGENT_RSA_SHA2_256 = 0x2;
  private static final int SSH_AGENT_RSA_SHA2_512 = 0x4;

  private static final int MAX_AGENT_IDENTITIES = 2048;

  private final byte[] buf = new byte[1024];
  private final Buffer buffer = new Buffer(buf);

  private AgentConnector connector;

  AgentProxy(AgentConnector connector) {
    this.connector = connector;
  }

  synchronized Vector<Identity> getIdentities() {
    Vector<Identity> identities = new Vector<>();

    int required_size = 1 + 4;
    buffer.reset();
    buffer.checkFreeSize(required_size);
    buffer.putInt(required_size - 4);
    buffer.putByte(SSH2_AGENTC_REQUEST_IDENTITIES);

    try {
      connector.query(buffer);
    } catch (AgentProxyException e) {
      buffer.rewind();
      buffer.putByte(SSH_AGENT_FAILURE);
      return identities;
    }

    int rcode = buffer.getByte();

    // System.out.println(rcode == SSH2_AGENT_IDENTITIES_ANSWER);

    if (rcode != SSH2_AGENT_IDENTITIES_ANSWER) {
      return identities;
    }

    int count = buffer.getInt();
    // System.out.println(count);
    if (count <= 0 || count > MAX_AGENT_IDENTITIES) {
      return identities;
    }

    for (int i = 0; i < count; i++) {
      byte[] blob = buffer.getString();
      String comment = Util.byte2str(buffer.getString());
      identities.add(new AgentIdentity(this, blob, comment));
    }

    return identities;
  }

  synchronized byte[] sign(byte[] blob, byte[] data, String alg) {
    int flags = 0x0;
    if (alg != null) {
      if (alg.equals("rsa-sha2-256")) {
        flags = SSH_AGENT_RSA_SHA2_256;
      } else if (alg.equals("rsa-sha2-512")) {
        flags = SSH_AGENT_RSA_SHA2_512;
      }
    }

    int required_size = 1 + 4 * 4 + blob.length + data.length;
    buffer.reset();
    buffer.checkFreeSize(required_size);
    buffer.putInt(required_size - 4);
    buffer.putByte(SSH2_AGENTC_SIGN_REQUEST);
    buffer.putString(blob);
    buffer.putString(data);
    buffer.putInt(flags);

    try {
      connector.query(buffer);
    } catch (AgentProxyException e) {
      buffer.rewind();
      buffer.putByte(SSH_AGENT_FAILURE);
    }

    int rcode = buffer.getByte();

    // System.out.println(rcode == SSH2_AGENT_SIGN_RESPONSE);

    if (rcode != SSH2_AGENT_SIGN_RESPONSE) {
      return null;
    }

    return buffer.getString();
  }

  synchronized boolean removeIdentity(byte[] blob) {
    int required_size = 1 + 4 * 2 + blob.length;
    buffer.reset();
    buffer.checkFreeSize(required_size);
    buffer.putInt(required_size - 4);
    buffer.putByte(SSH2_AGENTC_REMOVE_IDENTITY);
    buffer.putString(blob);

    try {
      connector.query(buffer);
    } catch (AgentProxyException e) {
      buffer.rewind();
      buffer.putByte(SSH_AGENT_FAILURE);
    }

    int rcode = buffer.getByte();

    // System.out.println(rcode == SSH_AGENT_SUCCESS);

    return rcode == SSH_AGENT_SUCCESS;
  }

  synchronized void removeAllIdentities() {
    int required_size = 1 + 4;
    buffer.reset();
    buffer.checkFreeSize(required_size);
    buffer.putInt(required_size - 4);
    buffer.putByte(SSH2_AGENTC_REMOVE_ALL_IDENTITIES);

    try {
      connector.query(buffer);
    } catch (AgentProxyException e) {
      buffer.rewind();
      buffer.putByte(SSH_AGENT_FAILURE);
    }

    // int rcode = buffer.getByte();

    // System.out.println(rcode == SSH_AGENT_SUCCESS);
  }

  synchronized boolean addIdentity(byte[] identity) {
    int required_size = 1 + 4 + identity.length;
    buffer.reset();
    buffer.checkFreeSize(required_size);
    buffer.putInt(required_size - 4);
    buffer.putByte(SSH2_AGENTC_ADD_IDENTITY);
    buffer.putByte(identity);

    try {
      connector.query(buffer);
    } catch (AgentProxyException e) {
      buffer.rewind();
      buffer.putByte(SSH_AGENT_FAILURE);
    }

    int rcode = buffer.getByte();

    // System.out.println(rcode == SSH_AGENT_SUCCESS);

    return rcode == SSH_AGENT_SUCCESS;
  }

  synchronized boolean isRunning() {
    int required_size = 1 + 4;
    buffer.reset();
    buffer.checkFreeSize(required_size);
    buffer.putInt(required_size - 4);
    buffer.putByte(SSH2_AGENTC_REQUEST_IDENTITIES);

    try {
      connector.query(buffer);
    } catch (AgentProxyException e) {
      return false;
    }

    int rcode = buffer.getByte();

    // System.out.println(rcode == SSH2_AGENT_IDENTITIES_ANSWER);

    return rcode == SSH2_AGENT_IDENTITIES_ANSWER;
  }

  synchronized AgentConnector getConnector() {
    return connector;
  }
}
