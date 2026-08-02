/*
 * Copyright (c) 2015-2018 ymnk, JCraft,Inc. All rights reserved.
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

class KeyPairECDSA extends KeyPair {

  private static byte[][] oids = {{(byte) 0x06, (byte) 0x08, (byte) 0x2a, (byte) 0x86, (byte) 0x48, // 256
      (byte) 0xce, (byte) 0x3d, (byte) 0x03, (byte) 0x01, (byte) 0x07},
      {(byte) 0x06, (byte) 0x05, (byte) 0x2b, (byte) 0x81, (byte) 0x04, // 384
          (byte) 0x00, (byte) 0x22},
      {(byte) 0x06, (byte) 0x05, (byte) 0x2b, (byte) 0x81, (byte) 0x04, // 521
          (byte) 0x00, (byte) 0x23},};

  private static String[] names = {"nistp256", "nistp384", "nistp521"};
  private static String[] keyTypeNames =
      Arrays.stream(names).map(s -> "ecdsa-sha2-" + s).toArray(String[]::new);

  private byte[] name = Util.str2byte(names[0]);
  private byte[] r_array;
  private byte[] s_array;
  private byte[] prv_array;

  private int key_size = 256;

  KeyPairECDSA(JSch.InstanceLogger instLogger) {
    this(instLogger, null, null, null, null);
  }

  KeyPairECDSA(JSch.InstanceLogger instLogger, byte[] pubkey) {
    this(instLogger, null, null, null, null);

    if (pubkey != null) {
      byte[] name = new byte[8];
      System.arraycopy(pubkey, 11, name, 0, 8);
      if (Util.array_equals(name, Util.str2byte("nistp384"))) {
        key_size = 384;
        this.name = name;
      }
      if (Util.array_equals(name, Util.str2byte("nistp521"))) {
        key_size = 521;
        this.name = name;
      }
    }
  }

  KeyPairECDSA(JSch.InstanceLogger instLogger, byte[] name, byte[] r_array, byte[] s_array,
      byte[] prv_array) {
    super(instLogger);
    if (name != null)
      this.name = name;
    this.r_array = r_array;
    this.s_array = s_array;
    this.prv_array = prv_array;
    if (prv_array != null)
      key_size = prv_array.length >= 64 ? 521 : (prv_array.length >= 48 ? 384 : 256);
  }

  @Override
  void generate(int key_size) throws JSchException {
    this.key_size = key_size;
    try {
      Class<? extends KeyPairGenECDSA> c =
          Class.forName(JSch.getConfig("keypairgen.ecdsa")).asSubclass(KeyPairGenECDSA.class);
      KeyPairGenECDSA keypairgen = c.getDeclaredConstructor().newInstance();
      keypairgen.init(key_size);
      prv_array = keypairgen.getD();
      r_array = keypairgen.getR();
      s_array = keypairgen.getS();
      name = Util.str2byte(names[prv_array.length >= 64 ? 2 : (prv_array.length >= 48 ? 1 : 0)]);
      keypairgen = null;
    } catch (Exception e) {
      throw new JSchException(e.toString(), e);
    }
  }

  private static final byte[] begin = Util.str2byte("-----BEGIN EC PRIVATE KEY-----");
  private static final byte[] end = Util.str2byte("-----END EC PRIVATE KEY-----");

  @Override
  byte[] getBegin() {
    return begin;
  }

  @Override
  byte[] getEnd() {
    return end;
  }

  @Override
  byte[] getPrivateKey() {

    byte[] tmp = new byte[1];
    tmp[0] = 1;

    byte[] oid = oids[(r_array.length >= 64) ? 2 : ((r_array.length >= 48) ? 1 : 0)];

    byte[] point = toPoint(r_array, s_array);

    int bar = ((point.length + 1) & 0x80) == 0 ? 3 : 4;
    byte[] foo = new byte[point.length + bar];
    System.arraycopy(point, 0, foo, bar, point.length);
    foo[0] = 0x03; // BITSTRING
    if (bar == 3) {
      foo[1] = (byte) (point.length + 1);
    } else {
      foo[1] = (byte) 0x81;
      foo[2] = (byte) (point.length + 1);
    }
    point = foo;

    int content = 1 + countLength(tmp.length) + tmp.length + 1 + countLength(prv_array.length)
        + prv_array.length + 1 + countLength(oid.length) + oid.length + 1
        + countLength(point.length) + point.length;

    int total = 1 + countLength(content) + content; // SEQUENCE

    byte[] plain = new byte[total];
    int index = 0;
    index = writeSEQUENCE(plain, index, content);
    index = writeINTEGER(plain, index, tmp);
    index = writeOCTETSTRING(plain, index, prv_array);
    index = writeDATA(plain, (byte) 0xa0, index, oid);
    index = writeDATA(plain, (byte) 0xa1, index, point);

    return plain;
  }

  @Override
  byte[] getOpenSSHv1PrivateKeyBlob() {
    byte[] keyTypeName = getKeyTypeName();
    byte[] ecPoint = toPoint(r_array, s_array);
    if (keyTypeName == null || ecPoint == null || prv_array == null) {
      return null;
    }

    Buffer _buf = null;
    try {
      int _bufLen = 4 + keyTypeName.length;
      _bufLen += 4 + name.length;
      _bufLen += 4 + ecPoint.length;
      _bufLen += 4 + prv_array.length;
      _bufLen += (prv_array[0] & 0x80) >>> 7;
      _buf = new Buffer(_bufLen);
      _buf.putString(keyTypeName);
      _buf.putString(name);
      _buf.putString(ecPoint);
      _buf.putMPInt(prv_array);

      return _buf.buffer;
    } catch (Exception e) {
      if (_buf != null) {
        Util.bzero(_buf.buffer);
      }
      throw e;
    }
  }

  @Override
  boolean parse(byte[] plain) {
    try {

      if (vendor == VENDOR_FSECURE) {
        /*
         * if(plain[0]!=0x30){ // FSecure return true; } return false;
         */
        return false;
      } else if (vendor == VENDOR_PUTTY || vendor == VENDOR_PUTTY_V3) {
        Buffer buf = new Buffer(plain);
        buf.skip(plain.length);

        try {
          byte[][] tmp = buf.getBytes(1, "");
          prv_array = tmp[0];
          key_size = prv_array.length >= 64 ? 521 : (prv_array.length >= 48 ? 384 : 256);
        } catch (JSchException e) {
          if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
            instLogger.getLogger().log(Logger.ERROR, "failed to parse key", e);
          }
          return false;
        }

        return true;
      }

      // OPENSSH Key v1 Format
      if (vendor == VENDOR_OPENSSH_V1) {
        Buffer prvKeyBuffer = new Buffer(plain);
        int checkInt1 = prvKeyBuffer.getInt(); // uint32 checkint1
        int checkInt2 = prvKeyBuffer.getInt(); // uint32 checkint2
        if (checkInt1 != checkInt2) {
          throw new JSchException("check failed");
        }

        String keyType = Util.byte2str(prvKeyBuffer.getString()); // string keytype
        if (!Arrays.asList(keyTypeNames).contains(keyType)) {
          throw new IllegalArgumentException("unknown key type " + keyType);
        }

        name = prvKeyBuffer.getString();
        String nameStr = Util.byte2str(name);
        if (!Arrays.asList(names).contains(nameStr)) {
          throw new IllegalArgumentException("unknown curve name " + nameStr);
        }

        if (!keyType.endsWith(nameStr)) {
          throw new IllegalArgumentException(
              "key type " + keyType + " does not match curve name " + nameStr);
        }

        switch (nameStr) {
          case "nistp521":
            key_size = 521;
            break;
          case "nistp384":
            key_size = 384;
            break;
          case "nistp256":
            key_size = 256;
            break;
        }

        byte[] ecPoint = prvKeyBuffer.getString();
        // https://datatracker.ietf.org/doc/html/rfc5480#section-2.2
        if (ecPoint[0] != (byte) 0x04) {
          throw new IllegalArgumentException("only uncompressed ECPoint supported");
        }
        byte[][] r_s = fromPoint(ecPoint);
        prv_array = prvKeyBuffer.getMPInt();
        publicKeyComment = Util.byte2str(prvKeyBuffer.getString());
        r_array = r_s[0];
        s_array = r_s[1];

        return true;
      }

      int index = 0;
      int length = 0;

      if (plain[index] != 0x30)
        return false;
      index++; // SEQUENCE
      length = plain[index++] & 0xff;
      if ((length & 0x80) != 0) {
        int foo = length & 0x7f;
        length = 0;
        while (foo-- > 0) {
          length = (length << 8) + (plain[index++] & 0xff);
        }
      }

      if (plain[index] != 0x02)
        return false;
      index++; // INTEGER

      length = plain[index++] & 0xff;
      if ((length & 0x80) != 0) {
        int foo = length & 0x7f;
        length = 0;
        while (foo-- > 0) {
          length = (length << 8) + (plain[index++] & 0xff);
        }
      }

      index += length;
      index++; // 0x04

      length = plain[index++] & 0xff;
      if ((length & 0x80) != 0) {
        int foo = length & 0x7f;
        length = 0;
        while (foo-- > 0) {
          length = (length << 8) + (plain[index++] & 0xff);
        }
      }

      prv_array = new byte[length];
      System.arraycopy(plain, index, prv_array, 0, length);

      index += length;

      index++; // 0xa0

      length = plain[index++] & 0xff;
      if ((length & 0x80) != 0) {
        int foo = length & 0x7f;
        length = 0;
        while (foo-- > 0) {
          length = (length << 8) + (plain[index++] & 0xff);
        }
      }

      byte[] oid_array = new byte[length];
      System.arraycopy(plain, index, oid_array, 0, length);
      index += length;

      for (int i = 0; i < oids.length; i++) {
        if (Util.array_equals(oids[i], oid_array)) {
          name = Util.str2byte(names[i]);
          break;
        }
      }

      index++; // 0xa1

      length = plain[index++] & 0xff;
      if ((length & 0x80) != 0) {
        int foo = length & 0x7f;
        length = 0;
        while (foo-- > 0) {
          length = (length << 8) + (plain[index++] & 0xff);
        }
      }

      byte[] Q_array = new byte[length];
      System.arraycopy(plain, index, Q_array, 0, length);
      index += length;

      byte[][] tmp = fromPoint(Q_array);
      r_array = tmp[0];
      s_array = tmp[1];

      if (prv_array != null)
        key_size = prv_array.length >= 64 ? 521 : (prv_array.length >= 48 ? 384 : 256);
    } catch (Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to parse key", e);
      }
      return false;
    }
    return true;
  }

  @Override
  public byte[] getPublicKeyBlob() {
    byte[] foo = super.getPublicKeyBlob();

    if (foo != null)
      return foo;

    if (r_array == null)
      return null;

    byte[][] tmp = new byte[3][];
    tmp[0] = Util.str2byte("ecdsa-sha2-" + Util.byte2str(name));
    tmp[1] = name;
    tmp[2] = new byte[1 + r_array.length + s_array.length];
    tmp[2][0] = 4; // POINT_CONVERSION_UNCOMPRESSED
    System.arraycopy(r_array, 0, tmp[2], 1, r_array.length);
    System.arraycopy(s_array, 0, tmp[2], 1 + r_array.length, s_array.length);

    return Buffer.fromBytes(tmp).buffer;
  }

  @Override
  byte[] getKeyTypeName() {
    return Util.str2byte("ecdsa-sha2-" + Util.byte2str(name));
  }

  @Override
  public int getKeyType() {
    return ECDSA;
  }

  @Override
  public int getKeySize() {
    return key_size;
  }

  @Override
  public byte[] getSignature(byte[] data) {
    byte[] keyCopy = null;
    try {
      Class<? extends SignatureECDSA> c =
          Class.forName(JSch.getConfig("ecdsa-sha2-" + Util.byte2str(name)))
              .asSubclass(SignatureECDSA.class);
      SignatureECDSA ecdsa = c.getDeclaredConstructor().newInstance();
      ecdsa.init();
      // https://github.com/mwiede/jsch/issues/739 : prv_array could be destroyed by ecdsa signing
      keyCopy = Arrays.copyOf(prv_array, prv_array.length);
      ecdsa.setPrvKey(keyCopy);

      ecdsa.update(data);
      byte[] sig = ecdsa.sign();

      byte[][] tmp = new byte[2][];
      tmp[0] = Util.str2byte("ecdsa-sha2-" + Util.byte2str(name));
      tmp[1] = sig;
      return Buffer.fromBytes(tmp).buffer;
    } catch (Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to generate signature", e);
      }
    } finally {
      Util.bzero(keyCopy);
    }
    return null;
  }

  @Override
  public byte[] getSignature(byte[] data, String al) {
    return getSignature(data);
  }

  @Override
  public Signature getVerifier() {
    try {
      Class<? extends SignatureECDSA> c =
          Class.forName(JSch.getConfig("ecdsa-sha2-" + Util.byte2str(name)))
              .asSubclass(SignatureECDSA.class);
      final SignatureECDSA ecdsa = c.getDeclaredConstructor().newInstance();
      ecdsa.init();

      if (r_array == null && s_array == null && getPublicKeyBlob() != null) {
        Buffer buf = new Buffer(getPublicKeyBlob());
        buf.getString(); // ecdsa-sha2-nistp256
        buf.getString(); // nistp256
        byte[][] tmp = fromPoint(buf.getString());
        r_array = tmp[0];
        s_array = tmp[1];
      }
      // https://github.com/mwiede/jsch/issues/739 : keys could be destroyed by ecdsa verification
      ecdsa.setPubKey(Arrays.copyOf(r_array, r_array.length),
          Arrays.copyOf(s_array, s_array.length));
      return ecdsa;
    } catch (Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to create verifier", e);
      }
    }
    return null;
  }

  @Override
  public Signature getVerifier(String alg) {
    return getVerifier();
  }

  static KeyPair fromSSHAgent(JSch.InstanceLogger instLogger, Buffer buf) throws JSchException {

    byte[][] tmp = buf.getBytes(5, "invalid key format");

    byte[] name = tmp[1]; // nistp256
    byte[][] foo = fromPoint(tmp[2]);
    byte[] r_array = foo[0];
    byte[] s_array = foo[1];

    byte[] prv_array = tmp[3];
    KeyPairECDSA kpair = new KeyPairECDSA(instLogger, name, r_array, s_array, prv_array);
    kpair.publicKeyComment = Util.byte2str(tmp[4]);
    kpair.vendor = VENDOR_OPENSSH;
    return kpair;
  }

  @Override
  public byte[] forSSHAgent() throws JSchException {
    if (isEncrypted()) {
      throw new JSchException("key is encrypted.");
    }
    Buffer buf = new Buffer();
    buf.putString(Util.str2byte("ecdsa-sha2-" + Util.byte2str(name)));
    buf.putString(name);
    buf.putString(toPoint(r_array, s_array));
    buf.putString(prv_array);
    buf.putString(Util.str2byte(publicKeyComment));
    byte[] result = new byte[buf.getLength()];
    buf.getByte(result, 0, result.length);
    return result;
  }

  static byte[] toPoint(byte[] r_array, byte[] s_array) {
    byte[] tmp = new byte[1 + r_array.length + s_array.length];
    tmp[0] = 0x04;
    System.arraycopy(r_array, 0, tmp, 1, r_array.length);
    System.arraycopy(s_array, 0, tmp, 1 + r_array.length, s_array.length);
    return tmp;
  }

  static byte[][] fromPoint(byte[] point) {
    int i = 0;
    while (point[i] != 4)
      i++;
    i++;
    byte[][] tmp = new byte[2][];
    byte[] r_array = new byte[(point.length - i) / 2];
    byte[] s_array = new byte[(point.length - i) / 2];
    // point[0] == 0x04 == POINT_CONVERSION_UNCOMPRESSED
    System.arraycopy(point, i, r_array, 0, r_array.length);
    System.arraycopy(point, i + r_array.length, s_array, 0, s_array.length);
    tmp[0] = r_array;
    tmp[1] = s_array;

    return tmp;
  }

  @Override
  public void dispose() {
    super.dispose();
    Util.bzero(prv_array);
  }
}
