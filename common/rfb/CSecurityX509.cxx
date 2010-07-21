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

#include <rfb/CSecurityX509.h>
#include <rfb/CConnection.h>
#include <rfb/Exception.h>
#include <rfb/LogWriter.h>

#include <gnutls/x509.h>

using namespace rfb;

StringParameter CSecurityX509::x509ca("x509ca", "X509 CA certificate", "", ConfViewer);
StringParameter CSecurityX509::x509crl("x509crl", "X509 CRL file", "", ConfViewer);

static LogWriter vlog("CSecurityX509");

CSecurityX509::CSecurityX509() : cert_cred(0)
{
  cafile = x509ca.getData();
  crlfile = x509crl.getData();
}

CSecurityX509::~CSecurityX509()
{
  shutdown();
  if (cert_cred)
    gnutls_certificate_free_credentials(cert_cred);
  delete[] cafile;
  delete[] crlfile;
}


void CSecurityX509::freeResources()
{
  if (cert_cred)
    gnutls_certificate_free_credentials(cert_cred);
  cert_cred = 0;
 }

void CSecurityX509::setParam(gnutls_session session)
{
  int kx_priority[] = { GNUTLS_KX_DHE_DSS, GNUTLS_KX_RSA, GNUTLS_KX_DHE_RSA, GNUTLS_KX_SRP, 0 };
  gnutls_kx_set_priority(session, kx_priority);

  gnutls_certificate_allocate_credentials(&cert_cred);

  if (*cafile && gnutls_certificate_set_x509_trust_file(cert_cred,cafile,GNUTLS_X509_FMT_PEM) < 0)
    throw AuthFailureException("load of CA cert failed");

  if (*crlfile && gnutls_certificate_set_x509_crl_file(cert_cred,crlfile,GNUTLS_X509_FMT_PEM) < 0)
    throw AuthFailureException("load of CRL failed");

  gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cert_cred);
}

void CSecurityX509::checkSession(gnutls_session session)
{
  int status;
  const gnutls_datum *cert_list;
  unsigned int cert_list_size = 0;
  unsigned int i;

  if (gnutls_certificate_type_get(session) != GNUTLS_CRT_X509)
    throw AuthFailureException("unsupported certificate type");

  cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
  if (!cert_list_size)
    throw AuthFailureException("unsupported certificate type");
  
  status = gnutls_certificate_verify_peers(session);
  if (status == GNUTLS_E_NO_CERTIFICATE_FOUND)
    throw AuthFailureException("no certificate sent");

  if (status < 0) {
    vlog.error("X509 verify failed: %s\n", gnutls_strerror (status));
    throw AuthFailureException("certificate verification failed");
  }  

  if (status & GNUTLS_CERT_SIGNER_NOT_FOUND)
    throw AuthFailureException("certificate issuer unknown");

  if (status & GNUTLS_CERT_INVALID)
    throw AuthFailureException("certificate not trusted");

  for (i = 0; i < cert_list_size; i++) {
    gnutls_x509_crt crt;
    gnutls_x509_crt_init(&crt);

    if (gnutls_x509_crt_import(crt, &cert_list[i],GNUTLS_X509_FMT_DER) < 0)
      throw AuthFailureException("Decoding of certificate failed");

    if (gnutls_x509_crt_check_hostname(crt, client->getServerName()) == 0) {
#if 0
      /* FIXME: This must be changed to OK/Cancel checkbox */
      throw AuthFailureException("Hostname mismatch"); /* Non-fatal for now... */
#endif
    }
    gnutls_x509_crt_deinit(crt);
  }
}

