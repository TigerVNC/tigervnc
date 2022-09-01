/* Copyright (C) 2022 Dinglan Peng
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

#include <assert.h>
#include <rdr/AESInStream.h>
#include <rdr/Exception.h>

#ifdef HAVE_NETTLE
using namespace rdr;

AESInStream::AESInStream(InStream* _in, const U8* key, int _keySize)
  : keySize(_keySize), in(_in), counter()
{
  if (keySize == 128)
    EAX_SET_KEY(&eaxCtx128, aes128_set_encrypt_key, aes128_encrypt, key);
  else if (keySize == 256)
    EAX_SET_KEY(&eaxCtx256, aes256_set_encrypt_key, aes256_encrypt, key);
  else
    assert(!"incorrect key size");
}

AESInStream::~AESInStream() {}

bool AESInStream::fillBuffer()
{
  if (!in->hasData(2))
    return false;
  const U8* ptr = in->getptr(2);
  size_t length = ((int)ptr[0] << 8) | (int)ptr[1];
  if (!in->hasData(2 + length + 16))
    return false;
  ensureSpace(length);
  ptr = in->getptr(2 + length + 16);
  const U8* ad = ptr;
  const U8* data = ptr + 2;
  const U8* mac = ptr + 2 + length;
  U8 macComputed[16];

  if (keySize == 128) {
    EAX_SET_NONCE(&eaxCtx128, aes128_encrypt, 16, counter);
    EAX_UPDATE(&eaxCtx128, aes128_encrypt, 2, ad);
    EAX_DECRYPT(&eaxCtx128, aes128_encrypt, length, (rdr::U8*)end, data);
    EAX_DIGEST(&eaxCtx128, aes128_encrypt, 16, macComputed);
  } else {
    EAX_SET_NONCE(&eaxCtx256, aes256_encrypt, 16, counter);
    EAX_UPDATE(&eaxCtx256, aes256_encrypt, 2, ad);
    EAX_DECRYPT(&eaxCtx256, aes256_encrypt, length, (rdr::U8*)end, data);
    EAX_DIGEST(&eaxCtx256, aes256_encrypt, 16, macComputed);
  }
  if (memcmp(mac, macComputed, 16) != 0)
    throw Exception("AESInStream: failed to authenticate message");
  in->setptr(2 + length + 16);
  end += length;

  // Update nonce by incrementing the counter as a
  // 128bit little endian unsigned integer
  for (int i = 0; i < 16; ++i) {
    // increment until there is no carry
    if (++counter[i] != 0) {
      break;
    }
  }
  return true;
}

#endif