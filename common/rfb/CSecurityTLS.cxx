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
#include <rfb/SSecurityVeNCrypt.h> 
#include <rfb/CConnection.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rfb/UserMsgBox.h>
#include <rdr/TLSInStream.h>
#include <rdr/TLSOutStream.h>
#include <os/os.h>
#include <os/print.h>

#include <gnutls/x509.h>

#if !defined(GNUTLS_VERSION_NUMBER) || (GNUTLS_VERSION_NUMBER < 0x020708)
#define GNUTLS_CERT_NOT_ACTIVATED 512
#define GNUTLS_CERT_EXPIRED 1024
#endif

#if !defined(GNUTLS_VERSION_NUMBER) || (GNUTLS_VERSION_NUMBER < 0x020301)
#define GNUTLS_CRT_PRINT_ONELINE 1
#endif

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
   x509ca.setDefaultStr(strdup(caDefault.buf));
 if (!fileexists(crlDefault.buf))
   x509crl.setDefaultStr(strdup(crlDefault.buf));
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
  gnutls_datum info;

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

  if (status & GNUTLS_CERT_REVOKED) {
    throw AuthFailureException("certificate has been revoked");
  }

  if (status & GNUTLS_CERT_NOT_ACTIVATED) {
    throw AuthFailureException("certificate has not been activated");
  }

  for (i = 0; i < cert_list_size; i++) {
    gnutls_x509_crt crt;
    gnutls_x509_crt_init(&crt);

    if (gnutls_x509_crt_import(crt, &cert_list[i],GNUTLS_X509_FMT_DER) < 0)
      throw AuthFailureException("decoding of certificate failed");

    #if defined(GNUTLS_VERSION_NUMBER) && (GNUTLS_VERSION_NUMBER >= 0x010706)
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
    #endif

    if (gnutls_x509_crt_check_hostname(crt, client->getServerName()) == 0) {
      char buf[255];
      sprintf(buf, "Hostname (%s) does not match any certificate, do you want to continue?", client->getServerName());
      vlog.debug("hostname mismatch");
      if(!msg->showMsgBox(UserMsgBox::M_YESNO, "hostname mismatch", buf))
        throw AuthFailureException("hostname mismatch");
    }

    if (status & GNUTLS_CERT_EXPIRED) {
      vlog.debug("certificate has expired");
      if (!msg->showMsgBox(UserMsgBox::M_YESNO, "certficate has expired", "The certificate of the server has expired, do you want to continue?"))
        throw AuthFailureException("certificate has expired");
    }

    if (status & GNUTLS_CERT_SIGNER_NOT_FOUND) {
      size_t out_size;
      char *homeDir = NULL;
      char *out_buf = NULL;
      char *certinfo = NULL;
      int len = 0;

      vlog.debug("certificate issuer unknown");

      len = snprintf(NULL, 0, "This certificate has been signed by an unknown authority:\n\n%s\n\nDo you want to save it and continue?\n ", info.data);
      if (len < 0)
        AuthFailureException("certificate decoding error");

      vlog.debug("%s", info.data);

      certinfo = new char[len];
      if (certinfo == NULL)
        throw AuthFailureException("Out of memory");

      snprintf(certinfo, len, "This certificate has been signed by an unknown authority:\n\n%s\n\nDo you want to save it and continue? ", info.data);

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
        AuthFailureException("Out of memory");

      // Save cert
      out_buf =  new char[out_size];
      if (out_buf == NULL)
        AuthFailureException("Out of memory");

      if (gnutls_x509_crt_export(crt, GNUTLS_X509_FMT_PEM, out_buf, &out_size)
	  < 0)
        AuthFailureException("certificate issuer unknown, and certificate "
			     "export failed");

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
    } else if (status & GNUTLS_CERT_INVALID)
        throw AuthFailureException("certificate not trusted");

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
}

