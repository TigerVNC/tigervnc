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

public class CSecurityPlain extends CSecurity {

  public CSecurityPlain() { }

  public boolean processMsg(CConnection cc) 
  {
    OutStream os = cc.getOutStream();

    StringBuffer username = new StringBuffer();
    StringBuffer password = new StringBuffer();

    CConn.upg.getUserPasswd(username, password);

    // Return the response to the server
    os.writeU32(username.length());
    os.writeU32(password.length());
	  os.writeBytes(username.toString().getBytes(), 0, username.length());
	  os.writeBytes(password.toString().getBytes(), 0, password.length());
    os.flush();
    return true;
  }

  public int getType() { return Security.secTypePlain; }
  public String description() { return "ask for username and password"; }

  static LogWriter vlog = new LogWriter("Plain");
}
