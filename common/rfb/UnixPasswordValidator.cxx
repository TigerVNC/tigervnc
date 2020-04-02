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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/Configuration.h>
#include <rfb/Exception.h>
#include <rfb/UnixPasswordValidator.h>
#include <rfb/pam.h>

using namespace rfb;

static StringParameter pamService
  ("PAMService", "Service name for PAM password validation", "vnc");
AliasParameter pam_service("pam_service", "Alias for PAMService",
                           &pamService);

int do_pam_auth(const char *service, const char *username,
	        const char *password);

bool UnixPasswordValidator::validateInternal(SConnection * sc,
					     const char *username,
					     const char *password)
{
  CharArray service(strDup(pamService.getData()));
  return do_pam_auth(service.buf, username, password);
}
