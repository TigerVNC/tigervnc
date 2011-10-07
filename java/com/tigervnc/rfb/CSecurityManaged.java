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

import java.io.IOException;

import com.tigervnc.rdr.*;
import com.tigervnc.vncviewer.*;

public class CSecurityManaged extends CSecurity {

  public CSecurityManaged() { }

  public boolean processMsg(CConnection cc) {
   InStream is = cc.getInStream();
   OutStream os = cc.getOutStream();

    StringBuffer username = new StringBuffer();

    CConn.upg.getUserPasswd(username, null);

    // Return the response to the server
    os.writeU8(username.length());
    try {
      byte[] utf8str = username.toString().getBytes("UTF8");
      os.writeBytes(utf8str, 0, username.length());
    } catch(java.io.UnsupportedEncodingException e) {
      e.printStackTrace();
    }
    os.flush();
    int serverPort = is.readU16();
    //if (serverPort==0) { return true; };
    String serverName = cc.getServerName();
    vlog.debug("Redirected to "+serverName+" port "+serverPort);
    try {
      CConn.getSocket().close();
      cc.setServerPort(serverPort);
      sock = new java.net.Socket(serverName, serverPort);
      sock.setTcpNoDelay(true);
      sock.setTrafficClass(0x10);
      CConn.setSocket(sock);
      vlog.debug("connected to host "+serverName+" port "+serverPort);
      cc.setStreams(new JavaInStream(sock.getInputStream()),
                    new JavaOutStream(sock.getOutputStream()));
      cc.initialiseProtocol();
    } catch (java.io.IOException e) {
      e.printStackTrace();
    }
    return false;
  }

  public int getType() { return Security.secTypeManaged; }

  java.net.Socket sock;
  UserPasswdGetter upg;

  static LogWriter vlog = new LogWriter("Managed");
  public String description() { return "No Encryption"; }

}
