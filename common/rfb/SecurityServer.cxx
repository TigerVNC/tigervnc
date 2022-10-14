/* 
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

#include <rdr/Exception.h>
#include <rfb/Security.h>
#include <rfb/SSecurityNone.h>
#include <rfb/SSecurityStack.h>
#include <rfb/SSecurityPlain.h>
#include <rfb/SSecurityVncAuth.h>
#include <rfb/SSecurityVeNCrypt.h>
#ifdef HAVE_GNUTLS
#include <rfb/SSecurityTLS.h>
#endif
#ifdef HAVE_NETTLE
#include <rfb/SSecurityRSAAES.h>
#endif

using namespace rdr;
using namespace rfb;

StringParameter SecurityServer::secTypes
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
 "TLSVnc,"
#endif
 "VncAuth",
ConfServer);

SSecurity* SecurityServer::GetSSecurity(SConnection* sc, U32 secType)
{
  if (!IsSupported(secType))
    goto bail;

  switch (secType) {
  case secTypeNone: return new SSecurityNone(sc);
  case secTypeVncAuth: return new SSecurityVncAuth(sc);
  case secTypeVeNCrypt: return new SSecurityVeNCrypt(sc, this);
  case secTypePlain: return new SSecurityPlain(sc);
#ifdef HAVE_GNUTLS
  case secTypeTLSNone:
    return new SSecurityStack(sc, secTypeTLSNone, new SSecurityTLS(sc, true));
  case secTypeTLSVnc:
    return new SSecurityStack(sc, secTypeTLSVnc, new SSecurityTLS(sc, true), new SSecurityVncAuth(sc));
  case secTypeTLSPlain:
    return new SSecurityStack(sc, secTypeTLSPlain, new SSecurityTLS(sc, true), new SSecurityPlain(sc));
  case secTypeX509None:
    return new SSecurityStack(sc, secTypeX509None, new SSecurityTLS(sc, false));
  case secTypeX509Vnc:
    return new SSecurityStack(sc, secTypeX509None, new SSecurityTLS(sc, false), new SSecurityVncAuth(sc));
  case secTypeX509Plain:
    return new SSecurityStack(sc, secTypeX509Plain, new SSecurityTLS(sc, false), new SSecurityPlain(sc));
#endif
#ifdef HAVE_NETTLE
  case secTypeRA2:
    return new SSecurityRSAAES(sc, secTypeRA2, 128, true);
  case secTypeRA2ne:
    return new SSecurityRSAAES(sc, secTypeRA2ne, 128, false);
  case secTypeRA256:
    return new SSecurityRSAAES(sc, secTypeRA256, 256, true);
  case secTypeRAne256:
    return new SSecurityRSAAES(sc, secTypeRAne256, 256, false);
#endif
  }

bail:
  throw Exception("Security type not supported");
}

