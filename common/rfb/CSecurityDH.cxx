/* 
 * Copyright (C) 2022 Dinglan Peng
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

#ifndef HAVE_NETTLE
#error "This header should not be compiled without HAVE_NETTLE defined"
#endif

#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <assert.h>

#include <nettle/aes.h>
#include <nettle/md5.h>
#include <nettle/bignum.h>
#include <rfb/CSecurityDH.h>
#include <rfb/CConnection.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rdr/RandomStream.h>
#include <rfb/Exception.h>
#include <os/os.h>

using namespace rfb;

const int MinKeyLength = 128;
const int MaxKeyLength = 1024;

CSecurityDH::CSecurityDH(CConnection* cc)
  : CSecurity(cc), keyLength(0)
{
  mpz_init(g);
  mpz_init(p);
  mpz_init(A);
  mpz_init(b);
  mpz_init(B);
  mpz_init(k);
}

CSecurityDH::~CSecurityDH()
{
  mpz_clear(g);
  mpz_clear(p);
  mpz_clear(A);
  mpz_clear(b);
  mpz_clear(B);
  mpz_clear(k);
}

bool CSecurityDH::processMsg()
{
  if (readKey()) {
    writeCredentials();
    return true;
  }
  return false;
}

bool CSecurityDH::readKey()
{
  rdr::InStream* is = cc->getInStream();
  if (!is->hasData(4))
    return false;
  is->setRestorePoint();
  rdr::U16 gen = is->readU16();
  keyLength = is->readU16();
  if (keyLength < MinKeyLength)
    throw AuthFailureException("DH key is too short");
  if (keyLength > MaxKeyLength)
    throw AuthFailureException("DH key is too long");
  if (!is->hasDataOrRestore(keyLength * 2))
    return false;
  is->clearRestorePoint();
  mpz_set_ui(g, gen);
  rdr::U8Array pBytes(keyLength);
  rdr::U8Array ABytes(keyLength);
  is->readBytes(pBytes.buf, keyLength);
  is->readBytes(ABytes.buf, keyLength);
  nettle_mpz_set_str_256_u(p, keyLength, pBytes.buf);
  nettle_mpz_set_str_256_u(A, keyLength, ABytes.buf);
  return true;
}

void CSecurityDH::writeCredentials()
{
  CharArray username;
  CharArray password;
  rdr::RandomStream rs;

  (CSecurity::upg)->getUserPasswd(isSecure(), &username.buf, &password.buf);
  rdr::U8Array bBytes(keyLength);
  if (!rs.hasData(keyLength))
    throw ConnFailedException("failed to generate DH private key");
  rs.readBytes(bBytes.buf, keyLength);
  nettle_mpz_set_str_256_u(b, keyLength, bBytes.buf);
  mpz_powm(k, A, b, p);
  mpz_powm(B, g, b, p);

  rdr::U8Array sharedSecret(keyLength);
  rdr::U8Array BBytes(keyLength);
  nettle_mpz_get_str_256(keyLength, sharedSecret.buf, k);
  nettle_mpz_get_str_256(keyLength, BBytes.buf, B);
  rdr::U8 key[16];
  struct md5_ctx md5Ctx;
  md5_init(&md5Ctx);
  md5_update(&md5Ctx, keyLength, sharedSecret.buf);
  md5_digest(&md5Ctx, 16, key);
  struct aes128_ctx aesCtx;
  aes128_set_encrypt_key(&aesCtx, key);

  char buf[128];
  if (!rs.hasData(128))
    throw ConnFailedException("failed to generate random padding");
  rs.readBytes(buf, 128);
  size_t len = strlen(username.buf);
  if (len >= 64)
    throw AuthFailureException("username is too long");
  memcpy(buf, username.buf, len + 1);
  len = strlen(password.buf);
  if (len >= 64)
    throw AuthFailureException("password is too long");
  memcpy(buf + 64, password.buf, len + 1);
  aes128_encrypt(&aesCtx, 128, (rdr::U8 *)buf, (rdr::U8 *)buf);

  rdr::OutStream* os = cc->getOutStream();
  os->writeBytes(buf, 128);
  os->writeBytes(BBytes.buf, keyLength);
  os->flush();
}
