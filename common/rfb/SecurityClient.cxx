/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2010 TigerVNC Team
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <rfb/CSecurityNone.h>
#include <rfb/CSecurityStack.h>
#include <rfb/CSecurityVeNCrypt.h>
#include <rfb/CSecurityVncAuth.h>
#include <rfb/CSecurityPlain.h>
#include <rdr/Exception.h>
#include <rfb/Security.h>
#ifdef HAVE_GNUTLS
#include <rfb/CSecurityTLS.h>
#endif

using namespace rdr;
using namespace rfb;

UserPasswdGetter *CSecurity::upg = NULL;
#ifdef HAVE_GNUTLS
UserMsgBox *CSecurityTLS::msg = NULL;
#endif

StringParameter SecurityClient::secTypes
("SecurityTypes",
 "Specify which security scheme to use (None, VncAuth)",
#ifdef HAVE_GNUTLS
 "VeNCrypt,X509Plain,TLSPlain,X509Vnc,TLSVnc,X509None,TLSNone,VncAuth,None",
#else
 "VncAuth,None",
#endif
ConfViewer);

CSecurity* SecurityClient::GetCSecurity(U32 secType)
{
  assert (CSecurity::upg != NULL); /* (upg == NULL) means bug in the viewer */
#ifdef HAVE_GNUTLS
  assert (CSecurityTLS::msg != NULL);
#endif

  if (!IsSupported(secType))
    goto bail;

  switch (secType) {
  case secTypeNone: return new CSecurityNone();
  case secTypeVncAuth: return new CSecurityVncAuth();
  case secTypeVeNCrypt: return new CSecurityVeNCrypt(this);
  case secTypePlain: return new CSecurityPlain();
#ifdef HAVE_GNUTLS
  case secTypeTLSNone:
    return new CSecurityStack(secTypeTLSNone, "TLS with no password",
			      new CSecurityTLS(true));
  case secTypeTLSVnc:
    return new CSecurityStack(secTypeTLSVnc, "TLS with VNCAuth",
			      new CSecurityTLS(true), new CSecurityVncAuth());
  case secTypeTLSPlain:
    return new CSecurityStack(secTypeTLSPlain, "TLS with Username/Password",
			      new CSecurityTLS(true), new CSecurityPlain());
  case secTypeX509None:
    return new CSecurityStack(secTypeX509None, "X509 with no password",
			      new CSecurityTLS(false));
  case secTypeX509Vnc:
    return new CSecurityStack(secTypeX509None, "X509 with VNCAuth",
			      new CSecurityTLS(false), new CSecurityVncAuth());
  case secTypeX509Plain:
    return new CSecurityStack(secTypeX509Plain, "X509 with Username/Password",
			      new CSecurityTLS(false), new CSecurityPlain());
#endif
  }

bail:
  throw Exception("Security type not supported");
}

void SecurityClient::setDefaults()
{
#ifdef HAVE_GNUTLS
    CSecurityTLS::setDefaults();
#endif
}
