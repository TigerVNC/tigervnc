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

#include <rdr/TLSException.h>

#include <string.h>
#include <stdio.h>
#ifdef HAVE_GNUTLS
#include <gnutls/gnutls.h>
#endif

using namespace rdr;

#ifdef HAVE_GNUTLS
TLSException::TLSException(const char* s, int err_)
  : Exception("%s: %s (%d)", s, gnutls_strerror(err_), err_), err(err_)
{
}
#endif /* HAVE_GNUTLS */

