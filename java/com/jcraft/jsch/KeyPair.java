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

import com.jcraft.jsch.asn1.ASN1;
import com.jcraft.jsch.asn1.ASN1Exception;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;
import java.util.Arrays;
import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

public abstract class KeyPair {

  /** DEFERRED should not be be used. */
  public static final int DEFERRED = -1;

  public static final int ERROR = 0;
  public static final int DSA = 1;
  public static final int RSA = 2;
  public static final int ECDSA = 3;
  public static final int UNKNOWN = 4;
  public static final int ED25519 = 5;
  public static final int ED448 = 6;

  static final int VENDOR_OPENSSH = 0;
  static final int VENDOR_FSECURE = 1;
  static final int VENDOR_PUTTY = 2;
  static final int VENDOR_PKCS8 = 3;
  static final int VENDOR_OPENSSH_V1 = 4;
  static final int VENDOR_PUTTY_V3 = 5;

  int vendor = VENDOR_OPENSSH;

  private static final byte[] AUTH_MAGIC = Util.str2byte("openssh-key-v1\0");
  private static final byte[] cr = Util.str2byte("\n");
  private static final byte[] OPENSSH_V1_BEGIN =
      Util.str2byte("-----BEGIN OPENSSH PRIVATE KEY-----");
  private static final byte[] OPENSSH_V1_END = Util.str2byte("-----END OPENSSH PRIVATE KEY-----");
  private static final byte[] OPENSSH_V1_NONE = Util.str2byte("none");
  private static final String OPENSSH_V1_DEFAULT_CIPHERNAME = "aes256-ctr";
  private static final String OPENSSH_V1_KDFNAME = "bcrypt";
  private static final int OPENSSH_V1_SALT_LEN = 16;
  private static final int OPENSSH_V1_DEFAULT_ROUNDS = 16;

  public static KeyPair genKeyPair(JSch jsch, int type) throws JSchException {
    return genKeyPair(jsch, type, 1024);
  }

  public static KeyPair genKeyPair(JSch jsch, int type, int key_size) throws JSchException {
    KeyPair kpair = null;
    if (type == DSA) {
      kpair = new KeyPairDSA(jsch.instLogger);
    } else if (type == RSA) {
      kpair = new KeyPairRSA(jsch.instLogger);
    } else if (type == ECDSA) {
      kpair = new KeyPairECDSA(jsch.instLogger);
    } else if (type == ED25519) {
      kpair = new KeyPairEd25519(jsch.instLogger);
    } else if (type == ED448) {
      kpair = new KeyPairEd448(jsch.instLogger);
    }
    if (kpair != null) {
      kpair.generate(key_size);
    }
    return kpair;
  }

  abstract void generate(int key_size) throws JSchException;

  abstract byte[] getBegin();

  abstract byte[] getEnd();

  public abstract int getKeySize();

  public abstract byte[] getSignature(byte[] data);

  public abstract byte[] getSignature(byte[] data, String alg);

  public abstract Signature getVerifier();

  public abstract Signature getVerifier(String alg);

  public abstract byte[] forSSHAgent() throws JSchException;

  public String getPublicKeyComment() {
    return publicKeyComment;
  }

  public void setPublicKeyComment(String publicKeyComment) {
    this.publicKeyComment = publicKeyComment;
  }

  protected String publicKeyComment = "no comment";

  JSch.InstanceLogger instLogger;
  protected Cipher cipher;
  private KDF kdf;
  private HASH sha1;
  private HASH hash;
  private Random random;

  KeyPair(JSch.InstanceLogger instLogger) {
    this.instLogger = instLogger;
  }

  static byte[][] header =
      {Util.str2byte("Proc-Type: 4,ENCRYPTED"), Util.str2byte("DEK-Info: DES-EDE3-CBC,")};

  abstract byte[] getPrivateKey();

  abstract byte[] getOpenSSHv1PrivateKeyBlob();

  /**
   * Writes the plain private key to the given output stream.
   *
   * @param out output stream
   * @see #writePrivateKey(OutputStream out, byte[] passphrase)
   */
  public void writePrivateKey(OutputStream out) {
    this.writePrivateKey(out, null);
  }

  /**
   * Writes the cyphered private key to the given output stream.
   *
   * @param out output stream
   * @param passphrase a passphrase to encrypt the private key
   */
  public void writePrivateKey(OutputStream out, byte[] passphrase) {
    byte[] plain = getPrivateKey();
    byte[][] _iv = new byte[1][];
    byte[] encoded = encrypt(plain, _iv, passphrase);
    if (encoded != plain)
      Util.bzero(plain);
    byte[] iv = _iv[0];
    byte[] prv = Util.toBase64(encoded, 0, encoded.length, true);

    try {
      out.write(getBegin());
      out.write(cr);
      if (passphrase != null) {
        out.write(header[0]);
        out.write(cr);
        out.write(header[1]);
        for (int i = 0; i < iv.length; i++) {
          out.write(b2a((byte) ((iv[i] >>> 4) & 0x0f)));
          out.write(b2a((byte) (iv[i] & 0x0f)));
        }
        out.write(cr);
        out.write(cr);
      }
      int i = 0;
      while (i < prv.length) {
        if (i + 64 < prv.length) {
          out.write(prv, i, 64);
          out.write(cr);
          i += 64;
          continue;
        }
        out.write(prv, i, prv.length - i);
        out.write(cr);
        break;
      }
      out.write(getEnd());
      out.write(cr);
      // out.close();
    } catch (Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to write private key", e);
      }
    }
  }

  /**
   * Writes the cyphered private key to the given output stream.
   *
   * @param out output stream
   * @param passphrase a passphrase to encrypt the private key
   */
  public void writeOpenSSHv1PrivateKey(OutputStream out, byte[] passphrase) throws JSchException {
    writeOpenSSHv1PrivateKey(out, passphrase, null, OPENSSH_V1_DEFAULT_ROUNDS);
  }

  /**
   * Writes the cyphered private key to the given output stream.
   *
   * @param out output stream
   * @param passphrase a passphrase to encrypt the private key
   * @param cipher the cipher to use
   */
  public void writeOpenSSHv1PrivateKey(OutputStream out, byte[] passphrase, String cipher)
      throws JSchException {
    writeOpenSSHv1PrivateKey(out, passphrase, cipher, OPENSSH_V1_DEFAULT_ROUNDS);
  }

  /**
   * Writes the cyphered private key to the given output stream.
   *
   * @param out output stream
   * @param passphrase a passphrase to encrypt the private key
   * @param rounds the number of KDF rounds to use
   */
  public void writeOpenSSHv1PrivateKey(OutputStream out, byte[] passphrase, int rounds)
      throws JSchException {
    writeOpenSSHv1PrivateKey(out, passphrase, null, rounds);
  }

  /**
   * Writes the cyphered private key to the given output stream.
   *
   * @param out output stream
   * @param passphrase a passphrase to encrypt the private key
   * @param cipher the cipher to use
   * @param rounds the number of KDF rounds to use
   */
  public void writeOpenSSHv1PrivateKey(OutputStream out, byte[] passphrase, String cipher,
      int rounds) throws JSchException {
    if (passphrase != null && passphrase.length == 0) {
      passphrase = null;
    }
    if (cipher == null) {
      cipher = OPENSSH_V1_DEFAULT_CIPHERNAME;
    }

    byte[] cipherName = OPENSSH_V1_NONE;
    byte[] kdfName = OPENSSH_V1_NONE;
    int nrKeys = 1;
    int cipherSize = 8;
    int tagSize = 0;
    byte[] publicKeyBlob = getPublicKeyBlob();
    if (publicKeyBlob == null) {
      throw new JSchException("Unable to get public key blob");
    }
    Buffer kdfOptions = new Buffer(0);

    if (random == null) {
      random = genRandom();
    }

    byte[] privateKeyBlob = null;
    Cipher _cipher = null;
    KDF _kdf = null;
    byte[] _keyiv = null;
    byte[] _key = null;
    byte[] _iv = null;
    Buffer privateKeys = null;
    Buffer _buf = null;
    byte[] prv = null;
    try {
      privateKeyBlob = getOpenSSHv1PrivateKeyBlob();
      if (privateKeyBlob == null) {
        throw new JSchException("Unable to get private key blob");
      }

      if (passphrase != null) {
        try {
          Class<? extends Cipher> c =
              Class.forName(JSch.getConfig(cipher)).asSubclass(Cipher.class);
          _cipher = c.getDeclaredConstructor().newInstance();
        } catch (Exception | LinkageError e) {
          if (e instanceof JSchException)
            throw (JSchException) e;
          throw new JSchException("cipher " + cipher + " is not available", e);
        }

        String kdf = OPENSSH_V1_KDFNAME;
        try {
          Class<? extends KDF> c = Class.forName(JSch.getConfig(kdf)).asSubclass(KDF.class);
          _kdf = c.getDeclaredConstructor().newInstance();
        } catch (Exception | LinkageError e) {
          if (e instanceof JSchException)
            throw (JSchException) e;
          throw new JSchException("kdf " + kdf + " is not available", e);
        }

        kdfName = Util.str2byte(kdf);
        byte[] _salt = new byte[OPENSSH_V1_SALT_LEN];
        random.fill(_salt, 0, _salt.length);

        int kdfOptionsLen = 4 + _salt.length;
        kdfOptionsLen += 4; // rounds
        kdfOptions = new Buffer(kdfOptionsLen);
        kdfOptions.putString(_salt);
        kdfOptions.putInt(rounds);
        _kdf.initWithOpenSSHv1KDFOptions(kdfOptions.buffer);

        cipherName = Util.str2byte(cipher);
        cipherSize = _cipher.getIVSize();
        tagSize = _cipher.getTagSize();
        _key = new byte[_cipher.getBlockSize()];
        int _ivLen = _cipher.getIVSize();
        if (_cipher.isChaCha20()) {
          _ivLen = 0;
        } else if (_cipher.isAEAD()) {
          _ivLen = 12;
        }
        _iv = new byte[_ivLen];
        _keyiv = _kdf.getKey(passphrase, _key.length + _iv.length);
        System.arraycopy(_keyiv, 0, _key, 0, _key.length);
        System.arraycopy(_keyiv, _key.length, _iv, 0, _iv.length);
        _cipher.init(Cipher.ENCRYPT_MODE, _key, _iv);
      }

      byte[] _c = new byte[4];
      random.fill(_c, 0, _c.length);
      int _check = ((_c[0] << 24) & 0xff000000) | ((_c[1] << 16) & 0x00ff0000)
          | ((_c[2] << 8) & 0x0000ff00) | ((_c[3]) & 0x000000ff);
      byte[] _publicKeyComment = Util.str2byte(getPublicKeyComment());
      if (_publicKeyComment == null) {
        _publicKeyComment = new byte[0];
      }

      int privateKeysLen = 4; // check
      privateKeysLen += 4; // check
      privateKeysLen += privateKeyBlob.length;
      privateKeysLen += 4 + _publicKeyComment.length;
      privateKeysLen += cipherSize;
      privateKeysLen += tagSize;
      privateKeys = new Buffer(privateKeysLen);
      privateKeys.putInt(_check);
      privateKeys.putInt(_check);
      privateKeys.putByte(privateKeyBlob);
      privateKeys.putString(_publicKeyComment);
      byte j = (byte) 1;
      while (privateKeys.getLength() % cipherSize != 0) {
        privateKeys.putByte(j++);
      }

      if (passphrase != null) {
        if (_cipher.isChaCha20()) {
          _cipher.update(0);
          _cipher.doFinal(privateKeys.buffer, -4, privateKeys.getLength(), privateKeys.buffer, 0);
        } else if (_cipher.isAEAD()) {
          _cipher.doFinal(privateKeys.buffer, 0, privateKeys.getLength(), privateKeys.buffer, 0);
        } else {
          _cipher.update(privateKeys.buffer, 0, privateKeys.getLength(), privateKeys.buffer, 0);
        }
      }

      int _bufLen = AUTH_MAGIC.length;
      _bufLen += 4 + cipherName.length;
      _bufLen += 4 + kdfName.length;
      _bufLen += 4 + kdfOptions.getLength();
      _bufLen += 4; // nrKeys
      _bufLen += 4 + publicKeyBlob.length;
      _bufLen += 4 + privateKeys.getLength();
      _bufLen += tagSize;
      _buf = new Buffer(_bufLen);
      _buf.putByte(AUTH_MAGIC);
      _buf.putString(cipherName);
      _buf.putString(kdfName);
      _buf.putString(kdfOptions.buffer);
      _buf.putInt(nrKeys);
      _buf.putString(publicKeyBlob);
      _buf.putString(privateKeys.buffer, 0, privateKeys.getLength());
      _buf.putByte(privateKeys.buffer, privateKeys.getLength(), tagSize);
      prv = Util.toBase64(_buf.buffer, 0, _buf.getLength(), true);

      out.write(OPENSSH_V1_BEGIN);
      out.write(cr);
      int i = 0;
      while (i < prv.length) {
        if (i + 70 < prv.length) {
          out.write(prv, i, 70);
          out.write(cr);
          i += 70;
          continue;
        }
        out.write(prv, i, prv.length - i);
        out.write(cr);
        break;
      }
      out.write(OPENSSH_V1_END);
      out.write(cr);
    } catch (Exception e) {
      if (e instanceof JSchException)
        throw (JSchException) e;
      throw new JSchException(e.toString(), e);
    } finally {
      Util.bzero(_keyiv);
      Util.bzero(_key);
      Util.bzero(_iv);
      Util.bzero(privateKeyBlob);
      if (privateKeys != null) {
        Util.bzero(privateKeys.buffer);
      }
      if (_buf != null) {
        Util.bzero(_buf.buffer);
      }
      Util.bzero(prv);
    }
  }

  private static byte[] space = Util.str2byte(" ");

  abstract byte[] getKeyTypeName();

  public abstract int getKeyType();

  /**
   * Wrapper to provide the String representation of {@code #getKeyTypeName()}.
   *
   * @return the standard SSH key type string
   */
  public String getKeyTypeString() {
    return Util.byte2str(getKeyTypeName());
  }

  /**
   * Returns the blob of the public key.
   *
   * @return blob of the public key
   */
  public byte[] getPublicKeyBlob() {
    // TODO JSchException should be thrown
    // if(publickeyblob == null)
    // throw new JSchException("public-key blob is not available");
    return publickeyblob;
  }

  /**
   * Writes the public key with the specified comment to the output stream.
   *
   * @param out output stream
   * @param comment comment
   */
  public void writePublicKey(OutputStream out, String comment) {
    byte[] pubblob = getPublicKeyBlob();
    byte[] pub = Util.toBase64(pubblob, 0, pubblob.length, true);
    try {
      out.write(getKeyTypeName());
      out.write(space);
      out.write(pub, 0, pub.length);
      out.write(space);
      out.write(Util.str2byte(comment));
      out.write(cr);
    } catch (Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to write public key", e);
      }
    }
  }

  /**
   * Writes the public key with the specified comment to the file.
   *
   * @param name file name
   * @param comment comment
   * @see #writePublicKey(OutputStream out, String comment)
   */
  public void writePublicKey(String name, String comment)
      throws FileNotFoundException, IOException {
    try (OutputStream fos = new FileOutputStream(name)) {
      writePublicKey(fos, comment);
    }
  }

  /**
   * Writes the public key with the specified comment to the output stream in the format defined in
   * http://www.ietf.org/rfc/rfc4716.txt
   *
   * @param out output stream
   * @param comment comment
   */
  public void writeSECSHPublicKey(OutputStream out, String comment) {
    byte[] pubblob = getPublicKeyBlob();
    byte[] pub = Util.toBase64(pubblob, 0, pubblob.length, true);
    try {
      out.write(Util.str2byte("---- BEGIN SSH2 PUBLIC KEY ----"));
      out.write(cr);
      out.write(Util.str2byte("Comment: \"" + comment + "\""));
      out.write(cr);
      int index = 0;
      while (index < pub.length) {
        int len = 70;
        if ((pub.length - index) < len)
          len = pub.length - index;
        out.write(pub, index, len);
        out.write(cr);
        index += len;
      }
      out.write(Util.str2byte("---- END SSH2 PUBLIC KEY ----"));
      out.write(cr);
    } catch (Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to write public key", e);
      }
    }
  }

  /**
   * Writes the public key with the specified comment to the output stream in the format defined in
   * http://www.ietf.org/rfc/rfc4716.txt
   *
   * @param name file name
   * @param comment comment
   * @see #writeSECSHPublicKey(OutputStream out, String comment)
   */
  public void writeSECSHPublicKey(String name, String comment)
      throws FileNotFoundException, IOException {
    try (OutputStream fos = new FileOutputStream(name)) {
      writeSECSHPublicKey(fos, comment);
    }
  }

  /**
   * Writes the plain private key to the file.
   *
   * @param name file name
   * @see #writePrivateKey(String name, byte[] passphrase)
   */
  public void writePrivateKey(String name) throws FileNotFoundException, IOException {
    this.writePrivateKey(name, null);
  }

  /**
   * Writes the cyphered private key to the file.
   *
   * @param name file name
   * @param passphrase a passphrase to encrypt the private key
   * @see #writePrivateKey(OutputStream out, byte[] passphrase)
   */
  public void writePrivateKey(String name, byte[] passphrase)
      throws FileNotFoundException, IOException {
    try (OutputStream fos = new FileOutputStream(name)) {
      writePrivateKey(fos, passphrase);
    }
  }

  /**
   * Writes the plain private key to the file.
   *
   * @param name file name
   * @see #writePrivateKey(String name, byte[] passphrase)
   */
  public void writeOpenSSHv1PrivateKey(String name)
      throws FileNotFoundException, IOException, JSchException {
    this.writeOpenSSHv1PrivateKey(name, null);
  }

  /**
   * Writes the cyphered private key to the file.
   *
   * @param name file name
   * @param passphrase a passphrase to encrypt the private key
   * @see #writePrivateKey(OutputStream out, byte[] passphrase)
   */
  public void writeOpenSSHv1PrivateKey(String name, byte[] passphrase)
      throws FileNotFoundException, IOException, JSchException {
    try (OutputStream fos = new FileOutputStream(name)) {
      writeOpenSSHv1PrivateKey(fos, passphrase);
    }
  }

  /**
   * Writes the cyphered private key to the file.
   *
   * @param name file name
   * @param passphrase a passphrase to encrypt the private key
   * @param rounds the number of KDF rounds to use
   * @see #writePrivateKey(OutputStream out, byte[] passphrase)
   */
  public void writeOpenSSHv1PrivateKey(String name, byte[] passphrase, int rounds)
      throws FileNotFoundException, IOException, JSchException {
    try (OutputStream fos = new FileOutputStream(name)) {
      writeOpenSSHv1PrivateKey(fos, passphrase, rounds);
    }
  }

  /**
   * Writes the cyphered private key to the file.
   *
   * @param name file name
   * @param passphrase a passphrase to encrypt the private key
   * @param cipher the cipher to use
   * @see #writePrivateKey(OutputStream out, byte[] passphrase)
   */
  public void writeOpenSSHv1PrivateKey(String name, byte[] passphrase, String cipher)
      throws FileNotFoundException, IOException, JSchException {
    try (OutputStream fos = new FileOutputStream(name)) {
      writeOpenSSHv1PrivateKey(fos, passphrase, cipher);
    }
  }

  /**
   * Writes the cyphered private key to the file.
   *
   * @param name file name
   * @param passphrase a passphrase to encrypt the private key
   * @param cipher the cipher to use
   * @param rounds the number of KDF rounds to use
   * @see #writePrivateKey(OutputStream out, byte[] passphrase)
   */
  public void writeOpenSSHv1PrivateKey(String name, byte[] passphrase, String cipher, int rounds)
      throws FileNotFoundException, IOException, JSchException {
    try (OutputStream fos = new FileOutputStream(name)) {
      writeOpenSSHv1PrivateKey(fos, passphrase, cipher, rounds);
    }
  }

  /**
   * Returns the finger-print of the public key.
   *
   * @return finger print
   */
  public String getFingerPrint() {
    HASH _hash = genFingerPrintHash();
    if (_hash == null)
      return null;
    byte[] kblob = getPublicKeyBlob();
    if (kblob == null)
      return null;
    return Util.getFingerPrint(_hash, kblob, true, false);
  }

  private byte[] encrypt(byte[] plain, byte[][] _iv, byte[] passphrase) {
    if (passphrase == null)
      return plain;

    if (cipher == null)
      cipher = genCipher();
    byte[] iv = _iv[0] = new byte[cipher.getIVSize()];

    if (random == null)
      random = genRandom();
    random.fill(iv, 0, iv.length);

    byte[] key = genKey(passphrase, iv);
    byte[] encoded = plain;

    // PKCS#5Padding
    {
      // int bsize=cipher.getBlockSize();
      int bsize = cipher.getIVSize();
      byte[] foo = new byte[(encoded.length / bsize + 1) * bsize];
      System.arraycopy(encoded, 0, foo, 0, encoded.length);
      int padding = bsize - encoded.length % bsize;
      for (int i = foo.length - 1; (foo.length - padding) <= i; i--) {
        foo[i] = (byte) padding;
      }
      encoded = foo;
    }

    try {
      cipher.init(Cipher.ENCRYPT_MODE, key, iv);
      cipher.update(encoded, 0, encoded.length, encoded, 0);
    } catch (Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to encrypt key", e);
      }
    }
    Util.bzero(key);
    return encoded;
  }

  abstract boolean parse(byte[] data);

  private byte[] decrypt(byte[] data, byte[] passphrase, byte[] iv) {

    try {
      byte[] key = genKey(passphrase, iv);
      cipher.init(Cipher.DECRYPT_MODE, key, iv);
      Util.bzero(key);
      byte[] plain = new byte[data.length - cipher.getTagSize()];
      if (cipher.isChaCha20()) {
        cipher.update(0);
        cipher.doFinal(data, -4, data.length - cipher.getTagSize(), plain, 0);
      } else if (cipher.isAEAD()) {
        cipher.doFinal(data, 0, data.length, plain, 0);
      } else {
        cipher.update(data, 0, data.length, plain, 0);
      }
      return plain;
    } catch (Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to decrypt key", e);
      }
    }
    return null;
  }

  int writeSEQUENCE(byte[] buf, int index, int len) {
    buf[index++] = 0x30;
    index = writeLength(buf, index, len);
    return index;
  }

  int writeINTEGER(byte[] buf, int index, byte[] data) {
    buf[index++] = 0x02;
    index = writeLength(buf, index, data.length);
    System.arraycopy(data, 0, buf, index, data.length);
    index += data.length;
    return index;
  }

  int writeOCTETSTRING(byte[] buf, int index, byte[] data) {
    buf[index++] = 0x04;
    index = writeLength(buf, index, data.length);
    System.arraycopy(data, 0, buf, index, data.length);
    index += data.length;
    return index;
  }

  int writeDATA(byte[] buf, byte n, int index, byte[] data) {
    buf[index++] = n;
    index = writeLength(buf, index, data.length);
    System.arraycopy(data, 0, buf, index, data.length);
    index += data.length;
    return index;
  }

  int countLength(int len) {
    int i = 1;
    if (len <= 0x7f)
      return i;
    while (len > 0) {
      len >>>= 8;
      i++;
    }
    return i;
  }

  int writeLength(byte[] data, int index, int len) {
    int i = countLength(len) - 1;
    if (i == 0) {
      data[index++] = (byte) len;
      return index;
    }
    data[index++] = (byte) (0x80 | i);
    int j = index + i;
    while (i > 0) {
      data[index + i - 1] = (byte) (len & 0xff);
      len >>>= 8;
      i--;
    }
    return j;
  }

  private Random genRandom() {
    if (random == null) {
      try {
        Class<? extends Random> c =
            Class.forName(JSch.getConfig("random")).asSubclass(Random.class);
        random = c.getDeclaredConstructor().newInstance();
      } catch (Exception | LinkageError e) {
        if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
          instLogger.getLogger().log(Logger.ERROR, "failed to create random", e);
        }
      }
    }
    return random;
  }

  private HASH genHash() {
    try {
      Class<? extends HASH> c = Class.forName(JSch.getConfig("md5")).asSubclass(HASH.class);
      hash = c.getDeclaredConstructor().newInstance();
      hash.init();
    } catch (Exception | LinkageError e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to create hash", e);
      }
    }
    return hash;
  }

  private HASH genFingerPrintHash() {
    HASH _hash = null;
    try {
      String _c = JSch.getConfig("FingerprintHash").toLowerCase(Locale.ROOT);
      Class<? extends HASH> c = Class.forName(JSch.getConfig(_c)).asSubclass(HASH.class);
      _hash = c.getDeclaredConstructor().newInstance();
      _hash.init();
    } catch (Exception | LinkageError e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to create hash", e);
      }
    }
    return _hash;
  }

  private Cipher genCipher() {
    try {
      Class<? extends Cipher> c =
          Class.forName(JSch.getConfig("3des-cbc")).asSubclass(Cipher.class);
      cipher = c.getDeclaredConstructor().newInstance();
    } catch (Exception | LinkageError e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to create cipher", e);
      }
    }
    return cipher;
  }

  /*
   * hash is MD5 h(0) <- hash(passphrase, iv); h(n) <- hash(h(n-1), passphrase, iv); key <-
   * (h(0),...,h(n))[0,..,key.length];
   */
  synchronized byte[] genKey(byte[] passphrase, byte[] iv) {
    if (cipher == null)
      cipher = genCipher();
    if (hash == null)
      hash = genHash();

    byte[] key = new byte[cipher.getBlockSize()];
    int hsize = hash.getBlockSize();
    byte[] hn = new byte[key.length / hsize * hsize + (key.length % hsize == 0 ? 0 : hsize)];
    try {
      byte[] tmp = null;
      if (vendor == VENDOR_OPENSSH) {
        for (int index = 0; index + hsize <= hn.length;) {
          if (tmp != null) {
            hash.update(tmp, 0, tmp.length);
          }
          hash.update(passphrase, 0, passphrase.length);
          hash.update(iv, 0, iv.length > 8 ? 8 : iv.length);
          tmp = hash.digest();
          System.arraycopy(tmp, 0, hn, index, tmp.length);
          index += tmp.length;
        }
        System.arraycopy(hn, 0, key, 0, key.length);
      } else if (vendor == VENDOR_OPENSSH_V1) {
        tmp = kdf.getKey(passphrase, key.length + iv.length);
        System.arraycopy(tmp, 0, key, 0, key.length);
        System.arraycopy(tmp, key.length, iv, 0, iv.length);
        Util.bzero(tmp);
      } else if (vendor == VENDOR_FSECURE) {
        for (int index = 0; index + hsize <= hn.length;) {
          if (tmp != null) {
            hash.update(tmp, 0, tmp.length);
          }
          hash.update(passphrase, 0, passphrase.length);
          tmp = hash.digest();
          System.arraycopy(tmp, 0, hn, index, tmp.length);
          index += tmp.length;
        }
        System.arraycopy(hn, 0, key, 0, key.length);
      } else if (vendor == VENDOR_PUTTY) {
        byte[] i = new byte[4];

        sha1.update(i, 0, i.length);
        sha1.update(passphrase, 0, passphrase.length);
        tmp = sha1.digest();
        System.arraycopy(tmp, 0, key, 0, tmp.length);
        Util.bzero(tmp);

        i[3] = (byte) 1;
        sha1.update(i, 0, i.length);
        sha1.update(passphrase, 0, passphrase.length);
        tmp = sha1.digest();
        System.arraycopy(tmp, 0, key, tmp.length, key.length - tmp.length);
        Util.bzero(tmp);
      } else if (vendor == VENDOR_PUTTY_V3) {
        tmp = kdf.getKey(passphrase, key.length + iv.length + 32);
        System.arraycopy(tmp, 0, key, 0, key.length);
        System.arraycopy(tmp, key.length, iv, 0, iv.length);
        Util.bzero(tmp);
      }
    } catch (Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "failed to generate key from passphrase", e);
      }
    }
    return key;
  }

  /**
   * @deprecated use #writePrivateKey(OutputStream out, byte[] passphrase)
   */
  @Deprecated
  public void setPassphrase(String passphrase) {
    throw new UnsupportedOperationException("deprecated");
  }

  /**
   * @deprecated use #writePrivateKey(String name, byte[] passphrase)
   */
  @Deprecated
  public void setPassphrase(byte[] passphrase) {
    throw new UnsupportedOperationException("deprecated");
  }

  protected boolean encrypted = false;
  protected byte[] data = null;
  private byte[] iv = null;
  private byte[] publickeyblob = null;

  public boolean isEncrypted() {
    return encrypted;
  }

  /**
   * @deprecated use #decrypt(byte[] _passphrase)
   */
  @Deprecated
  public boolean decrypt(String _passphrase) {
    if (_passphrase == null || _passphrase.length() == 0) {
      return !encrypted;
    }
    return decrypt(Util.str2byte(_passphrase));
  }

  public boolean decrypt(byte[] _passphrase) {

    if (!encrypted) {
      return true;
    }
    if (_passphrase == null) {
      return !encrypted;
    }
    byte[] bar = new byte[_passphrase.length];
    System.arraycopy(_passphrase, 0, bar, 0, bar.length);
    _passphrase = bar;
    byte[] foo = null;
    try {
      foo = decrypt(data, _passphrase, iv);
      if (parse(foo)) {
        encrypted = false;
        Util.bzero(data);
      }
    } finally {
      Util.bzero(_passphrase);
      Util.bzero(foo);
    }
    return !encrypted;
  }

  public static KeyPair load(JSch jsch, String prvkey) throws JSchException {
    String pubkey = prvkey + ".pub";
    if (!new File(pubkey).exists()) {
      pubkey = null;
    }
    return load(jsch.instLogger, prvkey, pubkey);
  }

  public static KeyPair load(JSch jsch, String prvfile, String pubfile) throws JSchException {
    return load(jsch.instLogger, prvfile, pubfile);
  }

  static KeyPair load(JSch.InstanceLogger instLogger, String prvfile, String pubfile)
      throws JSchException {

    byte[] prvkey = null;
    byte[] pubkey = null;

    try {
      prvkey = Util.fromFile(prvfile);
    } catch (IOException e) {
      throw new JSchException(e.toString(), e);
    }

    String _pubfile = pubfile;
    if (pubfile == null) {
      _pubfile = prvfile + ".pub";
    }

    try {
      pubkey = Util.fromFile(_pubfile);
    } catch (IOException e) {
      if (pubfile != null) {
        throw new JSchException(e.toString(), e);
      }
    }

    try {
      return load(instLogger, prvkey, pubkey);
    } finally {
      Util.bzero(prvkey);
    }
  }

  public static KeyPair load(JSch jsch, byte[] prvkey, byte[] pubkey) throws JSchException {
    return load(jsch.instLogger, prvkey, pubkey);
  }

  static KeyPair load(JSch.InstanceLogger instLogger, byte[] prvkey, byte[] pubkey)
      throws JSchException {

    byte[] iv = new byte[8]; // 8
    boolean encrypted = true;
    byte[] data = null;

    byte[] publickeyblob = null;

    int type = ERROR;
    int vendor = VENDOR_OPENSSH;
    String publicKeyComment = "";
    Cipher cipher = null;

    // prvkey from "ssh-add" command on the remote.
    if (pubkey == null && prvkey != null
        && (prvkey.length > 11 && prvkey[0] == 0 && prvkey[1] == 0 && prvkey[2] == 0 &&
        // length of key type string
            (prvkey[3] == 7 || prvkey[3] == 9 || prvkey[3] == 11 || prvkey[3] == 19))) {

      Buffer buf = new Buffer(prvkey);
      buf.skip(prvkey.length); // for using Buffer#available()
      String _type = Util.byte2str(buf.getString()); // ssh-rsa or ssh-dss
      buf.rewind();

      KeyPair kpair = null;
      if (_type.equals("ssh-rsa")) {
        kpair = KeyPairRSA.fromSSHAgent(instLogger, buf);
      } else if (_type.equals("ssh-dss")) {
        kpair = KeyPairDSA.fromSSHAgent(instLogger, buf);
      } else if (_type.equals("ecdsa-sha2-nistp256") || _type.equals("ecdsa-sha2-nistp384")
          || _type.equals("ecdsa-sha2-nistp521")) {
        kpair = KeyPairECDSA.fromSSHAgent(instLogger, buf);
      } else if (_type.equals("ssh-ed25519")) {
        kpair = KeyPairEd25519.fromSSHAgent(instLogger, buf);
      } else if (_type.equals("ssh-ed448")) {
        kpair = KeyPairEd448.fromSSHAgent(instLogger, buf);
      } else {
        throw new JSchException("privatekey: invalid key " + _type);
      }
      return kpair;
    }

    try {
      byte[] buf = prvkey;

      if (buf != null) {
        KeyPair ppk = loadPPK(instLogger, buf);
        if (ppk != null)
          return ppk;
      }

      int len = (buf != null ? buf.length : 0);
      int i = 0;

      // skip garbage lines.
      while (i < len) {
        if (buf[i] == '-' && i + 4 < len && buf[i + 1] == '-' && buf[i + 2] == '-'
            && buf[i + 3] == '-' && buf[i + 4] == '-') {
          break;
        }
        i++;
      }

      while (i < len) {
        if (buf[i] == 'B' && i + 3 < len && buf[i + 1] == 'E' && buf[i + 2] == 'G'
            && buf[i + 3] == 'I') {
          i += 6;
          if (i + 2 >= len)
            throw new JSchException("invalid privatekey");
          if (buf[i] == 'D' && buf[i + 1] == 'S' && buf[i + 2] == 'A') {
            type = DSA;
          } else if (buf[i] == 'R' && buf[i + 1] == 'S' && buf[i + 2] == 'A') {
            type = RSA;
          } else if (buf[i] == 'E' && buf[i + 1] == 'C') {
            type = ECDSA;
          } else if (buf[i] == 'S' && buf[i + 1] == 'S' && buf[i + 2] == 'H') { // FSecure
            type = UNKNOWN;
            vendor = VENDOR_FSECURE;
          } else if (i + 6 < len && buf[i] == 'P' && buf[i + 1] == 'R' && buf[i + 2] == 'I'
              && buf[i + 3] == 'V' && buf[i + 4] == 'A' && buf[i + 5] == 'T' && buf[i + 6] == 'E') {
            type = UNKNOWN;
            vendor = VENDOR_PKCS8;
            encrypted = false;
            i += 3;
          } else if (i + 8 < len && buf[i] == 'E' && buf[i + 1] == 'N' && buf[i + 2] == 'C'
              && buf[i + 3] == 'R' && buf[i + 4] == 'Y' && buf[i + 5] == 'P' && buf[i + 6] == 'T'
              && buf[i + 7] == 'E' && buf[i + 8] == 'D') {
            type = UNKNOWN;
            vendor = VENDOR_PKCS8;
            i += 5;
          } else if (isOpenSSHPrivateKey(buf, i, len)) {
            type = UNKNOWN;
            vendor = VENDOR_OPENSSH_V1;
          } else {
            throw new JSchException("invalid privatekey");
          }
          i += 3;
          continue;
        }
        if (buf[i] == 'A' && i + 7 < len && buf[i + 1] == 'E' && buf[i + 2] == 'S'
            && buf[i + 3] == '-' && buf[i + 4] == '2' && buf[i + 5] == '5' && buf[i + 6] == '6'
            && buf[i + 7] == '-') {
          i += 8;
          if (Session.checkCipher(JSch.getConfig("aes256-cbc"))) {
            Class<? extends Cipher> c =
                Class.forName(JSch.getConfig("aes256-cbc")).asSubclass(Cipher.class);
            cipher = c.getDeclaredConstructor().newInstance();
            // key=new byte[cipher.getBlockSize()];
            iv = new byte[cipher.getIVSize()];
          } else {
            throw new JSchException("privatekey: aes256-cbc is not available");
          }
          continue;
        }
        if (buf[i] == 'A' && i + 7 < len && buf[i + 1] == 'E' && buf[i + 2] == 'S'
            && buf[i + 3] == '-' && buf[i + 4] == '1' && buf[i + 5] == '9' && buf[i + 6] == '2'
            && buf[i + 7] == '-') {
          i += 8;
          if (Session.checkCipher(JSch.getConfig("aes192-cbc"))) {
            Class<? extends Cipher> c =
                Class.forName(JSch.getConfig("aes192-cbc")).asSubclass(Cipher.class);
            cipher = c.getDeclaredConstructor().newInstance();
            // key=new byte[cipher.getBlockSize()];
            iv = new byte[cipher.getIVSize()];
          } else {
            throw new JSchException("privatekey: aes192-cbc is not available");
          }
          continue;
        }
        if (buf[i] == 'A' && i + 7 < len && buf[i + 1] == 'E' && buf[i + 2] == 'S'
            && buf[i + 3] == '-' && buf[i + 4] == '1' && buf[i + 5] == '2' && buf[i + 6] == '8'
            && buf[i + 7] == '-') {
          i += 8;
          if (Session.checkCipher(JSch.getConfig("aes128-cbc"))) {
            Class<? extends Cipher> c =
                Class.forName(JSch.getConfig("aes128-cbc")).asSubclass(Cipher.class);
            cipher = c.getDeclaredConstructor().newInstance();
            // key=new byte[cipher.getBlockSize()];
            iv = new byte[cipher.getIVSize()];
          } else {
            throw new JSchException("privatekey: aes128-cbc is not available");
          }
          continue;
        }
        if (buf[i] == 'C' && i + 3 < len && buf[i + 1] == 'B' && buf[i + 2] == 'C'
            && buf[i + 3] == ',') {
          i += 4;
          for (int ii = 0; ii < iv.length; ii++) {
            iv[ii] = (byte) (((a2b(buf[i++]) << 4) & 0xf0) + (a2b(buf[i++]) & 0xf));
          }
          continue;
        }
        if (buf[i] == '\r' && i + 1 < buf.length && buf[i + 1] == '\n') {
          i++;
          continue;
        }
        if (buf[i] == '\n' && i + 1 < buf.length) {
          if (buf[i + 1] == '\n') {
            i += 2;
            break;
          }
          if (buf[i + 1] == '\r' && i + 2 < buf.length && buf[i + 2] == '\n') {
            i += 3;
            break;
          }
          boolean inheader = false;
          for (int j = i + 1; j < buf.length; j++) {
            if (buf[j] == '\n')
              break;
            // if(buf[j]=='\r') break;
            if (buf[j] == ':') {
              inheader = true;
              break;
            }
          }
          if (!inheader) {
            i++;
            if (vendor != VENDOR_PKCS8)
              encrypted = false; // no passphrase
            break;
          }
        }
        i++;
      }

      if (buf != null) {

        if (type == ERROR) {
          throw new JSchException("invalid privatekey");
        }

        int start = i;
        while (i < len) {
          if (buf[i] == '-') {
            break;
          }
          i++;
        }

        if ((len - i) == 0 || (i - start) == 0) {
          throw new JSchException("invalid privatekey");
        }

        // The content of 'buf' will be changed, so it should be copied.
        byte[] tmp = new byte[i - start];
        System.arraycopy(buf, start, tmp, 0, tmp.length);
        byte[] _buf = tmp;

        start = 0;
        i = 0;

        int _len = _buf.length;
        while (i < _len) {
          if (_buf[i] == '\n') {
            boolean xd = (i > 0 && _buf[i - 1] == '\r');
            // ignore \n (or \r\n)
            System.arraycopy(_buf, i + 1, _buf, i - (xd ? 1 : 0), _len - (i + 1));
            if (xd) {
              _len--;
              i--;
            }
            _len--;
            continue;
          }
          if (_buf[i] == '-') {
            break;
          }
          i++;
        }

        if (i - start > 0)
          data = Util.fromBase64(_buf, start, i - start);

        Util.bzero(_buf);
      }

      if (vendor == VENDOR_OPENSSH_V1) {
        return loadOpenSSHKeyv1(instLogger, data);
      } else if (data != null && data.length > 4 && // FSecure
          data[0] == (byte) 0x3f && data[1] == (byte) 0x6f && data[2] == (byte) 0xf9
          && data[3] == (byte) 0xeb) {

        Buffer _buf = new Buffer(data);
        _buf.getInt(); // 0x3f6ff9be
        _buf.getInt();
        byte[] _type = _buf.getString();
        String _cipher = Util.byte2str(_buf.getString());
        if (_cipher.equals("3des-cbc")) {
          _buf.getInt();
          byte[] foo = new byte[data.length - _buf.getOffSet()];
          _buf.getByte(foo);
          data = foo;
          encrypted = true;
          throw new JSchException(
              "cipher " + _cipher + " is not supported for this privatekey format");
        } else if (_cipher.equals("none")) {
          _buf.getInt();
          _buf.getInt();

          encrypted = false;

          byte[] foo = new byte[data.length - _buf.getOffSet()];
          _buf.getByte(foo);
          data = foo;
        } else {
          throw new JSchException(
              "cipher " + _cipher + " is not supported for this privatekey format");
        }
      }

      if (pubkey != null) {
        try {
          buf = pubkey;
          len = buf.length;
          if (buf.length > 4 && // FSecure's public key
              buf[0] == '-' && buf[1] == '-' && buf[2] == '-' && buf[3] == '-') {

            boolean valid = true;
            i = 0;
            do {
              i++;
            } while (buf.length > i && buf[i] != '\n');
            if (buf.length <= i) {
              valid = false;
            }

            while (valid) {
              if (buf[i] == '\n') {
                boolean inheader = false;
                for (int j = i + 1; j < buf.length; j++) {
                  if (buf[j] == '\n')
                    break;
                  if (buf[j] == ':') {
                    inheader = true;
                    break;
                  }
                }
                if (!inheader) {
                  i++;
                  break;
                }
              }
              i++;
            }
            if (buf.length <= i) {
              valid = false;
            }

            int start = i;
            while (valid && i < len) {
              if (buf[i] == '\n') {
                System.arraycopy(buf, i + 1, buf, i, len - i - 1);
                len--;
                continue;
              }
              if (buf[i] == '-') {
                break;
              }
              i++;
            }
            if (valid) {
              publickeyblob = Util.fromBase64(buf, start, i - start);
              if (prvkey == null || type == UNKNOWN) {
                if (publickeyblob[8] == 'd') {
                  type = DSA;
                } else if (publickeyblob[8] == 'r') {
                  type = RSA;
                }
              }
            }
          } else {
            if (buf[0] == 's' && buf[1] == 's' && buf[2] == 'h' && buf[3] == '-') {
              if (prvkey == null && buf.length > 7) {
                if (buf[4] == 'd') {
                  type = DSA;
                } else if (buf[4] == 'r') {
                  type = RSA;
                } else if (buf[4] == 'e' && buf[6] == '2') {
                  type = ED25519;
                } else if (buf[4] == 'e' && buf[6] == '4') {
                  type = ED448;
                }
              }
              i = 0;
              while (i < len) {
                if (buf[i] == ' ')
                  break;
                i++;
              }
              i++;
              if (i < len) {
                int start = i;
                while (i < len) {
                  if (buf[i] == ' ')
                    break;
                  i++;
                }
                publickeyblob = Util.fromBase64(buf, start, i - start);
              }
              if (i++ < len) {
                int start = i;
                while (i < len) {
                  if (buf[i] == '\n')
                    break;
                  i++;
                }
                if (i > 0 && buf[i - 1] == '\r')
                  i--;
                if (start < i) {
                  publicKeyComment = Util.byte2str(buf, start, i - start);
                }
              }
            } else if (buf[0] == 'e' && buf[1] == 'c' && buf[2] == 'd' && buf[3] == 's') {
              if (prvkey == null && buf.length > 7) {
                type = ECDSA;
              }
              i = 0;
              while (i < len) {
                if (buf[i] == ' ')
                  break;
                i++;
              }
              i++;
              if (i < len) {
                int start = i;
                while (i < len) {
                  if (buf[i] == ' ')
                    break;
                  i++;
                }
                publickeyblob = Util.fromBase64(buf, start, i - start);
              }
              if (i++ < len) {
                int start = i;
                while (i < len) {
                  if (buf[i] == '\n')
                    break;
                  i++;
                }
                if (i > 0 && buf[i - 1] == '\r')
                  i--;
                if (start < i) {
                  publicKeyComment = Util.byte2str(buf, start, i - start);
                }
              }
            }
          }
        } catch (Exception ee) {
          if (instLogger.getLogger().isEnabled(Logger.WARN)) {
            instLogger.getLogger().log(Logger.WARN, "failed to parse public key", ee);
          }
        }
      }

      KeyPair kpair = null;
      if (type == DSA) {
        kpair = new KeyPairDSA(instLogger);
      } else if (type == RSA) {
        kpair = new KeyPairRSA(instLogger);
      } else if (type == ECDSA) {
        kpair = new KeyPairECDSA(instLogger, pubkey);
      } else if (type == ED25519) {
        kpair = new KeyPairEd25519(instLogger, pubkey, null);
      } else if (type == ED448) {
        kpair = new KeyPairEd448(instLogger, pubkey, null);
      } else if (vendor == VENDOR_PKCS8) {
        kpair = new KeyPairPKCS8(instLogger);
      }

      if (kpair != null) {
        kpair.encrypted = encrypted;
        kpair.publickeyblob = publickeyblob;
        kpair.vendor = vendor;
        kpair.publicKeyComment = publicKeyComment;
        kpair.cipher = cipher;

        if (encrypted) {
          kpair.encrypted = true;
          kpair.iv = iv;
          kpair.data = data;
        } else {
          if (kpair.parse(data)) {
            kpair.encrypted = false;
          } else {
            throw new JSchException("invalid privatekey");
          }
        }
      }

      return kpair;
    } catch (Exception | LinkageError e) {
      Util.bzero(data);
      if (e instanceof JSchException)
        throw (JSchException) e;
      throw new JSchException(e.toString(), e);
    }
  }

  static KeyPair loadOpenSSHKeyv1(JSch.InstanceLogger instLogger, byte[] data)
      throws JSchException {
    if (data == null) {
      throw new JSchException("invalid privatekey");
    }

    Buffer buffer = new Buffer(data);
    byte[] magic = new byte[AUTH_MAGIC.length];
    buffer.getByte(magic);
    if (!Util.arraysequals(AUTH_MAGIC, magic)) {
      throw new JSchException("Invalid openssh v1 format.");
    }

    String cipherName = Util.byte2str(buffer.getString());
    String kdfName = Util.byte2str(buffer.getString()); // string kdfname
    byte[] kdfOptions = buffer.getString(); // string kdfoptions

    int nrKeys = buffer.getInt(); // int number of keys N; Should be 1
    if (nrKeys != 1) {
      throw new JSchException("We don't support having more than 1 key in the file (yet).");
    }

    byte[] publickeyblob = buffer.getString();
    KeyPair kpair = parsePubkeyBlob(instLogger, publickeyblob, null);
    kpair.encrypted = !"none".equals(cipherName);
    kpair.publickeyblob = publickeyblob;
    kpair.vendor = VENDOR_OPENSSH_V1;
    kpair.publicKeyComment = "";

    try {
      if (!kpair.encrypted) {
        kpair.data = buffer.getString();
        if (!kpair.parse(kpair.data)) {
          throw new JSchException("invalid privatekey");
        } else {
          Util.bzero(kpair.data);
        }
      } else {
        try {
          Class<? extends Cipher> c =
              Class.forName(JSch.getConfig(cipherName)).asSubclass(Cipher.class);
          kpair.cipher = c.getDeclaredConstructor().newInstance();
          int _ivLen = kpair.cipher.getIVSize();
          if (kpair.cipher.isChaCha20()) {
            _ivLen = 0;
          } else if (kpair.cipher.isAEAD()) {
            _ivLen = 12;
          }
          kpair.iv = new byte[_ivLen];

          // tag is not included with the encrypted data length
          byte[] _data = buffer.getString();
          byte[] _authdata = new byte[_data.length + kpair.cipher.getTagSize()];
          System.arraycopy(_data, 0, _authdata, 0, _data.length);
          buffer.getByte(_authdata, _data.length, _authdata.length - _data.length);
          kpair.data = _authdata;
        } catch (Exception | LinkageError e) {
          if (e instanceof JSchException)
            throw (JSchException) e;
          throw new JSchException("cipher " + cipherName + " is not available", e);
        }

        try {
          Class<? extends KDF> c = Class.forName(JSch.getConfig(kdfName)).asSubclass(KDF.class);
          kpair.kdf = c.getDeclaredConstructor().newInstance();
          kpair.kdf.initWithOpenSSHv1KDFOptions(kdfOptions);
        } catch (Exception | LinkageError e) {
          if (e instanceof JSchException)
            throw (JSchException) e;
          throw new JSchException("kdf " + kdfName + " is not available", e);
        }
      }

      return kpair;
    } catch (Exception e) {
      Util.bzero(kpair.data);
      throw e;
    }
  }

  private static boolean isOpenSSHPrivateKey(byte[] buf, int i, int len) {
    String ident = "OPENSSH PRIVATE KEY-----";
    return i + ident.length() < len
        && ident.equals(Util.byte2str(Arrays.copyOfRange(buf, i, i + ident.length())));
  }

  private static byte a2b(byte c) {
    if ('0' <= c && c <= '9')
      return (byte) (c - '0');
    return (byte) (c - 'a' + 10);
  }

  private static byte b2a(byte c) {
    if (0 <= c && c <= 9)
      return (byte) (c + '0');
    return (byte) (c - 10 + 'A');
  }

  public void dispose() {}

  @SuppressWarnings("deprecation")
  @Override
  public void finalize() {
    dispose();
  }

  public static byte[] extractX509SubjectPublicKeyInfo(byte[] x509SubjectPublicKeyInfo,
      byte[] algorithmIdentifier, int keyLen) throws JSchException {
    // SubjectPublicKeyInfo ::= SEQUENCE {
    // algorithm AlgorithmIdentifier,
    // subjectPublicKey BIT STRING }
    // AlgorithmIdentifier ::= SEQUENCE {
    // algorithm OBJECT IDENTIFIER,
    // parameters ANY DEFINED BY algorithm OPTIONAL }
    try {
      ASN1 subjectPublicKeyInfo = new ASN1(x509SubjectPublicKeyInfo);
      if (!subjectPublicKeyInfo.isSEQUENCE()) {
        throw new JSchException(
            "invalid public key value (SubjectPublicKeyInfo is not a SEQUENCE)");
      }

      ASN1[] subjectPublicKeyInfoSequence = subjectPublicKeyInfo.getContents();
      if (subjectPublicKeyInfoSequence.length != 2) {
        throw new JSchException("invalid public key value (SubjectPublicKeyInfo is wrong length)");
      }

      ASN1 algorithm = subjectPublicKeyInfoSequence[0];
      if (!algorithm.isSEQUENCE()) {
        throw new JSchException(
            "invalid public key value (SubjectPublicKeyInfo algorithm is not a SEQUENCE)");
      }

      ASN1[] algorithmSequence = algorithm.getContents();
      if (algorithmSequence.length < 1) {
        throw new JSchException(
            "invalid public key value (SubjectPublicKeyInfo algorithm is wrong length)");
      }

      ASN1 algorithmObject = algorithmSequence[0];
      if (!algorithmObject.isOBJECT()) {
        throw new JSchException(
            "invalid public key value (AlgorithmIdentifier algorithm is not an OBJECT IDENTIFIER)");
      }

      if (!Util.array_equals(algorithmObject.getContent(), algorithmIdentifier)) {
        throw new JSchException(
            "invalid public key value (AlgorithmIdentifier algorithm is wrong value)");
      }

      ASN1 subjectPublicKey = subjectPublicKeyInfoSequence[1];
      if (!subjectPublicKey.isBITSTRING()) {
        throw new JSchException(
            "invalid public key value (SubjectPublicKeyInfo subjectPublicKey is not a BIT STRING)");
      }

      byte[] subjectPublicKeyBitString = subjectPublicKey.getContent();
      if (subjectPublicKeyBitString[0] != 0 || subjectPublicKeyBitString.length - 1 != keyLen) {
        throw new JSchException(
            "invalid public key value (SubjectPublicKeyInfo subjectPublicKey is wrong length)");
      }

      return Arrays.copyOfRange(subjectPublicKeyBitString, 1, subjectPublicKeyBitString.length);
    } catch (ASN1Exception e) {
      throw new JSchException("invalid ASN.1 encoding", e);
    }
  }

  static KeyPair loadPPK(JSch.InstanceLogger instLogger, byte[] buf) throws JSchException {
    byte[] pubkey = null;
    byte[] prvkey = null;
    byte[] _prvkey = null;
    int lines = 0;

    Buffer buffer = new Buffer(buf);
    Map<String, String> v = new HashMap<>();

    while (true) {
      if (!parseHeader(buffer, v))
        break;
    }

    int ppkVersion;
    String typ = v.get("PuTTY-User-Key-File-2");
    if (typ == null) {
      typ = v.get("PuTTY-User-Key-File-3");
      if (typ == null) {
        return null;
      } else {
        ppkVersion = VENDOR_PUTTY_V3;
      }
    } else {
      ppkVersion = VENDOR_PUTTY;
    }

    try {
      lines = Integer.parseInt(v.get("Public-Lines"));
      pubkey = parseLines(buffer, lines);

      while (true) {
        if (!parseHeader(buffer, v))
          break;
      }

      lines = Integer.parseInt(v.get("Private-Lines"));
      _prvkey = parseLines(buffer, lines);

      while (true) {
        if (!parseHeader(buffer, v))
          break;
      }

      prvkey = Util.fromBase64(_prvkey, 0, _prvkey.length);
      pubkey = Util.fromBase64(pubkey, 0, pubkey.length);

      KeyPair kpair = parsePubkeyBlob(instLogger, pubkey, typ);
      kpair.encrypted = !v.get("Encryption").equals("none");
      kpair.publickeyblob = pubkey;
      kpair.vendor = ppkVersion;
      kpair.publicKeyComment = v.get("Comment");
      if (kpair.encrypted) {
        try {
          Class<? extends Cipher> c =
              Class.forName(JSch.getConfig("aes256-cbc")).asSubclass(Cipher.class);
          kpair.cipher = c.getDeclaredConstructor().newInstance();
          kpair.iv = new byte[kpair.cipher.getIVSize()];
        } catch (Exception | LinkageError e) {
          if (e instanceof JSchException)
            throw (JSchException) e;
          throw new JSchException("cipher aes256-cbc is not available", e);
        }

        if (ppkVersion == VENDOR_PUTTY) {
          try {
            Class<? extends HASH> c = Class.forName(JSch.getConfig("sha-1")).asSubclass(HASH.class);
            HASH sha1 = c.getDeclaredConstructor().newInstance();
            sha1.init();
            kpair.sha1 = sha1;
          } catch (Exception | LinkageError e) {
            if (e instanceof JSchException)
              throw (JSchException) e;
            throw new JSchException("hash sha-1 is not available", e);
          }
        } else {
          String kdfType = v.get("Key-Derivation");
          try {
            Class<? extends KDF> c = Class.forName(JSch.getConfig(kdfType)).asSubclass(KDF.class);
            kpair.kdf = c.getDeclaredConstructor().newInstance();
            kpair.kdf.initWithPPKv3Header(v);
          } catch (Exception | LinkageError e) {
            if (e instanceof JSchException)
              throw (JSchException) e;
            throw new JSchException("kdf " + kdfType + " is not available", e);
          }
        }

        kpair.data = prvkey;
      } else {
        kpair.data = prvkey;
        if (!kpair.parse(prvkey)) {
          throw new JSchException("invalid privatekey");
        } else {
          Util.bzero(prvkey);
        }
      }
      return kpair;
    } catch (Exception e) {
      Util.bzero(prvkey);
      throw e;
    } finally {
      Util.bzero(_prvkey);
    }
  }

  private static KeyPair parsePubkeyBlob(JSch.InstanceLogger instLogger, byte[] pubkeyblob,
      String typ) throws JSchException {
    Buffer _buf = new Buffer(pubkeyblob);
    _buf.skip(pubkeyblob.length);

    String pubkeyType = Util.byte2str(_buf.getString());
    if (typ == null || typ.equals("")) {
      typ = pubkeyType;
    } else if (!typ.equals(pubkeyType)) {
      throw new JSchException(
          "pubkeyblob type [" + pubkeyType + "] does not match expected type [" + typ + "]");
    }

    if (typ.equals("ssh-rsa")) {
      byte[] pub_array = new byte[_buf.getInt()];
      _buf.getByte(pub_array);
      byte[] n_array = new byte[_buf.getInt()];
      _buf.getByte(n_array);

      return new KeyPairRSA(instLogger, n_array, pub_array, null);
    } else if (typ.equals("ssh-dss")) {
      byte[] p_array = new byte[_buf.getInt()];
      _buf.getByte(p_array);
      byte[] q_array = new byte[_buf.getInt()];
      _buf.getByte(q_array);
      byte[] g_array = new byte[_buf.getInt()];
      _buf.getByte(g_array);
      byte[] y_array = new byte[_buf.getInt()];
      _buf.getByte(y_array);

      return new KeyPairDSA(instLogger, p_array, q_array, g_array, y_array, null);
    } else if (typ.equals("ecdsa-sha2-nistp256") || typ.equals("ecdsa-sha2-nistp384")
        || typ.equals("ecdsa-sha2-nistp521")) {
      byte[] name = _buf.getString(); // nistpXXX

      int len = _buf.getInt();
      int x04 = _buf.getByte(); // in case of x04 it is uncompressed
                                // https://tools.ietf.org/html/rfc5480#page-7
      byte[] r_array = new byte[(len - 1) / 2];
      byte[] s_array = new byte[(len - 1) / 2];
      _buf.getByte(r_array);
      _buf.getByte(s_array);

      return new KeyPairECDSA(instLogger, name, r_array, s_array, null);
    } else if (typ.equals("ssh-ed25519") || typ.equals("ssh-ed448")) {
      byte[] pub_array = new byte[_buf.getInt()];
      _buf.getByte(pub_array);

      if (typ.equals("ssh-ed25519")) {
        return new KeyPairEd25519(instLogger, pub_array, null);
      } else {
        return new KeyPairEd448(instLogger, pub_array, null);
      }
    } else {
      throw new JSchException("key type " + typ + " is not supported");
    }
  }

  private static byte[] parseLines(Buffer buffer, int lines) {
    byte[] buf = buffer.buffer;
    int index = buffer.index;
    byte[] data = null;

    int i = index;
    while (lines-- > 0) {
      while (buf.length > i) {
        byte c = buf[i++];
        if (c == '\r' || c == '\n') {
          int len = i - index - 1;
          if (data == null) {
            data = new byte[len];
            System.arraycopy(buf, index, data, 0, len);
          } else if (len > 0) {
            byte[] tmp = new byte[data.length + len];
            System.arraycopy(data, 0, tmp, 0, data.length);
            System.arraycopy(buf, index, tmp, data.length, len);
            Util.bzero(data); // clear
            data = tmp;
          }
          break;
        }
      }
      if (i < buf.length && buf[i] == '\n')
        i++;
      index = i;
    }

    if (data != null)
      buffer.index = index;

    return data;
  }

  private static boolean parseHeader(Buffer buffer, Map<String, String> v) {
    byte[] buf = buffer.buffer;
    int index = buffer.index;
    String key = null;
    String value = null;
    for (int i = index; i < buf.length; i++) {
      if (buf[i] == '\r' || buf[i] == '\n') {
        if (i + 1 < buf.length && buf[i + 1] == '\n') {
          i++;
        }
        break;
      }
      if (buf[i] == ':') {
        key = Util.byte2str(buf, index, i - index);
        i++;
        if (i < buf.length && buf[i] == ' ') {
          i++;
        }
        index = i;
        break;
      }
    }

    if (key == null)
      return false;

    for (int i = index; i < buf.length; i++) {
      if (buf[i] == '\r' || buf[i] == '\n') {
        value = Util.byte2str(buf, index, i - index);
        i++;
        if (i < buf.length && buf[i] == '\n') {
          i++;
        }
        index = i;
        break;
      }
    }

    if (value != null) {
      v.put(key, value);
      buffer.index = index;
    }

    return (key != null && value != null);
  }

  void copy(KeyPair kpair) {
    this.publickeyblob = kpair.publickeyblob;
    this.vendor = kpair.vendor;
    this.publicKeyComment = kpair.publicKeyComment;
    this.cipher = kpair.cipher;
  }
}
