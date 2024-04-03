/* 
 * Copyright (C) 2022 Dinglan Peng
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

#ifndef __C_SECURITY_MSLOGONII_H__
#define __C_SECURITY_MSLOGONII_H__

#ifndef HAVE_NETTLE
#error "This header should not be compiled without HAVE_NETTLE defined"
#endif

#include <nettle/bignum.h>
#include <rfb/CSecurity.h>
#include <rfb/Security.h>

namespace rfb {
  class CSecurityMSLogonII : public CSecurity {
  public:
    CSecurityMSLogonII(CConnection* cc);
    virtual ~CSecurityMSLogonII();
    bool processMsg() override;
    int getType() const override { return secTypeMSLogonII; }
    bool isSecure() const override { return false; }

  private:
    bool readKey();
    void writeCredentials();

    mpz_t g, p, A, b, B, k;
  };
}

#endif
