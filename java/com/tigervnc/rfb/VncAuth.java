/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

public class VncAuth {

  public static final int ok = 0;
  public static final int failed = 1;
  public static final int tooMany = 2; // deprecated

  public static final int challengeSize = 16;

  public static void encryptChallenge(byte[] challenge, String passwd) {
    byte[] key = new byte[8];
    for (int i = 0; i < 8 && i < passwd.length(); i++) {
      key[i] = (byte)passwd.charAt(i);
    }

    DesCipher des = new DesCipher(key);

    for (int j = 0; j < challengeSize; j += 8)
      des.encrypt(challenge,j,challenge,j);
  }

  void obfuscatePasswd(String passwd, byte[] obfuscated) {
    for (int i = 0; i < 8; i++) {
      if (i < passwd.length())
        obfuscated[i] = (byte)passwd.charAt(i);
      else
        obfuscated[i] = 0;
    }
    DesCipher des = new DesCipher(obfuscationKey);
    des.encrypt(obfuscated,0,obfuscated,0);
  }

  public static String unobfuscatePasswd(byte[] obfuscated) {
    DesCipher des = new DesCipher(obfuscationKey);
    des.decrypt(obfuscated,0,obfuscated,0);
    int len;
    for (len = 0; len < 8; len++) {
      if (obfuscated[len] == 0) break;
    }
    char[] plain = new char[len];
    for (int i = 0; i < len; i++) {
      plain[i] = (char)obfuscated[i];
    }
    return new String(plain);
  }

  static byte[] obfuscationKey = {23,82,107,6,35,78,88,7};
}
