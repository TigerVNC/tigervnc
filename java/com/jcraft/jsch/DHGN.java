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

abstract class DHGN extends KeyExchange {

  private static final int SSH_MSG_KEXDH_INIT = 30;
  private static final int SSH_MSG_KEXDH_REPLY = 31;

  private int state;

  DH dh;

  byte[] V_S;
  byte[] V_C;
  byte[] I_S;
  byte[] I_C;

  byte[] e;

  private Buffer buf;
  private Packet packet;

  abstract byte[] G();

  abstract byte[] P();

  abstract String sha_name();

  @Override
  public void init(Session session, byte[] V_S, byte[] V_C, byte[] I_S, byte[] I_C)
      throws Exception {
    this.V_S = V_S;
    this.V_C = V_C;
    this.I_S = I_S;
    this.I_C = I_C;

    try {
      Class<? extends HASH> c = Class.forName(session.getConfig(sha_name())).asSubclass(HASH.class);
      sha = c.getDeclaredConstructor().newInstance();
      sha.init();
    } catch (Exception e) {
      throw new JSchException(e.toString(), e);
    }

    buf = new Buffer();
    packet = new Packet(buf);

    try {
      Class<? extends DH> c = Class.forName(session.getConfig("dh")).asSubclass(DH.class);
      dh = c.getDeclaredConstructor().newInstance();
      dh.init();
    } catch (Exception e) {
      throw new JSchException(e.toString(), e);
    }

    dh.setP(P());
    dh.setG(G());
    // The client responds with:
    // byte SSH_MSG_KEXDH_INIT(30)
    // mpint e <- g^x mod p
    // x is a random number (1 < x < (p-1)/2)

    e = dh.getE();
    packet.reset();
    buf.putByte((byte) SSH_MSG_KEXDH_INIT);
    buf.putMPInt(e);

    if (V_S == null) { // This is a really ugly hack for Session.checkKexes ;-(
      return;
    }

    session.write(packet);

    if (session.getLogger().isEnabled(Logger.INFO)) {
      session.getLogger().log(Logger.INFO, "SSH_MSG_KEXDH_INIT sent");
      session.getLogger().log(Logger.INFO, "expecting SSH_MSG_KEXDH_REPLY");
    }

    state = SSH_MSG_KEXDH_REPLY;
  }

  @Override
  public boolean next(Buffer _buf) throws Exception {
    int i, j;

    switch (state) {
      case SSH_MSG_KEXDH_REPLY:
        // The server responds with:
        // byte SSH_MSG_KEXDH_REPLY(31)
        // string server public host key and certificates (K_S)
        // mpint f
        // string signature of H
        j = _buf.getInt();
        j = _buf.getByte();
        j = _buf.getByte();
        if (j != SSH_MSG_KEXDH_REPLY) {
          if (session.getLogger().isEnabled(Logger.ERROR)) {
            session.getLogger().log(Logger.ERROR, "type: must be SSH_MSG_KEXDH_REPLY " + j);
          }
          return false;
        }

        K_S = _buf.getString();

        byte[] f = _buf.getMPInt();
        byte[] sig_of_H = _buf.getString();

        dh.setF(f);

        dh.checkRange();

        K = encodeAsMPInt(normalize(dh.getK()), true);

        // The hash H is computed as the HASH hash of the concatenation of the
        // following:
        // string V_C, the client's version string (CR and NL excluded)
        // string V_S, the server's version string (CR and NL excluded)
        // string I_C, the payload of the client's SSH_MSG_KEXINIT
        // string I_S, the payload of the server's SSH_MSG_KEXINIT
        // string K_S, the host key
        // mpint e, exchange value sent by the client
        // mpint f, exchange value sent by the server
        // mpint K, the shared secret
        // This value is called the exchange hash, and it is used to authenti-
        // cate the key exchange.
        byte[] foo = encodeAsString(V_C, false);
        sha.update(foo, 0, foo.length);
        foo = encodeAsString(V_S, false);
        sha.update(foo, 0, foo.length);
        foo = encodeAsString(I_C, false);
        sha.update(foo, 0, foo.length);
        foo = encodeAsString(I_S, false);
        sha.update(foo, 0, foo.length);
        foo = encodeAsString(K_S, false);
        sha.update(foo, 0, foo.length);
        foo = encodeAsMPInt(e, false);
        sha.update(foo, 0, foo.length);
        foo = encodeAsMPInt(f, false);
        sha.update(foo, 0, foo.length);

        sha.update(K, 0, K.length);
        H = sha.digest();
        // System.err.print("H -> "); //dump(H, 0, H.length);

        i = 0;
        j = 0;
        j = ((K_S[i++] << 24) & 0xff000000) | ((K_S[i++] << 16) & 0x00ff0000)
            | ((K_S[i++] << 8) & 0x0000ff00) | ((K_S[i++]) & 0x000000ff);
        String alg = Util.byte2str(K_S, i, j);
        i += j;

        boolean result = verify(alg, K_S, i, sig_of_H);

        state = STATE_END;
        return result;
    }
    return false;
  }

  @Override
  public int getState() {
    return state;
  }
}
