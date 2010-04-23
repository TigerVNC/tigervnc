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

#include <rfb/SSecurityTLSBase.h>
#include <rfb/SConnection.h>
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

void SSecurityTLSBase::initGlobal()
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

SSecurityTLSBase::SSecurityTLSBase() : session(0)
{
  fis=0;
  fos=0;
}

void SSecurityTLSBase::shutdown()
{
  if(session)
    ;//gnutls_bye(session, GNUTLS_SHUT_RDWR);
}


SSecurityTLSBase::~SSecurityTLSBase()
{
  if (session) {
    //gnutls_bye(session, GNUTLS_SHUT_RDWR);
    gnutls_deinit(session);
  }
  if(fis)
    delete fis;
  if(fos)
    delete fos;
  /* FIXME: should be doing gnutls_global_deinit() at some point */
}

bool SSecurityTLSBase::processMsg(SConnection *sc)
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
    gnutls_bye(session, GNUTLS_SHUT_RDWR);
    freeResources();
    gnutls_deinit(session);
    session = 0;
    throw AuthFailureException("TLS Handshake failed");
  }

  vlog.debug("Handshake completed");

  sc->setStreams(fis=new rdr::TLSInStream(is,session),
		 fos=new rdr::TLSOutStream(os,session));

  return true;
}

#endif /* HAVE_GNUTLS */
