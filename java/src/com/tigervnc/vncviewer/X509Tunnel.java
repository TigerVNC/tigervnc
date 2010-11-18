/*
 * Copyright (C) 2003 Sun Microsystems, Inc.
 * Copyright (C) 2003-2010 Martin Koegler
 * Copyright (C) 2006 OCCAM Financial Technology
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

import java.util.*;
import java.net.*;
import javax.net.ssl.*;
import java.security.*;
import java.security.cert.*;

public class X509Tunnel extends TLSTunnelBase
{

  public X509Tunnel (Socket sock_)
  {
    super (sock_);
  }

  protected void setParam (SSLSocket sock)
  {
    String[]supported;
    ArrayList enabled = new ArrayList ();

    supported = sock.getSupportedCipherSuites ();

    for (int i = 0; i < supported.length; i++)
      if (!supported[i].matches (".*DH_anon.*"))
	enabled.add (supported[i]);

    sock.setEnabledCipherSuites ((String[])enabled.toArray (new String[0]));
  }

  protected void initContext (SSLContext sc) throws java.security.
    GeneralSecurityException
  {
    TrustManager[] myTM = new TrustManager[]
    {
    new MyX509TrustManager ()};
    sc.init (null, myTM, null);
  }


  class MyX509TrustManager implements X509TrustManager
  {

    X509TrustManager tm;

      MyX509TrustManager () throws java.security.GeneralSecurityException
    {
      TrustManagerFactory tmf =
	TrustManagerFactory.getInstance ("SunX509", "SunJSSE");
      KeyStore ks = KeyStore.getInstance ("JKS");
        tmf.init (ks);
        tm = (X509TrustManager) tmf.getTrustManagers ()[0];
    }
    public void checkClientTrusted (X509Certificate[]chain,
				    String authType) throws
      CertificateException
    {
      tm.checkClientTrusted (chain, authType);
    }

    public void checkServerTrusted (X509Certificate[]chain,
				    String authType)
      throws CertificateException
    {
      try
      {
	tm.checkServerTrusted (chain, authType);
      } catch (CertificateException e)
      {
	MessageBox m =
	  new MessageBox (e.toString (), MessageBox.MB_OKAYCANCEL);
	if (!m.result ())
	  throw e;
      }
    }

    public X509Certificate[] getAcceptedIssuers ()
    {
      return tm.getAcceptedIssuers ();
    }
  }
}
