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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_GNUTLS
#error "This header should not be compiled without HAVE_GNUTLS defined"
#endif

#include <rfb/CSecurityTLS.h>
#include <rfb/SSecurityVeNCrypt.h> 
#include <rfb/CConnection.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rdr/TLSInStream.h>
#include <rdr/TLSOutStream.h>

#include <gnutls/x509.h>

#define TLS_DEBUG

using namespace rfb;

StringParameter CSecurityTLS::x509ca("x509ca", "X509 CA certificate", "", ConfViewer);
StringParameter CSecurityTLS::x509crl("x509crl", "X509 CRL file", "", ConfViewer);

static LogWriter vlog("TLS");

#ifdef TLS_DEBUG
static void debug_log(int level, const char* str)
{
  vlog.debug(str);
}
#endif

void CSecurityTLS::initGlobal()
{
  static bool globalInitDone = false;

  if (!globalInitDone) {
    gnutls_global_init();

#ifdef TLS_DEBUG
    gnutls_global_set_log_level(10);
    gnutls_global_set_log_function(debug_log);
#endif

    globalInitDone = true;
  }
}

CSecurityTLS::CSecurityTLS(bool _anon) : session(0), anon_cred(0),
						 anon(_anon), fis(0), fos(0)
{
  cafile = x509ca.getData();
  crlfile = x509crl.getData();
}

void CSecurityTLS::shutdown()
{
  if (session)
    if (gnutls_bye(session, GNUTLS_SHUT_RDWR) != GNUTLS_E_SUCCESS)
      throw Exception("gnutls_bye failed");

  if (anon_cred) {
    gnutls_anon_free_client_credentials(anon_cred);
    anon_cred = 0;
  }

  if (cert_cred) {
    gnutls_certificate_free_credentials(cert_cred);
    cert_cred = 0;
  }

  if (session) {
    gnutls_deinit(session);
    session = 0;

    gnutls_global_deinit();
  }
}


CSecurityTLS::~CSecurityTLS()
{
  shutdown();

  if (fis)
    delete fis;
  if (fos)
    delete fos;

  delete[] cafile;
  delete[] crlfile;
}

bool CSecurityTLS::processMsg(CConnection* cc)
{
  rdr::InStream* is = cc->getInStream();
  rdr::OutStream* os = cc->getOutStream();
  client = cc;

  initGlobal();

  if (!session) {
    if (!is->checkNoWait(1))
      return false;

    if (is->readU8() == 0)
      return true;

    if (gnutls_init(&session, GNUTLS_CLIENT) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_init failed");

    if (gnutls_set_default_priority(session) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_set_default_priority failed");

    setParam();
    
    gnutls_transport_set_pull_function(session, rdr::gnutls_InStream_pull);
    gnutls_transport_set_push_function(session, rdr::gnutls_OutStream_push);
    gnutls_transport_set_ptr2(session,
			      (gnutls_transport_ptr) is,
			      (gnutls_transport_ptr) os);
  }

  int err;
  err = gnutls_handshake(session);
  if (err != GNUTLS_E_SUCCESS && !gnutls_error_is_fatal(err))
    return false;

  if (err != GNUTLS_E_SUCCESS) {
    vlog.error("TLS Handshake failed: %s\n", gnutls_strerror (err));
    shutdown();
    throw AuthFailureException("TLS Handshake failed");
  }

  checkSession();

  cc->setStreams(fis = new rdr::TLSInStream(is, session),
		 fos = new rdr::TLSOutStream(os, session));

  return true;
}

void CSecurityTLS::setParam()
{
  static const int kx_anon_priority[] = { GNUTLS_KX_ANON_DH, 0 };
  static const int kx_priority[] = { GNUTLS_KX_DHE_DSS, GNUTLS_KX_RSA,
				     GNUTLS_KX_DHE_RSA, GNUTLS_KX_SRP, 0 };

  if (anon) {
    if (gnutls_kx_set_priority(session, kx_anon_priority) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_kx_set_priority failed");

    if (gnutls_anon_allocate_client_credentials(&anon_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_anon_allocate_client_credentials failed");

    if (gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_credentials_set failed");

    vlog.debug("Anonymous session has been set");
  } else {
    if (gnutls_kx_set_priority(session, kx_priority) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_kx_set_priority failed");

    if (gnutls_certificate_allocate_credentials(&cert_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_certificate_allocate_credentials failed");

    if (*cafile && gnutls_certificate_set_x509_trust_file(cert_cred,cafile,GNUTLS_X509_FMT_PEM) < 0)
      throw AuthFailureException("load of CA cert failed");

    if (*crlfile && gnutls_certificate_set_x509_crl_file(cert_cred,crlfile,GNUTLS_X509_FMT_PEM) < 0)
      throw AuthFailureException("load of CRL failed");

    if (gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cert_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_credentials_set failed");

    vlog.debug("X509 session has been set");
  }
}

void CSecurityTLS::checkSession()
{
  int status;
  const gnutls_datum *cert_list;
  unsigned int cert_list_size = 0;
  unsigned int i;

  if (anon)
    return;

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
      throw AuthFailureException("Hostname mismatch"); /* Non-fatal for now... */
#endif
    }
    gnutls_x509_crt_deinit(crt);
  }
}

