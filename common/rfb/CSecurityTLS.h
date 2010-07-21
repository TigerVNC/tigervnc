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

#ifndef HAVE_GNUTLS
#error "This header should not be compiled without HAVE_GNUTLS defined"
#endif

#include <rfb/CSecurity.h>
#include <rfb/SSecurityVeNCrypt.h>
#include <rfb/Security.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <gnutls/gnutls.h>

namespace rfb {
  class CSecurityTLS : public CSecurity {
  public:
    CSecurityTLS(bool _anon);
    virtual ~CSecurityTLS();
    virtual bool processMsg(CConnection* cc);
    virtual int getType() const { return anon ? secTypeTLSNone : secTypeX509None; }
    virtual const char* description() const
      { return anon ? "TLS Encryption without VncAuth" : "X509 Encryption without VncAuth"; }

    static StringParameter x509ca;
    static StringParameter x509crl;

  protected:
    void shutdown();
    void freeResources();
    void setParam();
    void checkSession();
    CConnection *client;

  private:
    static void initGlobal();

    gnutls_session session;
    gnutls_anon_client_credentials anon_cred;
    gnutls_certificate_credentials cert_cred;
    bool anon;

    char *cafile, *crlfile;
    rdr::InStream* fis;
    rdr::OutStream* fos;
  };
}

#endif
