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

package com.jcraft.jsch.jce;

import com.jcraft.jsch.Buffer;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.security.KeyFactory;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.Signature;
import java.security.spec.DSAPrivateKeySpec;
import java.security.spec.DSAPublicKeySpec;

public class SignatureDSA implements com.jcraft.jsch.SignatureDSA {

  Signature signature;
  KeyFactory keyFactory;

  @Override
  public void init() throws Exception {
    signature = Signature.getInstance("SHA1withDSA");
    keyFactory = KeyFactory.getInstance("DSA");
  }

  @Override
  public void setPubKey(byte[] y, byte[] p, byte[] q, byte[] g) throws Exception {
    DSAPublicKeySpec dsaPubKeySpec = new DSAPublicKeySpec(new BigInteger(y), new BigInteger(p),
        new BigInteger(q), new BigInteger(g));
    PublicKey pubKey = keyFactory.generatePublic(dsaPubKeySpec);
    signature.initVerify(pubKey);
  }

  @Override
  public void setPrvKey(byte[] x, byte[] p, byte[] q, byte[] g) throws Exception {
    DSAPrivateKeySpec dsaPrivKeySpec = new DSAPrivateKeySpec(new BigInteger(x), new BigInteger(p),
        new BigInteger(q), new BigInteger(g));
    PrivateKey prvKey = keyFactory.generatePrivate(dsaPrivKeySpec);
    signature.initSign(prvKey);
  }

  @Override
  public byte[] sign() throws Exception {
    byte[] sig = signature.sign();
    /*
     * System.err.print("sign["+sig.length+"] "); for(int i=0; i<sig.length;i++){
     * System.err.print(Integer.toHexString(sig[i]&0xff)+":"); } System.err.println("");
     */
    // sig is in ASN.1
    // SEQUENCE::={ r INTEGER, s INTEGER }
    int len = 0;
    int index = 3;
    len = sig[index++] & 0xff;
    // System.err.println("! len="+len);
    byte[] r = new byte[len];
    System.arraycopy(sig, index, r, 0, r.length);
    index = index + len + 1;
    len = sig[index++] & 0xff;
    // System.err.println("!! len="+len);
    byte[] s = new byte[len];
    System.arraycopy(sig, index, s, 0, s.length);

    byte[] result = new byte[40];

    // result must be 40 bytes, but length of r and s may not be 20 bytes

    System.arraycopy(r, (r.length > 20) ? 1 : 0, result, (r.length > 20) ? 0 : 20 - r.length,
        (r.length > 20) ? 20 : r.length);
    System.arraycopy(s, (s.length > 20) ? 1 : 0, result, (s.length > 20) ? 20 : 40 - s.length,
        (s.length > 20) ? 20 : s.length);

    // System.arraycopy(sig, (sig[3]==20?4:5), result, 0, 20);
    // System.arraycopy(sig, sig.length-20, result, 20, 20);

    return result;
  }

  @Override
  public void update(byte[] foo) throws Exception {
    signature.update(foo);
  }

  @Override
  public boolean verify(byte[] sig) throws Exception {
    int i = 0;
    int j = 0;
    byte[] tmp;
    Buffer buf = new Buffer(sig);

    if (new String(buf.getString(), StandardCharsets.UTF_8).equals("ssh-dss")) {
      j = buf.getInt();
      i = buf.getOffSet();
      tmp = new byte[j];
      System.arraycopy(sig, i, tmp, 0, j);
      sig = tmp;
    }

    byte[] _frst = new byte[20];
    System.arraycopy(sig, 0, _frst, 0, 20);
    _frst = normalize(_frst);

    byte[] _scnd = new byte[20];
    System.arraycopy(sig, 20, _scnd, 0, 20);
    _scnd = normalize(_scnd);

    // ASN.1
    int frst = ((_frst[0] & 0x80) != 0 ? 1 : 0);
    int scnd = ((_scnd[0] & 0x80) != 0 ? 1 : 0);

    int length = _frst.length + _scnd.length + 6 + frst + scnd;
    tmp = new byte[length];
    tmp[0] = (byte) 0x30;
    tmp[1] = (byte) (_frst.length + _scnd.length + 4);
    tmp[1] += (byte) frst;
    tmp[1] += (byte) scnd;
    tmp[2] = (byte) 0x02;
    tmp[3] = (byte) _frst.length;
    tmp[3] += (byte) frst;
    System.arraycopy(_frst, 0, tmp, 4 + frst, _frst.length);
    tmp[4 + tmp[3]] = (byte) 0x02;
    tmp[5 + tmp[3]] = (byte) _scnd.length;
    tmp[5 + tmp[3]] += (byte) scnd;
    System.arraycopy(_scnd, 0, tmp, 6 + tmp[3] + scnd, _scnd.length);
    sig = tmp;

    return signature.verify(sig);
  }

  protected byte[] normalize(byte[] secret) {
    if (secret.length > 1 && secret[0] == 0 && (secret[1] & 0x80) == 0) {
      byte[] tmp = new byte[secret.length - 1];
      System.arraycopy(secret, 1, tmp, 0, tmp.length);
      return normalize(tmp);
    } else {
      return secret;
    }
  }
}
