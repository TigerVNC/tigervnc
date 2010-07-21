/* 
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

#ifndef __C_SECURITY_X509_H__
#define __C_SECURITY_X509_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_GNUTLS
#error "This header should not be included without HAVE_GNUTLS defined"
#endif

#include <rfb/CSecurityTLSBase.h>
#include <rfb/SSecurityVeNCrypt.h> /* To get secTypeX509None defined */

namespace rfb {

  class CSecurityX509 : public CSecurityTLSBase {
  public:
    CSecurityX509();
    virtual ~CSecurityX509();
    virtual int getType() const { return secTypeX509None; };
    virtual const char* description() const { return "X509 Encryption without VncAuth"; }

    static StringParameter x509ca;
    static StringParameter x509crl;

  protected:
    virtual void freeResources();
    virtual void setParam(gnutls_session session);
    virtual void checkSession(gnutls_session session);

  private:
    gnutls_certificate_credentials cert_cred;
    char *cafile;
    char *crlfile;
  };
}

#endif /* __C_SECURITY_TLS_H__ */
