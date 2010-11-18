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

public class TLSTunnel extends TLSTunnelBase
{

  public TLSTunnel (Socket sock_)
  {
    super (sock_);
  }


  protected void setParam (SSLSocket sock)
  {
    String[]supported;
    ArrayList enabled = new ArrayList ();

    supported = sock.getSupportedCipherSuites ();

    for (int i = 0; i < supported.length; i++)
      if (supported[i].matches (".*DH_anon.*"))
	enabled.add (supported[i]);

    sock.setEnabledCipherSuites ((String[])enabled.toArray (new String[0]));
  }

}
