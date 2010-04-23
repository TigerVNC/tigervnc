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

#ifndef __C_SECURITY_TLSBASE_H__
#define __C_SECURITY_TLSBASE_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GNUTLS

#include <rfb/CSecurity.h>
#include <rfb/Security.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <gnutls/gnutls.h>

namespace rfb {
  class CSecurityTLSBase : public CSecurity {
  public:
    CSecurityTLSBase();
    virtual ~CSecurityTLSBase();
    virtual bool processMsg(CConnection* cc);

  protected:
    void shutdown();
    virtual void freeResources() = 0;
    virtual void setParam(gnutls_session session) = 0;
    virtual void checkSession(gnutls_session session) = 0;
    CConnection *client;

  private:
    static void initGlobal();

    gnutls_session session;
    rdr::InStream* fis;
    rdr::OutStream* fos;
  };
}

#endif /* HAVE_GNUTLS */

#endif
