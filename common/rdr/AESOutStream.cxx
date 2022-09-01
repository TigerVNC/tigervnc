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
#include <rdr/Exception.h>
#include <rdr/AESOutStream.h>

#ifdef HAVE_NETTLE
using namespace rdr;

const int MaxMessageSize = 8192;

AESOutStream::AESOutStream(OutStream* _out, const U8* key, int _keySize)
  : keySize(_keySize), out(_out), counter()
{
  msg = new U8[MaxMessageSize + 16 + 2];
  if (keySize == 128)
    EAX_SET_KEY(&eaxCtx128, aes128_set_encrypt_key, aes128_encrypt, key);
  else if (keySize == 256)
    EAX_SET_KEY(&eaxCtx256, aes256_set_encrypt_key, aes256_encrypt, key);
  else
    assert(!"incorrect key size");
}

AESOutStream::~AESOutStream()
{
    delete[] msg;
}

void AESOutStream::flush()
{
  BufferedOutStream::flush();
  out->flush();
}

void AESOutStream::cork(bool enable)
{
  BufferedOutStream::cork(enable);
  out->cork(enable);
}

bool AESOutStream::flushBuffer()
{
  while (sentUpTo < ptr) {
    size_t n = ptr - sentUpTo;
    if (n > MaxMessageSize)
      n = MaxMessageSize;
    writeMessage(sentUpTo, n);
    sentUpTo += n;
  }
  return true;
}


void AESOutStream::writeMessage(const U8* data, size_t length)
{
  msg[0] = (length & 0xff00) >> 8;
  msg[1] = length & 0xff;

  if (keySize == 128) {
    EAX_SET_NONCE(&eaxCtx128, aes128_encrypt, 16, counter);
    EAX_UPDATE(&eaxCtx128, aes128_encrypt, 2, msg);
    EAX_ENCRYPT(&eaxCtx128, aes128_encrypt, length, msg + 2, data);
    EAX_DIGEST(&eaxCtx128, aes128_encrypt, 16, msg + 2 + length);
  } else {
    EAX_SET_NONCE(&eaxCtx256, aes256_encrypt, 16, counter);
    EAX_UPDATE(&eaxCtx256, aes256_encrypt, 2, msg);
    EAX_ENCRYPT(&eaxCtx256, aes256_encrypt, length, msg + 2, data);
    EAX_DIGEST(&eaxCtx256, aes256_encrypt, 16, msg + 2 + length);
  }
  out->writeBytes(msg, 2 + length + 16);
  out->flush();

  // Update nonce by incrementing the counter as a
  // 128bit little endian unsigned integer
  for (int i = 0; i < 16; ++i) {
    // increment until there is no carry
    if (++counter[i] != 0) {
      break;
    }
  }
}

#endif
