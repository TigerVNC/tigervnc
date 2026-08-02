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

import java.util.Arrays;

abstract class KeyPairEdDSA extends KeyPair {
  private byte[] pub_array;
  private byte[] prv_array;

  KeyPairEdDSA(JSch.InstanceLogger instLogger, byte[] pub_array, byte[] prv_array) {
    super(instLogger);
    this.pub_array = pub_array;
    this.prv_array = prv_array;
  }

  abstract String getSshName();

  abstract String getJceName();

  @Override
  void generate(int key_size) throws JSchException {
    try {
      Class<? extends KeyPairGenEdDSA> c =
          Class.forName(JSch.getConfig("keypairgen.eddsa")).asSubclass(KeyPairGenEdDSA.class);
      KeyPairGenEdDSA keypairgen = c.getDeclaredConstructor().newInstance();
      keypairgen.init(getJceName(), getKeySize());
      pub_array = keypairgen.getPub();
      prv_array = keypairgen.getPrv();

      keypairgen = null;
    } catch (Exception | LinkageError e) {
      throw new JSchException(e.toString(), e);
    }
  }

  // These methods appear to be for writing keys to a file.
  // And since writing VENDOR_OPENSSH_V1 isn't supported yet, have these methods fail.
  @Override
  byte[] getBegin() {
    throw new UnsupportedOperationException();
  }

  @Override
  byte[] getEnd() {
    throw new UnsupportedOperationException();
  }

  @Override
  byte[] getPrivateKey() {
    throw new UnsupportedOperationException();
  }

  @Override
  byte[] getOpenSSHv1PrivateKeyBlob() {
    byte[] keyTypeName = getKeyTypeName();
    if (keyTypeName == null || pub_array == null || prv_array == null) {
      return null;
    }

    byte[] sk = null;
    Buffer _buf = null;
    try {
      sk = new byte[prv_array.length + pub_array.length];
      System.arraycopy(prv_array, 0, sk, 0, prv_array.length);
      System.arraycopy(pub_array, 0, sk, prv_array.length, pub_array.length);

      int _bufLen = 4 + keyTypeName.length;
      _bufLen += 4 + pub_array.length;
      _bufLen += 4 + sk.length;
      _buf = new Buffer(_bufLen);
      _buf.putString(keyTypeName);
      _buf.putString(pub_array);
      _buf.putString(sk);

      return _buf.buffer;
    } catch (Exception e) {
      if (_buf != null) {
        Util.bzero(_buf.buffer);
      }
      throw e;
    } finally {
      Util.bzero(sk);
    }
  }

  @Override
  boolean parse(byte[] plain) {
    if (vendor == VENDOR_PUTTY || vendor == VENDOR_PUTTY_V3) {
      Buffer buf = new Buffer(plain);
      buf.skip(plain.length);

      try {
        byte[][] tmp = buf.getBytes(1, "");
        prv_array = tmp[0];
      } catch (JSchException e) {
        if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
          instLogger.getLogger().log(Logger.ERROR, "failed to parse key", e);
        }
        return false;
      }

      return true;
    } else if (vendor == VENDOR_OPENSSH_V1) {
      try {
        // OPENSSH Key v1 Format
        Buffer prvKeyBuffer = new Buffer(plain);
        int checkInt1 = prvKeyBuffer.getInt(); // uint32 checkint1
        int checkInt2 = prvKeyBuffer.getInt(); // uint32 checkint2
        if (checkInt1 != checkInt2) {
          throw new JSchException("check failed");
        }

        String keyType = Util.byte2str(prvKeyBuffer.getString()); // string keytype
        pub_array = prvKeyBuffer.getString(); // public key
        // OpenSSH stores private key in first half of string and duplicate copy of public key in
        // second half of string
        byte[] tmp = prvKeyBuffer.getString(); // secret key (private key + public key)
        prv_array = Arrays.copyOf(tmp, getKeySize());
        publicKeyComment = Util.byte2str(prvKeyBuffer.getString());

        return true;
      } catch (Exception e) {
        if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
          instLogger.getLogger().log(Logger.ERROR, "failed to parse key", e);
        }
        return false;
      }
    } else if (vendor == VENDOR_PKCS8) {
      try {
        Class<? extends KeyPairGenEdDSA> c =
            Class.forName(JSch.getConfig("keypairgen_fromprivate.eddsa"))
                .asSubclass(KeyPairGenEdDSA.class);
        KeyPairGenEdDSA keypairgen = c.getDeclaredConstructor().newInstance();
        keypairgen.init(getJceName(), plain);
        pub_array = keypairgen.getPub();
        prv_array = keypairgen.getPrv();
        return true;
      } catch (Exception | LinkageError e) {
        if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
          instLogger.getLogger().log(Logger.ERROR, "failed to parse key", e);
        }
        return false;
      }
    } else {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to parse key");
      }
      return false;
    }
  }

  @Override
  public byte[] getPublicKeyBlob() {
    byte[] foo = super.getPublicKeyBlob();
    if (foo != null)
      return foo;

    if (pub_array == null)
      return null;
    byte[][] tmp = new byte[2][];
    tmp[0] = getKeyTypeName();
    tmp[1] = pub_array;
    return Buffer.fromBytes(tmp).buffer;
  }

  @Override
  byte[] getKeyTypeName() {
    return Util.str2byte(getSshName());
  }

  @Override
  public byte[] getSignature(byte[] data) {
    return getSignature(data, getSshName());
  }

  @Override
  public byte[] getSignature(byte[] data, String alg) {
    try {
      Class<? extends SignatureEdDSA> c =
          Class.forName(JSch.getConfig(alg)).asSubclass(SignatureEdDSA.class);
      SignatureEdDSA eddsa = c.getDeclaredConstructor().newInstance();
      eddsa.init();
      eddsa.setPrvKey(prv_array);

      eddsa.update(data);
      byte[] sig = eddsa.sign();
      byte[][] tmp = new byte[2][];
      tmp[0] = Util.str2byte(alg);
      tmp[1] = sig;
      return Buffer.fromBytes(tmp).buffer;
    } catch (Exception | LinkageError e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to generate signature", e);
      }
    }
    return null;
  }

  @Override
  public Signature getVerifier() {
    return getVerifier(getSshName());
  }

  @Override
  public Signature getVerifier(String alg) {
    try {
      Class<? extends SignatureEdDSA> c =
          Class.forName(JSch.getConfig(alg)).asSubclass(SignatureEdDSA.class);
      SignatureEdDSA eddsa = c.getDeclaredConstructor().newInstance();
      eddsa.init();

      if (pub_array == null && getPublicKeyBlob() != null) {
        Buffer buf = new Buffer(getPublicKeyBlob());
        buf.getString();
        pub_array = buf.getString();
      }

      eddsa.setPubKey(pub_array);
      return eddsa;
    } catch (Exception | LinkageError e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to create verifier", e);
      }
    }
    return null;
  }

  @Override
  public byte[] forSSHAgent() throws JSchException {
    if (isEncrypted()) {
      throw new JSchException("key is encrypted.");
    }
    Buffer buf = new Buffer();
    buf.putString(getKeyTypeName());
    buf.putString(pub_array);
    byte[] tmp = new byte[prv_array.length + pub_array.length];
    System.arraycopy(prv_array, 0, tmp, 0, prv_array.length);
    System.arraycopy(pub_array, 0, tmp, prv_array.length, pub_array.length);
    buf.putString(tmp);
    buf.putString(Util.str2byte(publicKeyComment));
    byte[] result = new byte[buf.getLength()];
    buf.getByte(result, 0, result.length);
    return result;
  }

  @Override
  public void dispose() {
    super.dispose();
    Util.bzero(prv_array);
  }
}
