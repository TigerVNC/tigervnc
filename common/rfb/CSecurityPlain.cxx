/* Copyright (C) 2005 Martin Koegler
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

#include <rfb/CConnection.h>
#include <rfb/CSecurityPlain.h>
#include <rfb/UserPasswdGetter.h>
#include <rfb/util.h>

#include <rdr/OutStream.h>

using namespace rfb;

bool CSecurityPlain::processMsg()
{
   rdr::OutStream* os = cc->getOutStream();

  CharArray username;
  CharArray password;

  (CSecurity::upg)->getUserPasswd(cc->isSecure(), &username.buf, &password.buf);

  // Return the response to the server
  os->writeU32(strlen(username.buf));
  os->writeU32(strlen(password.buf));
  os->writeBytes(username.buf,strlen(username.buf));
  os->writeBytes(password.buf,strlen(password.buf));
  os->flush();
  return true;
}
