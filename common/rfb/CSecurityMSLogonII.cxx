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

#include <nettle/des.h>
#include <nettle/cbc.h>
#include <nettle/bignum.h>
#include <rfb/CSecurityMSLogonII.h>
#include <rfb/CConnection.h>
#include <rdr/InStream.h>
#include <rdr/OutStream.h>
#include <rdr/RandomStream.h>
#include <rfb/Exception.h>
#include <os/os.h>

using namespace rfb;

CSecurityMSLogonII::CSecurityMSLogonII(CConnection* cc_)
  : CSecurity(cc_)
{
  mpz_init(g);
  mpz_init(p);
  mpz_init(A);
  mpz_init(b);
  mpz_init(B);
  mpz_init(k);
}

CSecurityMSLogonII::~CSecurityMSLogonII()
{
  mpz_clear(g);
  mpz_clear(p);
  mpz_clear(A);
  mpz_clear(b);
  mpz_clear(B);
  mpz_clear(k);
}

bool CSecurityMSLogonII::processMsg()
{
  if (readKey()) {
    writeCredentials();
    return true;
  }
  return false;
}

bool CSecurityMSLogonII::readKey()
{
  rdr::InStream* is = cc->getInStream();
  if (!is->hasData(24))
    return false;
  uint8_t gBytes[8];
  uint8_t pBytes[8];
  uint8_t ABytes[8];
  is->readBytes(gBytes, 8);
  is->readBytes(pBytes, 8);
  is->readBytes(ABytes, 8);
  nettle_mpz_set_str_256_u(g, 8, gBytes);
  nettle_mpz_set_str_256_u(p, 8, pBytes);
  nettle_mpz_set_str_256_u(A, 8, ABytes);
  return true;
}

void CSecurityMSLogonII::writeCredentials()
{
  std::string username;
  std::string password;
  rdr::RandomStream rs;

  (CSecurity::upg)->getUserPasswd(isSecure(), &username, &password);

  std::vector<uint8_t> bBytes(8);
  if (!rs.hasData(8))
    throw Exception("failed to generate DH private key");
  rs.readBytes(bBytes.data(), bBytes.size());
  nettle_mpz_set_str_256_u(b, bBytes.size(), bBytes.data());
  mpz_powm(k, A, b, p);
  mpz_powm(B, g, b, p);

  uint8_t key[8];
  uint8_t reversedKey[8];
  uint8_t BBytes[8];
  uint8_t user[256];
  uint8_t pass[64];
  nettle_mpz_get_str_256(8, key, k);
  nettle_mpz_get_str_256(8, BBytes, B);
  for (int i = 0; i < 8; ++i) {
    uint8_t x = 0;
    for (int j = 0; j < 8; ++j) {
      x |= ((key[i] >> j) & 1) << (7 - j);
    }
    reversedKey[i] = x;
  }

  if (!rs.hasData(256 + 64))
    throw Exception("failed to generate random padding");
  rs.readBytes(user, 256);
  rs.readBytes(pass, 64);
  if (username.size() >= 256)
    throw Exception("username is too long");
  memcpy(user, username.c_str(), username.size() + 1);
  if (password.size() >= 64)
    throw Exception("password is too long");
  memcpy(pass, password.c_str(), password.size() + 1);

  // DES-CBC with the original key as IV, and the reversed one as the DES key
  struct CBC_CTX(struct des_ctx, DES_BLOCK_SIZE) ctx;
  des_fix_parity(8, reversedKey, reversedKey);
  des_set_key(&ctx.ctx, reversedKey);
  CBC_SET_IV(&ctx, key);
  CBC_ENCRYPT(&ctx, des_encrypt, 256, user, user);
  CBC_SET_IV(&ctx, key);
  CBC_ENCRYPT(&ctx, des_encrypt, 64, pass, pass);

  rdr::OutStream* os = cc->getOutStream();
  os->writeBytes(BBytes, 8);
  os->writeBytes(user, 256);
  os->writeBytes(pass, 64);
  os->flush();
}
