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
#error "This source should not be compiled without HAVE_GNUTLS defined"
#endif

#include <rfb/SSecurityTLS.h>
#include <rfb/SConnection.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rdr/TLSInStream.h>
#include <rdr/TLSOutStream.h>

#define DH_BITS 1024 /* XXX This should be configurable! */
#define TLS_DEBUG

using namespace rfb;

StringParameter SSecurityTLS::X509_CertFile
("x509cert", "specifies path to the x509 certificate in PEM format", "", ConfServer);

StringParameter SSecurityTLS::X509_KeyFile
("x509key", "specifies path to the key of the x509 certificate in PEM format", "", ConfServer);

static LogWriter vlog("TLS");

#ifdef TLS_DEBUG
static void debug_log(int level, const char* str)
{
  vlog.debug(str);
}
#endif

void SSecurityTLS::initGlobal()
{
  static bool globalInitDone = false;

  if (!globalInitDone) {
    if (gnutls_global_init() != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_global_init failed");

#ifdef TLS_DEBUG
    gnutls_global_set_log_level(10);
    gnutls_global_set_log_function(debug_log);
#endif

    globalInitDone = true;
  }
}

SSecurityTLS::SSecurityTLS(bool _anon) : session(0), dh_params(0),
						 anon_cred(0), cert_cred(0),
						 anon(_anon), fis(0), fos(0)
{
  certfile = X509_CertFile.getData();
  keyfile = X509_KeyFile.getData();
}

void SSecurityTLS::shutdown()
{
  if (session) {
    if (gnutls_bye(session, GNUTLS_SHUT_RDWR) != GNUTLS_E_SUCCESS) {
      /* FIXME: Treat as non-fatal error */
      vlog.error("TLS session wasn't terminated gracefully");
    }
  }

  if (dh_params) {
    gnutls_dh_params_deinit(dh_params);
    dh_params = 0;
  }

  if (anon_cred) {
    gnutls_anon_free_server_credentials(anon_cred);
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


SSecurityTLS::~SSecurityTLS()
{
  shutdown();

  if (fis)
    delete fis;
  if (fos)
    delete fos;

  delete[] keyfile;
  delete[] certfile;
}

bool SSecurityTLS::processMsg(SConnection *sc)
{
  rdr::InStream* is = sc->getInStream();
  rdr::OutStream* os = sc->getOutStream();

  vlog.debug("Process security message (session %p)", session);

  if (!session) {
    initGlobal();

    if (gnutls_init(&session, GNUTLS_SERVER) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_init failed");

    if (gnutls_set_default_priority(session) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_set_default_priority failed");

    try {
      setParams(session);
    }
    catch(...) {
      os->writeU8(0);
      throw;
    }

    gnutls_transport_set_pull_function(session,rdr::gnutls_InStream_pull);
    gnutls_transport_set_push_function(session,rdr::gnutls_OutStream_push);
    gnutls_transport_set_ptr2(session,
			      (gnutls_transport_ptr)is,
			      (gnutls_transport_ptr)os);
    os->writeU8(1);
    os->flush();
  }

  int err;
  if ((err = gnutls_handshake(session)) != GNUTLS_E_SUCCESS) {
    if (!gnutls_error_is_fatal(err)) {
      vlog.debug("Deferring completion of TLS handshake: %s", gnutls_strerror(err));
      return false;
    }
    vlog.error("TLS Handshake failed: %s", gnutls_strerror (err));
    shutdown();
    throw AuthFailureException("TLS Handshake failed");
  }

  vlog.debug("Handshake completed");

  sc->setStreams(fis=new rdr::TLSInStream(is,session),
		 fos=new rdr::TLSOutStream(os,session));

  return true;
}

void SSecurityTLS::setParams(gnutls_session session)
{
  static const int kx_anon_priority[] = { GNUTLS_KX_ANON_DH, 0 };
  static const int kx_priority[] = { GNUTLS_KX_DHE_DSS, GNUTLS_KX_RSA,
				     GNUTLS_KX_DHE_RSA, GNUTLS_KX_SRP, 0 };

  gnutls_kx_set_priority(session, anon ? kx_anon_priority : kx_priority);

  if (gnutls_dh_params_init(&dh_params) != GNUTLS_E_SUCCESS)
    throw AuthFailureException("gnutls_dh_params_init failed");

  if (gnutls_dh_params_generate2(dh_params, DH_BITS) != GNUTLS_E_SUCCESS)
    throw AuthFailureException("gnutls_dh_params_generate2 failed");

  if (anon) {
    if (gnutls_anon_allocate_server_credentials(&anon_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_anon_allocate_server_credentials failed");

    gnutls_anon_set_server_dh_params(anon_cred, dh_params);

    if (gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred)
        != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_credentials_set failed");

    vlog.debug("Anonymous session has been set");

  } else {
    if (gnutls_certificate_allocate_credentials(&cert_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_certificate_allocate_credentials failed");

    gnutls_certificate_set_dh_params(cert_cred, dh_params);

    if (gnutls_certificate_set_x509_key_file(cert_cred, certfile, keyfile,
        GNUTLS_X509_FMT_PEM) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("load of key failed");

    if (gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cert_cred)
        != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_credentials_set failed");

    vlog.debug("X509 session has been set");

  }

}
