/* 
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2005 Martin Koegler
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

#ifndef __C_SECURITY_TLS_H__
#define __C_SECURITY_TLS_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GNUTLS

#include <rfb/CSecurityTLSBase.h>
#include <rfb/SSecurityVeNCrypt.h>

namespace rfb {
  class CSecurityTLS : public CSecurityTLSBase {
  public:
    CSecurityTLS();
    virtual ~CSecurityTLS();
    virtual int getType() const { return secTypeTLSNone; };
    virtual const char* description() const { return "TLS Encryption without VncAuth"; }
  protected:
    virtual void freeResources();
    virtual void setParam(gnutls_session session);
    virtual void checkSession(gnutls_session session);

  private:
    gnutls_anon_client_credentials anon_cred;
  };
}

#endif /* HAVE_GNUTLS */

#endif /* __C_SECURITY_TLS_H__ */
