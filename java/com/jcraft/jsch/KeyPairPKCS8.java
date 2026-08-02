/*
 * Copyright (c) 2013-2018 ymnk, JCraft,Inc. All rights reserved.
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
import java.math.BigInteger;
import java.util.ArrayList;
import java.util.List;

class KeyPairPKCS8 extends KeyPair {
  private static final byte[] rsaEncryption = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0x86,
      (byte) 0xf7, (byte) 0x0d, (byte) 0x01, (byte) 0x01, (byte) 0x01};

  private static final byte[] dsaEncryption =
      {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0xce, (byte) 0x38, (byte) 0x04, (byte) 0x01};

  private static final byte[] ecPublicKey =
      {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0xce, (byte) 0x3d, (byte) 0x02, (byte) 0x01};

  private static final byte[] ed25519 = {(byte) 0x2b, (byte) 0x65, (byte) 0x70};

  private static final byte[] ed448 = {(byte) 0x2b, (byte) 0x65, (byte) 0x71};

  private static final byte[] secp256r1 = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0xce,
      (byte) 0x3d, (byte) 0x03, (byte) 0x01, (byte) 0x07};

  private static final byte[] secp384r1 =
      {(byte) 0x2b, (byte) 0x81, (byte) 0x04, (byte) 0x00, (byte) 0x22};

  private static final byte[] secp521r1 =
      {(byte) 0x2b, (byte) 0x81, (byte) 0x04, (byte) 0x00, (byte) 0x23};

  private static final byte[] pbes2 = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0x86,
      (byte) 0xf7, (byte) 0x0d, (byte) 0x01, (byte) 0x05, (byte) 0x0d};

  private static final byte[] pbkdf2 = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0x86,
      (byte) 0xf7, (byte) 0x0d, (byte) 0x01, (byte) 0x05, (byte) 0x0c};

  private static final byte[] scrypt = {(byte) 0x2b, (byte) 0x06, (byte) 0x01, (byte) 0x04,
      (byte) 0x01, (byte) 0xda, (byte) 0x47, (byte) 0x04, (byte) 0x0b};

  private static final byte[] aes128cbc = {(byte) 0x60, (byte) 0x86, (byte) 0x48, (byte) 0x01,
      (byte) 0x65, (byte) 0x03, (byte) 0x04, (byte) 0x01, (byte) 0x02};

  private static final byte[] aes192cbc = {(byte) 0x60, (byte) 0x86, (byte) 0x48, (byte) 0x01,
      (byte) 0x65, (byte) 0x03, (byte) 0x04, (byte) 0x01, (byte) 0x16};

  private static final byte[] aes256cbc = {(byte) 0x60, (byte) 0x86, (byte) 0x48, (byte) 0x01,
      (byte) 0x65, (byte) 0x03, (byte) 0x04, (byte) 0x01, (byte) 0x2a};

  private static final byte[] descbc =
      {(byte) 0x2b, (byte) 0x0e, (byte) 0x03, (byte) 0x02, (byte) 0x07};

  private static final byte[] des3cbc = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0x86,
      (byte) 0xf7, (byte) 0x0d, (byte) 0x03, (byte) 0x07};

  private static final byte[] rc2cbc = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0x86,
      (byte) 0xf7, (byte) 0x0d, (byte) 0x03, (byte) 0x02};

  private static final byte[] rc5cbc = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0x86,
      (byte) 0xf7, (byte) 0x0d, (byte) 0x03, (byte) 0x09};

  private static final byte[] pbeWithMD2AndDESCBC = {(byte) 0x2a, (byte) 0x86, (byte) 0x48,
      (byte) 0x86, (byte) 0xf7, (byte) 0x0d, (byte) 0x01, (byte) 0x05, (byte) 0x01};

  private static final byte[] pbeWithMD2AndRC2CBC = {(byte) 0x2a, (byte) 0x86, (byte) 0x48,
      (byte) 0x86, (byte) 0xf7, (byte) 0x0d, (byte) 0x01, (byte) 0x05, (byte) 0x04};

  private static final byte[] pbeWithMD5AndDESCBC = {(byte) 0x2a, (byte) 0x86, (byte) 0x48,
      (byte) 0x86, (byte) 0xf7, (byte) 0x0d, (byte) 0x01, (byte) 0x05, (byte) 0x03};

  private static final byte[] pbeWithMD5AndRC2CBC = {(byte) 0x2a, (byte) 0x86, (byte) 0x48,
      (byte) 0x86, (byte) 0xf7, (byte) 0x0d, (byte) 0x01, (byte) 0x05, (byte) 0x06};

  private static final byte[] pbeWithSHA1AndDESCBC = {(byte) 0x2a, (byte) 0x86, (byte) 0x48,
      (byte) 0x86, (byte) 0xf7, (byte) 0x0d, (byte) 0x01, (byte) 0x05, (byte) 0x0a};

  private static final byte[] pbeWithSHA1AndRC2CBC = {(byte) 0x2a, (byte) 0x86, (byte) 0x48,
      (byte) 0x86, (byte) 0xf7, (byte) 0x0d, (byte) 0x01, (byte) 0x05, (byte) 0x0b};

  private KeyPair kpair = null;

  KeyPairPKCS8(JSch.InstanceLogger instLogger) {
    super(instLogger);
  }

  @Override
  void generate(int key_size) throws JSchException {}

  private static final byte[] begin = Util.str2byte("-----BEGIN DSA PRIVATE KEY-----");
  private static final byte[] end = Util.str2byte("-----END DSA PRIVATE KEY-----");

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
    return null;
  }

  @Override
  byte[] getOpenSSHv1PrivateKeyBlob() {
    if (kpair != null) {
      return kpair.getOpenSSHv1PrivateKeyBlob();
    } else {
      return null;
    }
  }

  @Override
  boolean parse(byte[] plain) {

    /*
     * from RFC5208 PrivateKeyInfo ::= SEQUENCE { version Version, privateKeyAlgorithm
     * PrivateKeyAlgorithmIdentifier, privateKey PrivateKey, attributes [0] IMPLICIT Attributes
     * OPTIONAL } Version ::= INTEGER PrivateKeyAlgorithmIdentifier ::= AlgorithmIdentifier
     * PrivateKey ::= OCTET STRING Attributes ::= SET OF Attribute }
     */

    byte[] _data = null;
    byte[] prv_array = null;
    byte[] _plain = null;
    KeyPair _key = null;
    try {
      ASN1[] contents;

      ASN1 asn1 = new ASN1(plain);
      if (!asn1.isSEQUENCE()) {
        throw new ASN1Exception();
      }

      contents = asn1.getContents();
      if (contents.length < 3 || contents.length > 4) {
        throw new ASN1Exception();
      }
      if (!contents[0].isINTEGER()) {
        throw new ASN1Exception();
      }
      if (!contents[1].isSEQUENCE()) {
        throw new ASN1Exception();
      }
      if (!contents[2].isOCTETSTRING()) {
        throw new ASN1Exception();
      }
      // attributes [0] IMPLICIT Attributes OPTIONAL
      if (contents.length > 3 && !contents[3].isCONTEXTCONSTRUCTED(0)) {
        throw new ASN1Exception();
      }

      int version = ASN1.parseASN1IntegerAsInt(contents[0].getContent());
      if (version != 0) {
        throw new ASN1Exception();
      }

      ASN1 privateKeyAlgorithm = contents[1];
      ASN1 privateKey = contents[2];

      contents = privateKeyAlgorithm.getContents();
      if (contents.length == 0) {
        throw new ASN1Exception();
      }
      if (!contents[0].isOBJECT()) {
        throw new ASN1Exception();
      }
      byte[] privateKeyAlgorithmID = contents[0].getContent();

      _data = privateKey.getContent();

      KeyPair _kpair = null;
      if (Util.array_equals(privateKeyAlgorithmID, rsaEncryption)) {
        if (contents.length != 2) {
          throw new ASN1Exception();
        }
        if (!contents[1].isNULL()) {
          throw new ASN1Exception();
        }

        _kpair = new KeyPairRSA(instLogger);
        _kpair.copy(this);
        if (_kpair.parse(_data)) {
          kpair = _kpair;
          return true;
        } else {
          throw new JSchException("failed to parse RSA");
        }
      } else if (Util.array_equals(privateKeyAlgorithmID, dsaEncryption)) {
        List<byte[]> values = new ArrayList<>(3);

        if (contents.length > 1 && contents[1].isSEQUENCE()) {
          contents = contents[1].getContents();
          if (contents.length != 3) {
            throw new ASN1Exception();
          }
          if (!contents[0].isINTEGER()) {
            throw new ASN1Exception();
          }
          if (!contents[1].isINTEGER()) {
            throw new ASN1Exception();
          }
          if (!contents[2].isINTEGER()) {
            throw new ASN1Exception();
          }

          values.add(contents[0].getContent());
          values.add(contents[1].getContent());
          values.add(contents[2].getContent());
        }

        asn1 = new ASN1(_data);
        if (values.size() == 0) { // embedded DSA parameters format
          /*
           * SEQUENCE SEQUENCE INTEGER // P_array INTEGER // Q_array INTEGER // G_array INTEGER //
           * prv_array
           */
          if (!asn1.isSEQUENCE()) {
            throw new ASN1Exception();
          }

          contents = asn1.getContents();
          if (contents.length != 2) {
            throw new ASN1Exception();
          }
          if (!contents[0].isSEQUENCE()) {
            throw new ASN1Exception();
          }
          if (!contents[1].isINTEGER()) {
            throw new ASN1Exception();
          }

          prv_array = contents[1].getContent();

          contents = contents[0].getContents();
          if (contents.length != 3) {
            throw new ASN1Exception();
          }
          if (!contents[0].isINTEGER()) {
            throw new ASN1Exception();
          }
          if (!contents[1].isINTEGER()) {
            throw new ASN1Exception();
          }
          if (!contents[2].isINTEGER()) {
            throw new ASN1Exception();
          }

          values.add(contents[0].getContent());
          values.add(contents[1].getContent());
          values.add(contents[2].getContent());
        } else {
          /*
           * INTEGER // prv_array
           */
          if (!asn1.isINTEGER()) {
            throw new ASN1Exception();
          }
          prv_array = asn1.getContent();
        }

        byte[] P_array = values.get(0);
        byte[] Q_array = values.get(1);
        byte[] G_array = values.get(2);
        // Y = g^X mode p
        byte[] pub_array = (new BigInteger(G_array))
            .modPow(new BigInteger(prv_array), new BigInteger(P_array)).toByteArray();

        _key = new KeyPairDSA(instLogger, P_array, Q_array, G_array, pub_array, prv_array);
        _plain = _key.getPrivateKey();

        _kpair = new KeyPairDSA(instLogger);
        _kpair.copy(this);
        if (_kpair.parse(_plain)) {
          kpair = _kpair;
          return true;
        } else {
          throw new JSchException("failed to parse DSA");
        }
      } else if (Util.array_equals(privateKeyAlgorithmID, ecPublicKey)) {
        if (contents.length != 2) {
          throw new ASN1Exception();
        }
        if (!contents[1].isOBJECT()) {
          throw new ASN1Exception();
        }

        byte[] namedCurve = contents[1].getContent();
        byte[] name;
        if (!Util.array_equals(namedCurve, secp256r1)) {
          name = Util.str2byte("nistp256");
        } else if (!Util.array_equals(namedCurve, secp384r1)) {
          name = Util.str2byte("nistp384");
        } else if (!Util.array_equals(namedCurve, secp521r1)) {
          name = Util.str2byte("nistp521");
        } else {
          throw new JSchException("unsupported named curve oid: " + Util.toHex(namedCurve));
        }

        ASN1 ecPrivateKey = new ASN1(_data);
        if (!ecPrivateKey.isSEQUENCE()) {
          throw new ASN1Exception();
        }

        // ECPrivateKey ::= SEQUENCE {
        // version INTEGER { ecPrivkeyVer1(1) } (ecPrivkeyVer1),
        // privateKey OCTET STRING,
        // parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
        // publicKey [1] BIT STRING OPTIONAL
        // }
        contents = ecPrivateKey.getContents();
        if (contents.length < 3 || contents.length > 4) {
          throw new ASN1Exception();
        }
        if (!contents[0].isINTEGER()) {
          throw new ASN1Exception();
        }
        if (!contents[1].isOCTETSTRING()) {
          throw new ASN1Exception();
        }

        version = ASN1.parseASN1IntegerAsInt(contents[0].getContent());
        if (version != 1) {
          throw new ASN1Exception();
        }
        prv_array = contents[1].getContent();

        // publicKey is required here since there is no other way to derive it.
        ASN1 publicKey;
        if (contents.length == 3) {
          publicKey = contents[2];
        } else {
          publicKey = contents[3];

          // parameters [0] ECParameters {{ NamedCurve }} OPTIONAL
          if (!contents[2].isCONTEXTCONSTRUCTED(0)) {
            throw new ASN1Exception();
          }

          // NamedCurve isn't required here since it is already known.
          // But if it is included, they should be the same...
          ASN1[] goo = contents[2].getContents();
          if (goo.length != 1) {
            throw new ASN1Exception();
          }
          if (!goo[0].isOBJECT()) {
            throw new ASN1Exception();
          }
          if (!Util.array_equals(goo[0].getContent(), namedCurve)) {
            throw new ASN1Exception();
          }
        }

        // publicKey [1] BIT STRING OPTIONAL
        if (!publicKey.isCONTEXTCONSTRUCTED(1)) {
          throw new ASN1Exception();
        }
        contents = publicKey.getContents();
        if (contents.length != 1) {
          throw new ASN1Exception();
        }
        if (!contents[0].isBITSTRING()) {
          throw new ASN1Exception();
        }

        byte[] Q_array = contents[0].getContent();
        byte[][] tmp = KeyPairECDSA.fromPoint(Q_array);
        byte[] r_array = tmp[0];
        byte[] s_array = tmp[1];

        _key = new KeyPairECDSA(instLogger, name, r_array, s_array, prv_array);
        _plain = _key.getPrivateKey();

        _kpair = new KeyPairECDSA(instLogger);
        _kpair.copy(this);
        if (_kpair.parse(_plain)) {
          kpair = _kpair;
          return true;
        } else {
          throw new JSchException("failed to parse ECDSA");
        }
      } else if (Util.array_equals(privateKeyAlgorithmID, ed25519)
          || Util.array_equals(privateKeyAlgorithmID, ed448)) {
        if (contents.length != 1) {
          throw new ASN1Exception();
        }
        ASN1 curvePrivateKey = new ASN1(_data);
        if (!curvePrivateKey.isOCTETSTRING()) {
          throw new ASN1Exception();
        }

        prv_array = curvePrivateKey.getContent();
        if (Util.array_equals(privateKeyAlgorithmID, ed25519)) {
          _kpair = new KeyPairEd25519(instLogger);
        } else {
          _kpair = new KeyPairEd448(instLogger);
        }
        _kpair.copy(this);
        if (_kpair.parse(prv_array)) {
          kpair = _kpair;
          return true;
        } else {
          throw new JSchException("failed to parse EdDSA");
        }
      } else {
        throw new JSchException(
            "unsupported privateKeyAlgorithm oid: " + Util.toHex(privateKeyAlgorithmID));
      }
    } catch (ASN1Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "PKCS8: failed to parse key: ASN1 parsing error",
            e);
      }
      return false;
    } catch (Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "PKCS8: failed to parse key: " + e.getMessage(),
            e);
      }
      return false;
    } finally {
      Util.bzero(_data);
      Util.bzero(prv_array);
      Util.bzero(_plain);
      if (_key != null) {
        _key.dispose();
      }
    }
  }

  @Override
  public byte[] getPublicKeyBlob() {
    if (kpair != null) {
      return kpair.getPublicKeyBlob();
    } else {
      return super.getPublicKeyBlob();
    }
  }

  @Override
  byte[] getKeyTypeName() {
    if (kpair != null) {
      return kpair.getKeyTypeName();
    } else {
      return new byte[0];
    }
  }

  @Override
  public int getKeyType() {
    if (kpair != null) {
      return kpair.getKeyType();
    } else {
      return UNKNOWN;
    }
  }

  @Override
  public int getKeySize() {
    return kpair.getKeySize();
  }

  @Override
  public byte[] getSignature(byte[] data) {
    return kpair.getSignature(data);
  }

  @Override
  public byte[] getSignature(byte[] data, String alg) {
    return kpair.getSignature(data, alg);
  }

  @Override
  public Signature getVerifier() {
    return kpair.getVerifier();
  }

  @Override
  public Signature getVerifier(String alg) {
    return kpair.getVerifier(alg);
  }

  @Override
  public byte[] forSSHAgent() throws JSchException {
    return kpair.forSSHAgent();
  }

  @Override
  public boolean decrypt(byte[] _passphrase) {
    if (!isEncrypted()) {
      return true;
    }
    if (_passphrase == null) {
      return !isEncrypted();
    }

    /*
     * SEQUENCE SEQUENCE OBJECT :PBES2 SEQUENCE SEQUENCE OBJECT :PBKDF2 SEQUENCE OCTET STRING [HEX
     * DUMP]:E4E24ADC9C00BD4D INTEGER :0800 SEQUENCE OBJECT :aes-128-cbc OCTET STRING [HEX
     * DUMP]:5B66E6B3BF03944C92317BC370CC3AD0 OCTET STRING [HEX DUMP]:
     *
     * or
     *
     * SEQUENCE SEQUENCE OBJECT :PBES2 SEQUENCE SEQUENCE OBJECT :PBKDF2 SEQUENCE OCTET STRING [HEX
     * DUMP]:E4E24ADC9C00BD4D INTEGER :0800 SEQUENCE OBJECT :hmacWithSHA256 NULL SEQUENCE OBJECT
     * :aes-128-cbc OCTET STRING [HEX DUMP]:5B66E6B3BF03944C92317BC370CC3AD0 OCTET STRING [HEX
     * DUMP]:
     *
     * or
     *
     * SEQUENCE SEQUENCE OBJECT :pbeWithMD5AndDES-CBC SEQUENCE OCTET STRING [HEX
     * DUMP]:DBF75ECB69E3C0FC INTEGER :0800 OCTET STRING [HEX DUMP]
     */

    byte[] _data = null;
    byte[] key = null;
    byte[] plain = null;
    try {
      ASN1[] contents;

      ASN1 asn1 = new ASN1(data);
      if (!asn1.isSEQUENCE()) {
        throw new ASN1Exception();
      }

      contents = asn1.getContents();
      if (contents.length != 2) {
        throw new ASN1Exception();
      }
      if (!contents[0].isSEQUENCE()) {
        throw new ASN1Exception();
      }
      if (!contents[1].isOCTETSTRING()) {
        throw new ASN1Exception();
      }

      _data = contents[1].getContent();
      ASN1 pbes = contents[0];

      contents = pbes.getContents();
      if (contents.length != 2) {
        throw new ASN1Exception();
      }
      if (!contents[0].isOBJECT()) {
        throw new ASN1Exception();
      }
      if (!contents[1].isSEQUENCE()) {
        throw new ASN1Exception();
      }

      byte[] pbesid = contents[0].getContent();
      ASN1 pbesparam = contents[1];

      String kdfname;
      KDF kdfinst;
      byte[] encryptfuncid;
      ASN1 encryptparams;

      if (Util.array_equals(pbesid, pbes2)) {
        contents = pbesparam.getContents();
        if (contents.length != 2) {
          throw new ASN1Exception();
        }

        ASN1 kdf = contents[0];
        ASN1 encryptfunc = contents[1];

        if (!kdf.isSEQUENCE()) {
          throw new ASN1Exception();
        }
        if (!encryptfunc.isSEQUENCE()) {
          throw new ASN1Exception();
        }

        contents = encryptfunc.getContents();

        if (contents.length != 2) {
          throw new ASN1Exception();
        }
        if (!contents[0].isOBJECT()) {
          throw new ASN1Exception();
        }

        encryptfuncid = contents[0].getContent();
        encryptparams = contents[1];

        contents = kdf.getContents();
        if (contents.length != 2) {
          throw new ASN1Exception();
        }
        if (!contents[0].isOBJECT()) {
          throw new ASN1Exception();
        }
        if (!contents[1].isSEQUENCE()) {
          throw new ASN1Exception();
        }

        byte[] kdfid = contents[0].getContent();
        if (Util.array_equals(kdfid, pbkdf2)) {
          kdfname = "pbkdf2";
          kdfinst = getKDF(kdfname);
          kdfinst.initWithASN1(contents[1].getRaw());
        } else if (Util.array_equals(kdfid, scrypt)) {
          kdfname = "scrypt";
          kdfinst = getKDF(kdfname);
          kdfinst.initWithASN1(contents[1].getRaw());
        } else {
          throw new JSchException("unsupported kdf oid: " + Util.toHex(kdfid));
        }
      } else {
        String message;
        if (Util.array_equals(pbesid, pbeWithMD2AndDESCBC)) {
          message = "pbeWithMD2AndDES-CBC unsupported";
        } else if (Util.array_equals(pbesid, pbeWithMD2AndRC2CBC)) {
          message = "pbeWithMD2AndRC2-CBC unsupported";
        } else if (Util.array_equals(pbesid, pbeWithMD5AndDESCBC)) {
          message = "pbeWithMD5AndDES-CBC unsupported";
        } else if (Util.array_equals(pbesid, pbeWithMD5AndRC2CBC)) {
          message = "pbeWithMD5AndRC2-CBC unsupported";
        } else if (Util.array_equals(pbesid, pbeWithSHA1AndDESCBC)) {
          message = "pbeWithSHA1AndDES-CBC unsupported";
        } else if (Util.array_equals(pbesid, pbeWithSHA1AndRC2CBC)) {
          message = "pbeWithSHA1AndRC2-CBC unsupported";
        } else {
          message = "unsupported encryption oid: " + Util.toHex(pbesid);
        }
        throw new JSchException(message);
      }

      byte[][] ivp = new byte[1][];
      Cipher cipher = getCipher(encryptfuncid, encryptparams, ivp);
      byte[] iv = ivp[0];

      key = kdfinst.getKey(_passphrase, cipher.getBlockSize());
      if (key == null) {
        throw new JSchException("failed to generate key from KDF " + kdfname);
      }
      cipher.init(Cipher.DECRYPT_MODE, key, iv);
      plain = new byte[_data.length];
      cipher.update(_data, 0, _data.length, plain, 0);
      if (parse(plain)) {
        encrypted = false;
        Util.bzero(data);
        return true;
      } else {
        throw new JSchException("failed to parse decrypted key");
      }
    } catch (ASN1Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "PKCS8: failed to decrypt key: ASN1 parsing error",
            e);
      }
      return false;
    } catch (Exception e) {
      if (instLogger.getLogger().isEnabled(Logger.ERROR)) {
        instLogger.getLogger().log(Logger.ERROR, "PKCS8: failed to decrypt key: " + e.getMessage(),
            e);
      }
      return false;
    } finally {
      Util.bzero(_data);
      Util.bzero(key);
      Util.bzero(plain);
    }
  }

  static KDF getKDF(String name) throws JSchException {
    try {
      Class<? extends KDF> c = Class.forName(JSch.getConfig(name)).asSubclass(KDF.class);
      return c.getDeclaredConstructor().newInstance();
    } catch (LinkageError | Exception e) {
      throw new JSchException(name + " is not supported", e);
    }
  }

  static Cipher getCipher(byte[] id, ASN1 encryptparams, byte[][] ivp) throws Exception {
    String name = null;
    if (Util.array_equals(id, aes128cbc)) {
      name = "aes128-cbc";
    } else if (Util.array_equals(id, aes192cbc)) {
      name = "aes192-cbc";
    } else if (Util.array_equals(id, aes256cbc)) {
      name = "aes256-cbc";
    } else if (Util.array_equals(id, descbc)) {
      throw new JSchException("unsupported cipher function: des-cbc");
    } else if (Util.array_equals(id, des3cbc)) {
      throw new JSchException("unsupported cipher function: 3des-cbc");
    } else if (Util.array_equals(id, rc2cbc)) {
      throw new JSchException("unsupported cipher function: rc2-cbc");
    } else if (Util.array_equals(id, rc5cbc)) {
      throw new JSchException("unsupported cipher function: rc5-cbc");
    }

    if (name == null) {
      throw new JSchException("unsupported cipher function oid: " + Util.toHex(id));
    }

    if (!encryptparams.isOCTETSTRING()) {
      throw new ASN1Exception();
    }
    ivp[0] = encryptparams.getContent();

    try {
      Class<? extends Cipher> c = Class.forName(JSch.getConfig(name)).asSubclass(Cipher.class);
      return c.getDeclaredConstructor().newInstance();
    } catch (LinkageError | Exception e) {
      throw new JSchException(name + " is not supported", e);
    }
  }
}
