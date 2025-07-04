/* 
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
 * Copyright 2012-2025 Pierre Ossman for Cendio AB
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

#ifndef HAVE_GNUTLS
#error "This header should not be compiled without HAVE_GNUTLS defined"
#endif

#include <rfb/CSecurity.h>
#include <rfb/Security.h>

#include <gnutls/gnutls.h>

namespace rdr {
  class InStream;
  class OutStream;
  class TLSSocket;
}

namespace rfb {
  class CSecurityTLS : public CSecurity {
  public:
    CSecurityTLS(CConnection* cc, bool _anon);
    virtual ~CSecurityTLS();
    bool processMsg() override;
    int getType() const override { return anon ? secTypeTLSNone : secTypeX509None; }
    bool isSecure() const override { return !anon; }

    static core::StringParameter X509CA;
    static core::StringParameter X509CRL;

  protected:
    void shutdown();
    void freeResources();
    void setParam();
    void checkSession();
    CConnection *client;

  private:
    gnutls_session_t session;
    gnutls_anon_client_credentials_t anon_cred;
    gnutls_certificate_credentials_t cert_cred;
    bool anon;

    rdr::TLSSocket* tlssock;

    rdr::InStream* rawis;
    rdr::OutStream* rawos;
  };
}

#endif
