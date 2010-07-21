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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_GNUTLS
#error "This source should not be compiled without HAVE_GNUTLS defined"
#endif

#include <rfb/SSecurityX509.h>
#include <rfb/Exception.h>

#define DH_BITS 1024

using namespace rfb;

StringParameter SSecurityX509::X509_CertFile
("x509cert", "specifies path to the x509 certificate in PEM format", "", ConfServer);

StringParameter SSecurityX509::X509_KeyFile
("x509key", "specifies path to the key of the x509 certificate in PEM format", "", ConfServer);

SSecurityX509::SSecurityX509() : dh_params(0), cert_cred(0)
{
  certfile = X509_CertFile.getData();
  keyfile = X509_KeyFile.getData();
}

SSecurityX509::~SSecurityX509()
{
  shutdown();
  if (dh_params)
    gnutls_dh_params_deinit(dh_params);
  if (cert_cred)
    gnutls_certificate_free_credentials(cert_cred);
  delete[] keyfile;
  delete[] certfile;
}

void SSecurityX509::freeResources()
{
  if (dh_params)
    gnutls_dh_params_deinit(dh_params);
  dh_params=0;
  if (cert_cred)
    gnutls_certificate_free_credentials(cert_cred);
  cert_cred=0;
}

void SSecurityX509::setParams(gnutls_session session)
{
    static const int kx_priority[] = {GNUTLS_KX_DHE_DSS, GNUTLS_KX_RSA, GNUTLS_KX_DHE_RSA, GNUTLS_KX_SRP, 0};
    gnutls_kx_set_priority(session, kx_priority);

    if (gnutls_certificate_allocate_credentials(&cert_cred) < 0)
      goto error;
    if (gnutls_dh_params_init(&dh_params) < 0)
      goto error;
    if (gnutls_dh_params_generate2(dh_params, DH_BITS) < 0)
      goto error;
    gnutls_certificate_set_dh_params(cert_cred, dh_params);
    if (gnutls_certificate_set_x509_key_file(cert_cred, certfile, keyfile,GNUTLS_X509_FMT_PEM) < 0)
      throw AuthFailureException("load of key failed");
    if (gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cert_cred) < 0)
      goto error;
    return;

 error:
    throw AuthFailureException("setParams failed");
}

