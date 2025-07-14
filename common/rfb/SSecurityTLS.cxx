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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef HAVE_GNUTLS
#error "This source should not be compiled without HAVE_GNUTLS defined"
#endif

#include <stdlib.h>

#include <core/LogWriter.h>

#include <rfb/SSecurityTLS.h>
#include <rfb/SConnection.h>
#include <rfb/Exception.h>

#include <rdr/TLSException.h>
#include <rdr/TLSSocket.h>

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

core::StringParameter SSecurityTLS::X509_CertFile
("X509Cert", "Path to the X509 certificate in PEM format", "");

core::StringParameter SSecurityTLS::X509_KeyFile
("X509Key", "Path to the key of the X509 certificate in PEM format", "");

static core::LogWriter vlog("TLS");

SSecurityTLS::SSecurityTLS(SConnection* sc_, bool _anon)
  : SSecurity(sc_), session(nullptr), anon_cred(nullptr),
    cert_cred(nullptr), anon(_anon), tlssock(nullptr),
    rawis(nullptr), rawos(nullptr)
{
  int ret;

#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
  dh_params = nullptr;
#endif

  ret = gnutls_global_init();
  if (ret != GNUTLS_E_SUCCESS)
    throw rdr::tls_error("gnutls_global_init()", ret);
}

void SSecurityTLS::shutdown()
{
  if (tlssock)
    tlssock->shutdown();

#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
  if (dh_params) {
    gnutls_dh_params_deinit(dh_params);
    dh_params = nullptr;
  }
#endif

  if (anon_cred) {
    gnutls_anon_free_server_credentials(anon_cred);
    anon_cred = nullptr;
  }

  if (cert_cred) {
    gnutls_certificate_free_credentials(cert_cred);
    cert_cred = nullptr;
  }

  if (rawis && rawos) {
    sc->setStreams(rawis, rawos);
    rawis = nullptr;
    rawos = nullptr;
  }

  if (tlssock) {
    delete tlssock;
    tlssock = nullptr;
  }

  if (session) {
    gnutls_deinit(session);
    session = nullptr;
  }
}


SSecurityTLS::~SSecurityTLS()
{
  shutdown();

  gnutls_global_deinit();
}

bool SSecurityTLS::processMsg()
{
  int err;

  vlog.debug("Process security message (session %p)", session);

  if (!session) {
    rdr::InStream* is = sc->getInStream();
    rdr::OutStream* os = sc->getOutStream();

    err = gnutls_init(&session, GNUTLS_SERVER);
    if (err != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_init()", err);

    err = gnutls_set_default_priority(session);
    if (err != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_set_default_priority()", err);

    try {
      setParams();
    }
    catch(...) {
      os->writeU8(0);
      throw;
    }

    os->writeU8(1);
    os->flush();

    tlssock = new rdr::TLSSocket(is, os, session);

    rawis = is;
    rawos = os;
  }

  try {
    if (!tlssock->handshake())
      return false;
  } catch (std::exception&) {
    shutdown();
    throw;
  }

  vlog.debug("TLS handshake completed with %s",
             gnutls_session_get_desc(session));

  sc->setStreams(&tlssock->inStream(), &tlssock->outStream());

  return true;
}

void SSecurityTLS::setParams()
{
  static const char kx_anon_priority[] = "+ANON-ECDH:+ANON-DH";

  int ret;

  // Custom priority string specified?
  if (strcmp(Security::GnuTLSPriority, "") != 0) {
    std::string prio;
    const char *err;

    prio = (const char*)Security::GnuTLSPriority;
    if (anon) {
      prio += ":";
      prio += kx_anon_priority;
    }

    ret = gnutls_priority_set_direct(session, prio.c_str(), &err);
    if (ret != GNUTLS_E_SUCCESS) {
      if (ret == GNUTLS_E_INVALID_REQUEST)
        vlog.error("GnuTLS priority syntax error at: %s", err);
      throw rdr::tls_error("gnutls_set_priority_direct()", ret);
    }
  } else if (anon) {
    const char *err;

#if GNUTLS_VERSION_NUMBER >= 0x030603
    ret = gnutls_set_default_priority_append(session, kx_anon_priority, &err, 0);
    if (ret != GNUTLS_E_SUCCESS) {
      if (ret == GNUTLS_E_INVALID_REQUEST)
        vlog.error("GnuTLS priority syntax error at: %s", err);
      throw rdr::tls_error("gnutls_set_default_priority_append()", ret);
    }
#else
    std::string prio;

    // We don't know what the system default priority is, so we guess
    // it's what upstream GnuTLS has
    prio = "NORMAL";
    prio += ":";
    prio += kx_anon_priority;

    ret = gnutls_priority_set_direct(session, prio.c_str(), &err);
    if (ret != GNUTLS_E_SUCCESS) {
      if (ret == GNUTLS_E_INVALID_REQUEST)
        vlog.error("GnuTLS priority syntax error at: %s", err);
      throw rdr::tls_error("gnutls_set_priority_direct()", ret);
    }
#endif
  }

#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
  ret = gnutls_dh_params_init(&dh_params);
  if (ret != GNUTLS_E_SUCCESS)
    throw rdr::tls_error("gnutls_dh_params_init()", ret);

  ret = gnutls_dh_params_import_pkcs3(dh_params, &ffdhe_pkcs3_param,
                                      GNUTLS_X509_FMT_PEM);
  if (ret != GNUTLS_E_SUCCESS)
    throw rdr::tls_error("gnutls_dh_params_import_pkcs3()", ret);
#endif

  if (anon) {
    ret = gnutls_anon_allocate_server_credentials(&anon_cred);
    if (ret != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_anon_allocate_server_credentials()", ret);

#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
    gnutls_anon_set_server_dh_params(anon_cred, dh_params);
#endif

    ret = gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred);
    if (ret != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_credentials_set()", ret);

    vlog.debug("Anonymous session has been set");

  } else {
    ret = gnutls_certificate_allocate_credentials(&cert_cred);
    if (ret != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_certificate_allocate_credentials()", ret);

#if defined (SSECURITYTLS__USE_DEPRECATED_DH)
    gnutls_certificate_set_dh_params(cert_cred, dh_params);
#endif

    ret = gnutls_certificate_set_x509_key_file(cert_cred, X509_CertFile,
                                               X509_KeyFile,
                                               GNUTLS_X509_FMT_PEM);
    if (ret != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("Failed to load certificate and key", ret);

    ret = gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cert_cred);
    if (ret != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_credentials_set()", ret);

    vlog.debug("X509 session has been set");

  }

}
