/* Copyright (C) 2002-2003 RealVNC Ltd.  All Rights Reserved.
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
//
// vncAuth
//
// XXX not thread-safe, because d3des isn't - do we need to worry about this?
//

#include <string.h>
extern "C" {
#include <rfb/d3des.h>
}
#include <rfb/vncAuth.h>

using namespace rfb;

void rfb::vncAuthEncryptChallenge(rdr::U8* challenge, const char* passwd)
{
  unsigned char key[8] = { 0, };
  int len = strlen(passwd);
  if (len > 8) len = 8;
  for (int i = 0; i < len; i++)
    key[i] = passwd[i];

  deskey(key, EN0);

  for (int j = 0; j < vncAuthChallengeSize; j += 8)
    des(challenge+j, challenge+j);
}

static unsigned char obfuscationKey[] = {23,82,107,6,35,78,88,7};

void rfb::vncAuthObfuscatePasswd(char* passwd)
{
  for (int i = strlen(passwd); i < 8; i++)
    passwd[i] = 0;
  deskey(obfuscationKey, EN0);
  des((unsigned char*)passwd, (unsigned char*)passwd);
}

void rfb::vncAuthUnobfuscatePasswd(char* passwd)
{
  deskey(obfuscationKey, DE1);
  des((unsigned char*)passwd, (unsigned char*)passwd);
  passwd[8] = 0;
}
