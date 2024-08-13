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

CSecurityDH::CSecurityDH(CConnection* cc_)
  : CSecurity(cc_), keyLength(0)
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
  uint16_t gen = is->readU16();
  keyLength = is->readU16();
  if (keyLength < MinKeyLength)
    throw Exception("DH key is too short");
  if (keyLength > MaxKeyLength)
    throw Exception("DH key is too long");
  if (!is->hasDataOrRestore(keyLength * 2))
    return false;
  is->clearRestorePoint();
  mpz_set_ui(g, gen);
  std::vector<uint8_t> pBytes(keyLength);
  std::vector<uint8_t> ABytes(keyLength);
  is->readBytes(pBytes.data(), pBytes.size());
  is->readBytes(ABytes.data(), ABytes.size());
  nettle_mpz_set_str_256_u(p, pBytes.size(), pBytes.data());
  nettle_mpz_set_str_256_u(A, ABytes.size(), ABytes.data());
  return true;
}

void CSecurityDH::writeCredentials()
{
  std::string username;
  std::string password;
  rdr::RandomStream rs;

  (CSecurity::upg)->getUserPasswd(isSecure(), &username, &password);

  std::vector<uint8_t> bBytes(keyLength);
  if (!rs.hasData(keyLength))
    throw Exception("failed to generate DH private key");
  rs.readBytes(bBytes.data(), bBytes.size());
  nettle_mpz_set_str_256_u(b, bBytes.size(), bBytes.data());
  mpz_powm(k, A, b, p);
  mpz_powm(B, g, b, p);

  std::vector<uint8_t> sharedSecret(keyLength);
  std::vector<uint8_t> BBytes(keyLength);
  nettle_mpz_get_str_256(sharedSecret.size(), sharedSecret.data(), k);
  nettle_mpz_get_str_256(BBytes.size(), BBytes.data(), B);
  uint8_t key[16];
  struct md5_ctx md5Ctx;
  md5_init(&md5Ctx);
  md5_update(&md5Ctx, sharedSecret.size(), sharedSecret.data());
  md5_digest(&md5Ctx, 16, key);
  struct aes128_ctx aesCtx;
  aes128_set_encrypt_key(&aesCtx, key);

  uint8_t buf[128];
  if (!rs.hasData(128))
    throw Exception("failed to generate random padding");
  rs.readBytes(buf, 128);
  if (username.size() >= 64)
    throw Exception("username is too long");
  memcpy(buf, username.c_str(), username.size() + 1);
  if (password.size() >= 64)
    throw Exception("password is too long");
  memcpy(buf + 64, password.c_str(), password.size() + 1);
  aes128_encrypt(&aesCtx, 128, buf, buf);

  rdr::OutStream* os = cc->getOutStream();
  os->writeBytes(buf, 128);
  os->writeBytes(BBytes.data(), BBytes.size());
  os->flush();
}
