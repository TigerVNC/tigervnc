/*
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
 * Copyright (C) 2010 m-privacy GmbH
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
#error "This header should not be compiled without HAVE_GNUTLS defined"
#endif

#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include <core/LogWriter.h>
#include <core/string.h>
#include <core/xdgdirs.h>

#include <rfb/CSecurityTLS.h>
#include <rfb/CConnection.h>
#include <rfb/Exception.h>

#include <rdr/TLSException.h>
#include <rdr/TLSSocket.h>

#include <gnutls/x509.h>

using namespace rfb;

static const char* configdirfn(const char* fn);

core::StringParameter CSecurityTLS::X509CA("X509CA", "X509 CA certificate",
                                           configdirfn("x509_ca.pem"));
core::StringParameter CSecurityTLS::X509CRL("X509CRL", "X509 CRL file",
                                            configdirfn("x509_crl.pem"));

static core::LogWriter vlog("TLS");

static const char* configdirfn(const char* fn)
{
  static char full_path[PATH_MAX];
  const char* configdir;

  configdir = core::getvncconfigdir();
  if (configdir == nullptr)
    return "";

  snprintf(full_path, sizeof(full_path), "%s/%s", configdir, fn);
  return full_path;
}

CSecurityTLS::CSecurityTLS(CConnection* cc_, bool _anon)
  : CSecurity(cc_), session(nullptr),
    anon_cred(nullptr), cert_cred(nullptr),
    anon(_anon), tlssock(nullptr),
    rawis(nullptr), rawos(nullptr)
{
  int err = gnutls_global_init();
  if (err != GNUTLS_E_SUCCESS)
    throw rdr::tls_error("gnutls_global_init()", err);
}

void CSecurityTLS::shutdown()
{
  if (tlssock)
    tlssock->shutdown();

  if (anon_cred) {
    gnutls_anon_free_client_credentials(anon_cred);
    anon_cred = nullptr;
  }

  if (cert_cred) {
    gnutls_certificate_free_credentials(cert_cred);
    cert_cred = nullptr;
  }

  if (rawis && rawos) {
    cc->setStreams(rawis, rawos);
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


CSecurityTLS::~CSecurityTLS()
{
  shutdown();

  gnutls_global_deinit();
}

bool CSecurityTLS::processMsg()
{
  rdr::InStream* is = cc->getInStream();
  rdr::OutStream* os = cc->getOutStream();
  client = cc;

  if (!session) {
    int ret;

    if (!is->hasData(1))
      return false;

    if (is->readU8() == 0)
      throw protocol_error("Server failed to initialize TLS session");

    ret = gnutls_init(&session, GNUTLS_CLIENT);
    if (ret != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_init()", ret);

    ret = gnutls_set_default_priority(session);
    if (ret != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_set_default_priority()", ret);

    setParam();

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

  checkSession();

  cc->setStreams(&tlssock->inStream(), &tlssock->outStream());

  return true;
}

void CSecurityTLS::setParam()
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

  if (anon) {
    ret = gnutls_anon_allocate_client_credentials(&anon_cred);
    if (ret != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_anon_allocate_client_credentials()", ret);

    ret = gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred);
    if (ret != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_credentials_set()", ret);

    vlog.debug("Anonymous session has been set");
  } else {
    const char* hostname;
    size_t len;
    bool valid;

    ret = gnutls_certificate_allocate_credentials(&cert_cred);
    if (ret != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_certificate_allocate_credentials()", ret);

    if (gnutls_certificate_set_x509_system_trust(cert_cred) < 1)
      vlog.error("Could not load system certificate trust store");

    if (gnutls_certificate_set_x509_trust_file(cert_cred, X509CA, GNUTLS_X509_FMT_PEM) < 0)
      vlog.error("Could not load user specified certificate authority");

    if (gnutls_certificate_set_x509_crl_file(cert_cred, X509CRL, GNUTLS_X509_FMT_PEM) < 0)
      vlog.error("Could not load user specified certificate revocation list");

    ret = gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cert_cred);
    if (ret != GNUTLS_E_SUCCESS)
      throw rdr::tls_error("gnutls_credentials_set()", ret);

    // Only DNS hostnames are allowed, and some servers will reject the
    // connection if we provide anything else (e.g. an IPv6 address)
    hostname = client->getServerName();
    len = strlen(hostname);
    valid = true;
    for (size_t i = 0; i < len; i++) {
      if (!isalnum(hostname[i]) && hostname[i] != '.')
        valid = false;
    }

    if (valid) {
      if (gnutls_server_name_set(session, GNUTLS_NAME_DNS,
                                 client->getServerName(),
                                 strlen(client->getServerName())) != GNUTLS_E_SUCCESS)
        vlog.error("Failed to configure the server name for TLS handshake");
    }

    vlog.debug("X509 session has been set");
  }
}

void CSecurityTLS::checkSession()
{
  const unsigned allowed_errors = GNUTLS_CERT_INVALID |
				  GNUTLS_CERT_SIGNER_NOT_FOUND |
				  GNUTLS_CERT_SIGNER_NOT_CA |
				  GNUTLS_CERT_NOT_ACTIVATED |
				  GNUTLS_CERT_EXPIRED |
				  GNUTLS_CERT_INSECURE_ALGORITHM;
  unsigned int status;
  const gnutls_datum_t *cert_list;
  unsigned int cert_list_size = 0;
  int err, known;
  bool hostname_match;

  const char *hostsDir;
  gnutls_datum_t info;
  size_t len;

  if (anon)
    return;

  if (gnutls_certificate_type_get(session) != GNUTLS_CRT_X509) {
    gnutls_alert_send(session, GNUTLS_AL_FATAL,
                      GNUTLS_A_UNSUPPORTED_CERTIFICATE);
    throw protocol_error("Unsupported certificate type");
  }

  err = gnutls_certificate_verify_peers2(session, &status);
  if (err != 0) {
    vlog.error("Server certificate verification failed: %s", gnutls_strerror(err));
    gnutls_alert_send_appropriate(session, err);
    throw rdr::tls_error("Server certificate verification()", err);
  }

  if (status != 0) {
    gnutls_datum_t status_str;
    unsigned int fatal_status;

    fatal_status = status & (~allowed_errors);

    if (fatal_status != 0) {
      std::string error;

      err = gnutls_certificate_verification_status_print(fatal_status,
                                                         GNUTLS_CRT_X509,
                                                         &status_str,
                                                         0);
      if (err != GNUTLS_E_SUCCESS) {
        gnutls_alert_send_appropriate(session, err);
        throw rdr::tls_error("Failed to get certificate error description", err);
      }

      error = (const char*)status_str.data;

      gnutls_free(status_str.data);

      gnutls_alert_send(session, GNUTLS_AL_FATAL,
                        GNUTLS_A_BAD_CERTIFICATE);
      throw protocol_error(
        core::format("Invalid server certificate: %s", error.c_str()));
    }

    err = gnutls_certificate_verification_status_print(status,
                                                       GNUTLS_CRT_X509,
                                                       &status_str,
                                                       0);
    if (err != GNUTLS_E_SUCCESS) {
      gnutls_alert_send_appropriate(session, err);
      throw rdr::tls_error("Failed to get certificate error description", err);
    }

    vlog.info("Server certificate errors: %s", status_str.data);

    gnutls_free(status_str.data);
  }

  /* Process overridable errors later */

  cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
  if (!cert_list_size) {
    gnutls_alert_send(session, GNUTLS_AL_FATAL,
                      GNUTLS_A_UNSUPPORTED_CERTIFICATE);
    throw protocol_error("Empty certificate chain");
  }

  /* Process only server's certificate, not issuer's certificate */
  gnutls_x509_crt_t crt;
  gnutls_x509_crt_init(&crt);

  err = gnutls_x509_crt_import(crt, &cert_list[0], GNUTLS_X509_FMT_DER);
  if (err != GNUTLS_E_SUCCESS) {
    gnutls_alert_send_appropriate(session, err);
    throw rdr::tls_error("Failed to decode server certificate", err);
  }

  if (gnutls_x509_crt_check_hostname(crt, client->getServerName()) == 0) {
    vlog.info("Server certificate doesn't match given server name");
    hostname_match = false;
  } else {
    hostname_match = true;
  }

  if ((status == 0) && hostname_match) {
    /* Everything is fine (hostname + verification) */
    gnutls_x509_crt_deinit(crt);
    return;
  }

  /* Certificate has some user overridable problems, so TOFU time */

  hostsDir = core::getvncstatedir();
  if (hostsDir == nullptr) {
    throw std::runtime_error("Could not obtain VNC state directory "
                             "path for known hosts storage");
  }

  std::string dbPath;
  dbPath = (std::string)hostsDir + "/x509_known_hosts";

  known = gnutls_verify_stored_pubkey(dbPath.c_str(), nullptr,
                                      client->getServerName(), nullptr,
                                      GNUTLS_CRT_X509, &cert_list[0], 0);

  /* Previously known? */
  if (known == GNUTLS_E_SUCCESS) {
    vlog.info("Server certificate found in known hosts file");
    gnutls_x509_crt_deinit(crt);
    return;
  }

  if ((known != GNUTLS_E_NO_CERTIFICATE_FOUND) &&
      (known != GNUTLS_E_CERTIFICATE_KEY_MISMATCH)) {
    gnutls_alert_send_appropriate(session, known);
    throw rdr::tls_error("Could not load known hosts database", known);
  }

  err = gnutls_x509_crt_print(crt, GNUTLS_CRT_PRINT_ONELINE, &info);
  if (err != GNUTLS_E_SUCCESS) {
    gnutls_alert_send_appropriate(session, known);
    throw rdr::tls_error("Could not find certificate to display", err);
  }

  len = strlen((char*)info.data);
  for (size_t i = 0; i < len - 1; i++) {
    if (info.data[i] == ',' && info.data[i + 1] == ' ')
      info.data[i] = '\n';
  }

  /* New host */
  if (known == GNUTLS_E_NO_CERTIFICATE_FOUND) {
    std::string text;

    vlog.info("Server host not previously known");
    vlog.info("%s", info.data);

    if (status & (GNUTLS_CERT_INVALID |
                  GNUTLS_CERT_SIGNER_NOT_FOUND |
                  GNUTLS_CERT_SIGNER_NOT_CA)) {
      text = core::format(
        "This certificate has been signed by an unknown authority:\n"
        "\n"
        "%s\n"
        "\n"
        "Someone could be trying to impersonate the site and you "
        "should not continue.\n"
        "\n"
        "Do you want to make an exception for this server?",
        info.data);

      if (!cc->showMsgBox(MsgBoxFlags::M_YESNO,
                           "Unknown certificate issuer",
                           text.c_str())) {
        gnutls_alert_send(session, GNUTLS_AL_FATAL,
                          GNUTLS_A_UNKNOWN_CA);
        throw auth_cancelled();
      }

      status &= ~(GNUTLS_CERT_INVALID |
                  GNUTLS_CERT_SIGNER_NOT_FOUND |
                  GNUTLS_CERT_SIGNER_NOT_CA);
    }

    if (status & GNUTLS_CERT_NOT_ACTIVATED) {
      text = core::format(
        "This certificate is not yet valid:\n"
        "\n"
        "%s\n"
        "\n"
        "Someone could be trying to impersonate the site and you "
        "should not continue.\n"
        "\n"
        "Do you want to make an exception for this server?",
        info.data);

      if (!cc->showMsgBox(MsgBoxFlags::M_YESNO,
                           "Certificate is not yet valid",
                           text.c_str())) {
        gnutls_alert_send(session, GNUTLS_AL_FATAL,
                          GNUTLS_A_BAD_CERTIFICATE);
        throw auth_cancelled();
      }

      status &= ~GNUTLS_CERT_NOT_ACTIVATED;
    }

    if (status & GNUTLS_CERT_EXPIRED) {
      text = core::format(
        "This certificate has expired:\n"
        "\n"
        "%s\n"
        "\n"
        "Someone could be trying to impersonate the site and you "
        "should not continue.\n"
        "\n"
        "Do you want to make an exception for this server?",
        info.data);

      if (!cc->showMsgBox(MsgBoxFlags::M_YESNO,
                           "Expired certificate",
                           text.c_str())) {
        gnutls_alert_send(session, GNUTLS_AL_FATAL,
                          GNUTLS_A_BAD_CERTIFICATE);
        throw auth_cancelled();
      }

      status &= ~GNUTLS_CERT_EXPIRED;
    }

    if (status & GNUTLS_CERT_INSECURE_ALGORITHM) {
      text = core::format(
        "This certificate uses an insecure algorithm:\n"
        "\n"
        "%s\n"
        "\n"
        "Someone could be trying to impersonate the site and you "
        "should not continue.\n"
        "\n"
        "Do you want to make an exception for this server?",
        info.data);

      if (!cc->showMsgBox(MsgBoxFlags::M_YESNO,
                           "Insecure certificate algorithm",
                           text.c_str())) {
        gnutls_alert_send(session, GNUTLS_AL_FATAL,
                          GNUTLS_A_BAD_CERTIFICATE);
        throw auth_cancelled();
      }

      status &= ~GNUTLS_CERT_INSECURE_ALGORITHM;
    }

    if (status != 0) {
      vlog.error("Unhandled certificate problems: 0x%x", status);
      gnutls_alert_send(session, GNUTLS_AL_FATAL,
                        GNUTLS_A_BAD_CERTIFICATE);
      throw std::logic_error("Unhandled certificate problems");
    }

    if (!hostname_match) {
      text = core::format(
        "The specified hostname \"%s\" does not match the certificate "
        "provided by the server:\n"
        "\n"
        "%s\n"
        "\n"
        "Someone could be trying to impersonate the site and you "
        "should not continue.\n"
        "\n"
        "Do you want to make an exception for this server?",
        client->getServerName(), info.data);

      if (!cc->showMsgBox(MsgBoxFlags::M_YESNO,
                           "Certificate hostname mismatch",
                           text.c_str())) {
        gnutls_alert_send(session, GNUTLS_AL_FATAL,
                          GNUTLS_A_BAD_CERTIFICATE);
        throw auth_cancelled();
      }
    }
  } else if (known == GNUTLS_E_CERTIFICATE_KEY_MISMATCH) {
    std::string text;

    vlog.info("Server host key mismatch");
    vlog.info("%s", info.data);

    if (status & (GNUTLS_CERT_INVALID |
                  GNUTLS_CERT_SIGNER_NOT_FOUND |
                  GNUTLS_CERT_SIGNER_NOT_CA)) {
      text = core::format(
        "This host is previously known with a different certificate, "
        "and the new certificate has been signed by an unknown "
        "authority:\n"
        "\n"
        "%s\n"
        "\n"
        "Someone could be trying to impersonate the site and you "
        "should not continue.\n"
        "\n"
        "Do you want to make an exception for this server?",
        info.data);

      if (!cc->showMsgBox(MsgBoxFlags::M_YESNO,
                           "Unexpected server certificate",
                           text.c_str())) {
        gnutls_alert_send(session, GNUTLS_AL_FATAL,
                          GNUTLS_A_UNKNOWN_CA);
        throw auth_cancelled();
      }

      status &= ~(GNUTLS_CERT_INVALID |
                  GNUTLS_CERT_SIGNER_NOT_FOUND |
                  GNUTLS_CERT_SIGNER_NOT_CA);
    }

    if (status & GNUTLS_CERT_NOT_ACTIVATED) {
      text = core::format(
        "This host is previously known with a different certificate, "
        "and the new certificate is not yet valid:\n"
        "\n"
        "%s\n"
        "\n"
        "Someone could be trying to impersonate the site and you "
        "should not continue.\n"
        "\n"
        "Do you want to make an exception for this server?",
        info.data);

      if (!cc->showMsgBox(MsgBoxFlags::M_YESNO,
                           "Unexpected server certificate",
                           text.c_str())) {
        gnutls_alert_send(session, GNUTLS_AL_FATAL,
                          GNUTLS_A_BAD_CERTIFICATE);
        throw auth_cancelled();
      }

      status &= ~GNUTLS_CERT_NOT_ACTIVATED;
    }

    if (status & GNUTLS_CERT_EXPIRED) {
      text = core::format(
        "This host is previously known with a different certificate, "
        "and the new certificate has expired:\n"
        "\n"
        "%s\n"
        "\n"
        "Someone could be trying to impersonate the site and you "
        "should not continue.\n"
        "\n"
        "Do you want to make an exception for this server?",
        info.data);

      if (!cc->showMsgBox(MsgBoxFlags::M_YESNO,
                           "Unexpected server certificate",
                           text.c_str())) {
        gnutls_alert_send(session, GNUTLS_AL_FATAL,
                          GNUTLS_A_BAD_CERTIFICATE);
        throw auth_cancelled();
      }

      status &= ~GNUTLS_CERT_EXPIRED;
    }

    if (status & GNUTLS_CERT_INSECURE_ALGORITHM) {
      text = core::format(
        "This host is previously known with a different certificate, "
        "and the new certificate uses an insecure algorithm:\n"
        "\n"
        "%s\n"
        "\n"
        "Someone could be trying to impersonate the site and you "
        "should not continue.\n"
        "\n"
        "Do you want to make an exception for this server?",
        info.data);

      if (!cc->showMsgBox(MsgBoxFlags::M_YESNO,
                           "Unexpected server certificate",
                           text.c_str())) {
        gnutls_alert_send(session, GNUTLS_AL_FATAL,
                          GNUTLS_A_BAD_CERTIFICATE);
        throw auth_cancelled();
      }

      status &= ~GNUTLS_CERT_INSECURE_ALGORITHM;
    }

    if (status != 0) {
      vlog.error("Unhandled certificate problems: 0x%x", status);
      gnutls_alert_send(session, GNUTLS_AL_FATAL,
                        GNUTLS_A_BAD_CERTIFICATE);
      throw std::logic_error("Unhandled certificate problems");
    }

    if (!hostname_match) {
      text = core::format(
        "This host is previously known with a different certificate, "
        "and the specified hostname \"%s\" does not match the new "
        "certificate provided by the server:\n"
        "\n"
        "%s\n"
        "\n"
        "Someone could be trying to impersonate the site and you "
        "should not continue.\n"
        "\n"
        "Do you want to make an exception for this server?",
        client->getServerName(), info.data);

      if (!cc->showMsgBox(MsgBoxFlags::M_YESNO,
                           "Unexpected server certificate",
                           text.c_str())) {
        gnutls_alert_send(session, GNUTLS_AL_FATAL,
                          GNUTLS_A_BAD_CERTIFICATE);
        throw auth_cancelled();
      }
    }
  }

  if (gnutls_store_pubkey(dbPath.c_str(), nullptr,
                          client->getServerName(), nullptr,
                          GNUTLS_CRT_X509, &cert_list[0], 0, 0))
    vlog.error("Failed to store server certificate to known hosts database");

  vlog.info("Exception added for server host");

  gnutls_x509_crt_deinit(crt);
  gnutls_free(info.data);
}

