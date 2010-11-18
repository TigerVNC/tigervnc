/*
 * Copyright (C) 2003 Sun Microsystems, Inc.
 * Copyright (C) 2003-2010 Martin Koegler
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

package com.tigervnc.vncviewer;

import java.util.ArrayList;
import java.net.*;
import javax.net.ssl.*;

public abstract class TLSTunnelBase
{

  public TLSTunnelBase (Socket sock_)
  {
    sock = sock_;
  }

  protected void initContext (SSLContext sc) throws java.security.
    GeneralSecurityException
  {
    sc.init (null, null, null);
  }

  public void setup (RfbProto cc) throws Exception
  {
    if (cc.readU8 () == 0)
	throw new Exception("Setup on the server failed");
    try
    {
      SSLSocketFactory sslfactory;
      SSLSocket sslsock;
      SSLContext sc = SSLContext.getInstance ("TLS");
      System.out.println("Generating TLS context");
      initContext (sc);
      System.out.println("Doing TLS handshake");
      sslfactory = sc.getSocketFactory ();
      sslsock = (SSLSocket) sslfactory.createSocket (sock,
						     sock.getInetAddress ().
						     getHostName (),
						     sock.getPort (), true);

      setParam (sslsock);

      /* Not neccessary - just ensures that we know what cipher
       * suite we are using for the output of toString()
       */
      sslsock.startHandshake ();

      System.out.println("TLS done");

      cc.setStreams (sslsock.getInputStream (),
		     sslsock.getOutputStream ());
    }
    catch (java.io.IOException e)
    {
      throw new Exception("TLS handshake failed " + e.toString ());
    }
    catch (java.security.GeneralSecurityException e)
    {
      throw new Exception("TLS handshake failed " + e.toString ());
    }
  }


  protected abstract void setParam (SSLSocket sock);

  Socket sock;

}
