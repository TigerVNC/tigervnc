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

#include <rfb/CSecurityStack.h>

using namespace rfb;

CSecurityStack::CSecurityStack(CConnection* cc, int Type,
                               CSecurity* s0, CSecurity* s1)
  : CSecurity(cc), type(Type)
{
  state = 0;
  state0 = s0;
  state1 = s1;
}

CSecurityStack::~CSecurityStack()
{
  delete state0;
  delete state1;
}

bool CSecurityStack::processMsg()
{
  bool res=true;
  if (state == 0) {
    if (state0)
      res = state0->processMsg();

    if (!res)
      return res;

    state++;
  }

  if (state == 1) {
    if(state1)
      res = state1->processMsg();

    if(!res)
      return res;

    state++;
  }

  return res;
}

bool CSecurityStack::isSecure() const
{
  if (state0 && state0->isSecure())
    return true;
  if (state == 1 && state1 && state1->isSecure())
    return true;
  return false;
}
