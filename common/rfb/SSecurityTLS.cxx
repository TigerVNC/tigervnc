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

#ifndef HAVE_GNUTLS
#error "This source should not be compiled without HAVE_GNUTLS defined"
#endif

#include <rfb/SSecurityTLS.h>
#include <rfb/Exception.h>

#define DH_BITS 1024

#undef TLS_DEBUG

using namespace rfb;

SSecurityTLS::SSecurityTLS() : dh_params(0), anon_cred(0)
{
}

SSecurityTLS::~SSecurityTLS()
{
  shutdown();
  if (dh_params)
    gnutls_dh_params_deinit(dh_params);
  if (anon_cred)
    gnutls_anon_free_server_credentials(anon_cred);
}

void SSecurityTLS::freeResources()
{
  if (dh_params)
    gnutls_dh_params_deinit(dh_params);
  dh_params = 0;
  if (anon_cred)
    gnutls_anon_free_server_credentials(anon_cred);
  anon_cred = 0;
}

void SSecurityTLS::setParams(gnutls_session session)
{
  static const int kx_priority[] = {GNUTLS_KX_ANON_DH, 0};
  gnutls_kx_set_priority(session, kx_priority);

  if (gnutls_anon_allocate_server_credentials(&anon_cred) != GNUTLS_E_SUCCESS)
    throw AuthFailureException("gnutls_anon_allocate_server_credentials failed");

  if (gnutls_dh_params_init(&dh_params) != GNUTLS_E_SUCCESS)
    throw AuthFailureException("gnutls_dh_params_init failed");

  if (gnutls_dh_params_generate2(dh_params, DH_BITS) != GNUTLS_E_SUCCESS)
    throw AuthFailureException("gnutls_dh_params_generate2 failed");

  gnutls_anon_set_server_dh_params(anon_cred, dh_params);

  if (gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred) 
    != GNUTLS_E_SUCCESS)
    throw AuthFailureException("gnutls_credentials_set failed");

}

