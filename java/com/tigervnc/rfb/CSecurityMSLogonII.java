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
import java.security.SecureRandom;

import com.tigervnc.rdr.*;

public class CSecurityMSLogonII extends CSecurity {

  private static byte[] bigIntToBytes(BigInteger n, int bytes) {
    byte[] arr = n.toByteArray();
    int len = arr.length < bytes ? arr.length : bytes;
    byte[] res = new byte[bytes];
    System.arraycopy(arr, arr.length - len, res, bytes - len, len);
    return res;
  }

  // Encrypts data in place with DES-CBC using the given 8-byte IV.
  private static void cbcEncrypt(DesCipher des, byte[] iv, byte[] data) {
    byte[] prev = iv.clone();
    for (int off = 0; off < data.length; off += 8) {
      for (int i = 0; i < 8; i++)
        data[off + i] ^= prev[i];
      des.encrypt(data, off, data, off);
      System.arraycopy(data, off, prev, 0, 8);
    }
  }

  public boolean processMsg(CConnection cc) {
    readKey(cc);
    writeCredentials(cc);
    return true;
  }

  private void readKey(CConnection cc) {
    InStream is = cc.getInStream();

    byte[] gBytes = new byte[8];
    byte[] pBytes = new byte[8];
    byte[] aBytes = new byte[8];
    is.readBytes(ByteBuffer.wrap(gBytes), 8);
    is.readBytes(ByteBuffer.wrap(pBytes), 8);
    is.readBytes(ByteBuffer.wrap(aBytes), 8);

    g = new BigInteger(1, gBytes);
    p = new BigInteger(1, pBytes);
    A = new BigInteger(1, aBytes);
  }

  private void writeCredentials(CConnection cc) {
    StringBuffer username = new StringBuffer();
    StringBuffer password = new StringBuffer();
    CSecurity.upg.getUserPasswd(isSecure(), username, password);

    SecureRandom rand = new SecureRandom();

    byte[] bBytes = new byte[8];
    rand.nextBytes(bBytes);
    BigInteger b = new BigInteger(1, bBytes);

    BigInteger k = A.modPow(b, p);
    BigInteger B = g.modPow(b, p);

    byte[] key = bigIntToBytes(k, 8);
    byte[] BBytes = bigIntToBytes(B, 8);

    // DesCipher (originally written for classic VNC auth) already
    // performs RFB's well-known bit-reversed-DES-key quirk internally,
    // which is equivalent to the explicit reverseBits()+fixParity() step
    // the C++ implementation does before handing the key to a standard
    // DES library -- so the raw shared-secret bytes are used directly
    // here, not manually reversed (verified byte-for-byte against nettle).
    DesCipher des = new DesCipher(key);

    // Random-padded, matching the server's expectation that unused
    // buffer space not be predictable/zeroed.
    byte[] user = new byte[256];
    byte[] pass = new byte[64];
    rand.nextBytes(user);
    rand.nextBytes(pass);

    byte[] usernameBytes;
    byte[] passwordBytes;
    try {
      usernameBytes = username.toString().getBytes("UTF-8");
      passwordBytes = password.toString().getBytes("UTF-8");
    } catch (UnsupportedEncodingException e) {
      throw new AuthFailureException("UTF-8 is not supported");
    }
    if (usernameBytes.length >= 256)
      throw new AuthFailureException("Username is too long");
    if (passwordBytes.length >= 64)
      throw new AuthFailureException("Password is too long");
    System.arraycopy(usernameBytes, 0, user, 0, usernameBytes.length);
    user[usernameBytes.length] = 0;
    System.arraycopy(passwordBytes, 0, pass, 0, passwordBytes.length);
    pass[passwordBytes.length] = 0;

    // DES-CBC with the original (non bit-reversed) key as IV.
    cbcEncrypt(des, key, user);
    cbcEncrypt(des, key, pass);

    OutStream os = cc.getOutStream();
    os.writeBytes(BBytes, 0, BBytes.length);
    os.writeBytes(user, 0, user.length);
    os.writeBytes(pass, 0, pass.length);
    os.flush();
  }

  public int getType() { return Security.secTypeMSLogonII; }
  public String description() { return "MS-Logon II"; }

  private BigInteger g, p, A;
}
