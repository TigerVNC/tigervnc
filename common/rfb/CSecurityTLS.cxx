/*
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
 * Copyright (C) 2010 m-privacy GmbH
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
#error "This header should not be compiled without HAVE_GNUTLS defined"
#endif

#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include <rfb/CSecurityTLS.h>
#include <rfb/CConnection.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rfb/UserMsgBox.h>
#include <rdr/TLSInStream.h>
#include <rdr/TLSOutStream.h>
#include <os/os.h>

#include <gnutls/x509.h>

/*
 * GNUTLS 2.6.5 and older didn't have some variables defined so don't use them.
 * GNUTLS 1.X.X defined LIBGNUTLS_VERSION_NUMBER so treat it as "old" gnutls as
 * well
 */
#if (defined(GNUTLS_VERSION_NUMBER) && GNUTLS_VERSION_NUMBER < 0x020606) || \
    defined(LIBGNUTLS_VERSION_NUMBER)
#define WITHOUT_X509_TIMES
#endif

/* Ancient GNUTLS... */
#if !defined(GNUTLS_VERSION_NUMBER) && !defined(LIBGNUTLS_VERSION_NUMBER)
#define WITHOUT_X509_TIMES
#endif

using namespace rfb;

static const char* homedirfn(const char* fn);

StringParameter CSecurityTLS::X509CA("X509CA", "X509 CA certificate",
                                     homedirfn("x509_ca.pem"),
                                     ConfViewer);
StringParameter CSecurityTLS::X509CRL("X509CRL", "X509 CRL file",
                                     homedirfn("x509_crl.pem"),
                                     ConfViewer);

static LogWriter vlog("TLS");

static const char* homedirfn(const char* fn)
{
  static char full_path[PATH_MAX];
  char* homedir = NULL;

  if (getvnchomedir(&homedir) == -1)
    return "";

  snprintf(full_path, sizeof(full_path), "%s%s", homedir, fn);

  delete [] homedir;

  return full_path;
}

CSecurityTLS::CSecurityTLS(CConnection* cc, bool _anon)
  : CSecurity(cc), session(NULL), anon_cred(NULL), cert_cred(NULL),
    anon(_anon), tlsis(NULL), tlsos(NULL), rawis(NULL), rawos(NULL)
{
  cafile = X509CA.getData();
  crlfile = X509CRL.getData();

  if (gnutls_global_init() != GNUTLS_E_SUCCESS)
    throw AuthFailureException("gnutls_global_init failed");
}

void CSecurityTLS::shutdown()
{
  if (session) {
    int ret;
    // FIXME: We can't currently wait for the response, so we only send
    //        our close and hope for the best
    ret = gnutls_bye(session, GNUTLS_SHUT_WR);
    if ((ret != GNUTLS_E_SUCCESS) && (ret != GNUTLS_E_INVALID_SESSION))
      vlog.error("TLS shutdown failed: %s", gnutls_strerror(ret));
  }

  if (anon_cred) {
    gnutls_anon_free_client_credentials(anon_cred);
    anon_cred = 0;
  }

  if (cert_cred) {
    gnutls_certificate_free_credentials(cert_cred);
    cert_cred = 0;
  }

  if (rawis && rawos) {
    cc->setStreams(rawis, rawos);
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


CSecurityTLS::~CSecurityTLS()
{
  shutdown();

  delete[] cafile;
  delete[] crlfile;

  gnutls_global_deinit();
}

bool CSecurityTLS::processMsg()
{
  rdr::InStream* is = cc->getInStream();
  rdr::OutStream* os = cc->getOutStream();
  client = cc;

  if (!session) {
    if (!is->hasData(1))
      return false;

    if (is->readU8() == 0)
      throw AuthFailureException("Server failed to initialize TLS session");

    if (gnutls_init(&session, GNUTLS_CLIENT) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_init failed");

    if (gnutls_set_default_priority(session) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_set_default_priority failed");

    setParam();

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

    vlog.error("TLS Handshake failed: %s\n", gnutls_strerror (err));
    shutdown();
    throw AuthFailureException("TLS Handshake failed");
  }

  vlog.debug("TLS handshake completed with %s",
             gnutls_session_get_desc(session));

  checkSession();

  cc->setStreams(tlsis, tlsos);

  return true;
}

void CSecurityTLS::setParam()
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

  if (anon) {
    if (gnutls_anon_allocate_client_credentials(&anon_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_anon_allocate_client_credentials failed");

    if (gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_credentials_set failed");

    vlog.debug("Anonymous session has been set");
  } else {
    if (gnutls_certificate_allocate_credentials(&cert_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_certificate_allocate_credentials failed");

    if (gnutls_certificate_set_x509_system_trust(cert_cred) < 1)
      vlog.error("Could not load system certificate trust store");

    if (*cafile && gnutls_certificate_set_x509_trust_file(cert_cred,cafile,GNUTLS_X509_FMT_PEM) < 0)
      vlog.error("Could not load user specified certificate authority");

    if (*crlfile && gnutls_certificate_set_x509_crl_file(cert_cred,crlfile,GNUTLS_X509_FMT_PEM) < 0)
      vlog.error("Could not load user specified certificate revocation list");

    if (gnutls_credentials_set(session, GNUTLS_CRD_CERTIFICATE, cert_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_credentials_set failed");

    if (gnutls_server_name_set(session, GNUTLS_NAME_DNS,
                               client->getServerName(),
                               strlen(client->getServerName())) != GNUTLS_E_SUCCESS)
      vlog.error("Failed to configure the server name for TLS handshake");

    vlog.debug("X509 session has been set");
  }
}

void CSecurityTLS::checkSession()
{
  const unsigned allowed_errors = GNUTLS_CERT_INVALID |
				  GNUTLS_CERT_SIGNER_NOT_FOUND |
				  GNUTLS_CERT_SIGNER_NOT_CA |
				  GNUTLS_CERT_EXPIRED;
  unsigned int status;
  const gnutls_datum_t *cert_list;
  unsigned int cert_list_size = 0;
  int err;

  char *homeDir;
  gnutls_datum_t info;
  size_t len;

  if (anon)
    return;

  if (gnutls_certificate_type_get(session) != GNUTLS_CRT_X509)
    throw AuthFailureException("unsupported certificate type");

  err = gnutls_certificate_verify_peers2(session, &status);
  if (err != 0) {
    vlog.error("server certificate verification failed: %s", gnutls_strerror(err));
    throw AuthFailureException("server certificate verification failed");
  }

  if (status & GNUTLS_CERT_REVOKED)
    throw AuthFailureException("server certificate has been revoked");

#ifndef WITHOUT_X509_TIMES
  if (status & GNUTLS_CERT_NOT_ACTIVATED)
    throw AuthFailureException("server certificate has not been activated");

  if (status & GNUTLS_CERT_EXPIRED) {
    vlog.debug("server certificate has expired");
    if (!msg->showMsgBox(UserMsgBox::M_YESNO, "certificate has expired",
			 "The certificate of the server has expired, "
			 "do you want to continue?"))
      throw AuthFailureException("server certificate has expired");
  }
#endif
  /* Process other errors later */

  cert_list = gnutls_certificate_get_peers(session, &cert_list_size);
  if (!cert_list_size)
    throw AuthFailureException("empty certificate chain");

  /* Process only server's certificate, not issuer's certificate */
  gnutls_x509_crt_t crt;
  gnutls_x509_crt_init(&crt);

  if (gnutls_x509_crt_import(crt, &cert_list[0], GNUTLS_X509_FMT_DER) < 0)
    throw AuthFailureException("decoding of certificate failed");

  if (gnutls_x509_crt_check_hostname(crt, client->getServerName()) == 0) {
    CharArray text;
    vlog.debug("hostname mismatch");
    text.format("Hostname (%s) does not match the server certificate, "
                "do you want to continue?", client->getServerName());
    if (!msg->showMsgBox(UserMsgBox::M_YESNO,
                         "Certificate hostname mismatch", text.buf))
      throw AuthFailureException("Certificate hostname mismatch");
  }

  if (status == 0) {
    /* Everything is fine (hostname + verification) */
    gnutls_x509_crt_deinit(crt);
    return;
  }
    
  if (status & GNUTLS_CERT_INVALID)
    vlog.debug("server certificate invalid");
  if (status & GNUTLS_CERT_SIGNER_NOT_FOUND)
    vlog.debug("server cert signer not found");
  if (status & GNUTLS_CERT_SIGNER_NOT_CA)
    vlog.debug("server cert signer not CA");
  if (status & GNUTLS_CERT_EXPIRED)
    vlog.debug("server certificate has expired");

  if (status & GNUTLS_CERT_INSECURE_ALGORITHM)
    throw AuthFailureException("The server certificate uses an insecure algorithm");

  if ((status & (~allowed_errors)) != 0) {
    /* No other errors are allowed */
    vlog.debug("GNUTLS status of certificate verification: 0x%x", status);
    throw AuthFailureException("Invalid status of server certificate verification");
  }

  /* Certificate is fine, except we don't know the issuer, so TOFU time */

  homeDir = NULL;
  if (getvnchomedir(&homeDir) == -1) {
    throw AuthFailureException("Could not obtain VNC home directory "
                               "path for known hosts storage");
  }

  CharArray dbPath(strlen(homeDir) + 16 + 1);
  sprintf(dbPath.buf, "%sx509_known_hosts", homeDir);
  delete [] homeDir;

  err = gnutls_verify_stored_pubkey(dbPath.buf, NULL,
                                    client->getServerName(), NULL,
                                    GNUTLS_CRT_X509, &cert_list[0], 0);

  /* Previously known? */
  if (err == GNUTLS_E_SUCCESS) {
    vlog.debug("Server certificate found in known hosts file");
    gnutls_x509_crt_deinit(crt);
    return;
  }

  if ((err != GNUTLS_E_NO_CERTIFICATE_FOUND) &&
      (err != GNUTLS_E_CERTIFICATE_KEY_MISMATCH)) {
    throw AuthFailureException("Could not load known hosts database");
  }

  if (gnutls_x509_crt_print(crt, GNUTLS_CRT_PRINT_ONELINE, &info))
    throw AuthFailureException("Could not find certificate to display");

  len = strlen((char*)info.data);
  for (size_t i = 0; i < len - 1; i++) {
    if (info.data[i] == ',' && info.data[i + 1] == ' ')
      info.data[i] = '\n';
  }

  /* New host */
  if (err == GNUTLS_E_NO_CERTIFICATE_FOUND) {
    CharArray text;

    vlog.debug("Server host not previously known");
    vlog.debug("%s", info.data);

    if (status & (GNUTLS_CERT_SIGNER_NOT_FOUND |
                  GNUTLS_CERT_SIGNER_NOT_CA)) {
      text.format("This certificate has been signed by an unknown "
                  "authority:\n\n%s\n\nSomeone could be trying to "
                  "impersonate the site and you should not "
                  "continue.\n\nDo you want to make an exception "
                  "for this server?", info.data);

      if (!msg->showMsgBox(UserMsgBox::M_YESNO,
                           "Unknown certificate issuer",
                           text.buf))
        throw AuthFailureException("Unknown certificate issuer");
    }

    if (status & GNUTLS_CERT_EXPIRED) {
      text.format("This certificate has expired:\n\n%s\n\nSomeone "
                  "could be trying to impersonate the site and you "
                  "should not continue.\n\nDo you want to make an "
                  "exception for this server?", info.data);

      if (!msg->showMsgBox(UserMsgBox::M_YESNO,
                           "Expired certificate",
                           text.buf))
        throw AuthFailureException("Expired certificate");
    }
  } else if (err == GNUTLS_E_CERTIFICATE_KEY_MISMATCH) {
    CharArray text;

    vlog.debug("Server host key mismatch");
    vlog.debug("%s", info.data);

    if (status & (GNUTLS_CERT_SIGNER_NOT_FOUND |
                  GNUTLS_CERT_SIGNER_NOT_CA)) {
      text.format("This host is previously known with a different "
                  "certificate, and the new certificate has been "
                  "signed by an unknown authority:\n\n%s\n\nSomeone "
                  "could be trying to impersonate the site and you "
                  "should not continue.\n\nDo you want to make an "
                  "exception for this server?", info.data);

      if (!msg->showMsgBox(UserMsgBox::M_YESNO,
                           "Unexpected server certificate",
                           text.buf))
        throw AuthFailureException("Unexpected server certificate");
    }

    if (status & GNUTLS_CERT_EXPIRED) {
      text.format("This host is previously known with a different "
                  "certificate, and the new certificate has expired:"
                  "\n\n%s\n\nSomeone could be trying to impersonate "
                  "the site and you should not continue.\n\nDo you "
                  "want to make an exception for this server?",
                  info.data);

      if (!msg->showMsgBox(UserMsgBox::M_YESNO,
                           "Unexpected server certificate",
                           text.buf))
        throw AuthFailureException("Unexpected server certificate");
    }
  }

  if (gnutls_store_pubkey(dbPath.buf, NULL, client->getServerName(),
                          NULL, GNUTLS_CRT_X509, &cert_list[0], 0, 0))
    vlog.error("Failed to store server certificate to known hosts database");

  gnutls_x509_crt_deinit(crt);
  /*
   * GNUTLS doesn't correctly export gnutls_free symbol which is
   * a function pointer. Linking with Visual Studio 2008 Express will
   * fail when you call gnutls_free().
   */
#if WIN32
  free(info.data);
#else
  gnutls_free(info.data);
#endif
}

