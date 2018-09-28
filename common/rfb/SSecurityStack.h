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
#ifndef __RFB_SSECURITYSTACK_H__
#define __RFB_SSECURITYSTACK_H__

#include <rfb/SSecurity.h>

namespace rfb {

  class SSecurityStack : public SSecurity {
  public:
    SSecurityStack(SConnection* sc, int Type,
                   SSecurity* s0 = NULL, SSecurity* s1 = NULL);
    ~SSecurityStack();
    virtual bool processMsg();
    virtual int getType() const { return type; };
    virtual const char* getUserName() const;
    virtual SConnection::AccessRights getAccessRights() const;
  protected:
    short state;
    SSecurity* state0;
    SSecurity* state1;
    int type;
  };
}
#endif
