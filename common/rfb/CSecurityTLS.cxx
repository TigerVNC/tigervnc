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

#include <rfb/CSecurityTLS.h>


using namespace rfb;

CSecurityTLS::CSecurityTLS() :  anon_cred(0)
{
}

CSecurityTLS::~CSecurityTLS()
{
  shutdown();
  if (anon_cred)
    gnutls_anon_free_client_credentials (anon_cred);
}


void CSecurityTLS::freeResources()
{
  if (anon_cred)
    gnutls_anon_free_client_credentials(anon_cred);
  anon_cred=0;
 }

void CSecurityTLS::setParam(gnutls_session session)
{
  int kx_priority[] = { GNUTLS_KX_ANON_DH, 0 };
  gnutls_kx_set_priority(session, kx_priority);

  gnutls_anon_allocate_client_credentials(&anon_cred);
  gnutls_credentials_set(session, GNUTLS_CRD_ANON, anon_cred);
}

void CSecurityTLS::checkSession(gnutls_session session)
{

}

#endif /* HAVE_GNUTLS */
