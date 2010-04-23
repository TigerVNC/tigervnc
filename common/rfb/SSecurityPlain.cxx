/* Copyright (C) 2005 Martin Koegler
 * Copyright (C) 2006 OCCAM Financial Technology
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

#include <rfb/SSecurityPlain.h>
#include <rfb/SConnection.h>
#include <rfb/Exception.h>
#include <rdr/InStream.h>

using namespace rfb;

StringParameter PasswordValidator::plainUsers
("PlainUsers",
 "Users permitted to access via Plain security type (including TLSPlain, X509Plain etc.)",
 "");

bool PasswordValidator::validUser(const char* username)
{
  CharArray users(strDup(plainUsers.getValueStr())), user;
  while (users.buf) {
    strSplit(users.buf, ',', &user.buf, &users.buf);
#ifdef WIN32
    if(0==stricmp(user.buf, "*"))
	  return true;
    if(0==stricmp(user.buf, username))
	  return true;
#else
    if(!strcmp (user.buf, "*"))
	  return true;
    if(!strcmp (user.buf, username))
	  return true;
#endif
  }
  return false;
}

SSecurityPlain::SSecurityPlain(PasswordValidator* _valid)
{
  valid=_valid;
  state=0;
}

bool SSecurityPlain::processMsg(SConnection* sc)
{
  rdr::InStream* is = sc->getInStream();
  char* pw;
  char *uname;
  CharArray password;

  if(state==0)
  {
    if(!is->checkNoWait(8))
      return false;
    ulen=is->readU32();
    plen=is->readU32();
    state=1;
  }
  if(state==1)
  {
    if(is->checkNoWait(ulen+plen+2))
      return false;
    state=2;
    pw=new char[plen+1];
    uname=new char[ulen+1];
    username.replaceBuf(uname);
    password.replaceBuf(pw);
    is->readBytes(uname,ulen);
    is->readBytes(pw,plen);
    pw[plen]=0;
    uname[ulen]=0;
    plen=0;
    if(!valid->validate(sc,uname,pw))
	  throw AuthFailureException("invalid password or username");
    return true;
  }
  return true;
}

