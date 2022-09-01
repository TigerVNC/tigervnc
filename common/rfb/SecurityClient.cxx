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
#ifdef HAVE_NETTLE
#include <rfb/CSecurityRSAAES.h>
#endif

using namespace rdr;
using namespace rfb;

UserPasswdGetter *CSecurity::upg = NULL;
#if defined(HAVE_GNUTLS) || defined(HAVE_NETTLE)
UserMsgBox *CSecurity::msg = NULL;
#endif

StringParameter SecurityClient::secTypes
("SecurityTypes",
 "Specify which security scheme to use (None, VncAuth, Plain"
#ifdef HAVE_GNUTLS
 ", TLSNone, TLSVnc, TLSPlain, X509None, X509Vnc, X509Plain"
#endif
#ifdef HAVE_NETTLE
 ", RA2, RA2ne, RA2_256, RA2ne_256"
#endif
 ")",
#ifdef HAVE_GNUTLS
 "X509Plain,TLSPlain,X509Vnc,TLSVnc,X509None,TLSNone,"
#endif
#ifdef HAVE_NETTLE
 "RA2,RA2_256,RA2ne,RA2ne_256,"
#endif
 "VncAuth,None",
ConfViewer);

CSecurity* SecurityClient::GetCSecurity(CConnection* cc, U32 secType)
{
  assert (CSecurity::upg != NULL); /* (upg == NULL) means bug in the viewer */
#if defined(HAVE_GNUTLS) || defined(HAVE_NETTLE)
  assert (CSecurity::msg != NULL);
#endif

  if (!IsSupported(secType))
    goto bail;

  switch (secType) {
  case secTypeNone: return new CSecurityNone(cc);
  case secTypeVncAuth: return new CSecurityVncAuth(cc);
  case secTypeVeNCrypt: return new CSecurityVeNCrypt(cc, this);
  case secTypePlain: return new CSecurityPlain(cc);
#ifdef HAVE_GNUTLS
  case secTypeTLSNone:
    return new CSecurityStack(cc, secTypeTLSNone,
                              new CSecurityTLS(cc, true));
  case secTypeTLSVnc:
    return new CSecurityStack(cc, secTypeTLSVnc,
                              new CSecurityTLS(cc, true),
                              new CSecurityVncAuth(cc));
  case secTypeTLSPlain:
    return new CSecurityStack(cc, secTypeTLSPlain,
                              new CSecurityTLS(cc, true),
                              new CSecurityPlain(cc));
  case secTypeX509None:
    return new CSecurityStack(cc, secTypeX509None,
                              new CSecurityTLS(cc, false));
  case secTypeX509Vnc:
    return new CSecurityStack(cc, secTypeX509Vnc,
                              new CSecurityTLS(cc, false),
                              new CSecurityVncAuth(cc));
  case secTypeX509Plain:
    return new CSecurityStack(cc, secTypeX509Plain,
                              new CSecurityTLS(cc, false),
                              new CSecurityPlain(cc));
#endif
#ifdef HAVE_NETTLE
  case secTypeRA2:
    return new CSecurityRSAAES(cc, secTypeRA2, 128, true);
  case secTypeRA2ne:
    return new CSecurityRSAAES(cc, secTypeRA2ne, 128, false);
  case secTypeRA256:
    return new CSecurityRSAAES(cc, secTypeRA256, 256, true);
  case secTypeRAne256:
    return new CSecurityRSAAES(cc, secTypeRAne256, 256, false);
#endif
  }

bail:
  throw Exception("Security type not supported");
}
