/* Copyright (C) 2012 Pierre Ossman for Cendio AB
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

#ifndef __RDR_TLSERRNO_H__
#define __RDR_TLSERRNO_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GNUTLS

#include <errno.h>

namespace rdr {

  static inline void gnutls_errno_helper(gnutls_session session, int _errno)
  {
#if defined(HAVE_GNUTLS_SET_ERRNO)
    gnutls_transport_set_errno(session, _errno);
#elif defined(HAVE_GNUTLS_SET_GLOBAL_ERRNO)
    gnutls_transport_set_global_errno(_errno);
#else
    errno = _errno;
#endif
  }
};

#endif

#endif
