/* Copyright (C) 2002-2005 RealVNC Ltd.  All Rights Reserved.
 * Copyright 2023 Pierre Ossman for Cendio AB
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
// XXX not thread-safe, because d3des isn't - do we need to worry about this?
//

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>

extern "C" {
#include <rfb/d3des.h>
}

#include <rdr/Exception.h>
#include <rfb/obfuscate.h>

static unsigned char d3desObfuscationKey[] = {23,82,107,6,35,78,88,7};

std::vector<uint8_t> rfb::obfuscate(const char *str)
{
  std::vector<uint8_t> buf(8);

  assert(str != nullptr);

  size_t l = strlen(str), i;
  for (i=0; i<8; i++)
    buf[i] = i<l ? str[i] : 0;
  deskey(d3desObfuscationKey, EN0);
  des(buf.data(), buf.data());

  return buf;
}

std::string rfb::deobfuscate(const uint8_t *data, size_t len)
{
  char buf[9];

  if (len != 8)
    throw rdr::Exception("bad obfuscated password length");

  assert(data != nullptr);

  deskey(d3desObfuscationKey, DE1);
  des((uint8_t*)data, (uint8_t*)buf);
  buf[8] = 0;

  return buf;
}
