/*
 * Copyright (C) 2004 Red Hat Inc.
 * Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
 * Copyright (C) 2010 m-privacy GmbH
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

StringParameter CSecurityTLS::X509CA("X509CA", "X509 CA certificate", "", ConfViewer);
StringParameter CSecurityTLS::X509CRL("X509CRL", "X509 CRL file", "", ConfViewer);

static LogWriter vlog("TLS");

CSecurityTLS::CSecurityTLS(CConnection* cc, bool _anon)
  : CSecurity(cc), session(NULL), anon_cred(NULL), cert_cred(NULL),
    anon(_anon), tlsis(NULL), tlsos(NULL), rawis(NULL), rawos(NULL)
{
  cafile = X509CA.getData();
  crlfile = X509CRL.getData();

  if (gnutls_global_init() != GNUTLS_E_SUCCESS)
    throw AuthFailureException("gnutls_global_init failed");
}

void CSecurityTLS::setDefaults()
{
  char* homeDir = NULL;

  if (getvnchomedir(&homeDir) == -1) {
    vlog.error("Could not obtain VNC home directory path");
    return;
  }

  int len = strlen(homeDir) + 1;
  CharArray caDefault(len + 11);
  CharArray crlDefault(len + 12);
  sprintf(caDefault.buf, "%sx509_ca.pem", homeDir);
  sprintf(crlDefault.buf, "%s509_crl.pem", homeDir);
  delete [] homeDir;

 if (!fileexists(caDefault.buf))
   X509CA.setDefaultStr(caDefault.buf);
 if (!fileexists(crlDefault.buf))
   X509CRL.setDefaultStr(crlDefault.buf);
}

void CSecurityTLS::shutdown(bool needbye)
{
  if (session && needbye)
    if (gnutls_bye(session, GNUTLS_SHUT_RDWR) != GNUTLS_E_SUCCESS)
      vlog.error("gnutls_bye failed");

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
  shutdown(true);

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
    if (!is->checkNoWait(1))
      return false;

    if (is->readU8() == 0) {
      rdr::U32 result = is->readU32();
      CharArray reason;
      if (result == secResultFailed || result == secResultTooMany)
        reason.buf = is->readString();
      else
        reason.buf = strDup("protocol error");
      throw AuthFailureException(reason.buf);
    }

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
    if (!gnutls_error_is_fatal(err))
      return false;

    vlog.error("TLS Handshake failed: %s\n", gnutls_strerror (err));
    shutdown(false);
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

  if (anon) {
    if (gnutls_anon_allocate_client_credentials(&anon_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_anon_allocate_client_credentials failed");

    if (gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_credentials_set failed");

    vlog.debug("Anonymous session has been set");
  } else {
    if (gnutls_certificate_allocate_credentials(&cert_cred) != GNUTLS_E_SUCCESS)
      throw AuthFailureException("gnutls_certificate_allocate_credentials failed");

    if (gnutls_certificate_set_x509_system_trust(cert_cred) != GNUTLS_E_SUCCESS)
      vlog.error("Could not load system certificate trust store");

    if (*cafile && gnutls_certificate_set_x509_trust_file(cert_cred,cafile,GNUTLS_X509_FMT_PEM) < 0)
      throw AuthFailureException("load of CA cert failed");

    /* Load previously saved certs */
    char *homeDir = NULL;
    int err;
    if (getvnchomedir(&homeDir) == -1)
      vlog.error("Could not obtain VNC home directory path");
    else {
      CharArray caSave(strlen(homeDir) + 19 + 1);
      sprintf(caSave.buf, "%sx509_savedcerts.pem", homeDir);
      delete [] homeDir;

      err = gnutls_certificate_set_x509_trust_file(cert_cred, caSave.buf,
                                                   GNUTLS_X509_FMT_PEM);
      if (err < 0)
        vlog.debug("Failed to load saved server certificates from %s", caSave.buf);
    }

    if (*crlfile && gnutls_certificate_set_x509_crl_file(cert_cred,crlfile,GNUTLS_X509_FMT_PEM) < 0)
      throw AuthFailureException("load of CRL failed");

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
				  GNUTLS_CERT_SIGNER_NOT_CA;
  unsigned int status;
  const gnutls_datum_t *cert_list;
  unsigned int cert_list_size = 0;
  int err;
  gnutls_datum_t info;

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
    char buf[255];
    vlog.debug("hostname mismatch");
    snprintf(buf, sizeof(buf), "Hostname (%s) does not match any certificate, "
			       "do you want to continue?", client->getServerName());
    buf[sizeof(buf) - 1] = '\0';
    if (!msg->showMsgBox(UserMsgBox::M_YESNO, "hostname mismatch", buf))
      throw AuthFailureException("hostname mismatch");
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

  if (status & GNUTLS_CERT_INSECURE_ALGORITHM)
    throw AuthFailureException("The server certificate uses an insecure algorithm");

  if ((status & (~allowed_errors)) != 0) {
    /* No other errors are allowed */
    vlog.debug("GNUTLS status of certificate verification: %u", status);
    throw AuthFailureException("Invalid status of server certificate verification");
  }

  vlog.debug("Saved server certificates don't match");

  if (gnutls_x509_crt_print(crt, GNUTLS_CRT_PRINT_ONELINE, &info)) {
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
    throw AuthFailureException("Could not find certificate to display");
  }

  size_t out_size = 0;
  char *out_buf = NULL;
  char *certinfo = NULL;
  int len = 0;

  vlog.debug("certificate issuer unknown");

  len = snprintf(NULL, 0, "This certificate has been signed by an unknown "
                          "authority:\n\n%s\n\nDo you want to save it and "
                          "continue?\n ", info.data);
  if (len < 0)
    throw AuthFailureException("certificate decoding error");

  vlog.debug("%s", info.data);

  certinfo = new char[len];
  if (certinfo == NULL)
    throw AuthFailureException("Out of memory");

  snprintf(certinfo, len, "This certificate has been signed by an unknown "
                          "authority:\n\n%s\n\nDo you want to save it and "
                          "continue? ", info.data);

  for (int i = 0; i < len - 1; i++)
    if (certinfo[i] == ',' && certinfo[i + 1] == ' ')
      certinfo[i] = '\n';

  if (!msg->showMsgBox(UserMsgBox::M_YESNO, "certificate issuer unknown",
		       certinfo)) {
    delete [] certinfo;
    throw AuthFailureException("certificate issuer unknown");
  }

  delete [] certinfo;

  if (gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_PEM, NULL, &out_size)
      == GNUTLS_E_SHORT_MEMORY_BUFFER)
    throw AuthFailureException("Out of memory");

  // Save cert
  out_buf =  new char[out_size];
  if (out_buf == NULL)
    throw AuthFailureException("Out of memory");

  if (gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_PEM, out_buf, &out_size) < 0)
    throw AuthFailureException("certificate issuer unknown, and certificate "
                               "export failed");

  char *homeDir = NULL;
  if (getvnchomedir(&homeDir) == -1)
    vlog.error("Could not obtain VNC home directory path");
  else {
    FILE *f;
    CharArray caSave(strlen(homeDir) + 1 + 19);
    sprintf(caSave.buf, "%sx509_savedcerts.pem", homeDir);
    delete [] homeDir;
    f = fopen(caSave.buf, "a+");
    if (!f)
      msg->showMsgBox(UserMsgBox::M_OK, "certificate save failed",
                      "Could not save the certificate");
    else {
      fprintf(f, "%s\n", out_buf);
      fclose(f);
    }
  }

  delete [] out_buf;

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

