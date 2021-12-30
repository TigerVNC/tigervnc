/* Copyright (C) 2005-2006 Martin Koegler
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <rfb/WinPasswdValidator.h>
#include <windows.h>
#include <tchar.h>

using namespace rfb;

// This method will only work for Windows NT, 2000, and XP (and possibly Vista)
bool WinPasswdValidator::validateInternal(rfb::SConnection* sc,
					  const char* username,
					  const char* password)
{
	TCHAR* user = (TCHAR*) strDup(username);
	TCHAR* pass = (TCHAR*) strDup(password);
	TCHAR* domain = (TCHAR*) strDup(".");
	HANDLE handle;

	BOOL ret = LogonUser(user, domain, pass, LOGON32_LOGON_NETWORK,
			     LOGON32_PROVIDER_DEFAULT, &handle);
	delete [] user;
	delete [] pass;
	delete [] domain;

	if (ret != 0) {
		CloseHandle(handle);
		return true;
	}

	return false;
}
