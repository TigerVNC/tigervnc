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

import java.nio.*;

import com.tigervnc.rdr.*;
import com.tigervnc.vncviewer.*;

public class CSecurityVncAuth extends CSecurity {

  public CSecurityVncAuth() { }

  private static final int vncAuthChallengeSize = 16;

  public boolean processMsg(CConnection cc)
  {
    InStream is = cc.getInStream();
    OutStream os = cc.getOutStream();

    // Read the challenge & obtain the user's password
    ByteBuffer challenge = ByteBuffer.allocate(vncAuthChallengeSize);
    is.readBytes(challenge, vncAuthChallengeSize);
    StringBuffer passwd = new StringBuffer();
    upg.getUserPasswd(cc.isSecure(), null, passwd);

    // Calculate the correct response
    byte[] key = new byte[8];
    int pwdLen = passwd.length();
    byte[] utf8str = new byte[pwdLen];
    try {
      utf8str = passwd.toString().getBytes("UTF8");
    } catch(java.io.UnsupportedEncodingException e) {
      e.printStackTrace();
    }
    for (int i=0; i<8; i++)
      key[i] = i<pwdLen ? utf8str[i] : 0;
    DesCipher des = new DesCipher(key);
    for (int j = 0; j < vncAuthChallengeSize; j += 8)
      des.encrypt(challenge.array(),j,challenge.array(),j);

    // Return the response to the server
    os.writeBytes(challenge.array(), 0, vncAuthChallengeSize);
    os.flush();
    return true;
  }

  public int getType() { return Security.secTypeVncAuth; }
  public String description() { return "No Encryption"; }

  static LogWriter vlog = new LogWriter("VncAuth");
}
