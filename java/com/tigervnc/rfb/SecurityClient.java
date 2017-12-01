/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
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

public class SecurityClient extends Security {

  public SecurityClient() { super(secTypes); }

  public CSecurity GetCSecurity(int secType)
  {
    assert (CSecurity.upg != null); /* (upg == null) means bug in the viewer */
    assert (CSecurityTLS.msg != null);

    if (!IsSupported(secType))
      throw new Exception("Security type not supported");

    switch (secType) {
    case Security.secTypeNone: return (new CSecurityNone());
    case Security.secTypeVncAuth: return (new CSecurityVncAuth());
    case Security.secTypeVeNCrypt: return (new CSecurityVeNCrypt(this));
    case Security.secTypePlain: return (new CSecurityPlain());
    case Security.secTypeIdent: return (new CSecurityIdent());
    case Security.secTypeTLSNone:
      return (new CSecurityStack(secTypeTLSNone, "TLS with no password",
  			      new CSecurityTLS(true), null));
    case Security.secTypeTLSVnc:
      return (new CSecurityStack(secTypeTLSVnc, "TLS with VNCAuth",
  			      new CSecurityTLS(true), new CSecurityVncAuth()));
    case Security.secTypeTLSPlain:
      return (new CSecurityStack(secTypeTLSPlain, "TLS with Username/Password",
  			      new CSecurityTLS(true), new CSecurityPlain()));
    case Security.secTypeTLSIdent:
      return (new CSecurityStack(secTypeTLSIdent, "TLS with username only",
  			      new CSecurityTLS(true), new CSecurityIdent()));
    case Security.secTypeX509None:
      return (new CSecurityStack(secTypeX509None, "X509 with no password",
  			      new CSecurityTLS(false), null));
    case Security.secTypeX509Vnc:
      return (new CSecurityStack(secTypeX509Vnc, "X509 with VNCAuth",
  			      new CSecurityTLS(false), new CSecurityVncAuth()));
    case Security.secTypeX509Plain:
      return (new CSecurityStack(secTypeX509Plain, "X509 with Username/Password",
  			      new CSecurityTLS(false), new CSecurityPlain()));
    case Security.secTypeX509Ident:
      return (new CSecurityStack(secTypeX509Ident, "X509 with username only",
  			      new CSecurityTLS(false), new CSecurityIdent()));
    default:
      throw new Exception("Security type not supported");
    }

  }

  public static void setDefaults()
  {
      CSecurityTLS.setDefaults();
  }

  public static StringParameter secTypes
  = new StringParameter("SecurityTypes",
                        "Specify which security scheme to use (None, VncAuth, Plain, Ident, TLSNone, TLSVnc, TLSPlain, TLSIdent, X509None, X509Vnc, X509Plain, X509Ident)",
                        "X509Ident,X509Plain,TLSIdent,TLSPlain,X509Vnc,TLSVnc,X509None,TLSNone,Ident,VncAuth,None", Configuration.ConfigurationObject.ConfViewer);

}
