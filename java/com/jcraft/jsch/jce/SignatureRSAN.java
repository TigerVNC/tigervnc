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
import com.jcraft.jsch.SignatureRSA;
import java.math.BigInteger;
import java.nio.charset.StandardCharsets;
import java.security.KeyFactory;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.Signature;
import java.security.spec.RSAPrivateKeySpec;
import java.security.spec.RSAPublicKeySpec;

abstract class SignatureRSAN implements SignatureRSA {

  Signature signature;
  KeyFactory keyFactory;

  abstract String getName();

  @Override
  public void init() throws Exception {
    String name = getName();
    String foo = "SHA1withRSA";
    if (name.equals("rsa-sha2-256") || name.equals("ssh-rsa-sha256@ssh.com"))
      foo = "SHA256withRSA";
    else if (name.equals("rsa-sha2-512") || name.equals("ssh-rsa-sha512@ssh.com"))
      foo = "SHA512withRSA";
    else if (name.equals("ssh-rsa-sha384@ssh.com"))
      foo = "SHA384withRSA";
    else if (name.equals("ssh-rsa-sha224@ssh.com"))
      foo = "SHA224withRSA";
    signature = Signature.getInstance(foo);
    keyFactory = KeyFactory.getInstance("RSA");
  }

  @Override
  public void setPubKey(byte[] e, byte[] n) throws Exception {
    RSAPublicKeySpec rsaPubKeySpec = new RSAPublicKeySpec(new BigInteger(n), new BigInteger(e));
    PublicKey pubKey = keyFactory.generatePublic(rsaPubKeySpec);
    signature.initVerify(pubKey);
  }

  @Override
  public void setPrvKey(byte[] d, byte[] n) throws Exception {
    RSAPrivateKeySpec rsaPrivKeySpec = new RSAPrivateKeySpec(new BigInteger(n), new BigInteger(d));
    PrivateKey prvKey = keyFactory.generatePrivate(rsaPrivKeySpec);
    signature.initSign(prvKey);
  }

  @Override
  public byte[] sign() throws Exception {
    byte[] sig = signature.sign();
    return sig;
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

    String foo = new String(buf.getString(), StandardCharsets.UTF_8);
    if (foo.equals("ssh-rsa") || foo.equals("rsa-sha2-256") || foo.equals("rsa-sha2-512")
        || foo.equals("ssh-rsa-sha224@ssh.com") || foo.equals("ssh-rsa-sha256@ssh.com")
        || foo.equals("ssh-rsa-sha384@ssh.com") || foo.equals("ssh-rsa-sha512@ssh.com")) {
      if (!foo.equals(getName()))
        return false;
      j = buf.getInt();
      i = buf.getOffSet();
      tmp = new byte[j];
      System.arraycopy(sig, i, tmp, 0, j);
      sig = tmp;
    }

    return signature.verify(sig);
  }
}
