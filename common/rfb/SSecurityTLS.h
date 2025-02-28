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

#ifndef __S_SECURITY_TLS_H__
#define __S_SECURITY_TLS_H__

#ifndef HAVE_GNUTLS
#error "This header should not be included without HAVE_GNUTLS defined"
#endif

#include <rfb/Security.h>
#include <rfb/SSecurity.h>

#include <gnutls/gnutls.h>

/* In GnuTLS 3.6.0 DH parameter generation was deprecated. RFC7919 is used instead.
 * GnuTLS before 3.6.0 doesn't know about RFC7919 so we will have to import it.
 */
#if GNUTLS_VERSION_NUMBER < 0x030600
#define SSECURITYTLS__USE_DEPRECATED_DH
#endif

namespace rdr {
  class InStream;
  class OutStream;
  class TLSInStream;
  class TLSOutStream;
}

namespace rfb {

  class StringParameter;

  class SSecurityTLS : public SSecurity {
  public:
    SSecurityTLS(SConnection* sc, bool _anon);
    virtual ~SSecurityTLS();
    bool processMsg() override;
    const char* getUserName() const override {return nullptr;}
    int getType() const override { return anon ? secTypeTLSNone : secTypeX509None;}

    static core::StringParameter X509_CertFile;
    static core::StringParameter X509_KeyFile;

  protected:
    void shutdown();
    void setParams();

  private:
    gnutls_session_t session;
#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
    gnutls_dh_params_t dh_params;
#endif
    gnutls_anon_server_credentials_t anon_cred;
    gnutls_certificate_credentials_t cert_cred;

    bool anon;

    rdr::TLSInStream* tlsis;
    rdr::TLSOutStream* tlsos;

    rdr::InStream* rawis;
    rdr::OutStream* rawos;
  };

}

#endif
