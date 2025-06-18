/*
 * Copyright (C) 2004 Red Hat Inc.
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

#include <core/string.h>

#include <rdr/TLSException.h>

#include <string.h>
#include <stdio.h>
#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

using namespace rdr;

#ifdef HAVE_GNUTLS
tls_error::tls_error(const char* s, int err_, int alert_) noexcept
  : std::runtime_error(core::format("%s: %s (%d)", s,
                                    strerror(err_, alert_), err_)),
    err(err_), alert(alert_)
{
}

const char* tls_error::strerror(int err_, int alert_) const noexcept
{
  const char* msg;

  msg = nullptr;

  if ((alert_ != -1) &&
      ((err_ == GNUTLS_E_WARNING_ALERT_RECEIVED) ||
       (err_ == GNUTLS_E_FATAL_ALERT_RECEIVED)))
    msg = gnutls_alert_get_name((gnutls_alert_description_t)alert_);

  if (msg == nullptr)
    msg = gnutls_strerror(err_);

  return msg;
}
#endif /* HAVE_GNUTLS */

