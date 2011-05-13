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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 * USA.
 */

package com.tigervnc.rfb;

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
    byte[] challenge = new byte[vncAuthChallengeSize];
    is.readBytes(challenge, 0, vncAuthChallengeSize);
    StringBuffer passwd = new StringBuffer();
    CConn.upg.getUserPasswd(null, passwd);

    // Calculate the correct response
    byte[] key = new byte[8];
    int pwdLen = passwd.length();
    for (int i=0; i<8; i++)
      key[i] = i<pwdLen ? (byte)passwd.charAt(i) : 0;
    DesCipher des = new DesCipher(key);
    for (int j = 0; j < vncAuthChallengeSize; j += 8)
      des.encrypt(challenge,j,challenge,j);

    // Return the response to the server
    os.writeBytes(challenge, 0, vncAuthChallengeSize);
    os.flush();
    return true;
  }

  public int getType() { return Security.secTypeVncAuth; }
  public String description() { return "No Encryption"; }

  static LogWriter vlog = new LogWriter("VncAuth");
}
