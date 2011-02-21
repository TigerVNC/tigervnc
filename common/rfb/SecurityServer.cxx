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

using namespace rdr;
using namespace rfb;

StringParameter SecurityServer::secTypes
("SecurityTypes",
 "Specify which security scheme to use (None, VncAuth)",
#ifdef HAVE_GNUTLS
 "TLSVnc,VncAuth",
#else
 "VncAuth",
#endif
ConfServer);

SSecurity* SecurityServer::GetSSecurity(U32 secType)
{
  if (!IsSupported(secType))
    goto bail;

  switch (secType) {
  case secTypeNone: return new SSecurityNone();
  case secTypeVncAuth: return new SSecurityVncAuth();
  case secTypeVeNCrypt: return new SSecurityVeNCrypt(this);
  case secTypePlain: return new SSecurityPlain();
#ifdef HAVE_GNUTLS
  case secTypeTLSNone:
    return new SSecurityStack(secTypeTLSNone, new SSecurityTLS(true));
  case secTypeTLSVnc:
    return new SSecurityStack(secTypeTLSVnc, new SSecurityTLS(true), new SSecurityVncAuth());
  case secTypeTLSPlain:
    return new SSecurityStack(secTypeTLSPlain, new SSecurityTLS(true), new SSecurityPlain());
  case secTypeX509None:
    return new SSecurityStack(secTypeX509None, new SSecurityTLS(false));
  case secTypeX509Vnc:
    return new SSecurityStack(secTypeX509None, new SSecurityTLS(false), new SSecurityVncAuth());
  case secTypeX509Plain:
    return new SSecurityStack(secTypeX509Plain, new SSecurityTLS(false), new SSecurityPlain());
#endif
  }

bail:
  throw Exception("Security type not supported");
}

