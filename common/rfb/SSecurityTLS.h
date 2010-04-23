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

#ifndef __S_SECURITY_TLS_H__
#define __S_SECURITY_TLS_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GNUTLS

#include <rfb/SSecurityTLSBase.h>
#include <rfb/SSecurityVeNCrypt.h>

namespace rfb {

  class SSecurityTLS : public SSecurityTLSBase {
  public:
    SSecurityTLS();
    virtual ~SSecurityTLS();
    virtual int getType() const {return secTypeTLSNone;}
  protected:
    virtual void freeResources();
    virtual void setParams(gnutls_session session);

  private:
    static void initGlobal();

    gnutls_dh_params dh_params;
    gnutls_anon_server_credentials anon_cred;
  };

}
#endif /* HAVE_GNUTLS */

#endif /* __S_SECURITY_TLS_H__ */
