/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2010 TigerVNC Team
 * Copyright (C) 2012-2025 Pierre Ossman for Cendio AB
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

#include <core/Exception.h>
#include <core/LogWriter.h>

#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rdr/TLSException.h>
#include <rdr/TLSSocket.h>

#include <errno.h>

#ifdef HAVE_GNUTLS

using namespace rdr;

static core::LogWriter vlog("TLSSocket");

TLSSocket::TLSSocket(InStream* in_, OutStream* out_,
                     gnutls_session_t session_)
  : session(session_), in(in_), out(out_), tlsin(this), tlsout(this)
{
  gnutls_transport_set_pull_function(
    session, [](gnutls_transport_ptr_t sock, void* data, size_t size) {
      return ((TLSSocket*)sock)->pull(data, size);
    });
  gnutls_transport_set_push_function(
    session, [](gnutls_transport_ptr_t sock, const void* data, size_t size) {
      return ((TLSSocket*)sock)->push(data, size);
    });
  gnutls_transport_set_ptr(session, this);
}

TLSSocket::~TLSSocket()
{
  gnutls_transport_set_pull_function(session, nullptr);
  gnutls_transport_set_push_function(session, nullptr);
  gnutls_transport_set_ptr(session, nullptr);
}

bool TLSSocket::handshake()
{
  int err;

  err = gnutls_handshake(session);
  if (err != GNUTLS_E_SUCCESS) {
    gnutls_alert_description_t alert;
    const char* msg;

    if ((err == GNUTLS_E_PULL_ERROR) || (err == GNUTLS_E_PUSH_ERROR))
      std::rethrow_exception(saved_exception);

    alert = gnutls_alert_get(session);
    msg = nullptr;

    if ((err == GNUTLS_E_WARNING_ALERT_RECEIVED) ||
        (err == GNUTLS_E_FATAL_ALERT_RECEIVED))
      msg = gnutls_alert_get_name(alert);

    if (msg == nullptr)
      msg = gnutls_strerror(err);

    if (!gnutls_error_is_fatal(err)) {
      vlog.debug("Deferring completion of TLS handshake: %s", msg);
      return false;
    }

    vlog.error("TLS Handshake failed: %s\n", msg);
    gnutls_alert_send_appropriate(session, err);
    throw rdr::tls_error("TLS Handshake failed", err, alert);
  }

  return true;
}

void TLSSocket::shutdown()
{
  int ret;

  try {
    if (tlsout.hasBufferedData()) {
      tlsout.cork(false);
      tlsout.flush();
      if (tlsout.hasBufferedData())
        vlog.error("Failed to flush remaining socket data on close");
    }
  } catch (std::exception& e) {
    vlog.error("Failed to flush remaining socket data on close: %s", e.what());
  }

  // FIXME: We can't currently wait for the response, so we only send
  //        our close and hope for the best
  ret = gnutls_bye(session, GNUTLS_SHUT_WR);
  if ((ret != GNUTLS_E_SUCCESS) && (ret != GNUTLS_E_INVALID_SESSION))
    vlog.error("TLS shutdown failed: %s", gnutls_strerror(ret));
}

size_t TLSSocket::readTLS(uint8_t* buf, size_t len)
{
  int n;

  while (true) {
    streamEmpty = false;
    n = gnutls_record_recv(session, (void *) buf, len);
    if (n == GNUTLS_E_INTERRUPTED || n == GNUTLS_E_AGAIN) {
      // GnuTLS returns GNUTLS_E_AGAIN for a bunch of other scenarios
      // other than the pull function returning EAGAIN, so we have to
      // double check that the underlying stream really is empty
      if (!streamEmpty)
        continue;
      else
        return 0;
    }
    break;
  };

  if (n == GNUTLS_E_PULL_ERROR)
    std::rethrow_exception(saved_exception);

  if (n < 0) {
    gnutls_alert_send_appropriate(session, n);
    throw tls_error("readTLS", n, gnutls_alert_get(session));
  }

  if (n == 0)
    throw end_of_stream();

  return n;
}

size_t TLSSocket::writeTLS(const uint8_t* data, size_t length)
{
  int n;

  n = gnutls_record_send(session, data, length);
  if (n == GNUTLS_E_INTERRUPTED || n == GNUTLS_E_AGAIN)
    return 0;

  if (n == GNUTLS_E_PUSH_ERROR)
    std::rethrow_exception(saved_exception);

  if (n < 0) {
    gnutls_alert_send_appropriate(session, n);
    throw tls_error("writeTLS", n, gnutls_alert_get(session));
  }

  return n;
}

ssize_t TLSSocket::pull(void* data, size_t size)
{
  streamEmpty = false;
  saved_exception = nullptr;

  try {
    if (!in->hasData(1)) {
      streamEmpty = true;
      gnutls_transport_set_errno(session, EAGAIN);
      return -1;
    }

    if (in->avail() < size)
      size = in->avail();

    in->readBytes((uint8_t*)data, size);
  } catch (end_of_stream&) {
    return 0;
  } catch (std::exception& e) {
    core::socket_error* se;
    vlog.error("Failure reading TLS data: %s", e.what());
    se = dynamic_cast<core::socket_error*>(&e);
    if (se)
      gnutls_transport_set_errno(session, se->err);
    else
      gnutls_transport_set_errno(session, EINVAL);
    saved_exception = std::current_exception();
    return -1;
  }

  return size;
}

ssize_t TLSSocket::push(const void* data, size_t size)
{
  saved_exception = nullptr;

  try {
    out->writeBytes((const uint8_t*)data, size);
    out->flush();
  } catch (std::exception& e) {
    core::socket_error* se;
    vlog.error("Failure sending TLS data: %s", e.what());
    se = dynamic_cast<core::socket_error*>(&e);
    if (se)
      gnutls_transport_set_errno(session, se->err);
    else
      gnutls_transport_set_errno(session, EINVAL);
    saved_exception = std::current_exception();
    return -1;
  }

  return size;
}

#endif
