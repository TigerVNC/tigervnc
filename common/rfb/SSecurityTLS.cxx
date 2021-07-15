/* 
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
 * Copyright (C) 2012-2021 Pierre Ossman for Cendio AB
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

#include <stdlib.h>

#include <rfb/SSecurityTLS.h>
#include <rfb/SConnection.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rdr/TLSInStream.h>
#include <rdr/TLSOutStream.h>
#include <gnutls/x509.h>

#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
/* FFDHE (RFC-7919) 2048-bit parameters, PEM-encoded */
static unsigned char ffdhe2048[] =
  "-----BEGIN DH PARAMETERS-----\n"
  "MIIBDAKCAQEA//////////+t+FRYortKmq/cViAnPTzx2LnFg84tNpWp4TZBFGQz\n"
  "+8yTnc4kmz75fS/jY2MMddj2gbICrsRhetPfHtXV/WVhJDP1H18GbtCFY2VVPe0a\n"
  "87VXE15/V8k1mE8McODmi3fipona8+/och3xWKE2rec1MKzKT0g6eXq8CrGCsyT7\n"
  "YdEIqUuyyOP7uWrat2DX9GgdT0Kj3jlN9K5W7edjcrsZCwenyO4KbXCeAvzhzffi\n"
  "7MA0BM0oNC9hkXL+nOmFg/+OTxIy7vKBg8P+OxtMb61zO7X8vC7CIAXFjvGDfRaD\n"
  "ssbzSibBsu/6iGtCOGEoXJf//////////wIBAgICAOE=\n"
  "-----END DH PARAMETERS-----\n";

static const gnutls_datum_t ffdhe_pkcs3_param = {
  ffdhe2048,
  sizeof(ffdhe2048)
};
#endif

using namespace rfb;

StringParameter SSecurityTLS::X509_CertFile
("X509Cert", "Path to the X509 certificate in PEM format", "", ConfServer);

StringParameter SSecurityTLS::X509_KeyFile
("X509Key", "Path to the key of the X509 certificate in PEM format", "", ConfServer);

static LogWriter vlog("TLS");

SSecurityTLS::SSecurityTLS(SConnection* sc, bool _anon)
  : SSecurity(sc), session(NULL), anon_cred(NULL),
    cert_cred(NULL), anon(_anon), tlsis(NULL), tlsos(NULL),
    rawis(NULL), rawos(NULL)
{
#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
  dh_params = NULL;
#endif

  certfile = X509_CertFile.getData();
  keyfile = X509_KeyFile.getData();

  if (gnutls_global_init() != GNUTLS_E_SUCCESS)
    throw AuthFailureException("gnutls_global_init failed");
}

void SSecurityTLS::shutdown()
{
  if (session) {
    int ret;
    // FIXME: We can't currently wait for the response, so we only send
    //        our close and hope for the best
    ret = gnutls_bye(session, GNUTLS_SHUT_WR);
    if ((ret != GNUTLS_E_SUCCESS) && (ret != GNUTLS_E_INVALID_SESSION))
      vlog.error("TLS shutdown failed: %s", gnutls_strerror(ret));
  }

#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
  if (dh_params) {
    gnutls_dh_params_deinit(dh_params);
    dh_params = 0;
  }
#endif

  if (anon_cred) {
    gnutls_anon_free_server_credentials(anon_cred);
    anon_cred = 0;
  }

  if (cert_cred) {
    gnutls_certificate_free_credentials(cert_cred);
    cert_cred = 0;
  }

  if (rawis && rawos) {
    sc->setStreams(rawis, rawos);
    rawis = NULL;
    rawos = NULL;
  }

  if (tlsis) {
    delete tlsis;
    tlsis = NULL;
  }
  if (tlsos) {
    delete tlsos;
    tlsos = NULL;
  }

  if (session) {
    gnutls_deinit(session);
    session = 0;
  }
}


SSecurityTLS::~SSecurityTLS()
{
  shutdown();

  delete[] keyfile;
  delete[] certfile;

  gnutls_global_deinit();
}

bool SSecurityTLS::processMsg()
{
  vlog.debug("Process security message (session %p)", session);

  if (!session) {
    rdr::InStream* is = sc->getInStream();
    rdr::OutStream* os = sc->getOutStream();

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

    os->writeU8(1);
    os->flush();

    // Create these early as they set up the push/pull functions
    // for GnuTLS
    tlsis = new rdr::TLSInStream(is, session);
    tlsos = new rdr::TLSOutStream(os, session);

    rawis = is;
    rawos = os;
  }

  int err;
  err = gnutls_handshake(session);
  if (err != GNUTLS_E_SUCCESS) {
    if (!gnutls_error_is_fatal(err)) {
      vlog.debug("Deferring completion of TLS handshake: %s", gnutls_strerror(err));
      return false;
    }
    vlog.error("TLS Handshake failed: %s", gnutls_strerror (err));
    shutdown();
    throw AuthFailureException("TLS Handshake failed");
  }

  vlog.debug("TLS handshake completed with %s",
             gnutls_session_get_desc(session));

  sc->setStreams(tlsis, tlsos);

  return true;
}

void SSecurityTLS::setParams(gnutls_session_t session)
{
  static const char kx_anon_priority[] = ":+ANON-ECDH:+ANON-DH";

  int ret;

  // Custom priority string specified?
  if (strcmp(Security::GnuTLSPriority, "") != 0) {
    char *prio;
    const char *err;

    prio = (char*)malloc(strlen(Security::GnuTLSPriority) +
                         strlen(kx_anon_priority) + 1);
    if (prio == NULL)
      throw AuthFailureException("Not enough memory for GnuTLS priority string");

    strcpy(prio, Security::GnuTLSPriority);
    if (anon)
      strcat(prio, kx_anon_priority);

    ret = gnutls_priority_set_direct(session, prio, &err);

    free(prio);

    if (ret != GNUTLS_E_SUCCESS) {
      if (ret == GNUTLS_E_INVALID_REQUEST)
        vlog.error("GnuTLS priority syntax error at: %s", err);
      throw AuthFailureException("gnutls_set_priority_direct failed");
    }
  } else if (anon) {
    const char *err;

#if GNUTLS_VERSION_NUMBER >= 0x030603
    // gnutls_set_default_priority_appends() expects a normal priority string that
    // doesn't start with ":".
    ret = gnutls_set_default_priority_append(session, kx_anon_priority + 1, &err, 0);
    if (ret != GNUTLS_E_SUCCESS) {
      if (ret == GNUTLS_E_INVALID_REQUEST)
        vlog.error("GnuTLS priority syntax error at: %s", err);
      throw AuthFailureException("gnutls_set_default_priority_append failed");
    }
#else
    // We don't know what the system default priority is, so we guess
    // it's what upstream GnuTLS has
    static const char gnutls_default_priority[] = "NORMAL";
    char *prio;

    prio = (char*)malloc(strlen(gnutls_default_priority) +
                         strlen(kx_anon_priority) + 1);
    if (prio == NULL)
      throw AuthFailureException("Not enough memory for GnuTLS priority string");

    strcpy(prio, gnutls_default_priority);
    strcat(prio, kx_anon_priority);

    ret = gnutls_priority_set_direct(session, prio, &err);

    free(prio);

    if (ret != GNUTLS_E_SUCCESS) {
      if (ret == GNUTLS_E_INVALID_REQUEST)
        vlog.error("GnuTLS priority syntax error at: %s", err);
      throw AuthFailureException("gnutls_set_priority_direct failed");
    }
#endif
  }

#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
  if (gnutls_dh_params_init(&dh_params) != GNUTLS_E_SUCCESS)
    throw AuthFailureException("gnutls_dh_params_init failed");

  if (gnutls_dh_params_import_pkcs3(dh_params, &ffdhe_pkcs3_param, GNUTLS_X509_FMT_PEM) != GNUTLS_E_SUCCESS)
    throw AuthFailureException("gnutls_dh_params_import_pkcs3 failed");
#endif

  if (anon) {
    if (gnutls_anon_allocate_server_credentials(&anon_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_anon_allocate_server_credentials failed");

#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
    gnutls_anon_set_server_dh_params(anon_cred, dh_params);
#endif

    if (gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred)
        != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_credentials_set failed");

    vlog.debug("Anonymous session has been set");

  } else {
    if (gnutls_certificate_allocate_credentials(&cert_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_certificate_allocate_credentials failed");

#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
    gnutls_certificate_set_dh_params(cert_cred, dh_params);
#endif

    switch (gnutls_certificate_set_x509_key_file(cert_cred, certfile, keyfile, GNUTLS_X509_FMT_PEM)) {
    case GNUTLS_E_SUCCESS:
      break;
    case GNUTLS_E_CERTIFICATE_KEY_MISMATCH:
      throw AuthFailureException("Private key does not match certificate");
    case GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE:
      throw AuthFailureException("Unsupported certificate type");
    default:
      throw AuthFailureException("Error loading X509 certificate or key");
    }

    if (gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cert_cred)
        != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_credentials_set failed");

    vlog.debug("X509 session has been set");

  }

}
