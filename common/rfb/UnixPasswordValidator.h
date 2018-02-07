/* 
 * Copyright (C) 2006 Martin Koegler
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

#ifndef __PASSWORD_VALIDATOR_H__
#define __PASSWORD_VALIDATOR_H__

#include <rfb/SSecurityPlain.h>

namespace rfb
{
  class UnixPasswordValidator: public PasswordValidator {
  protected:
    bool validateInternal(SConnection * sc, const char *username,
			   const char *password) override;
  };
}

#endif
