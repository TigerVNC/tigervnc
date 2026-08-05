/* Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
 * Copyright (C) 2011-2017 Brian P. Hinz
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

import com.tigervnc.rdr.*;
import com.tigervnc.vncviewer.*;

public class CSecurityPlain extends CSecurity {

  public CSecurityPlain() { }

  public boolean processMsg(CConnection cc)
  {
    OutStream os = cc.getOutStream();

    StringBuffer username = new StringBuffer();
    StringBuffer password = new StringBuffer();

    upg.getUserPasswd(cc.isSecure(), username, password);

    // Return the response to the server
    byte[] userBytes, passBytes;
    try {
      userBytes = username.toString().getBytes("UTF-8");
      passBytes = password.toString().getBytes("UTF-8");
    } catch(java.io.UnsupportedEncodingException e) {
      userBytes = username.toString().getBytes();
      passBytes = password.toString().getBytes();
    }
    os.writeU32(userBytes.length);
    os.writeU32(passBytes.length);
    os.writeBytes(userBytes, 0, userBytes.length);
    os.writeBytes(passBytes, 0, passBytes.length);
    os.flush();
    return true;
  }

  public int getType() { return Security.secTypePlain; }
  public String description() { return "ask for username and password"; }

  static LogWriter vlog = new LogWriter("Plain");
}
