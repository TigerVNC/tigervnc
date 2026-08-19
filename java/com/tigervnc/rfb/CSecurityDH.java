/* Copyright (C) 2022 Dinglan Peng
 * Copyright (C) 2026 Brian P. Hinz
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 * USA.
 */

package com.tigervnc.rfb;

import java.io.UnsupportedEncodingException;
import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.SecureRandom;

import javax.crypto.Cipher;
import javax.crypto.spec.SecretKeySpec;

import com.tigervnc.rdr.*;

public class CSecurityDH extends CSecurity {

  private static final int MinKeyLength = 128;
  private static final int MaxKeyLength = 1024;

  private static byte[] bigIntToBytes(BigInteger n, int bytes) {
    byte[] arr = n.toByteArray();
    int len = arr.length < bytes ? arr.length : bytes;
    byte[] res = new byte[bytes];
    System.arraycopy(arr, arr.length - len, res, bytes - len, len);
    return res;
  }

  public boolean processMsg(CConnection cc) {
    readKey(cc);
    writeCredentials(cc);
    return true;
  }

  private void readKey(CConnection cc) {
    InStream is = cc.getInStream();

    int gen = is.readU16();
    keyLength = is.readU16();
    if (keyLength < MinKeyLength)
      throw new AuthFailureException("Received Diffie-Hellman key is too short");
    if (keyLength > MaxKeyLength)
      throw new AuthFailureException("Received Diffie-Hellman key is too long");

    byte[] pBytes = new byte[keyLength];
    byte[] aBytes = new byte[keyLength];
    is.readBytes(ByteBuffer.wrap(pBytes), keyLength);
    is.readBytes(ByteBuffer.wrap(aBytes), keyLength);

    g = BigInteger.valueOf(gen);
    p = new BigInteger(1, pBytes);
    A = new BigInteger(1, aBytes);

    // A modulus of 0 or 1 is degenerate: BigInteger.modPow() would either
    // throw ArithmeticException (0, or negative) or silently derive an
    // always-zero, trivially-predictable shared secret (1), so reject
    // both explicitly rather than let either happen deep in the key
    // exchange (see GHSA-r4vv-jmq7-c5ph, filed against the C++ viewer's
    // equivalent unchecked-modulus code).
    if (p.compareTo(BigInteger.ONE) <= 0)
      throw new AuthFailureException("Received invalid Diffie-Hellman modulus");
  }

  private void writeCredentials(CConnection cc) {
    StringBuffer username = new StringBuffer();
    StringBuffer password = new StringBuffer();
    CSecurity.upg.getUserPasswd(isSecure(), username, password);

    SecureRandom rand = new SecureRandom();

    byte[] bBytes = new byte[keyLength];
    rand.nextBytes(bBytes);
    BigInteger b = new BigInteger(1, bBytes);

    BigInteger k = A.modPow(b, p);
    BigInteger B = g.modPow(b, p);

    byte[] sharedSecret = bigIntToBytes(k, keyLength);
    byte[] BBytes = bigIntToBytes(B, keyLength);

    byte[] key;
    try {
      MessageDigest md5 = MessageDigest.getInstance("MD5");
      key = md5.digest(sharedSecret);
    } catch (NoSuchAlgorithmException e) {
      throw new AuthFailureException("MD5 algorithm is not supported");
    }

    // The 128-byte credentials buffer is random-padded, matching the
    // server's expectation that unused space not be predictable/zeroed.
    byte[] buf = new byte[128];
    rand.nextBytes(buf);

    byte[] usernameBytes;
    byte[] passwordBytes;
    try {
      usernameBytes = username.toString().getBytes("UTF-8");
      passwordBytes = password.toString().getBytes("UTF-8");
    } catch (UnsupportedEncodingException e) {
      throw new AuthFailureException("UTF-8 is not supported");
    }
    if (usernameBytes.length >= 64)
      throw new AuthFailureException("Username is too long");
    if (passwordBytes.length >= 64)
      throw new AuthFailureException("Password is too long");
    System.arraycopy(usernameBytes, 0, buf, 0, usernameBytes.length);
    buf[usernameBytes.length] = 0;
    System.arraycopy(passwordBytes, 0, buf, 64, passwordBytes.length);
    buf[64 + passwordBytes.length] = 0;

    byte[] encrypted;
    try {
      Cipher cipher = Cipher.getInstance("AES/ECB/NoPadding");
      cipher.init(Cipher.ENCRYPT_MODE, new SecretKeySpec(key, "AES"));
      encrypted = cipher.doFinal(buf);
    } catch (java.lang.Exception e) {
      throw new AuthFailureException("Failed to encrypt DH credentials: "+e.getMessage());
    }

    OutStream os = cc.getOutStream();
    os.writeBytes(encrypted, 0, encrypted.length);
    os.writeBytes(BBytes, 0, BBytes.length);
    os.flush();
  }

  public int getType() { return Security.secTypeDH; }
  public String description() { return "Diffie-Hellman"; }

  private int keyLength;
  private BigInteger g, p, A;
}
