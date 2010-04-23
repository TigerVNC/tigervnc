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

#ifdef HAVE_GNUTLS

#include <rfb/CSecurityTLSBase.h>
#include <rfb/CConnection.h>
#include <rfb/LogWriter.h>
#include <rfb/Exception.h>
#include <rdr/TLSInStream.h>
#include <rdr/TLSOutStream.h>

#define TLS_DEBUG

using namespace rfb;

static LogWriter vlog("TLS");

#ifdef TLS_DEBUG
static void debug_log(int level, const char* str)
{
  vlog.debug(str);
}
#endif

void CSecurityTLSBase::initGlobal()
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

CSecurityTLSBase::CSecurityTLSBase() : session(0)
{
  fis = 0;
  fos = 0;
}

void CSecurityTLSBase::shutdown()
{
  if(session)
    ;//gnutls_bye(session, GNUTLS_SHUT_RDWR);
}


CSecurityTLSBase::~CSecurityTLSBase()
{
  if (session) {
    //gnutls_bye(session, GNUTLS_SHUT_RDWR);
    gnutls_deinit (session);
    session = 0;
  }
  if (fis)
    delete fis;
  if (fos)
    delete fos;
  /* FIXME: should be doing gnutls_global_deinit() at some point */
}

bool CSecurityTLSBase::processMsg(CConnection* cc)
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

    gnutls_init(&session, GNUTLS_CLIENT);
    gnutls_set_default_priority(session);

    setParam(session);
    
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
    gnutls_bye(session, GNUTLS_SHUT_RDWR);
    freeResources();
    gnutls_deinit(session);
    session = 0;
    throw AuthFailureException("TLS Handshake failed");
  }
  checkSession(session);

  cc->setStreams(fis = new rdr::TLSInStream(is, session),
		 fos = new rdr::TLSOutStream(os, session));

  return true;
}

#endif /* HAVE_GNUTLS */
