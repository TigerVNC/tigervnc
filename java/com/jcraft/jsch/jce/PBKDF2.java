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

package com.jcraft.jsch.jce;

import com.jcraft.jsch.JSchException;
import com.jcraft.jsch.KDF;
import com.jcraft.jsch.asn1.ASN1;
import com.jcraft.jsch.asn1.ASN1Exception;
import java.security.NoSuchAlgorithmException;
import java.security.spec.InvalidKeySpecException;
import java.util.Arrays;
import javax.crypto.SecretKeyFactory;
import javax.crypto.spec.PBEKeySpec;

public class PBKDF2 implements KDF {
  private static final byte[] hmacWithSha1 = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0x86,
      (byte) 0xf7, (byte) 0x0d, (byte) 0x02, (byte) 0x07};

  private static final byte[] hmacWithSha224 = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0x86,
      (byte) 0xf7, (byte) 0x0d, (byte) 0x02, (byte) 0x08};

  private static final byte[] hmacWithSha256 = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0x86,
      (byte) 0xf7, (byte) 0x0d, (byte) 0x02, (byte) 0x09};

  private static final byte[] hmacWithSha384 = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0x86,
      (byte) 0xf7, (byte) 0x0d, (byte) 0x02, (byte) 0x0a};

  private static final byte[] hmacWithSha512 = {(byte) 0x2a, (byte) 0x86, (byte) 0x48, (byte) 0x86,
      (byte) 0xf7, (byte) 0x0d, (byte) 0x02, (byte) 0x0b};

  private static final byte[] hmacWithSha512224 = {(byte) 0x2a, (byte) 0x86, (byte) 0x48,
      (byte) 0x86, (byte) 0xf7, (byte) 0x0d, (byte) 0x02, (byte) 0x0c};

  private static final byte[] hmacWithSha512256 = {(byte) 0x2a, (byte) 0x86, (byte) 0x48,
      (byte) 0x86, (byte) 0xf7, (byte) 0x0d, (byte) 0x02, (byte) 0x0d};

  private SecretKeyFactory skf;
  private byte[] salt;
  private int iterations;
  private String jceName;

  @Override
  public void initWithASN1(byte[] asn1) throws Exception {
    try {
      ASN1 prf = null;
      ASN1 content = new ASN1(asn1);
      if (!content.isSEQUENCE()) {
        throw new ASN1Exception();
      }
      ASN1[] contents = content.getContents();
      if (contents.length < 2 || contents.length > 4) {
        throw new ASN1Exception();
      }
      if (!contents[0].isOCTETSTRING()) {
        throw new ASN1Exception();
      }
      if (!contents[1].isINTEGER()) {
        throw new ASN1Exception();
      }

      if (contents.length == 4) {
        if (!contents[2].isINTEGER()) {
          throw new ASN1Exception();
        }
        if (!contents[3].isSEQUENCE()) {
          throw new ASN1Exception();
        }
        prf = contents[3];
      } else if (contents.length == 3) {
        if (contents[2].isSEQUENCE()) {
          prf = contents[2];
        } else if (!contents[2].isINTEGER()) {
          throw new ASN1Exception();
        }
      }

      byte[] prfid = null;
      salt = contents[0].getContent();
      iterations = ASN1.parseASN1IntegerAsInt(contents[1].getContent());

      if (prf != null) {
        contents = prf.getContents();
        if (contents.length != 2) {
          throw new ASN1Exception();
        }
        if (!contents[0].isOBJECT()) {
          throw new ASN1Exception();
        }
        if (!contents[1].isNULL()) {
          throw new ASN1Exception();
        }
        prfid = contents[0].getContent();
      }

      jceName = getJceName(prfid);
      skf = SecretKeyFactory.getInstance(jceName);
    } catch (Exception e) {
      if (e instanceof JSchException)
        throw (JSchException) e;
      if (e instanceof NoSuchAlgorithmException)
        throw new JSchException("unsupported pbkdf2 algorithm: " + jceName, e);
      if (e instanceof ASN1Exception || e instanceof ArithmeticException)
        throw new JSchException("invalid ASN1", e);
      throw new JSchException("pbkdf2 unavailable", e);
    }
  }

  @Override
  public byte[] getKey(byte[] _pass, int size) {
    char[] pass = new char[_pass.length];
    for (int i = 0; i < _pass.length; i++) {
      pass[i] = (char) (_pass[i] & 0xff);
    }
    try {
      PBEKeySpec spec = new PBEKeySpec(pass, salt, iterations, size * 8);
      byte[] key = skf.generateSecret(spec).getEncoded();
      return key;
    } catch (InvalidKeySpecException e) {
    }
    return null;
  }

  static String getJceName(byte[] id) throws JSchException {
    String name = null;
    if (id == null || Arrays.equals(id, hmacWithSha1)) {
      name = "PBKDF2WithHmacSHA1";
    } else if (Arrays.equals(id, hmacWithSha224)) {
      name = "PBKDF2WithHmacSHA224";
    } else if (Arrays.equals(id, hmacWithSha256)) {
      name = "PBKDF2WithHmacSHA256";
    } else if (Arrays.equals(id, hmacWithSha384)) {
      name = "PBKDF2WithHmacSHA384";
    } else if (Arrays.equals(id, hmacWithSha512)) {
      name = "PBKDF2WithHmacSHA512";
    } else if (Arrays.equals(id, hmacWithSha512224)) {
      name = "PBKDF2WithHmacSHA512/224";
    } else if (Arrays.equals(id, hmacWithSha512256)) {
      name = "PBKDF2WithHmacSHA512/256";
    }

    if (name == null) {
      throw new JSchException("unsupported pbkdf2 function oid: " + toHex(id));
    }
    return name;
  }

  static String toHex(byte[] str) {
    StringBuilder sb = new StringBuilder();
    for (int i = 0; i < str.length; i++) {
      String foo = Integer.toHexString(str[i] & 0xff);
      sb.append("0x" + (foo.length() == 1 ? "0" : "") + foo);
      if (i + 1 < str.length)
        sb.append(":");
    }
    return sb.toString();
  }
}
