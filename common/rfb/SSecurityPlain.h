/* Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2006 OCCAM Financial Technology
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
#ifndef __RFB_SSECURITYPLAIN_H__
#define __RFB_SSECURITYPLAIN_H__

#include <rfb/Security.h>
#include <rfb/SSecurity.h>

namespace rfb {

  class StringParameter;

  class PasswordValidator {
  public:
    bool validate(SConnection* sc, const char *username, const char *password)
      { return validUser(username) ? validateInternal(sc, username, password) : false; }
    static core::StringParameter plainUsers;

    virtual ~PasswordValidator() { }

  protected:
    virtual bool validateInternal(SConnection* sc, const char *username, const char *password)=0;
    static bool validUser(const char* username);
  };

  class SSecurityPlain : public SSecurity {
  public:
    SSecurityPlain(SConnection* sc);
    bool processMsg() override;
    int getType() const override { return secTypePlain; };
    const char* getUserName() const override { return username; }

    virtual ~SSecurityPlain() { }

  private:
    PasswordValidator* valid;
    unsigned int ulen, plen, state;
    char username[1024];
  };

}
#endif

