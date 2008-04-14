//
//  Copyright (C) 2002 Constantin Kaplinsky, Inc.  All Rights Reserved.
//
//  This is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This software is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this software; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//

//
// HTTPConnectSocket.java together with HTTPConnectSocketFactory.java
// implement an alternate way to connect to VNC servers via one or two
// HTTP proxies supporting the HTTP CONNECT method.
//

import java.net.*;
import java.io.*;

class HTTPConnectSocket extends Socket {

  public HTTPConnectSocket(String host, int port,
			   String proxyHost, int proxyPort)
    throws IOException {

    // Connect to the specified HTTP proxy
    super(proxyHost, proxyPort);

    // Send the CONNECT request
    getOutputStream().write(("CONNECT " + host + ":" + port +
			     " HTTP/1.0\r\n\r\n").getBytes());

    // Read the first line of the response
    DataInputStream is = new DataInputStream(getInputStream());
    String str = is.readLine();

    // Check the HTTP error code -- it should be "200" on success
    if (!str.startsWith("HTTP/1.0 200 ")) {
      if (str.startsWith("HTTP/1.0 "))
	str = str.substring(9);
      throw new IOException("Proxy reports \"" + str + "\"");
    }

    // Success -- skip remaining HTTP headers
    do {
      str = is.readLine();
    } while (str.length() != 0);
  }
}

